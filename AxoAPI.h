#pragma once
#include <string>
#include <functional>

#define AXO_ID_AUTO  0
#define AXO_DROP_SELF ""

class Level;
class Player;
class ItemInstance;

enum AxoCreativeTab {
    AxoTab_BuildingBlocks = 0,
    AxoTab_Decoration     = 1,
    AxoTab_Redstone       = 2,
    AxoTab_Transport      = 3,
    AxoTab_Materials      = 4,
    AxoTab_Food           = 5,
    AxoTab_ToolsArmor     = 6,
    AxoTab_Brewing        = 7,
    AxoTab_Misc           = 12,
};

#define AXO_SATURATION_POOR          0.1f
#define AXO_SATURATION_LOW           0.3f
#define AXO_SATURATION_NORMAL        0.6f
#define AXO_SATURATION_GOOD          0.8f
#define AXO_SATURATION_MAX           1.0f
#define AXO_SATURATION_SUPERNATURAL  1.2f

struct AxoFoodEffect {
    std::string effectName = "";
    int         duration   = 0;
    int         amplifier  = 0;
};

struct AxoFoodDef {
    int          nutrition    = 4;
    float        saturation   = AXO_SATURATION_NORMAL;
    bool         isMeat       = false;
    bool         canAlwaysEat = false;
    AxoFoodEffect effect;
};

struct AxoItemDef {
    int              id           = AXO_ID_AUTO;
    std::wstring     iconName;
    std::string      name;
    int              maxStackSize = 64;
    int              creativeTab  = AxoTab_Misc;
    std::function<void()> onUse   = nullptr;
    std::function<void()> onUseOn = nullptr;
    int              attackDamage = 1;
    float            miningSpeed  = 1.0f;
    bool             isPickaxe    = false;
    bool             isAxe        = false;
    bool             isShovel     = false;
    bool             isHandheld   = false;
    bool             isEdible     = false;
    AxoFoodDef       food;
};

enum AxoMaterial {
    AxoMat_Stone = 0,
    AxoMat_Wood  = 1,
    AxoMat_Grass = 2,
    AxoMat_Dirt  = 3,
    AxoMat_Metal = 4,
};

struct AxoBlockSpawnDef {
    bool        enabled      = false;
    bool        likeOre      = true;
    bool        likeGrass    = false;
    int         frequency    = 8;
    int         veinSize     = 8;
    int         yLevelMin    = 0;
    int         yLevelMax    = 64;
    std::string inBiome      = "";
    bool        onWater      = false;
    bool        onTerrain    = true;
    bool        inNether     = false;
    bool        inOverworld  = true;
};

enum AxoRenderShape {
    AxoShape_Cube  = 0,
    AxoShape_Cross = 1,
};

struct AxoBlockDef {
    int              id                   = AXO_ID_AUTO;
    std::wstring     iconName;
    std::string      name;
    AxoMaterial      material             = AxoMat_Stone;
    float            hardness             = 1.5f;
    float            resistance           = 10.0f;
    int              creativeTab          = 0;
    std::string      dropItemName         = AXO_DROP_SELF;
    int              dropCount            = 1;
    AxoRenderShape   renderShape          = AxoShape_Cube;
    bool             noCollision          = false;
    bool             canBeBrokenByHand    = false;
    std::string      canBePlacedOnlyOn    = "";
    std::string      customModel          = "";
    bool             hasDifferentSides    = false;
    std::wstring     iconTop;
    std::wstring     iconBottom;
    std::wstring     iconNorth;
    std::wstring     iconSouth;
    std::wstring     iconEast;
    std::wstring     iconWest;
    AxoBlockSpawnDef spawn;
    std::function<void(int x, int y, int z, Level*, Player*, ItemInstance*)> onDestroyed = nullptr;
};

struct AxoBlockDefInternal {
    int              id;
    std::wstring     iconName;
    std::string      name;
    AxoMaterial      material;
    float            hardness;
    float            resistance;
    int              creativeTab;
    int              dropItemId;
    int              dropCount;
    AxoRenderShape   renderShape;
    bool             noCollision;
    bool             canBeBrokenByHand;
    int              placeOnTileId;
    std::string      customModel;
    bool             hasDifferentSides;
    std::wstring     iconTop;
    std::wstring     iconBottom;
    std::wstring     iconNorth;
    std::wstring     iconSouth;
    std::wstring     iconEast;
    std::wstring     iconWest;
    AxoBlockSpawnDef spawn;
    std::function<void(int, int, int, Level*, Player*, ItemInstance*)> onDestroyed = nullptr;
};

struct AxoCraftingSlot {
    std::string itemName = "";
    int         count    = 1;
};

enum AxoRecipeGroup {
    AxoRecipe_Food        = 0,
    AxoRecipe_Tools       = 1,
    AxoRecipe_Armor       = 2,
    AxoRecipe_Mechanisms  = 3,
    AxoRecipe_Transport   = 4,
    AxoRecipe_Structures  = 5,
    AxoRecipe_Decoration  = 6,
};

struct AxoBiomeDef {
    int              id           = AXO_ID_AUTO;
    std::string      name;
    float            temperature  = 0.5f;
    float            downfall     = 0.5f;
    float            depth        = 0.1f;
    float            scale        = 0.3f;
    float            hilliness    = 0.0f;
    int              grassColor   = 0x79C05A;
    int              foliageColor = 0x59AE30;
    int              waterColor   = 0x3F76E4;
    int              skyColor     = 0x78A7FF;
    bool             hasRain      = true;
    bool             hasSnow      = false;
    int              spawnWeight  = 10;
    std::string      topMaterial  = "grass";
    std::string      material     = "dirt";
    int              treeCount    = 0;
    int              grassCount   = 1;
    int              flowerCount  = 2;
};

struct AxoRecipeDef {
    std::string     resultItemName;
    int             resultCount     = 1;
    bool            isShaped        = true;
    bool            isFurnace       = false;
    int             recipeGroup     = AxoRecipe_Decoration;
    AxoCraftingSlot grid[9];
    std::string     ingredients[9];
    int             ingredientCount = 0;
    std::string     furnaceInputName;
    float           furnaceXP       = 0.1f;
};

struct AxoCropGrowDrop {
    std::string itemName;
    int         count          = 1;
    int         seedDropCount  = 1;
    int         bonusDropMax   = 0;
};

struct AxoCropDef {
    int              id              = AXO_ID_AUTO;
    std::string      name;
    std::wstring     stageTextures[8];
    std::wstring     seedIconName;
    std::string      seedName;
    int              seedCreativeTab = AxoTab_Materials;
    AxoCropGrowDrop  growDrop;
};

struct AxoMod {
    const char* id;
};

struct AxoAPITable {
    void (*Log)(const char* modId, const char* msg);
    bool (*RegisterItem)(const AxoItemDef* def);
    bool (*RegisterBlock)(const AxoBlockDef* def);
    bool (*RegisterRecipe)(const AxoRecipeDef* def);
    bool (*RegisterBiome)(const AxoBiomeDef* def);
    bool (*RegisterCrop)(const AxoCropDef* def);
};

#ifndef MOD_ID
#  define MOD_ID "unknown_mod"
#endif

#ifdef AXO_MOD
static AxoAPITable* gAxoAPI = nullptr;
inline void AxoMod_SetAPI(AxoAPITable* api) { gAxoAPI = api; }
#else
extern AxoAPITable* gAxoAPI;
#endif

#define AxoAPI_Log(msg)             (gAxoAPI->Log(MOD_ID, msg))
#define AxoAPI_RegisterItem(def)    (gAxoAPI->RegisterItem(def))
#define AxoAPI_RegisterBlock(def)   (gAxoAPI->RegisterBlock(def))
#define AxoAPI_RegisterRecipe(def)  (gAxoAPI->RegisterRecipe(def))
#define AxoAPI_RegisterBiome(def)   (gAxoAPI->RegisterBiome(def))
#define AxoAPI_RegisterCrop(def)    (gAxoAPI->RegisterCrop(def))
