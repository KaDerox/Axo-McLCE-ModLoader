#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
//  AxoAPI — C-stable ABI (safe across DLL / compiler / CRT boundaries)
//
//  All public structs use only plain C types (const char*, const wchar_t*,
//  function pointers).  This ensures mods compiled with ANY MSVC version,
//  ANY CRT configuration (/MT, /MD, Debug, Release), or even other compilers
//  can safely interop with the mod loader.
//
//  Game-internal C++ types (with std::string, std::function etc.) are defined
//  below, guarded by #ifndef AXO_MOD so mods never see them.
// ═══════════════════════════════════════════════════════════════════════════════

class Level;
class Player;
class ItemInstance;

#define AXO_ID_AUTO  0
#define AXO_DROP_SELF ""

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
    AxoTab_Mods           = 15,
};

#define AXO_SATURATION_POOR          0.1f
#define AXO_SATURATION_LOW           0.3f
#define AXO_SATURATION_NORMAL        0.6f
#define AXO_SATURATION_GOOD          0.8f
#define AXO_SATURATION_MAX           1.0f
#define AXO_SATURATION_SUPERNATURAL  1.2f

// ── Public structs (C-stable ABI) ───────────────────────────────────────────

struct AxoFoodEffect {
    const char* effectName = "";
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
    const wchar_t*   iconName     = L"";
    const char*      name         = "";
    int              maxStackSize = 64;
    int              creativeTab  = AxoTab_Misc;
    void           (*onUse)()    = nullptr;
    void           (*onUseOn)()  = nullptr;
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
    const char* inBiome      = "";
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
    const wchar_t*   iconName             = L"";
    const char*      name                 = "";
    AxoMaterial      material             = AxoMat_Stone;
    float            hardness             = 1.5f;
    float            resistance           = 10.0f;
    int              creativeTab          = 0;
    const char*      dropItemName         = AXO_DROP_SELF;
    int              dropCount            = 1;
    AxoRenderShape   renderShape          = AxoShape_Cube;
    bool             noCollision          = false;
    bool             canBeBrokenByHand    = false;
    const char*      canBePlacedOnlyOn    = "";
    const char*      customModel          = "";
    bool             hasDifferentSides    = false;
    const wchar_t*   iconTop              = L"";
    const wchar_t*   iconBottom           = L"";
    const wchar_t*   iconNorth            = L"";
    const wchar_t*   iconSouth            = L"";
    const wchar_t*   iconEast             = L"";
    const wchar_t*   iconWest             = L"";
    AxoBlockSpawnDef spawn;
    void           (*onDestroyed)(int x, int y, int z, Level*, Player*, ItemInstance*) = nullptr;
};

struct AxoCraftingSlot {
    const char* itemName = "";
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
    const char*      name         = "";
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
    const char*      topMaterial  = "grass";
    const char*      material     = "dirt";
    int              treeCount    = 0;
    int              grassCount   = 1;
    int              flowerCount  = 2;
};

struct AxoRecipeDef {
    const char*     resultItemName = "";
    int             resultCount     = 1;
    bool            isShaped        = true;
    bool            isFurnace       = false;
    int             recipeGroup     = AxoRecipe_Decoration;
    AxoCraftingSlot grid[9];
    const char*     ingredients[9]  = {};
    int             ingredientCount = 0;
    const char*     furnaceInputName = "";
    float           furnaceXP       = 0.1f;
};

struct AxoCropGrowDrop {
    const char* itemName       = "";
    int         count          = 1;
    int         seedDropCount  = 1;
    int         bonusDropMax   = 0;
};

struct AxoCropDef {
    int              id              = AXO_ID_AUTO;
    const char*      name            = "";
    const wchar_t*   stageTextures[8] = {};
    const wchar_t*   seedIconName    = L"";
    const char*      seedName        = "";
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
    // --- Entity / World interaction (v2) ---
    bool (*SpawnEntity)(Level* level, int entityId, double x, double y, double z);
    bool (*DropItem)(Level* level, int itemId, int count, int auxData, double x, double y, double z);
    bool (*StrikeLightning)(Level* level, double x, double y, double z);
    bool (*SpawnTnt)(Level* level, double x, double y, double z, int fuse);
    bool (*SpawnFallingBlock)(Level* level, double x, double y, double z, int tileId, int data);
    bool (*Explode)(Level* level, double x, double y, double z, float radius, bool fire, bool destroyBlocks);
    // --- Creative tab registration (v2) ---
    int  (*RegisterCreativeTab)(const wchar_t* name);
};

#ifndef MOD_ID
#  define MOD_ID "unknown_mod"
#endif

#ifdef AXO_MOD
// ── Mod-side helpers ────────────────────────────────────────────────────────
static AxoAPITable* gAxoAPI = nullptr;
inline void AxoMod_SetAPI(AxoAPITable* api) { gAxoAPI = api; }

#else
// ═══════════════════════════════════════════════════════════════════════════════
//  Game-internal types — NOT for mod use.
//  These mirror the public structs but use std::string / std::wstring /
//  std::function so the engine can safely own the data.
// ═══════════════════════════════════════════════════════════════════════════════
#include <string>
#include <functional>

extern AxoAPITable* gAxoAPI;

// Null-safe conversions from C pointers → C++ owned strings
inline std::string  SafeStr (const char*    s) { return s ? s : ""; }
inline std::wstring SafeWStr(const wchar_t* s) { return s ? s : L""; }

struct AxoFoodEffectI {
    std::string effectName;
    int         duration   = 0;
    int         amplifier  = 0;
};

struct AxoFoodDefI {
    int            nutrition    = 4;
    float          saturation   = AXO_SATURATION_NORMAL;
    bool           isMeat       = false;
    bool           canAlwaysEat = false;
    AxoFoodEffectI effect;
};

struct AxoItemDefI {
    int                   id           = AXO_ID_AUTO;
    std::wstring          iconName;
    std::string           name;
    int                   maxStackSize = 64;
    int                   creativeTab  = AxoTab_Misc;
    std::function<void()> onUse        = nullptr;
    std::function<void()> onUseOn      = nullptr;
    int                   attackDamage = 1;
    float                 miningSpeed  = 1.0f;
    bool                  isPickaxe    = false;
    bool                  isAxe        = false;
    bool                  isShovel     = false;
    bool                  isHandheld   = false;
    bool                  isEdible     = false;
    AxoFoodDefI           food;
};

struct AxoBlockSpawnDefI {
    bool        enabled      = false;
    bool        likeOre      = true;
    bool        likeGrass    = false;
    int         frequency    = 8;
    int         veinSize     = 8;
    int         yLevelMin    = 0;
    int         yLevelMax    = 64;
    std::string inBiome;
    bool        onWater      = false;
    bool        onTerrain    = true;
    bool        inNether     = false;
    bool        inOverworld  = true;
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
    AxoBlockSpawnDefI spawn;
    std::function<void(int, int, int, Level*, Player*, ItemInstance*)> onDestroyed = nullptr;
};

struct AxoBlockDefI {
    int              id                   = AXO_ID_AUTO;
    std::wstring     iconName;
    std::string      name;
    AxoMaterial      material             = AxoMat_Stone;
    float            hardness             = 1.5f;
    float            resistance           = 10.0f;
    int              creativeTab          = 0;
    std::string      dropItemName;
    int              dropCount            = 1;
    AxoRenderShape   renderShape          = AxoShape_Cube;
    bool             noCollision          = false;
    bool             canBeBrokenByHand    = false;
    std::string      canBePlacedOnlyOn;
    std::string      customModel;
    bool             hasDifferentSides    = false;
    std::wstring     iconTop;
    std::wstring     iconBottom;
    std::wstring     iconNorth;
    std::wstring     iconSouth;
    std::wstring     iconEast;
    std::wstring     iconWest;
    AxoBlockSpawnDefI spawn;
    std::function<void(int, int, int, Level*, Player*, ItemInstance*)> onDestroyed = nullptr;
};

struct AxoCraftingSlotI {
    std::string itemName;
    int         count = 1;
};

struct AxoRecipeDefI {
    std::string      resultItemName;
    int              resultCount     = 1;
    bool             isShaped        = true;
    bool             isFurnace       = false;
    int              recipeGroup     = AxoRecipe_Decoration;
    AxoCraftingSlotI grid[9];
    std::string      ingredients[9];
    int              ingredientCount = 0;
    std::string      furnaceInputName;
    float            furnaceXP       = 0.1f;
};

struct AxoBiomeDefI {
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

struct AxoCropGrowDropI {
    std::string itemName;
    int         count          = 1;
    int         seedDropCount  = 1;
    int         bonusDropMax   = 0;
};

struct AxoCropDefI {
    int              id              = AXO_ID_AUTO;
    std::string      name;
    std::wstring     stageTextures[8];
    std::wstring     seedIconName;
    std::string      seedName;
    int              seedCreativeTab = AxoTab_Materials;
    AxoCropGrowDropI growDrop;
};

// ── Conversion helpers (C → Internal) ───────────────────────────────────────

inline AxoBlockSpawnDefI ToInternalSpawn(const AxoBlockSpawnDef& d) {
    AxoBlockSpawnDefI r;
    r.enabled = d.enabled;  r.likeOre = d.likeOre;  r.likeGrass = d.likeGrass;
    r.frequency = d.frequency;  r.veinSize = d.veinSize;
    r.yLevelMin = d.yLevelMin;  r.yLevelMax = d.yLevelMax;
    r.inBiome = SafeStr(d.inBiome);
    r.onWater = d.onWater;  r.onTerrain = d.onTerrain;
    r.inNether = d.inNether;  r.inOverworld = d.inOverworld;
    return r;
}

inline AxoItemDefI ToInternal(const AxoItemDef& d) {
    AxoItemDefI r;
    r.id = d.id;
    r.iconName     = SafeWStr(d.iconName);
    r.name         = SafeStr(d.name);
    r.maxStackSize = d.maxStackSize;
    r.creativeTab  = d.creativeTab;
    r.onUse        = d.onUse;
    r.onUseOn      = d.onUseOn;
    r.attackDamage = d.attackDamage;
    r.miningSpeed  = d.miningSpeed;
    r.isPickaxe    = d.isPickaxe;  r.isAxe = d.isAxe;
    r.isShovel     = d.isShovel;   r.isHandheld = d.isHandheld;
    r.isEdible     = d.isEdible;
    r.food.nutrition    = d.food.nutrition;
    r.food.saturation   = d.food.saturation;
    r.food.isMeat       = d.food.isMeat;
    r.food.canAlwaysEat = d.food.canAlwaysEat;
    r.food.effect.effectName = SafeStr(d.food.effect.effectName);
    r.food.effect.duration   = d.food.effect.duration;
    r.food.effect.amplifier  = d.food.effect.amplifier;
    return r;
}

inline AxoBlockDefI ToInternal(const AxoBlockDef& d) {
    AxoBlockDefI r;
    r.id = d.id;
    r.iconName      = SafeWStr(d.iconName);
    r.name          = SafeStr(d.name);
    r.material      = d.material;
    r.hardness      = d.hardness;
    r.resistance    = d.resistance;
    r.creativeTab   = d.creativeTab;
    r.dropItemName  = SafeStr(d.dropItemName);
    r.dropCount     = d.dropCount;
    r.renderShape   = d.renderShape;
    r.noCollision   = d.noCollision;
    r.canBeBrokenByHand = d.canBeBrokenByHand;
    r.canBePlacedOnlyOn = SafeStr(d.canBePlacedOnlyOn);
    r.customModel   = SafeStr(d.customModel);
    r.hasDifferentSides = d.hasDifferentSides;
    r.iconTop    = SafeWStr(d.iconTop);    r.iconBottom = SafeWStr(d.iconBottom);
    r.iconNorth  = SafeWStr(d.iconNorth);  r.iconSouth  = SafeWStr(d.iconSouth);
    r.iconEast   = SafeWStr(d.iconEast);   r.iconWest   = SafeWStr(d.iconWest);
    r.spawn       = ToInternalSpawn(d.spawn);
    r.onDestroyed = d.onDestroyed;
    return r;
}

inline AxoRecipeDefI ToInternal(const AxoRecipeDef& d) {
    AxoRecipeDefI r;
    r.resultItemName = SafeStr(d.resultItemName);
    r.resultCount    = d.resultCount;
    r.isShaped       = d.isShaped;
    r.isFurnace      = d.isFurnace;
    r.recipeGroup    = d.recipeGroup;
    for (int i = 0; i < 9; i++) {
        r.grid[i].itemName = SafeStr(d.grid[i].itemName);
        r.grid[i].count    = d.grid[i].count;
        r.ingredients[i]   = SafeStr(d.ingredients[i]);
    }
    r.ingredientCount  = d.ingredientCount;
    r.furnaceInputName = SafeStr(d.furnaceInputName);
    r.furnaceXP        = d.furnaceXP;
    return r;
}

inline AxoBiomeDefI ToInternal(const AxoBiomeDef& d) {
    AxoBiomeDefI r;
    r.id = d.id;
    r.name         = SafeStr(d.name);
    r.temperature  = d.temperature;   r.downfall    = d.downfall;
    r.depth        = d.depth;         r.scale       = d.scale;
    r.hilliness    = d.hilliness;
    r.grassColor   = d.grassColor;    r.foliageColor = d.foliageColor;
    r.waterColor   = d.waterColor;    r.skyColor     = d.skyColor;
    r.hasRain      = d.hasRain;       r.hasSnow      = d.hasSnow;
    r.spawnWeight  = d.spawnWeight;
    r.topMaterial  = SafeStr(d.topMaterial);
    r.material     = SafeStr(d.material);
    r.treeCount    = d.treeCount;
    r.grassCount   = d.grassCount;    r.flowerCount  = d.flowerCount;
    return r;
}

inline AxoCropDefI ToInternal(const AxoCropDef& d) {
    AxoCropDefI r;
    r.id   = d.id;
    r.name = SafeStr(d.name);
    for (int i = 0; i < 8; i++) r.stageTextures[i] = SafeWStr(d.stageTextures[i]);
    r.seedIconName    = SafeWStr(d.seedIconName);
    r.seedName        = SafeStr(d.seedName);
    r.seedCreativeTab = d.seedCreativeTab;
    r.growDrop.itemName      = SafeStr(d.growDrop.itemName);
    r.growDrop.count         = d.growDrop.count;
    r.growDrop.seedDropCount = d.growDrop.seedDropCount;
    r.growDrop.bonusDropMax  = d.growDrop.bonusDropMax;
    return r;
}

#endif // AXO_MOD

// ── Convenience macros ──────────────────────────────────────────────────────

#define AxoAPI_Log(msg)             (gAxoAPI->Log(MOD_ID, msg))
#define AxoAPI_RegisterItem(def)    (gAxoAPI->RegisterItem(def))
#define AxoAPI_RegisterBlock(def)   (gAxoAPI->RegisterBlock(def))
#define AxoAPI_RegisterRecipe(def)  (gAxoAPI->RegisterRecipe(def))
#define AxoAPI_RegisterBiome(def)   (gAxoAPI->RegisterBiome(def))
#define AxoAPI_RegisterCrop(def)    (gAxoAPI->RegisterCrop(def))
#define AxoAPI_SpawnEntity(lv,id,x,y,z)          (gAxoAPI->SpawnEntity(lv,id,x,y,z))
#define AxoAPI_DropItem(lv,id,n,aux,x,y,z)       (gAxoAPI->DropItem(lv,id,n,aux,x,y,z))
#define AxoAPI_StrikeLightning(lv,x,y,z)         (gAxoAPI->StrikeLightning(lv,x,y,z))
#define AxoAPI_SpawnTnt(lv,x,y,z,fuse)           (gAxoAPI->SpawnTnt(lv,x,y,z,fuse))
#define AxoAPI_SpawnFallingBlock(lv,x,y,z,tid,d) (gAxoAPI->SpawnFallingBlock(lv,x,y,z,tid,d))
#define AxoAPI_Explode(lv,x,y,z,r,f,d)           (gAxoAPI->Explode(lv,x,y,z,r,f,d))
#define AxoAPI_RegisterCreativeTab(name)           (gAxoAPI->RegisterCreativeTab(name))