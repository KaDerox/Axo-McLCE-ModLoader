#include "stdafx.h"

#include "AxoAPI.h"
#include "AxoModLoader.h"
#include "AxoWorldGen.h"
#include "AxoModelLoader.h"

#include "EntityIO.h"
#include "ItemEntity.h"
#include "ItemInstance.h"
#include "LightningBolt.h"
#include "PrimedTnt.h"
#include "FallingTile.h"
#include "Mob.h"
#include "Level.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdio>

extern bool AxoItem_CreateFromDef(const AxoItemDefI& def);
extern void AxoItem_AddToCreativeMenu(int itemId, int creativeTab);
extern bool AxoBlock_CreateFromDef(const AxoBlockDefInternal& def);
extern void AxoBlock_AddToCreativeMenu(int blockId, int creativeTab);
extern bool AxoRecipe_CreateFromDef(const AxoRecipeDefI& def);
extern bool AxoBiome_CreateFromDef(const AxoBiomeDefI& def);
extern bool AxoCrop_CreateFromDef(const AxoCropDefI& def);

static std::vector<AxoBiomeDefI> sPendingBiomes;
static std::vector<AxoCropDefI>  sPendingCrops;

AxoAPITable* gAxoAPI = nullptr;

static std::vector<AxoItemDefI>   sPendingItems;
static std::vector<AxoBlockDefI>  sPendingBlocks;
static std::vector<AxoRecipeDefI> sPendingRecipes;

static int sNextItemId  = 422;
static int sNextBlockId = 174;

struct RegisteredItem  { int id; int creativeTab; std::string name; };
struct RegisteredBlock { int id; int creativeTab; std::string name; };
static std::vector<RegisteredItem>  sRegisteredItems;
static std::vector<RegisteredBlock> sRegisteredBlocks;

static const std::unordered_map<std::string, int>& GetVanillaTileMap() {
    static const std::unordered_map<std::string, int> kMap = {
        {"stone",                  1}, {"grass",                  2}, {"dirt",                   3},
        {"cobblestone",            4}, {"wood",                   5}, {"sapling",                6},
        {"bedrock",                7}, {"water",                  8}, {"water_still",             9},
        {"lava",                  10}, {"lava_still",            11}, {"sand",                  12},
        {"gravel",                13}, {"gold_ore",              14}, {"iron_ore",              15},
        {"coal_ore",              16}, {"log",                   17}, {"leaves",                18},
        {"sponge",                19}, {"glass",                 20}, {"lapis_ore",             21},
        {"lapis_block",           22}, {"dispenser",             23}, {"sandstone",             24},
        {"noteblock",             25}, {"bed",                   26}, {"rail_powered",          27},
        {"rail_detector",         28}, {"piston_sticky",         29}, {"web",                   30},
        {"tallgrass",             31}, {"deadbush",              32}, {"piston",                33},
        {"piston_head",           34}, {"wool",                  35}, {"flower",                37},
        {"rose",                  38}, {"mushroom_brown",        39}, {"mushroom_red",          40},
        {"gold_block",            41}, {"iron_block",            42}, {"double_slab",           43},
        {"slab",                  44}, {"brick_block",           45}, {"tnt",                   46},
        {"bookshelf",             47}, {"mossy_cobblestone",     48}, {"obsidian",              49},
        {"torch",                 50}, {"fire",                  51}, {"spawner",               52},
        {"stairs_wood",           53}, {"chest",                 54}, {"redstone_wire",         55},
        {"diamond_ore",           56}, {"diamond_block",         57}, {"workbench",             58},
        {"crops",                 59}, {"farmland",              60}, {"furnace",               61},
        {"furnace_lit",           62}, {"sign_post",             63}, {"door_wood",             64},
        {"ladder",                65}, {"rail",                  66}, {"stairs_stone",          67},
        {"sign_wall",             68}, {"lever",                 69}, {"pressure_plate_stone",  70},
        {"door_iron",             71}, {"pressure_plate_wood",   72}, {"redstone_ore",          73},
        {"redstone_ore_lit",      74}, {"torch_redstone",        75}, {"torch_redstone_lit",    76},
        {"button_stone",          77}, {"snow_layer",            78}, {"ice",                   79},
        {"snow_block",            80}, {"cactus",                81}, {"clay_block",            82},
        {"reeds",                 83}, {"jukebox",               84}, {"fence",                 85},
        {"pumpkin",               86}, {"netherrack",            87}, {"soulsand",              88},
        {"glowstone",             89}, {"portal",                90}, {"pumpkin_lit",           91},
        {"cake",                  92}, {"repeater",              93}, {"repeater_lit",          94},
        {"trapdoor",              96}, {"silverfish",            97}, {"stonebrick",            98},
        {"mushroom_cap_brown",    99}, {"mushroom_cap_red",     100}, {"iron_bars",            101},
        {"glass_pane",           102}, {"melon",                103}, {"pumpkin_stem",         104},
        {"melon_stem",           105}, {"vine",                 106}, {"fence_gate",           107},
        {"stairs_brick",         108}, {"stairs_stonebrick",    109}, {"mycelium",             110},
        {"lilypad",              111}, {"nether_brick_block",   112}, {"nether_brick_fence",   113},
        {"stairs_netherbrick",   114}, {"nether_wart",          115}, {"enchanting_table",     116},
        {"brewing_stand",        117}, {"cauldron",             118}, {"end_portal",           119},
        {"end_portal_frame",     120}, {"end_stone",            121}, {"dragon_egg",           122},
        {"redstone_lamp",        123}, {"redstone_lamp_lit",    124}, {"stairs_sandstone",     128},
        {"emerald_ore",          129}, {"ender_chest",          130}, {"tripwire_hook",        131},
        {"tripwire",             132}, {"emerald_block",        133}, {"stairs_spruce",        134},
        {"stairs_birch",         135}, {"stairs_jungle",        136}, {"command_block",        137},
        {"beacon",               138}, {"cobblestone_wall",     139}, {"flower_pot",           140},
        {"carrots",              141}, {"potatoes",             142}, {"button_wood",          143},
        {"skull",                144}, {"anvil",                145}, {"chest_trapped",        146},
        {"pressure_plate_gold",  147}, {"pressure_plate_iron",  148}, {"comparator",          149},
        {"comparator_lit",       150}, {"daylight_sensor",      151}, {"redstone_block",       152},
        {"quartz_ore",           153}, {"hopper",               154}, {"quartz_block",         155},
        {"stairs_quartz",        156}, {"rail_activator",       157}, {"dropper",              158},
    };
    return kMap;
}

static int ResolveTileName(const std::string& name) {
    if (name.empty()) return 0;
    const auto& map = GetVanillaTileMap();
    auto it = map.find(name);
    if (it != map.end()) return it->second;
    for (const auto& r : sRegisteredBlocks) {
        if (r.name == name) return r.id;
    }
    printf("[AxoLoader] ResolveTileName: unknown tile \"%s\"\n", name.c_str());
    return 0;
}

static const std::unordered_map<std::string, int>& GetVanillaItemMap() {
    static const std::unordered_map<std::string, int> kMap = {
        {"shovel_iron",          256}, {"pickaxe_iron",        257}, {"axe_iron",            258},
        {"flint_and_steel",      259}, {"apple",               260}, {"bow",                 261},
        {"arrow",                262}, {"coal",                263}, {"diamond",              264},
        {"iron_ingot",           265}, {"gold_ingot",          266}, {"sword_iron",           267},
        {"sword_wood",           268}, {"shovel_wood",         269}, {"pickaxe_wood",         270},
        {"axe_wood",             271}, {"sword_stone",         272}, {"shovel_stone",         273},
        {"pickaxe_stone",        274}, {"axe_stone",           275}, {"sword_diamond",        276},
        {"shovel_diamond",       277}, {"pickaxe_diamond",     278}, {"axe_diamond",          279},
        {"stick",                280}, {"bowl",                281}, {"mushroom_stew",        282},
        {"sword_gold",           283}, {"shovel_gold",         284}, {"pickaxe_gold",         285},
        {"axe_gold",             286}, {"string",              287}, {"feather",              288},
        {"gunpowder",            289}, {"hoe_wood",            290}, {"hoe_stone",            291},
        {"hoe_iron",             292}, {"hoe_diamond",         293}, {"hoe_gold",             294},
        {"seeds_wheat",          295}, {"wheat",               296}, {"bread",                297},
        {"helmet_leather",       298}, {"chestplate_leather",  299}, {"leggings_leather",     300},
        {"boots_leather",        301}, {"helmet_chain",        302}, {"chestplate_chain",     303},
        {"leggings_chain",       304}, {"boots_chain",         305}, {"helmet_iron",          306},
        {"chestplate_iron",      307}, {"leggings_iron",       308}, {"boots_iron",           309},
        {"helmet_diamond",       310}, {"chestplate_diamond",  311}, {"leggings_diamond",     312},
        {"boots_diamond",        313}, {"helmet_gold",         314}, {"chestplate_gold",      315},
        {"leggings_gold",        316}, {"boots_gold",          317}, {"flint",                318},
        {"porkchop_raw",         319}, {"porkchop_cooked",     320}, {"painting",             321},
        {"apple_gold",           322}, {"sign",                323}, {"door_wood",            324},
        {"bucket_empty",         325}, {"bucket_water",        326}, {"bucket_lava",          327},
        {"minecart",             328}, {"saddle",              329}, {"door_iron",            330},
        {"redstone",             331}, {"snowball",            332}, {"boat",                 333},
        {"leather",              334}, {"bucket_milk",         335}, {"brick",                336},
        {"clay",                 337}, {"reeds",               338}, {"paper",                339},
        {"book",                 340}, {"slimeball",           341}, {"minecart_chest",       342},
        {"minecart_furnace",     343}, {"egg",                 344}, {"compass",              345},
        {"fishing_rod",          346}, {"clock",               347}, {"glowstone_dust",       348},
        {"fish_raw",             349}, {"fish_cooked",         350}, {"dye",                  351},
        {"bone",                 352}, {"sugar",               353}, {"cake",                 354},
        {"bed",                  355}, {"repeater",            356}, {"cookie",               357},
        {"map",                  358}, {"shears",              359}, {"melon",                360},
        {"seeds_pumpkin",        361}, {"seeds_melon",         362}, {"beef_raw",             363},
        {"beef_cooked",          364}, {"chicken_raw",         365}, {"chicken_cooked",       366},
        {"rotten_flesh",         367}, {"ender_pearl",         368}, {"blaze_rod",            369},
        {"ghast_tear",           370}, {"gold_nugget",         371}, {"nether_wart",          372},
        {"potion",               373}, {"glass_bottle",        374}, {"spider_eye",           375},
        {"fermented_spider_eye", 376}, {"blaze_powder",        377}, {"magma_cream",          378},
        {"brewing_stand",        379}, {"cauldron",            380}, {"eye_of_ender",         381},
        {"glistering_melon",     382}, {"spawn_egg",           383}, {"exp_bottle",           384},
        {"fireball",             385}, {"emerald",             388}, {"item_frame",           389},
        {"flower_pot",           390}, {"carrots",             391}, {"potato",               392},
        {"potato_baked",         393}, {"potato_poisonous",    394}, {"empty_map",            395},
        {"carrot_golden",        396}, {"skull",               397}, {"carrot_on_stick",      398},
        {"nether_star",          399}, {"pumpkin_pie",         400}, {"fireworks",            401},
        {"fireworks_charge",     402}, {"enchanted_book",      403}, {"comparator",           404},
        {"nether_brick",         405}, {"nether_quartz",       406}, {"minecart_tnt",         407},
        {"minecart_hopper",      408}, {"horse_armor_iron",    417}, {"horse_armor_gold",     418},
        {"horse_armor_diamond",  419}, {"lead",                420}, {"name_tag",             421},
    };
    return kMap;
}

static int ResolveDropItemName(const std::string& name, int blockId) {
    if (name.empty()) return 0;
    const auto& map = GetVanillaItemMap();
    auto it = map.find(name);
    if (it != map.end()) return it->second;
    for (const auto& r : sRegisteredItems) {
        if (r.name == name) return r.id;
    }
    printf("[AxoLoader] RegisterBlock id=%d: unknown dropItemName \"%s\", dropping self.\n",
           blockId, name.c_str());
    return 0;
}

static void Impl_Log(const char* modId, const char* msg) {
    printf("[AxoLoader][%s] %s\n", modId, msg);
    fflush(stdout);
}

static bool Impl_RegisterItem(const AxoItemDef* def) {
    if (!def) return false;
    AxoItemDefI resolved = ToInternal(*def);
    if (resolved.id == AXO_ID_AUTO) {
        if (sNextItemId > 31999) {
            printf("[AxoLoader] RegisterItem: no free item IDs left.\n");
            return false;
        }
        resolved.id = sNextItemId++;
    } else if (resolved.id < 422 || resolved.id > 31999) {
        printf("[AxoLoader] RegisterItem: id %d out of range (422-31999).\n", resolved.id);
        return false;
    }
    sPendingItems.push_back(resolved);
    printf("[AxoLoader] Queued item id=%d \"%s\"\n", resolved.id, resolved.name.c_str());
    return true;
}

static bool Impl_RegisterBlock(const AxoBlockDef* def) {
    if (!def) return false;
    AxoBlockDefI resolved = ToInternal(*def);
    if (resolved.id == AXO_ID_AUTO) {
        if (sNextBlockId > 255) {
            printf("[AxoLoader] RegisterBlock: no free block IDs left.\n");
            return false;
        }
        resolved.id = sNextBlockId++;
    } else if (resolved.id < 174 || resolved.id > 255) {
        printf("[AxoLoader] RegisterBlock: id %d out of range (174-255).\n", resolved.id);
        return false;
    }
    sPendingBlocks.push_back(resolved);
    printf("[AxoLoader] Queued block id=%d \"%s\" drop=\"%s\"\n",
           resolved.id, resolved.name.c_str(), resolved.dropItemName.c_str());
    return true;
}

static bool Impl_RegisterRecipe(const AxoRecipeDef* def) {
    if (!def) return false;
    sPendingRecipes.push_back(ToInternal(*def));
    printf("[AxoLoader] Queued recipe -> \"%s\" (x%d)\n",
           sPendingRecipes.back().resultItemName.c_str(), def->resultCount);
    return true;
}

static int sNextBiomeId = 23;

static bool Impl_RegisterBiome(const AxoBiomeDef* def) {
    if (!def) return false;
    AxoBiomeDefI resolved = ToInternal(*def);
    if (resolved.id == AXO_ID_AUTO) {
        if (sNextBiomeId > 255) {
            printf("[AxoLoader] RegisterBiome: no free biome IDs left.\n");
            return false;
        }
        resolved.id = sNextBiomeId++;
    }
    if (resolved.id < 23 || resolved.id > 255) {
        printf("[AxoLoader] RegisterBiome: id %d out of range (23-255).\n", resolved.id);
        return false;
    }
    sPendingBiomes.push_back(resolved);
    printf("[AxoLoader] Queued biome id=%d \"%s\"\n", resolved.id, resolved.name.c_str());
    return true;
}

static bool Impl_RegisterCrop(const AxoCropDef* def) {
    if (!def) return false;
    AxoCropDefI resolved = ToInternal(*def);
    if (resolved.id == AXO_ID_AUTO) {
        if (sNextBlockId > 255) {
            printf("[AxoLoader] RegisterCrop: no free block IDs left.\n");
            return false;
        }
        resolved.id = sNextBlockId++;
    }
    sPendingCrops.push_back(resolved);
    printf("[AxoLoader] Queued crop id=%d \"%s\"\n", resolved.id, resolved.name.c_str());
    return true;
}

static bool Impl_SpawnEntity(Level* level, int entityId, double x, double y, double z) {
    if (!level) return false;
    auto entity = EntityIO::newById(entityId, level);
    if (!entity) {
        printf("[AxoLoader] SpawnEntity: unknown entity id %d\n", entityId);
        return false;
    }
    entity->moveTo(x, y, z, ((float)rand() / (float)RAND_MAX) * 360.0f, 0.0f);
    auto mob = dynamic_pointer_cast<Mob>(entity);
    if (mob) mob->finalizeMobSpawn(nullptr);
    return level->addEntity(entity);
}

static bool Impl_DropItem(Level* level, int itemId, int count, int auxData, double x, double y, double z) {
    if (!level || count <= 0) {
        printf("[AxoLoader] DropItem: SKIP level=%p count=%d\n", level, count);
        return false;
    }
    auto item = make_shared<ItemInstance>(itemId, count, auxData);
    if (!item || !item->getItem()) {
        printf("[AxoLoader] DropItem: INVALID item id=%d (getItem=%p)\n", itemId, item ? item->getItem() : nullptr);
        return false;
    }
    auto ent = make_shared<ItemEntity>(level, x, y, z, item);
    ent->throwTime = 10;
    printf("[AxoLoader] DropItem: id=%d count=%d aux=%d pos=(%.1f,%.1f,%.1f) entId=%d\n",
           itemId, count, auxData, x, y, z, ent->entityId);
    bool ok = level->addEntity(ent);
    printf("[AxoLoader] DropItem: addEntity returned %s\n", ok ? "TRUE" : "FALSE");
    return ok;
}

static bool Impl_StrikeLightning(Level* level, double x, double y, double z) {
    if (!level) return false;
    auto bolt = make_shared<LightningBolt>(level, x, y, z);
    return level->addGlobalEntity(bolt);
}

static bool Impl_SpawnTnt(Level* level, double x, double y, double z, int fuse) {
    if (!level) return false;
    auto tnt = make_shared<PrimedTnt>(level, x, y, z, nullptr);
    tnt->life = fuse;
    return level->addEntity(tnt);
}

static bool Impl_SpawnFallingBlock(Level* level, double x, double y, double z, int tileId, int data) {
    if (!level) return false;
    auto fb = make_shared<FallingTile>(level, x, y, z, tileId, data);
    return level->addEntity(fb);
}

static AxoAPITable sAPITable = {
    Impl_Log,
    Impl_RegisterItem,
    Impl_RegisterBlock,
    Impl_RegisterRecipe,
    Impl_RegisterBiome,
    Impl_RegisterCrop,
    Impl_SpawnEntity,
    Impl_DropItem,
    Impl_StrikeLightning,
    Impl_SpawnTnt,
    Impl_SpawnFallingBlock
};

// ═══════════════════════════════════════════════════════════════════════════════
//  V1 backward compatibility — old mods compiled against the original
//  Axo_McLCE_ModLoader AxoAPI.h (std::string-based structs, 6 pointers).
//
//  MSVC 2015+ uses the same binary layout for std::string / std::wstring /
//  std::function in both Debug and Release builds.  Only the CRT heap differs.
//  Our V1 shims receive pointers to old-layout structs (which are
//  layout-identical to our AxoItemDefI / AxoBlockDefI / etc.), copy the data
//  into game-owned storage, and feed them into the normal registration pipeline.
// ═══════════════════════════════════════════════════════════════════════════════

static bool V1_RegisterItem(const AxoItemDefI* def) {
    if (!def) return false;
    AxoItemDefI resolved = *def;
    if (resolved.id == AXO_ID_AUTO) {
        if (sNextItemId > 31999) {
            printf("[AxoLoader/V1] RegisterItem: no free item IDs left.\n");
            return false;
        }
        resolved.id = sNextItemId++;
    } else if (resolved.id < 422 || resolved.id > 31999) {
        printf("[AxoLoader/V1] RegisterItem: id %d out of range.\n", resolved.id);
        return false;
    }
    sPendingItems.push_back(resolved);
    printf("[AxoLoader/V1] Queued item id=%d \"%s\"\n", resolved.id, resolved.name.c_str());
    return true;
}

static bool V1_RegisterBlock(const AxoBlockDefI* def) {
    if (!def) return false;
    AxoBlockDefI resolved = *def;
    if (resolved.id == AXO_ID_AUTO) {
        if (sNextBlockId > 255) {
            printf("[AxoLoader/V1] RegisterBlock: no free block IDs left.\n");
            return false;
        }
        resolved.id = sNextBlockId++;
    } else if (resolved.id < 174 || resolved.id > 255) {
        printf("[AxoLoader/V1] RegisterBlock: id %d out of range.\n", resolved.id);
        return false;
    }
    sPendingBlocks.push_back(resolved);
    printf("[AxoLoader/V1] Queued block id=%d \"%s\"\n", resolved.id, resolved.name.c_str());
    return true;
}

static bool V1_RegisterRecipe(const AxoRecipeDefI* def) {
    if (!def) return false;
    sPendingRecipes.push_back(*def);
    printf("[AxoLoader/V1] Queued recipe -> \"%s\"\n", def->resultItemName.c_str());
    return true;
}

static bool V1_RegisterBiome(const AxoBiomeDefI* def) {
    if (!def) return false;
    AxoBiomeDefI resolved = *def;
    if (resolved.id == AXO_ID_AUTO) {
        if (sNextBiomeId > 255) {
            printf("[AxoLoader/V1] RegisterBiome: no free biome IDs left.\n");
            return false;
        }
        resolved.id = sNextBiomeId++;
    }
    if (resolved.id < 23 || resolved.id > 255) {
        printf("[AxoLoader/V1] RegisterBiome: id %d out of range.\n", resolved.id);
        return false;
    }
    sPendingBiomes.push_back(resolved);
    printf("[AxoLoader/V1] Queued biome id=%d \"%s\"\n", resolved.id, resolved.name.c_str());
    return true;
}

static bool V1_RegisterCrop(const AxoCropDefI* def) {
    if (!def) return false;
    AxoCropDefI resolved = *def;
    if (resolved.id == AXO_ID_AUTO) {
        if (sNextBlockId > 255) {
            printf("[AxoLoader/V1] RegisterCrop: no free block IDs left.\n");
            return false;
        }
        resolved.id = sNextBlockId++;
    }
    sPendingCrops.push_back(resolved);
    printf("[AxoLoader/V1] Queued crop id=%d \"%s\"\n", resolved.id, resolved.name.c_str());
    return true;
}

// V1 API table — 6 function pointers matching the original AxoAPITable layout.
// Old mods read only the first 6 slots.  We pad with nullptr for the v2 slots
// so the table is safely castable to AxoAPITable*.
static AxoAPITable sV1APITable = {
    Impl_Log,                                                           // Log is unchanged
    (decltype(AxoAPITable::RegisterItem))   V1_RegisterItem,
    (decltype(AxoAPITable::RegisterBlock))  V1_RegisterBlock,
    (decltype(AxoAPITable::RegisterRecipe)) V1_RegisterRecipe,
    (decltype(AxoAPITable::RegisterBiome))  V1_RegisterBiome,
    (decltype(AxoAPITable::RegisterCrop))   V1_RegisterCrop,
    nullptr, nullptr, nullptr, nullptr, nullptr                         // v2 slots
};

AxoAPITable* AxoAPI_GetTable() {
    return &sAPITable;
}

AxoAPITable* AxoAPI_GetTableV1() {
    return &sV1APITable;
}

void AxoAPI_FlushRegistrations() {
    printf("[AxoLoader] FlushRegistrations: %u item(s)...\n", (unsigned)sPendingItems.size());
    for (const auto& def : sPendingItems) {
        if (AxoItem_CreateFromDef(def))
            sRegisteredItems.push_back({def.id, def.creativeTab, def.name});
    }
    sPendingItems.clear();
}

void AxoAPI_FlushBlockRegistrations() {
    printf("[AxoLoader] FlushBlockRegistrations: %u block(s)...\n", (unsigned)sPendingBlocks.size());
    for (const auto& def : sPendingBlocks) {
        AxoBlockDefInternal internal;
        internal.id            = def.id;
        internal.iconName      = def.iconName;
        internal.name          = def.name;
        internal.material      = def.material;
        internal.hardness      = def.hardness;
        internal.resistance    = def.resistance;
        internal.creativeTab   = def.creativeTab;
        internal.dropItemId    = ResolveDropItemName(def.dropItemName, def.id);
        internal.dropCount     = def.dropCount;
        internal.renderShape       = def.renderShape;
        internal.noCollision       = def.noCollision;
        internal.canBeBrokenByHand = def.canBeBrokenByHand;
        internal.placeOnTileId     = ResolveTileName(def.canBePlacedOnlyOn);
        internal.customModel       = def.customModel;
        internal.hasDifferentSides = def.hasDifferentSides;
        internal.iconTop           = def.iconTop;
        internal.iconBottom        = def.iconBottom;
        internal.iconNorth         = def.iconNorth;
        internal.iconSouth         = def.iconSouth;
        internal.iconEast          = def.iconEast;
        internal.iconWest          = def.iconWest;
        internal.spawn         = def.spawn;
        internal.onDestroyed   = def.onDestroyed;
        if (AxoBlock_CreateFromDef(internal)) {
            sRegisteredBlocks.push_back({def.id, def.creativeTab, def.name});
            AxoWorldGen_RegisterSpawn(def.id, def.spawn);
            if (!def.customModel.empty())
                AxoModelLoader_Register(def.id, def.customModel);
        }
    }
    sPendingBlocks.clear();
}

void AxoAPI_FlushCreativeMenu() {
    printf("[AxoLoader] FlushCreativeMenu: %u item(s), %u block(s)...\n",
           (unsigned)sRegisteredItems.size(), (unsigned)sRegisteredBlocks.size());
    for (const auto& r : sRegisteredItems)
        AxoItem_AddToCreativeMenu(r.id, r.creativeTab);
    for (const auto& r : sRegisteredBlocks)
        AxoBlock_AddToCreativeMenu(r.id, r.creativeTab);
}

void AxoAPI_FlushRecipeRegistrations() {
    printf("[AxoLoader] FlushRecipeRegistrations: %u recipe(s)...\n", (unsigned)sPendingRecipes.size());
    for (const auto& def : sPendingRecipes)
        AxoRecipe_CreateFromDef(def);
    sPendingRecipes.clear();
}

void AxoAPI_FlushBiomeRegistrations() {
    printf("[AxoLoader] FlushBiomeRegistrations: %u biome(s)...\n", (unsigned)sPendingBiomes.size());
    for (const auto& def : sPendingBiomes)
        AxoBiome_CreateFromDef(def);
    sPendingBiomes.clear();
}

void AxoAPI_FlushCropRegistrations() {
    printf("[AxoLoader] FlushCropRegistrations: %u crop(s)...\n", (unsigned)sPendingCrops.size());
    for (const auto& def : sPendingCrops)
        AxoCrop_CreateFromDef(def);
    sPendingCrops.clear();
}

void AxoAPI_RegisterCropSeedForCreative(int seedItemId, int creativeTab) {
    sRegisteredItems.push_back({seedItemId, creativeTab, ""});
}

int AxoAPI_ResolveItemName(const std::string& name) {
    const auto& map = GetVanillaItemMap();
    auto it = map.find(name);
    if (it != map.end()) return it->second;
    const auto& tileMap = GetVanillaTileMap();
    auto it2 = tileMap.find(name);
    if (it2 != tileMap.end()) return it2->second;
    for (const auto& r : sRegisteredItems) {
        if (r.name == name) return r.id;
    }
    for (const auto& r : sRegisteredBlocks) {
        if (r.name == name) return r.id;
    }
    return -1;
}

int AxoAPI_ResolveBlockName(const std::string& name) {
    const auto& map = GetVanillaTileMap();
    auto it = map.find(name);
    if (it != map.end()) return it->second;
    for (const auto& r : sRegisteredBlocks) {
        if (r.name == name) return r.id;
    }
    printf("[AxoLoader] ResolveBlockName: unknown block \"%s\"\n", name.c_str());
    return -1;
}