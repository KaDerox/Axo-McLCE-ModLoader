// HelloMod — minimal AxoAPI v2 example
#define AXO_MOD
#define MOD_ID "hello_mod"
#include "AxoAPI.h"

extern "C" __declspec(dllexport)
void ModEntry(AxoMod* mod, AxoAPITable* api) {
    AxoMod_SetAPI(api);
    AxoAPI_Log("Hello from HelloMod!");

    // ── Register a custom item ──────────────────────────────────────────
    AxoItemDef item = {};
    item.name         = "Hello Item";
    item.iconName     = L"hello_item";
    item.maxStackSize = 16;
    item.creativeTab  = AxoTab_Misc;
    AxoAPI_RegisterItem(&item);

    // ── Register a custom block ─────────────────────────────────────────
    AxoBlockDef block = {};
    block.name      = "Hello Block";
    block.iconName  = L"hello_block";
    block.material  = AxoMat_Stone;
    block.hardness  = 2.0f;
    block.canBeBrokenByHand = true;
    block.creativeTab = AxoTab_BuildingBlocks;
    AxoAPI_RegisterBlock(&block);

    // ── Register a recipe: 4 cobblestone → Hello Block ──────────────────
    AxoRecipeDef recipe = {};
    recipe.resultItemName = "Hello Block";
    recipe.resultCount    = 1;
    recipe.isShaped       = false;
    recipe.recipeGroup    = AxoRecipe_Decoration;
    recipe.ingredients[0] = "Cobblestone";
    recipe.ingredients[1] = "Cobblestone";
    recipe.ingredients[2] = "Cobblestone";
    recipe.ingredients[3] = "Cobblestone";
    recipe.ingredientCount = 4;
    AxoAPI_RegisterRecipe(&recipe);

    AxoAPI_Log("HelloMod loaded!");
}

extern "C" __declspec(dllexport)
void OnTick() {
    // Called every game tick — do per-frame logic here
}

extern "C" __declspec(dllexport)
void OnShutdown() {
    // Called when the game shuts down — clean up here
}
