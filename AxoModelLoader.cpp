#include "stdafx.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <cstdio>
#include <cstring>

#include "..\..\Minecraft.World\IconRegister.h"
#include "..\..\Minecraft.World\Icon.h"
#include "..\..\Minecraft.World\Tile.h"
#include "..\..\Minecraft.World\Level.h"
#include "..\Tesselator.h"
#include "..\TileRenderer.h"
#include "AxoModLoader.h"
#include "AxoModelLoader.h"

static std::unordered_map<int, AxoBlockModel>       sModels;
static std::unordered_map<std::string, std::string> sStoredJSON;

static std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n\"");
    size_t b = s.find_last_not_of(" \t\r\n\"");
    if (a == std::string::npos) return "";
    return s.substr(a, b - a + 1);
}

static std::string GetJSONValue(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos >= json.size()) return "";
    if (json[pos] == '"') {
        size_t end = json.find('"', pos + 1);
        if (end == std::string::npos) return "";
        return json.substr(pos + 1, end - pos - 1);
    }
    if (json[pos] == '[') {
        size_t end = json.find(']', pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos + 1);
    }
    size_t end = json.find_first_of(",}\n", pos);
    return Trim(json.substr(pos, end - pos));
}

static bool ParseArray3(const std::string& arr, float out[3]) {
    const char* p = arr.c_str();
    while (*p && *p != '[') p++;
    if (!*p) return false;
    p++;
    for (int i = 0; i < 3; i++) {
        while (*p && (*p == ' ' || *p == ',')) p++;
        out[i] = (float)atof(p) / 16.0f;
        while (*p && *p != ',' && *p != ']') p++;
    }
    return true;
}

static std::string ExtractFaceBlock(const std::string& json, const std::string& faceName) {
    std::string search = "\"" + faceName + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find('{', pos);
    if (pos == std::string::npos) return "";
    int depth = 0;
    size_t end = pos;
    for (size_t i = pos; i < json.size(); i++) {
        if (json[i] == '{') depth++;
        else if (json[i] == '}') { depth--; if (depth == 0) { end = i; break; } }
    }
    return json.substr(pos, end - pos + 1);
}

static std::vector<std::string> ExtractElementBlocks(const std::string& json) {
    std::vector<std::string> result;
    size_t pos = json.find("\"elements\"");
    if (pos == std::string::npos) return result;
    pos = json.find('[', pos);
    if (pos == std::string::npos) return result;
    pos++;
    while (true) {
        pos = json.find('{', pos);
        if (pos == std::string::npos) break;
        int depth = 0;
        size_t end = pos;
        for (size_t i = pos; i < json.size(); i++) {
            if (json[i] == '{') depth++;
            else if (json[i] == '}') { depth--; if (depth == 0) { end = i; break; } }
        }
        result.push_back(json.substr(pos, end - pos + 1));
        pos = end + 1;
        size_t next = json.find_first_not_of(" \t\r\n,", pos);
        if (next == std::string::npos || json[next] == ']') break;
    }
    return result;
}

static const char* kFaceNames[6] = { "down", "up", "north", "south", "west", "east" };
static const float kFaceBrightness[6] = { 0.5f, 1.0f, 0.8f, 0.8f, 0.6f, 0.6f };

static std::string NormalizeTextureName(const std::string& tex) {
    std::string t = tex;
    size_t colon = t.find(':');
    if (colon != std::string::npos) t = t.substr(colon + 1);
    size_t slash = t.rfind('/');
    if (slash != std::string::npos) t = t.substr(slash + 1);
    slash = t.rfind('\\');
    if (slash != std::string::npos) t = t.substr(slash + 1);
    return t;
}

static std::unordered_map<std::string, std::string> ParseTexturesMap(const std::string& json) {
    std::unordered_map<std::string, std::string> texMap;
    size_t pos = json.find("\"textures\"");
    if (pos == std::string::npos) return texMap;
    pos = json.find('{', pos);
    if (pos == std::string::npos) return texMap;
    size_t end = json.find('}', pos);
    if (end == std::string::npos) return texMap;
    std::string block = json.substr(pos + 1, end - pos - 1);
    size_t p = 0;
    while (p < block.size()) {
        size_t q1 = block.find('"', p);
        if (q1 == std::string::npos) break;
        size_t q2 = block.find('"', q1 + 1);
        if (q2 == std::string::npos) break;
        std::string key = block.substr(q1 + 1, q2 - q1 - 1);
        size_t col = block.find(':', q2);
        if (col == std::string::npos) break;
        size_t v1 = block.find('"', col);
        if (v1 == std::string::npos) break;
        size_t v2 = block.find('"', v1 + 1);
        if (v2 == std::string::npos) break;
        std::string val = block.substr(v1 + 1, v2 - v1 - 1);
        texMap[key] = val;
        p = v2 + 1;
    }
    return texMap;
}

static AxoBlockModel ParseModel(const std::string& jsonContent) {
    AxoBlockModel model;
    auto texMap = ParseTexturesMap(jsonContent);
    for (auto& kv : texMap) kv.second = NormalizeTextureName(kv.second);
    auto elementBlocks = ExtractElementBlocks(jsonContent);
    for (const auto& elemJson : elementBlocks) {
        AxoModelElement elem;
        memset(&elem, 0, sizeof(elem));
        std::string fromStr = GetJSONValue(elemJson, "from");
        std::string toStr   = GetJSONValue(elemJson, "to");
        if (!ParseArray3(fromStr, elem.from) || !ParseArray3(toStr, elem.to)) continue;
        for (int f = 0; f < 6; f++) {
            std::string faceBlock = ExtractFaceBlock(elemJson, kFaceNames[f]);
            if (faceBlock.empty()) continue;
            std::string tex = GetJSONValue(faceBlock, "texture");
            if (tex.empty()) continue;
            if (tex[0] == '#') tex = tex.substr(1);
            if (tex == "missing") continue;
            auto it = texMap.find(tex);
            if (it != texMap.end()) tex = it->second;
            tex = NormalizeTextureName(tex);
            if (tex.empty() || tex == "missing") continue;
            elem.faces[f].textureName = std::wstring(tex.begin(), tex.end());
            elem.faces[f].present     = true;
        }
        model.elements.push_back(elem);
    }
    return model;
}

void AxoModelLoader_StoreJSON(const std::string& modelName, const std::string& jsonContent) {
    sStoredJSON[modelName] = jsonContent;
}

void AxoModelLoader_Register(int blockId, const std::string& modelName) {
    auto it = sStoredJSON.find(modelName);
    if (it == sStoredJSON.end()) {
        printf("[AxoLoader] Model \"%s\" not found for block id=%d\n", modelName.c_str(), blockId);
        return;
    }
    AxoBlockModel model = ParseModel(it->second);
    if (!model.elements.empty()) {
        sModels[blockId] = model;
        printf("[AxoLoader] Registered model \"%s\" for block id=%d elements=%d\n",
            modelName.c_str(), blockId, (int)model.elements.size());
    }
}

void AxoModelLoader_ResolveIcons(int blockId, IconRegister* iconRegister) {
    auto it = sModels.find(blockId);
    if (it == sModels.end()) return;
    for (auto& elem : it->second.elements) {
        for (int f = 0; f < 6; f++) {
            if (!elem.faces[f].present) continue;
            Icon* icon = AxoModLoader_GetTerrainIcon(elem.faces[f].textureName);
            if (!icon) icon = iconRegister->registerIcon(elem.faces[f].textureName);
            if (!icon) icon = AxoModLoader_GetTerrainIcon(L"stone");
            if (!icon) icon = iconRegister->registerIcon(L"stone");
            elem.faces[f].icon = icon;
        }
    }
}

bool AxoModelLoader_HasModel(int blockId) {
    return sModels.find(blockId) != sModels.end();
}

const AxoBlockModel* AxoModelLoader_GetModel(int blockId) {
    auto it = sModels.find(blockId);
    if (it == sModels.end()) return nullptr;
    return &it->second;
}

bool AxoModelLoader_TessellateForRenderTile(TileRenderer* renderer, Tile* tile, float brightness) {
    auto it = sModels.find(tile->id);
    if (it == sModels.end()) return false;

    Tesselator* t = Tesselator::getInstance();
    bool rendered = false;

    static const float kNormals[6][3] = {
        {0,-1,0},{0,1,0},{0,0,-1},{0,0,1},{-1,0,0},{1,0,0}
    };

    for (const auto& elem : it->second.elements) {
        renderer->setShape(
            elem.from[0], elem.from[1], elem.from[2],
            elem.to[0],   elem.to[1],   elem.to[2]
        );
        for (int f = 0; f < 6; f++) {
            if (!elem.faces[f].present || !elem.faces[f].icon) continue;
            float br = kFaceBrightness[f] * brightness;
            t->color(br, br, br);
            t->begin();
            t->normal(kNormals[f][0], kNormals[f][1], kNormals[f][2]);
            switch (f) {
                case 0: renderer->renderFaceDown(tile, 0,0,0, elem.faces[f].icon); break;
                case 1: renderer->renderFaceUp  (tile, 0,0,0, elem.faces[f].icon); break;
                case 2: renderer->renderNorth   (tile, 0,0,0, elem.faces[f].icon); break;
                case 3: renderer->renderSouth   (tile, 0,0,0, elem.faces[f].icon); break;
                case 4: renderer->renderWest    (tile, 0,0,0, elem.faces[f].icon); break;
                case 5: renderer->renderEast    (tile, 0,0,0, elem.faces[f].icon); break;
            }
            t->end();
            rendered = true;
        }
    }
    t->color(1.0f, 1.0f, 1.0f);
    return rendered;
}

bool AxoModelLoader_Tessellate(TileRenderer* renderer, Tile* tile, int x, int y, int z) {
    auto it = sModels.find(tile->id);
    if (it == sModels.end()) return false;

    Tesselator* t = Tesselator::getInstance();
    bool rendered = false;

    for (const auto& elem : it->second.elements) {
        renderer->setShape(
            elem.from[0], elem.from[1], elem.from[2],
            elem.to[0],   elem.to[1],   elem.to[2]
        );
        for (int f = 0; f < 6; f++) {
            if (!elem.faces[f].present || !elem.faces[f].icon) continue;
            t->color(kFaceBrightness[f], kFaceBrightness[f], kFaceBrightness[f]);
            switch (f) {
                case 0: renderer->renderFaceDown(tile, x, y, z, elem.faces[f].icon); break;
                case 1: renderer->renderFaceUp  (tile, x, y, z, elem.faces[f].icon); break;
                case 2: renderer->renderNorth   (tile, x, y, z, elem.faces[f].icon); break;
                case 3: renderer->renderSouth   (tile, x, y, z, elem.faces[f].icon); break;
                case 4: renderer->renderWest    (tile, x, y, z, elem.faces[f].icon); break;
                case 5: renderer->renderEast    (tile, x, y, z, elem.faces[f].icon); break;
            }
            rendered = true;
        }
    }
    t->color(1.0f, 1.0f, 1.0f);
    return rendered;
}