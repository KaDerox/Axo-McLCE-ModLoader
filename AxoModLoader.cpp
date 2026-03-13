#include "stdafx.h"

#include "AxoModLoader.h"
#include "AxoAPI.h"
#include "..\..\Minecraft.World\Recipes.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShlDisp.h>
#include <ShlObj.h>
#include <ObjBase.h>
#include <OleAuto.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "shell32.lib")

#include <string>
#include <vector>
#include <fstream>
#include <cstdio>

struct LoadedMod {
    std::string id;
    HMODULE     handle;

    typedef void(*FnModEntry)   (AxoMod*, AxoAPITable*);
    typedef void(*FnOnTick)     ();
    typedef void(*FnOnShutdown) ();

    FnModEntry   fnModEntry;
    FnOnTick     fnOnTick;
    FnOnShutdown fnOnShutdown;

    LoadedMod() : handle(NULL), fnModEntry(NULL), fnOnTick(NULL), fnOnShutdown(NULL) {}
};

static std::vector<LoadedMod>    sLoadedMods;
static std::vector<std::wstring> sQueuedTerrainIcons;

static std::string PathJoin(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    char last = a[a.size() - 1];
    if (last == '\\' || last == '/') return a + b;
    return a + "\\" + b;
}

static std::wstring ToWide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, NULL, 0);
    std::wstring w(len, 0);
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &w[0], len);
    if (!w.empty() && w.back() == L'\0') w.pop_back();
    return w;
}

static void MakeDirs(const std::string& path) {
    std::string cur;
    for (size_t i = 0; i <= path.size(); ++i) {
        char c = (i < path.size()) ? path[i] : '\0';
        if (c == '\\' || c == '/' || c == '\0') {
            if (!cur.empty()) CreateDirectoryA(cur.c_str(), NULL);
        }
        if (c != '\0') cur += c;
    }
}

static bool PathExists(const std::string& path) {
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

static std::string GetFilename(const std::string& path) {
    size_t sl = path.find_last_of("\\/");
    return (sl == std::string::npos) ? path : path.substr(sl + 1);
}

static std::string GetStem(const std::string& fn) {
    size_t dot = fn.rfind('.');
    return (dot == std::string::npos) ? fn : fn.substr(0, dot);
}

static void RemoveAll(const std::string& dir) {
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(PathJoin(dir, "*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;
        std::string child = PathJoin(dir, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) RemoveAll(child);
        else DeleteFileA(child.c_str());
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    RemoveDirectoryA(dir.c_str());
}

static void CopyDirFiles(const std::string& srcDir, const std::string& destDir) {
    MakeDirs(destDir);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(PathJoin(srcDir, "*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;
        std::string src  = PathJoin(srcDir, fd.cFileName);
        std::string dest = PathJoin(destDir, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) CopyDirFiles(src, dest);
        else CopyFileA(src.c_str(), dest.c_str(), FALSE);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

static std::string FindDLL(const std::string& dir) {
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(PathJoin(dir, "*.dll").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return "";
    std::string result = PathJoin(dir, fd.cFileName);
    FindClose(h);
    return result;
}

static std::vector<std::string> ListZips(const std::string& dir) {
    std::vector<std::string> out;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(PathJoin(dir, "*.zip").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return out;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            out.push_back(PathJoin(dir, fd.cFileName));
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return out;
}

static int CountFilesRecursive(const std::string& dir) {
    int n = 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(PathJoin(dir, "*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) n += CountFilesRecursive(PathJoin(dir, fd.cFileName));
        else ++n;
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return n;
}

static void WaitForShellCopy(const std::string& destDir, int timeoutMs = 15000) {
    int elapsed = 0, prev = -1;
    while (elapsed < timeoutMs) {
        Sleep(300);
        elapsed += 300;
        int cur = CountFilesRecursive(destDir);
        if (cur > 0 && cur == prev) return;
        prev = cur;
    }
}

static bool ExtractZipToDir(const std::string& zipPath, const std::string& destDir) {
    MakeDirs(destDir);
    char absZip[MAX_PATH], absDest[MAX_PATH];
    if (!GetFullPathNameA(zipPath.c_str(),  MAX_PATH, absZip,  NULL)) return false;
    if (!GetFullPathNameA(destDir.c_str(), MAX_PATH, absDest, NULL)) return false;

    CoInitialize(NULL);
    IShellDispatch* pShell = NULL;
    HRESULT hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IShellDispatch, (void**)&pShell);
    if (FAILED(hr)) { CoUninitialize(); return false; }

    VARIANT vZip  = {}; vZip.vt  = VT_BSTR; vZip.bstrVal  = SysAllocString(ToWide(absZip).c_str());
    VARIANT vDest = {}; vDest.vt = VT_BSTR; vDest.bstrVal = SysAllocString(ToWide(absDest).c_str());

    Folder* pZip  = NULL;
    Folder* pDest = NULL;
    pShell->NameSpace(vZip,  &pZip);
    pShell->NameSpace(vDest, &pDest);
    VariantClear(&vZip);
    VariantClear(&vDest);

    bool ok = false;
    if (pZip && pDest) {
        FolderItems* pItems = NULL;
        pZip->Items(&pItems);
        if (pItems) {
            VARIANT vItems = {}; vItems.vt = VT_DISPATCH; vItems.pdispVal = pItems;
            VARIANT vFlags = {}; vFlags.vt = VT_I4;       vFlags.lVal     = 4 | 16 | 1024;
            pDest->CopyHere(vItems, vFlags);
            WaitForShellCopy(absDest);
            pItems->Release();
            ok = true;
        }
        if (pZip)  pZip->Release();
        if (pDest) pDest->Release();
    }
    pShell->Release();
    CoUninitialize();
    return ok;
}

static std::string ReadModId(const std::string& manifestPath) {
    std::ifstream f(manifestPath.c_str());
    if (!f) return "unknown";
    std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    size_t pos   = json.find("\"mod_id\"");
    if (pos == std::string::npos) return "unknown";
    size_t colon = json.find(':', pos);
    size_t q1    = json.find('"', colon + 1);
    size_t q2    = json.find('"', q1 + 1);
    if (q1 == std::string::npos || q2 == std::string::npos) return "unknown";
    return json.substr(q1 + 1, q2 - q1 - 1);
}

static void AxoLoader_CreateConsole() {
    if (AllocConsole()) {
        FILE* f = NULL;
        freopen_s(&f, "CONOUT$", "w", stdout);
        freopen_s(&f, "CONOUT$", "w", stderr);
        freopen_s(&f, "CONIN$",  "r", stdin);
        SetConsoleTitleA("AxoLoader Debug Console");
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD size = { 200, 5000 };
        SetConsoleScreenBufferSize(hOut, size);
        printf("================================================\n");
        printf("  AxoLoader - Minecraft LCE Mod Loader\n");
        printf("================================================\n");
    }
}

void AxoModLoader_MidInit() {
    printf("[AxoLoader] MidInit...\n");
    AxoAPI_FlushBlockRegistrations();
    printf("[AxoLoader] MidInit done.\n");
}

void AxoModLoader_Init(const char*) {
    printf("[AxoLoader] Init...\n");
    AxoAPI_FlushCreativeMenu();
    AxoAPI_FlushRecipeRegistrations();
    Recipes::getInstance()->buildRecipeIngredientsArray();
    printf("[AxoLoader] Init done. %u mod(s) active.\n", (unsigned)sLoadedMods.size());
}

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#include "..\PreStitchedTextureMap.h"

bool AxoModLoader_IsTerrainIconQueued(const std::wstring& name) {
    for (size_t i = 0; i < sQueuedTerrainIcons.size(); ++i)
        if (sQueuedTerrainIcons[i] == name) return true;
    return false;
}

struct AtlasSlot { int row, col; };

static const AtlasSlot kFreeSlots[] = {
    {10,1},{10,2},{10,6},
    {11,1},{11,2},{11,3},{11,4},{11,5},{11,6},
    {12,1},{12,2},{12,3},{12,4},{12,5},{12,6},{12,8},
    {14,5},{14,6},
};
static const int kFreeSlotsCount = (int)(sizeof(kFreeSlots)/sizeof(kFreeSlots[0]));
static int sNextFreeSlot = 0;

static bool AllocSlot(int& row, int& col) {
    if (sNextFreeSlot >= kFreeSlotsCount) {
        printf("[AxoLoader] ERROR: No more free item texture slots!\n");
        return false;
    }
    row = kFreeSlots[sNextFreeSlot].row;
    col = kFreeSlots[sNextFreeSlot].col;
    sNextFreeSlot++;
    return true;
}

static const AtlasSlot kFreeTerrainSlots[] = {
    {20,0},{20,1},{20,2},{20,3},{20,4},{20,5},{20,6},{20,7},{20,8},{20,9},{20,10},{20,11},{20,12},{20,13},{20,14},{20,15},
    {21,0},{21,1},{21,2},{21,3},{21,4},{21,5},{21,6},{21,7},{21,8},{21,9},{21,10},{21,11},{21,12},{21,13},{21,14},{21,15},
    {22,0},{22,1},{22,2},{22,3},{22,4},{22,5},{22,6},{22,7},{22,8},{22,9},{22,10},{22,11},{22,12},{22,13},{22,14},{22,15},
    {23,0},{23,1},{23,2},{23,3},{23,4},{23,5},{23,6},{23,7},{23,8},{23,9},{23,10},{23,11},{23,12},{23,13},{23,14},{23,15},
    {24,0},{24,1},{24,2},{24,3},{24,4},{24,5},{24,6},{24,7},{24,8},{24,9},{24,10},{24,11},{24,12},{24,13},{24,14},{24,15},
    {25,0},{25,1},{25,2},{25,3},{25,4},{25,5},{25,6},{25,7},{25,8},{25,9},{25,10},{25,11},{25,12},{25,13},{25,14},{25,15},
    {26,0},{26,1},{26,2},{26,3},{26,4},{26,5},{26,6},{26,7},{26,8},{26,9},{26,10},{26,11},{26,12},{26,13},{26,14},{26,15},
    {27,0},{27,1},{27,2},{27,3},{27,4},{27,5},{27,6},{27,7},{27,8},{27,9},{27,10},{27,11},{27,12},{27,13},{27,14},{27,15},
    {28,0},{28,1},{28,2},{28,3},{28,4},{28,5},{28,6},{28,7},{28,8},{28,9},{28,10},{28,11},{28,12},{28,13},{28,14},{28,15},
    {29,0},{29,1},{29,2},{29,3},{29,4},{29,5},{29,6},{29,7},{29,8},{29,9},{29,10},{29,11},{29,12},{29,13},{29,14},{29,15},
    {30,0},{30,1},{30,2},{30,3},{30,4},{30,5},{30,6},{30,7},{30,8},{30,9},{30,10},{30,11},{30,12},{30,13},{30,14},{30,15},
    {31,0},{31,1},{31,2},{31,3},{31,4},{31,5},{31,6},{31,7},{31,8},{31,9},{31,10},{31,11},{31,12},{31,13},{31,14},{31,15},
};
static const int kFreeTerrainSlotsCount = (int)(sizeof(kFreeTerrainSlots)/sizeof(kFreeTerrainSlots[0]));
static int sNextFreeTerrainSlot = 0;

static bool AllocTerrainSlot(int& row, int& col) {
    if (sNextFreeTerrainSlot >= kFreeTerrainSlotsCount) {
        printf("[AxoLoader] ERROR: No more free terrain texture slots!\n");
        return false;
    }
    row = kFreeTerrainSlots[sNextFreeTerrainSlot].row;
    col = kFreeTerrainSlots[sNextFreeTerrainSlot].col;
    sNextFreeTerrainSlot++;
    return true;
}

static ULONG_PTR sGdiplusToken = 0;

static void GdiplusStart() {
    Gdiplus::GdiplusStartupInput inp;
    Gdiplus::GdiplusStartup(&sGdiplusToken, &inp, NULL);
}

static void GdiplusStop() {
    if (sGdiplusToken) {
        Gdiplus::GdiplusShutdown(sGdiplusToken);
        sGdiplusToken = 0;
    }
}

static bool PasteIntoAtlas(const std::wstring& atlasPath, const std::wstring& srcPath, int row, int col) {
    if (GetFileAttributesW(atlasPath.c_str()) == INVALID_FILE_ATTRIBUTES) return false;
    if (GetFileAttributesW(srcPath.c_str())   == INVALID_FILE_ATTRIBUTES) return false;

    const int SLOT_SIZE = 16;

    Gdiplus::Bitmap* atlas = Gdiplus::Bitmap::FromFile(atlasPath.c_str());
    if (!atlas || atlas->GetLastStatus() != Gdiplus::Ok) { delete atlas; return false; }

    Gdiplus::Bitmap* src = Gdiplus::Bitmap::FromFile(srcPath.c_str());
    if (!src || src->GetLastStatus() != Gdiplus::Ok) { delete atlas; delete src; return false; }

    Gdiplus::Bitmap* resized = new Gdiplus::Bitmap(SLOT_SIZE, SLOT_SIZE, PixelFormat32bppARGB);
    {
        Gdiplus::Graphics g(resized);
        g.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
        g.DrawImage(src, 0, 0, SLOT_SIZE, SLOT_SIZE);
    }
    delete src;

    {
        Gdiplus::Graphics g(atlas);
        g.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
        g.DrawImage(resized, col * SLOT_SIZE, row * SLOT_SIZE, SLOT_SIZE, SLOT_SIZE);
    }
    delete resized;

    CLSID pngClsid;
    bool foundEncoder = false;
    {
        UINT num = 0, size = 0;
        Gdiplus::GetImageEncodersSize(&num, &size);
        std::vector<BYTE> buf(size);
        Gdiplus::ImageCodecInfo* codecs = (Gdiplus::ImageCodecInfo*)buf.data();
        Gdiplus::GetImageEncoders(num, size, codecs);
        for (UINT i = 0; i < num; ++i) {
            if (wcscmp(codecs[i].MimeType, L"image/png") == 0) {
                pngClsid = codecs[i].Clsid;
                foundEncoder = true;
                break;
            }
        }
    }
    if (!foundEncoder) { delete atlas; return false; }

    std::wstring tempPath = atlasPath + L".tmp";
    Gdiplus::Status st = atlas->Save(tempPath.c_str(), &pngClsid);
    delete atlas;
    if (st != Gdiplus::Ok) { DeleteFileW(tempPath.c_str()); return false; }

    DeleteFileW(atlasPath.c_str());
    return MoveFileW(tempPath.c_str(), atlasPath.c_str()) != 0;
}

void AxoModLoader_PreInit(const char* modsDir) {
    AxoLoader_CreateConsole();
    GdiplusStart();

    const std::wstring atlasPath   = L"Common\\res\\TitleUpdate\\res\\items.png";
    const std::wstring atlasBackup = L"Common\\res\\TitleUpdate\\res\\items.orig.png";

    if (GetFileAttributesW(atlasBackup.c_str()) == INVALID_FILE_ATTRIBUTES)
        CopyFileW(atlasPath.c_str(), atlasBackup.c_str(), FALSE);
    else
        CopyFileW(atlasBackup.c_str(), atlasPath.c_str(), FALSE);

    std::vector<std::string> zips = ListZips(modsDir);
    printf("[AxoLoader] PreInit: %u ZIP(s) found\n", (unsigned)zips.size());

    for (size_t i = 0; i < zips.size(); ++i) {
        std::string fn    = GetFilename(zips[i]);
        std::string stem  = GetStem(fn);
        std::string tmpDir = PathJoin(PathJoin(modsDir, "_tmp"), stem);

        RemoveAll(tmpDir);
        MakeDirs(tmpDir);
        if (!ExtractZipToDir(zips[i], tmpDir)) {
            printf("[AxoLoader] ExtractZip FAILED: %s\n", fn.c_str());
            continue;
        }

        std::string texDir = PathJoin(PathJoin(tmpDir, "textures"), "items");
        if (!PathExists(texDir)) continue;

        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA(PathJoin(texDir, "*.png").c_str(), &fd);
        if (hFind == INVALID_HANDLE_VALUE) continue;

        do {
            std::string iconKey = GetStem(fd.cFileName);
            std::string pngPath = PathJoin(texDir, fd.cFileName);
            int row, col;
            if (!AllocSlot(row, col)) { FindClose(hFind); goto done; }
            std::wstring wPngPath(pngPath.begin(), pngPath.end());
            if (PasteIntoAtlas(atlasPath, wPngPath, row, col)) {
                std::wstring wIconKey(iconKey.begin(), iconKey.end());
                PreStitchedTextureMap::AxoQueueIcon(wIconKey, row, col);
                printf("[AxoLoader] Queued item icon \"%s\" -> slot(%d,%d)\n", iconKey.c_str(), row, col);
            }
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }

done:
    GdiplusStop();

    const std::wstring terrainPath   = L"Common\\res\\TitleUpdate\\res\\terrain.png";
    const std::wstring terrainBackup = L"Common\\res\\TitleUpdate\\res\\terrain.orig.png";

    if (GetFileAttributesW(terrainBackup.c_str()) == INVALID_FILE_ATTRIBUTES)
        CopyFileW(terrainPath.c_str(), terrainBackup.c_str(), FALSE);
    else
        CopyFileW(terrainBackup.c_str(), terrainPath.c_str(), FALSE);

    GdiplusStart();

    for (size_t i = 0; i < zips.size(); ++i) {
        std::string stem   = GetStem(GetFilename(zips[i]));
        std::string tmpDir = PathJoin(PathJoin(modsDir, "_tmp"), stem);
        std::string texDir = PathJoin(PathJoin(tmpDir, "textures"), "terrain");
        if (!PathExists(texDir)) continue;

        WIN32_FIND_DATAA fd2;
        HANDLE hFind2 = FindFirstFileA(PathJoin(texDir, "*.png").c_str(), &fd2);
        if (hFind2 == INVALID_HANDLE_VALUE) continue;

        do {
            std::string iconKey = GetStem(fd2.cFileName);
            std::string pngPath = PathJoin(texDir, fd2.cFileName);
            int row, col;
            if (!AllocTerrainSlot(row, col)) { FindClose(hFind2); goto done_terrain; }
            std::wstring wPngPath(pngPath.begin(), pngPath.end());
            if (PasteIntoAtlas(terrainPath, wPngPath, row, col)) {
                std::wstring wIconKey(iconKey.begin(), iconKey.end());
                PreStitchedTextureMap::AxoQueueTerrainIcon(wIconKey, row, col);
                sQueuedTerrainIcons.push_back(wIconKey);
                printf("[AxoLoader] Queued terrain icon \"%s\" -> slot(%d,%d)\n", iconKey.c_str(), row, col);
            }
        } while (FindNextFileA(hFind2, &fd2));
        FindClose(hFind2);
    }

done_terrain:
    GdiplusStop();

    AxoAPITable* apiTable = AxoAPI_GetTable();

    for (size_t i = 0; i < zips.size(); ++i) {
        std::string stem   = GetStem(GetFilename(zips[i]));
        std::string tmpDir = PathJoin(PathJoin(modsDir, "_tmp"), stem);
        if (!PathExists(tmpDir)) continue;

        std::string modId   = ReadModId(PathJoin(tmpDir, "manifest.json"));
        std::string dllPath = FindDLL(tmpDir);
        if (dllPath.empty()) {
            printf("[AxoLoader] No DLL in %s\n", stem.c_str());
            continue;
        }

        HMODULE hMod = LoadLibraryA(dllPath.c_str());
        if (!hMod) {
            printf("[AxoLoader] LoadLibrary FAILED err=%lu\n", GetLastError());
            continue;
        }

        typedef void(*FnModEntry)(AxoMod*, AxoAPITable*);
        FnModEntry fnEntry = (FnModEntry)GetProcAddress(hMod, "ModEntry");
        if (!fnEntry) {
            printf("[AxoLoader] ModEntry not found in %s\n", stem.c_str());
            FreeLibrary(hMod);
            continue;
        }

        LoadedMod lm;
        lm.id           = modId;
        lm.handle       = hMod;
        lm.fnModEntry   = fnEntry;
        lm.fnOnTick     = (LoadedMod::FnOnTick)    GetProcAddress(hMod, "OnTick");
        lm.fnOnShutdown = (LoadedMod::FnOnShutdown) GetProcAddress(hMod, "OnShutdown");
        sLoadedMods.push_back(lm);

        AxoMod modInfo = {};
        modInfo.id = modId.c_str();
        printf("[AxoLoader] Calling ModEntry: %s\n", modId.c_str());
        fnEntry(&modInfo, apiTable);
    }

    AxoAPI_FlushRegistrations();
    printf("[AxoLoader] PreInit complete. Mods: %u\n", (unsigned)sLoadedMods.size());
}

void AxoModLoader_Tick() {
    for (size_t i = 0; i < sLoadedMods.size(); ++i)
        if (sLoadedMods[i].fnOnTick) sLoadedMods[i].fnOnTick();
}

void AxoModLoader_Shutdown() {
    printf("[AxoLoader] Shutdown (%u mod(s)).\n", (unsigned)sLoadedMods.size());
    for (size_t i = 0; i < sLoadedMods.size(); ++i) {
        if (sLoadedMods[i].fnOnShutdown) sLoadedMods[i].fnOnShutdown();
        FreeLibrary(sLoadedMods[i].handle);
    }
    sLoadedMods.clear();
    gAxoAPI = NULL;
}
