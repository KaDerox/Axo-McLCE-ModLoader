#include "stdafx.h"

#include "AxoModLoader.h"
#include "AxoAPI.h"
#include "AxoModelLoader.h"
#include "..\..\Minecraft.World\Recipes.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#pragma comment(lib, "ole32.lib")

#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <cstdio>

struct LoadedMod {
    std::string id;
    HMODULE     handle;
    bool        faulted;

    typedef void(*FnModEntry)   (AxoMod*, AxoAPITable*);
    typedef void(*FnOnTick)     ();
    typedef void(*FnOnShutdown) ();

    FnModEntry   fnModEntry;
    FnOnTick     fnOnTick;
    FnOnShutdown fnOnShutdown;

    LoadedMod() : handle(NULL), faulted(false), fnModEntry(NULL), fnOnTick(NULL), fnOnShutdown(NULL) {}
};

static std::vector<LoadedMod>    sLoadedMods;
static std::vector<std::wstring> sQueuedTerrainIcons;
static std::unordered_map<std::wstring, Icon*> sTerrainIconCache;

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

// ══════════════════════════════════════════════════════════════════════════════
//  Minimal inflate (RFC 1951 raw deflate) — replaces Shell COM ZIP extraction
//  so that mod loading works on Wine / Linux where IShellDispatch is broken.
// ══════════════════════════════════════════════════════════════════════════════

struct InflBitReader {
    const unsigned char* src;
    size_t len, pos;
    unsigned buf;
    int cnt;
    InflBitReader(const unsigned char* s, size_t l)
        : src(s), len(l), pos(0), buf(0), cnt(0) {}

    unsigned read(int n) {
        while (cnt < n) {
            if (pos < len) buf |= (unsigned)src[pos++] << cnt;
            cnt += 8;
        }
        unsigned v = buf & ((1u << n) - 1);
        buf >>= n; cnt -= n;
        return v;
    }
    void byteAlign() { buf = 0; cnt = 0; }
};

struct InflHuff {
    unsigned short counts[16];
    unsigned short symbols[288];

    void build(const unsigned char* lens, int n) {
        memset(counts, 0, sizeof(counts));
        for (int i = 0; i < n; i++) counts[lens[i]]++;
        counts[0] = 0;
        unsigned short off[16];
        off[0] = 0; off[1] = 0;
        for (int i = 1; i < 15; i++) off[i + 1] = off[i] + counts[i];
        for (int i = 0; i < n; i++)
            if (lens[i]) symbols[off[lens[i]]++] = (unsigned short)i;
    }

    int decode(InflBitReader& br) const {
        int code = 0, first = 0, idx = 0;
        for (int len = 1; len <= 15; len++) {
            code |= (int)br.read(1);
            int c = counts[len];
            if (code - first < c) return symbols[idx + (code - first)];
            idx += c;
            first = (first + c) << 1;
            code <<= 1;
        }
        return -1;
    }
};

static const unsigned short kLenBase[29]  = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
static const unsigned char  kLenExtra[29] = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
static const unsigned short kDstBase[30]  = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
static const unsigned char  kDstExtra[30] = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
static const unsigned char  kCLOrder[19]  = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};

static bool RawInflate(const unsigned char* src, size_t srcLen,
                       unsigned char* dst, size_t dstLen) {
    InflBitReader br(src, srcLen);
    size_t wp = 0;
    int bfinal;

    do {
        bfinal = (int)br.read(1);
        unsigned btype = br.read(2);

        if (btype == 0) {
            // Stored block
            br.byteAlign();
            unsigned len = br.read(16);
            br.read(16); // nlen
            for (unsigned i = 0; i < len; i++) {
                if (wp >= dstLen) return false;
                dst[wp++] = (unsigned char)br.read(8);
            }
        } else if (btype == 1 || btype == 2) {
            InflHuff lit, dist;

            if (btype == 1) {
                // Fixed Huffman tables
                unsigned char ll[288], dl[30];
                for (int i =   0; i <= 143; i++) ll[i] = 8;
                for (int i = 144; i <= 255; i++) ll[i] = 9;
                for (int i = 256; i <= 279; i++) ll[i] = 7;
                for (int i = 280; i <= 287; i++) ll[i] = 8;
                lit.build(ll, 288);
                for (int i = 0; i < 30; i++) dl[i] = 5;
                dist.build(dl, 30);
            } else {
                // Dynamic Huffman tables
                unsigned hlit  = br.read(5) + 257;
                unsigned hdist = br.read(5) + 1;
                unsigned hclen = br.read(4) + 4;
                unsigned char cl[19] = {};
                for (unsigned i = 0; i < hclen; i++)
                    cl[kCLOrder[i]] = (unsigned char)br.read(3);
                InflHuff ct;
                ct.build(cl, 19);

                unsigned char lens[288 + 32] = {};
                unsigned total = hlit + hdist;
                for (unsigned i = 0; i < total; ) {
                    int s = ct.decode(br);
                    if (s < 0) return false;
                    if (s < 16) {
                        lens[i++] = (unsigned char)s;
                    } else if (s == 16) {
                        unsigned r = br.read(2) + 3;
                        unsigned char prev = i > 0 ? lens[i - 1] : 0;
                        while (r-- && i < total) lens[i++] = prev;
                    } else if (s == 17) {
                        unsigned r = br.read(3) + 3;
                        while (r-- && i < total) lens[i++] = 0;
                    } else {
                        unsigned r = br.read(7) + 11;
                        while (r-- && i < total) lens[i++] = 0;
                    }
                }
                lit.build(lens, hlit);
                dist.build(lens + hlit, hdist);
            }

            // Decode literal/length stream
            for (;;) {
                int sym = lit.decode(br);
                if (sym < 0) return false;
                if (sym < 256) {
                    if (wp >= dstLen) return false;
                    dst[wp++] = (unsigned char)sym;
                } else if (sym == 256) {
                    break;
                } else {
                    int li = sym - 257;
                    if (li < 0 || li >= 29) return false;
                    unsigned length   = kLenBase[li] + br.read(kLenExtra[li]);
                    int di = dist.decode(br);
                    if (di < 0 || di >= 30) return false;
                    unsigned distance = kDstBase[di] + br.read(kDstExtra[di]);
                    if (wp < distance) return false;
                    for (unsigned i = 0; i < length; i++) {
                        if (wp >= dstLen) return false;
                        dst[wp] = dst[wp - distance];
                        wp++;
                    }
                }
            }
        } else {
            return false; // reserved block type
        }
    } while (!bfinal);

    return true;
}

// ── ZIP format helpers ──────────────────────────────────────────────────────
static unsigned short ZipU16(const unsigned char* p) {
    return p[0] | ((unsigned short)p[1] << 8);
}
static unsigned int ZipU32(const unsigned char* p) {
    return p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

static bool ExtractZipToDir(const std::string& zipPath, const std::string& destDir) {
    MakeDirs(destDir);

    // Read entire ZIP into memory
    HANDLE hFile = CreateFileA(zipPath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize < 22) {
        CloseHandle(hFile);
        return false;
    }
    std::vector<unsigned char> zip(fileSize);
    DWORD rd = 0;
    ReadFile(hFile, zip.data(), fileSize, &rd, NULL);
    CloseHandle(hFile);
    if (rd != fileSize) return false;

    // Find End of Central Directory (sig 0x06054b50)
    int eocd = -1;
    for (int i = (int)fileSize - 22; i >= 0 && i >= (int)fileSize - 65557; --i) {
        if (ZipU32(&zip[i]) == 0x06054b50) { eocd = i; break; }
    }
    if (eocd < 0) {
        printf("[AxoLoader] ZIP: no EOCD in %s\n", zipPath.c_str());
        return false;
    }

    unsigned short numEntries = ZipU16(&zip[eocd + 10]);
    unsigned int   cdOffset   = ZipU32(&zip[eocd + 16]);

    int extracted = 0;
    unsigned int pos = cdOffset;

    for (int e = 0; e < numEntries; e++) {
        if (pos + 46 > fileSize || ZipU32(&zip[pos]) != 0x02014b50) break;

        unsigned short method     = ZipU16(&zip[pos + 10]);
        unsigned int   compSize   = ZipU32(&zip[pos + 20]);
        unsigned int   uncompSize = ZipU32(&zip[pos + 24]);
        unsigned short nameLen    = ZipU16(&zip[pos + 28]);
        unsigned short extraLen   = ZipU16(&zip[pos + 30]);
        unsigned short commentLen = ZipU16(&zip[pos + 32]);
        unsigned int   localOff   = ZipU32(&zip[pos + 42]);

        std::string name((const char*)&zip[pos + 46], nameLen);
        pos += 46 + nameLen + extraLen + commentLen;

        // Build output path
        std::string out = destDir;
        if (!out.empty() && out.back() != '\\') out += '\\';
        for (char ch : name) out += (ch == '/') ? '\\' : ch;

        bool isDir = !name.empty() && (name.back() == '/' || name.back() == '\\');
        if (isDir) { MakeDirs(out); continue; }

        // Create parent directories
        size_t sep = out.find_last_of('\\');
        if (sep != std::string::npos) MakeDirs(out.substr(0, sep));

        // Locate file data via local header
        if (localOff + 30 > fileSize || ZipU32(&zip[localOff]) != 0x04034b50) continue;
        unsigned short lNameLen  = ZipU16(&zip[localOff + 26]);
        unsigned short lExtraLen = ZipU16(&zip[localOff + 28]);
        unsigned int   dataOff   = localOff + 30 + lNameLen + lExtraLen;
        if (dataOff + compSize > fileSize) continue;

        bool wrote = false;

        if (method == 0) {
            // STORED
            HANDLE hOut = CreateFileA(out.c_str(), GENERIC_WRITE, 0, NULL,
                                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hOut != INVALID_HANDLE_VALUE) {
                if (uncompSize > 0) {
                    DWORD w; WriteFile(hOut, &zip[dataOff], uncompSize, &w, NULL);
                }
                CloseHandle(hOut);
                wrote = true;
            }
        } else if (method == 8) {
            // DEFLATED
            if (uncompSize == 0) {
                HANDLE hOut = CreateFileA(out.c_str(), GENERIC_WRITE, 0, NULL,
                                          CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hOut != INVALID_HANDLE_VALUE) CloseHandle(hOut);
                wrote = true;
            } else {
                std::vector<unsigned char> inflated(uncompSize);
                if (RawInflate(&zip[dataOff], compSize, inflated.data(), uncompSize)) {
                    HANDLE hOut = CreateFileA(out.c_str(), GENERIC_WRITE, 0, NULL,
                                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hOut != INVALID_HANDLE_VALUE) {
                        DWORD w; WriteFile(hOut, inflated.data(), uncompSize, &w, NULL);
                        CloseHandle(hOut);
                        wrote = true;
                    }
                } else {
                    printf("[AxoLoader] ZIP: inflate failed: %s\n", name.c_str());
                }
            }
        } else {
            printf("[AxoLoader] ZIP: unsupported method %d: %s\n", method, name.c_str());
        }

        if (wrote) extracted++;
    }

    printf("[AxoLoader] ZIP: %d file(s) extracted from %s\n",
           extracted, GetFilename(zipPath).c_str());
    return extracted > 0;
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

static int ReadApiVersion(const std::string& manifestPath) {
    std::ifstream f(manifestPath.c_str());
    if (!f) return 1;
    std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    size_t pos = json.find("\"api_version\"");
    if (pos == std::string::npos) return 1;  // absent = v1 (old mod)
    size_t colon = json.find(':', pos);
    if (colon == std::string::npos) return 1;
    size_t i = colon + 1;
    while (i < json.size() && (json[i] == ' ' || json[i] == '\t')) i++;
    if (i >= json.size()) return 1;
    return atoi(json.c_str() + i);
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
    AxoAPI_FlushBiomeRegistrations();
    AxoAPI_FlushCropRegistrations();
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
#include "..\Minecraft.World\Icon.h"

bool AxoModLoader_IsTerrainIconQueued(const std::wstring& name) {
    for (size_t i = 0; i < sQueuedTerrainIcons.size(); ++i)
        if (sQueuedTerrainIcons[i] == name) return true;
    return false;
}

void AxoModLoader_CacheTerrainIcon(const std::wstring& name, Icon* icon) {
    sTerrainIconCache[name] = icon;
}

Icon* AxoModLoader_GetTerrainIcon(const std::wstring& name) {
    auto it = sTerrainIconCache.find(name);
    if (it != sTerrainIconCache.end()) return it->second;
    return nullptr;
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

// ── SEH wrappers (no C++ objects allowed in __try functions) ──────────────────
static DWORD SafeCallModEntry(LoadedMod::FnModEntry fn, AxoMod* mod, AxoAPITable* api) {
    __try {
        fn(mod, api);
        return 0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}

static DWORD SafeCallOnTick(LoadedMod::FnOnTick fn) {
    __try {
        fn();
        return 0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}

static DWORD SafeCallOnShutdown(LoadedMod::FnOnShutdown fn) {
    __try {
        fn();
        return 0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
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

    for (size_t i = 0; i < zips.size(); ++i) {
        std::string stem      = GetStem(GetFilename(zips[i]));
        std::string tmpDir    = PathJoin(PathJoin(modsDir, "_tmp"), stem);
        std::string mdlDir    = PathJoin(PathJoin(tmpDir, "models"), "blocks");
        if (!PathExists(mdlDir)) continue;
        WIN32_FIND_DATAA fdm;
        HANDLE hFindM = FindFirstFileA(PathJoin(mdlDir, "*.json").c_str(), &fdm);
        if (hFindM == INVALID_HANDLE_VALUE) continue;
        do {
            std::string modelName = GetStem(fdm.cFileName);
            std::string jsonPath  = PathJoin(mdlDir, fdm.cFileName);
            std::ifstream f(jsonPath.c_str());
            if (!f) continue;
            std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            AxoModelLoader_StoreJSON(modelName, content);
            printf("[AxoLoader] Stored model JSON: %s\n", modelName.c_str());
        } while (FindNextFileA(hFindM, &fdm));
        FindClose(hFindM);
    }

    AxoAPITable* apiTableV2 = AxoAPI_GetTable();
    AxoAPITable* apiTableV1 = AxoAPI_GetTableV1();

    for (size_t i = 0; i < zips.size(); ++i) {
        std::string stem   = GetStem(GetFilename(zips[i]));
        std::string tmpDir = PathJoin(PathJoin(modsDir, "_tmp"), stem);
        printf("[AxoLoader] [%u/%u] Processing mod: %s\n", (unsigned)(i+1), (unsigned)zips.size(), stem.c_str());
        fflush(stdout);

        if (!PathExists(tmpDir)) {
            printf("[AxoLoader]   tmpDir not found, skip\n"); fflush(stdout);
            continue;
        }

        std::string manifestPath = PathJoin(tmpDir, "manifest.json");
        std::string modId   = ReadModId(manifestPath);
        int         apiVer  = ReadApiVersion(manifestPath);
        printf("[AxoLoader]   mod_id = \"%s\"  api_version = %d\n", modId.c_str(), apiVer); fflush(stdout);

        AxoAPITable* apiTable = (apiVer >= 2) ? apiTableV2 : apiTableV1;
        if (apiVer < 2) {
            printf("[AxoLoader]   Using V1 compatibility layer (old ABI)\n"); fflush(stdout);
        }

        std::string dllPath = FindDLL(tmpDir);
        if (dllPath.empty()) {
            printf("[AxoLoader]   No DLL in %s\n", stem.c_str()); fflush(stdout);
            continue;
        }
        printf("[AxoLoader]   DLL: %s\n", dllPath.c_str()); fflush(stdout);

        HMODULE hMod = LoadLibraryA(dllPath.c_str());
        if (!hMod) {
            DWORD err = GetLastError();
            printf("[AxoLoader]   LoadLibrary FAILED err=%lu\n", err); fflush(stdout);
            if (apiVer < 2) {
                printf("[AxoLoader]   (V1 mod may need Visual C++ Debug Runtime — install Visual Studio or recompile with /MT)\n");
                fflush(stdout);
            }
            continue;
        }
        printf("[AxoLoader]   LoadLibrary OK (0x%p)\n", (void*)hMod); fflush(stdout);

        typedef void(*FnModEntry)(AxoMod*, AxoAPITable*);
        FnModEntry fnEntry = (FnModEntry)GetProcAddress(hMod, "ModEntry");
        if (!fnEntry) {
            printf("[AxoLoader]   ModEntry export not found in %s\n", stem.c_str()); fflush(stdout);
            FreeLibrary(hMod);
            continue;
        }
        printf("[AxoLoader]   ModEntry at 0x%p\n", (void*)fnEntry); fflush(stdout);

        LoadedMod::FnOnTick     fnTick = (LoadedMod::FnOnTick)    GetProcAddress(hMod, "OnTick");
        LoadedMod::FnOnShutdown fnShut = (LoadedMod::FnOnShutdown) GetProcAddress(hMod, "OnShutdown");
        printf("[AxoLoader]   OnTick=%p  OnShutdown=%p\n", (void*)fnTick, (void*)fnShut); fflush(stdout);

        LoadedMod lm;
        lm.id           = modId;
        lm.handle       = hMod;
        lm.fnModEntry   = fnEntry;
        lm.fnOnTick     = fnTick;
        lm.fnOnShutdown = fnShut;
        sLoadedMods.push_back(lm);

        AxoMod modInfo = {};
        modInfo.id = modId.c_str();
        printf("[AxoLoader] Calling ModEntry: %s (API v%d)\n", modId.c_str(), apiVer); fflush(stdout);

        DWORD exCode = SafeCallModEntry(fnEntry, &modInfo, apiTable);
        printf("[AxoLoader]   ModEntry returned (exCode=0x%08lX)\n", exCode); fflush(stdout);

        if (exCode != 0) {
            printf("[AxoLoader] CRASH in ModEntry of '%s' (exception 0x%08lX) — mod disabled!\n",
                   modId.c_str(), exCode); fflush(stdout);
            sLoadedMods.back().faulted = true;
            sLoadedMods.back().fnOnTick = NULL;
            sLoadedMods.back().fnOnShutdown = NULL;
        }
        printf("[AxoLoader]   Done with %s\n", stem.c_str()); fflush(stdout);
    }

    printf("[AxoLoader] All mods loaded. Calling FlushRegistrations...\n"); fflush(stdout);
    AxoAPI_FlushRegistrations();
    printf("[AxoLoader] PreInit complete. Mods: %u\n", (unsigned)sLoadedMods.size()); fflush(stdout);
}

void AxoModLoader_Tick() {
    for (size_t i = 0; i < sLoadedMods.size(); ++i) {
        if (sLoadedMods[i].faulted || !sLoadedMods[i].fnOnTick) continue;
        DWORD exCode = SafeCallOnTick(sLoadedMods[i].fnOnTick);
        if (exCode != 0) {
            printf("[AxoLoader] CRASH in OnTick of '%s' (exception 0x%08lX) — mod disabled!\n",
                   sLoadedMods[i].id.c_str(), exCode);
            sLoadedMods[i].faulted = true;
            sLoadedMods[i].fnOnTick = NULL;
        }
    }
}

void AxoModLoader_Shutdown() {
    printf("[AxoLoader] Shutdown (%u mod(s)).\n", (unsigned)sLoadedMods.size());
    for (size_t i = 0; i < sLoadedMods.size(); ++i) {
        if (sLoadedMods[i].fnOnShutdown && !sLoadedMods[i].faulted) {
            DWORD exCode = SafeCallOnShutdown(sLoadedMods[i].fnOnShutdown);
            if (exCode != 0) {
                printf("[AxoLoader] CRASH in OnShutdown of '%s' (0x%08lX) — ignored.\n",
                       sLoadedMods[i].id.c_str(), exCode);
            }
        }
        FreeLibrary(sLoadedMods[i].handle);
    }
    sLoadedMods.clear();
    gAxoAPI = NULL;
}