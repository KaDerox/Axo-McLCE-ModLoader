#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
//  AxoAPI v2 — C-stable ABI (safe across DLL / compiler / CRT boundaries)
//
//  All public structs use only plain C types (const char*, const wchar_t*,
//  function pointers).  Mods compiled with ANY MSVC version, ANY CRT
//  configuration (/MT, /MD, Debug, Release), or even other compilers can
//  safely interop with the mod loader.
//
//  Usage:
//    1. #define AXO_MOD   before including this header
//    2. #define MOD_ID    "my_mod"
//    3. In your ModEntry(), call  AxoMod_SetAPI(api);
//    4. Use AxoAPI_Register*() macros to register content.
//    5. Set "api_version": 2  in your manifest.json.
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

// ── Items ───────────────────────────────────────────────────────────────────

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

// ── Blocks ──────────────────────────────────────────────────────────────────

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

// ── Recipes ─────────────────────────────────────────────────────────────────

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

// ── Biomes ──────────────────────────────────────────────────────────────────

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

// ── Crops ───────────────────────────────────────────────────────────────────

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

// ── Mod entry point ─────────────────────────────────────────────────────────

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

// ── Mod-side helpers ────────────────────────────────────────────────────────
static AxoAPITable* gAxoAPI = nullptr;
inline void AxoMod_SetAPI(AxoAPITable* api) { gAxoAPI = api; }

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
