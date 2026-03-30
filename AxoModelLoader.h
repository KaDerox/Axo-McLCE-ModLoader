#pragma once
#include <string>
#include <vector>

#define SHAPE_AXO_CUSTOM_MODEL 50

class Icon;
class IconRegister;
class TileRenderer;
class Tile;

struct AxoModelFace {
    std::wstring textureName;
    Icon*        icon    = nullptr;
    bool         present = false;
};

struct AxoModelElement {
    float        from[3];
    float        to[3];
    AxoModelFace faces[6];
};

struct AxoBlockModel {
    std::vector<AxoModelElement> elements;
};

void  AxoModelLoader_StoreJSON(const std::string& modelName, const std::string& jsonContent);
void  AxoModelLoader_Register(int blockId, const std::string& modelName);
void  AxoModelLoader_ResolveIcons(int blockId, IconRegister* iconRegister);
bool  AxoModelLoader_Tessellate(TileRenderer* renderer, Tile* tile, int x, int y, int z);
bool  AxoModelLoader_TessellateForRenderTile(TileRenderer* renderer, Tile* tile, float brightness);
bool  AxoModelLoader_HasModel(int blockId);
const AxoBlockModel* AxoModelLoader_GetModel(int blockId);