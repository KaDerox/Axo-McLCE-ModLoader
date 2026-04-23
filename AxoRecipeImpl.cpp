#include "stdafx.h"
#include <map>
#include "..\..\Minecraft.World\net.minecraft.world.item.h"
#include "..\..\Minecraft.World\Item.h"
#include "..\..\Minecraft.World\FurnaceRecipes.h"
#include "..\..\Minecraft.World\Recipes.h"
#include "AxoAPI.h"
#include "AxoModLoader.h"

static int ResolveItemName(const std::string& name) {
    return AxoAPI_ResolveItemName(name);
}

static wchar_t GroupFromTab(int group) {
    switch (group) {
    case 0:  return L'F';
    case 1:  return L'T';
    case 2:  return L'A';
    case 3:  return L'M';
    case 4:  return L'V';
    case 5:  return L'S';
    default: return L'D';
    }
}

bool AxoRecipe_CreateFromDef(const AxoRecipeDefI& def) {

    if (def.isFurnace) {
        int inputId  = ResolveItemName(def.furnaceInputName);
        int outputId = ResolveItemName(def.resultItemName);
        if (inputId == -1) {
            printf("[AxoLoader] Furnace recipe: input \"%s\" not found\n", def.furnaceInputName.c_str());
            return false;
        }
        if (outputId == -1) {
            printf("[AxoLoader] Furnace recipe: output \"%s\" not found\n", def.resultItemName.c_str());
            return false;
        }
        FurnaceRecipes* fr = FurnaceRecipes::getInstance();
        if (!fr) { printf("[AxoLoader] FurnaceRecipes not initialized\n"); return false; }
        Item* outItem = Item::items[outputId];
        if (!outItem) { printf("[AxoLoader] Furnace recipe: output item ptr null for id %d\n", outputId); return false; }
        fr->addFurnaceRecipy(inputId, new ItemInstance(outItem, def.resultCount, 0), def.furnaceXP);
        printf("[AxoLoader] Furnace recipe registered: %s -> %s (x%d)\n",
            def.furnaceInputName.c_str(), def.resultItemName.c_str(), def.resultCount);
        return true;
    }

    if (!def.isShaped) {
        int resultId = ResolveItemName(def.resultItemName);
        if (resultId == -1) {
            printf("[AxoLoader] Shapeless recipe: result \"%s\" not found\n", def.resultItemName.c_str());
            return false;
        }
        Item* resultItem = Item::items[resultId];
        if (!resultItem) { printf("[AxoLoader] Shapeless recipe: result item ptr null for id %d\n", resultId); return false; }

        std::vector<Item*> items;
        for (int idx = 0; idx < def.ingredientCount; idx++) {
            const auto& ing = def.ingredients[idx];
            int id = ResolveItemName(ing);
            if (id == -1) {
                printf("[AxoLoader] Shapeless recipe: ingredient \"%s\" not found\n", ing.c_str());
                return false;
            }
            Item* item = Item::items[id];
            if (!item) { printf("[AxoLoader] Shapeless recipe: ingredient item ptr null for id %d\n", id); return false; }
            items.push_back(item);
        }

        Recipes* r = Recipes::getInstance();
        if (!r) { printf("[AxoLoader] Recipes not initialized\n"); return false; }

        ItemInstance* result = new ItemInstance(resultItem, def.resultCount, 0);
        switch (items.size()) {
        case 1: r->addShapelessRecipy(result, L"ig",          items[0], GroupFromTab(def.recipeGroup)); break;
        case 2: r->addShapelessRecipy(result, L"iig",         items[0], items[1], GroupFromTab(def.recipeGroup)); break;
        case 3: r->addShapelessRecipy(result, L"iiig",        items[0], items[1], items[2], GroupFromTab(def.recipeGroup)); break;
        case 4: r->addShapelessRecipy(result, L"iiiig",       items[0], items[1], items[2], items[3], GroupFromTab(def.recipeGroup)); break;
        case 5: r->addShapelessRecipy(result, L"iiiiig",      items[0], items[1], items[2], items[3], items[4], GroupFromTab(def.recipeGroup)); break;
        case 6: r->addShapelessRecipy(result, L"iiiiiig",     items[0], items[1], items[2], items[3], items[4], items[5], GroupFromTab(def.recipeGroup)); break;
        case 7: r->addShapelessRecipy(result, L"iiiiiiig",    items[0], items[1], items[2], items[3], items[4], items[5], items[6], GroupFromTab(def.recipeGroup)); break;
        case 8: r->addShapelessRecipy(result, L"iiiiiiiig",   items[0], items[1], items[2], items[3], items[4], items[5], items[6], items[7], GroupFromTab(def.recipeGroup)); break;
        case 9: r->addShapelessRecipy(result, L"iiiiiiiiig",  items[0], items[1], items[2], items[3], items[4], items[5], items[6], items[7], items[8], GroupFromTab(def.recipeGroup)); break;
        default:
            printf("[AxoLoader] Shapeless recipe: too many ingredients (%zu)\n", items.size());
            delete result;
            return false;
        }
        printf("[AxoLoader] Shapeless recipe registered: -> %s (x%d)\n",
            def.resultItemName.c_str(), def.resultCount);
        return true;
    }

    {
        int resultId = ResolveItemName(def.resultItemName);
        if (resultId == -1) {
            printf("[AxoLoader] Shaped recipe: result \"%s\" not found\n", def.resultItemName.c_str());
            return false;
        }
        Item* resultItem = Item::items[resultId];
        if (!resultItem) { printf("[AxoLoader] Shaped recipe: result item ptr null for id %d\n", resultId); return false; }

        std::map<wchar_t, Item*> uniqueItems;
        int resolvedGrid[9];
        wchar_t nextSymbol = L'A';
        std::map<int, wchar_t> idToSymbol;

        for (int i = 0; i < 9; i++) {
            if (def.grid[i].itemName.empty()) {
                resolvedGrid[i] = -1;
                continue;
            }
            int id = ResolveItemName(def.grid[i].itemName);
            if (id == -1) {
                printf("[AxoLoader] Shaped recipe: ingredient \"%s\" not found\n", def.grid[i].itemName.c_str());
                return false;
            }
            Item* item = Item::items[id];
            if (!item) { printf("[AxoLoader] Shaped recipe: ingredient item ptr null for id %d\n", id); return false; }
            resolvedGrid[i] = id;
            if (idToSymbol.find(id) == idToSymbol.end()) {
                idToSymbol[id] = nextSymbol;
                uniqueItems[nextSymbol] = item;
                nextSymbol++;
            }
        }

        std::wstring row1, row2, row3;
        for (int i = 0; i < 3; i++) row1 += (resolvedGrid[i] == -1) ? L' ' : idToSymbol[resolvedGrid[i]];
        for (int i = 3; i < 6; i++) row2 += (resolvedGrid[i] == -1) ? L' ' : idToSymbol[resolvedGrid[i]];
        for (int i = 6; i < 9; i++) row3 += (resolvedGrid[i] == -1) ? L' ' : idToSymbol[resolvedGrid[i]];

        Recipes* r = Recipes::getInstance();
        if (!r) { printf("[AxoLoader] Recipes not initialized\n"); return false; }

        ItemInstance* result = new ItemInstance(resultItem, def.resultCount, 0);
        switch (uniqueItems.size()) {
        case 0:
            printf("[AxoLoader] Shaped recipe: empty grid\n");
            delete result;
            return false;
        case 1: {
            auto it = uniqueItems.begin();
            r->addShapedRecipy(result, L"ssscig",
                row1.c_str(), row2.c_str(), row3.c_str(),
                it->first, it->second, GroupFromTab(def.recipeGroup));
        } break;
        case 2: {
            auto it = uniqueItems.begin();
            wchar_t s0=it->first; Item* i0=it->second; ++it;
            wchar_t s1=it->first; Item* i1=it->second;
            r->addShapedRecipy(result, L"ssscicig",
                row1.c_str(), row2.c_str(), row3.c_str(),
                s0, i0, s1, i1, GroupFromTab(def.recipeGroup));
        } break;
        case 3: {
            auto it = uniqueItems.begin();
            wchar_t s0=it->first; Item* i0=it->second; ++it;
            wchar_t s1=it->first; Item* i1=it->second; ++it;
            wchar_t s2=it->first; Item* i2=it->second;
            r->addShapedRecipy(result, L"ssscicicig",
                row1.c_str(), row2.c_str(), row3.c_str(),
                s0, i0, s1, i1, s2, i2, GroupFromTab(def.recipeGroup));
        } break;
        case 4: {
            auto it = uniqueItems.begin();
            wchar_t s0=it->first; Item* i0=it->second; ++it;
            wchar_t s1=it->first; Item* i1=it->second; ++it;
            wchar_t s2=it->first; Item* i2=it->second; ++it;
            wchar_t s3=it->first; Item* i3=it->second;
            r->addShapedRecipy(result, L"ssscicicicig",
                row1.c_str(), row2.c_str(), row3.c_str(),
                s0, i0, s1, i1, s2, i2, s3, i3, GroupFromTab(def.recipeGroup));
        } break;
        case 5: {
            auto it = uniqueItems.begin();
            wchar_t s0=it->first; Item* i0=it->second; ++it;
            wchar_t s1=it->first; Item* i1=it->second; ++it;
            wchar_t s2=it->first; Item* i2=it->second; ++it;
            wchar_t s3=it->first; Item* i3=it->second; ++it;
            wchar_t s4=it->first; Item* i4=it->second;
            r->addShapedRecipy(result, L"sssicicicicig",
                row1.c_str(), row2.c_str(), row3.c_str(),
                s0, i0, s1, i1, s2, i2, s3, i3, s4, i4, GroupFromTab(def.recipeGroup));
        } break;
        default:
            printf("[AxoLoader] Shaped recipe: too many unique ingredients (%zu)\n", uniqueItems.size());
            delete result;
            return false;
        }
        printf("[AxoLoader] Shaped recipe registered: -> %s (x%d)\n",
            def.resultItemName.c_str(), def.resultCount);
        return true;
    }
}
