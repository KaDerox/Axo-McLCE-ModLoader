#include "stdafx.h"

#include <memory>
#include <string>
#include <functional>
#include <cstdio>

#include "..\..\Minecraft.World\net.minecraft.locale.h"
#include "..\..\Minecraft.World\net.minecraft.world.h"
#include "..\..\Minecraft.World\net.minecraft.world.entity.h"
#include "..\..\Minecraft.World\net.minecraft.world.entity.item.h"
#include "..\..\Minecraft.World\net.minecraft.world.level.h"
#include "..\..\Minecraft.World\net.minecraft.world.level.tile.h"
#include "..\..\Minecraft.World\net.minecraft.world.item.h"
#include "..\..\Minecraft.World\HtmlString.h"

#include "..\..\Minecraft.World\Tile.h"
#include "..\..\Minecraft.World\Material.h"
#include "..\..\Minecraft.World\IconRegister.h"
#include "..\..\Minecraft.World\Icon.h"

#include "..\Common\UI\IUIScene_CreativeMenu.h"
#include "..\..\Minecraft.World\ItemInstance.h"
#include "..\..\Minecraft.World\TileItem.h"

#include "AxoAPI.h"

bool AxoModLoader_IsTerrainIconQueued(const std::wstring& name);

class AxoBlock : public Tile {
    std::string mDisplayName;
    int         mDropItemId;
    int         mDropCount;

public:
    explicit AxoBlock(const AxoBlockDef& def)
        : Tile(def.id, Material::stone)
        , mDisplayName(def.name)
        , mDropItemId(def.dropItemId)
        , mDropCount(def.dropCount)
    {
        setIconName(def.iconName);
        setDestroyTime(def.hardness);
        setExplodeable(def.resistance);
    }

    void registerIcons(IconRegister* iconRegister) override {
        if (AxoModLoader_IsTerrainIconQueued(iconName)) {
            icon = iconRegister->registerIcon(iconName);
        } else {
            printf("[AxoLoader] WARNING: No terrain texture for \"%ls\", using stone fallback.\n", iconName.c_str());
            icon = iconRegister->registerIcon(L"stone");
        }
    }

    int getResource(int /*data*/, Random* /*random*/, int /*bonusLevel*/) override {
        return (mDropItemId > 0) ? mDropItemId : id;
    }

    int getResourceCount(Random* /*random*/) override {
        return mDropCount;
    }

    wstring getTileItemIconName() override {
        return L"";
    }

    wstring getName() override {
        return std::wstring(mDisplayName.begin(), mDisplayName.end());
    }
};

bool AxoBlock_CreateFromDef(const AxoBlockDef& def) {
    new AxoBlock(def);

    if (Item::items[def.id] == nullptr)
        Item::items[def.id] = new TileItem(def.id - 256);

    Tile::propagate[def.id] = false;

    printf("[AxoLoader] Created AxoBlock id=%d \"%s\"\n", def.id, def.name.c_str());
    return true;
}

void AxoBlock_AddToCreativeMenu(int blockId, int creativeTab) {
    IUIScene_CreativeMenu::ECreative_Inventory_Groups group = IUIScene_CreativeMenu::eCreativeInventory_BuildingBlocks;
    if (creativeTab >= 0 && creativeTab < IUIScene_CreativeMenu::eCreativeInventoryGroupsCount)
        group = (IUIScene_CreativeMenu::ECreative_Inventory_Groups)creativeTab;
    IUIScene_CreativeMenu::AxoAddToGroup(group, shared_ptr<ItemInstance>(new ItemInstance(blockId, 1, 0)));
    printf("[AxoLoader] Added block id=%d to creative group %d\n", blockId, (int)group);
}