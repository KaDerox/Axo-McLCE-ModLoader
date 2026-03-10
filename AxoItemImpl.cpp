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

#include "..\..\Minecraft.World\Item.h"
#include "..\..\Minecraft.World\IconRegister.h"
#include "..\..\Minecraft.World\Icon.h"

#include "..\Common\UI\IUIScene_CreativeMenu.h"

#include "AxoAPI.h"

class AxoItem : public Item {
public:
    std::wstring           mIconName;
    std::string            mDisplayName;
    std::function<void()>  mOnUse;
    std::function<void()>  mOnUseOn;

    explicit AxoItem(const AxoItemDef& def)
        : Item(def.id - 256)
        , mIconName(def.iconName)
        , mDisplayName(def.name)
        , mOnUse(def.onUse)
        , mOnUseOn(def.onUseOn)
    {
        maxStackSize = def.maxStackSize;
        setIconName(mIconName);
    }

    void registerIcons(IconRegister* iconRegister) override {
        icon = iconRegister->registerIcon(mIconName);
    }

    std::wstring getName() override {
        return std::wstring(mDisplayName.begin(), mDisplayName.end());
    }

    std::wstring getHoverName(shared_ptr<ItemInstance>) override {
        return std::wstring(mDisplayName.begin(), mDisplayName.end());
    }
};

bool AxoItem_CreateFromDef(const AxoItemDef& def) {
    if (Item::items[def.id] != NULL) {
        printf("[AxoLoader] id %d already taken, skipping \"%s\".\n", def.id, def.name.c_str());
        return false;
    }
    new AxoItem(def);
    printf("[AxoLoader] Created AxoItem id=%d \"%s\"\n", def.id, def.name.c_str());
    return true;
}

void AxoItem_AddToCreativeMenu(int itemId, int creativeTab) {
    IUIScene_CreativeMenu::ECreative_Inventory_Groups group = IUIScene_CreativeMenu::eCreativeInventory_Misc;
    if (creativeTab >= 0 && creativeTab < IUIScene_CreativeMenu::eCreativeInventoryGroupsCount)
        group = (IUIScene_CreativeMenu::ECreative_Inventory_Groups)creativeTab;
    IUIScene_CreativeMenu::AxoAddToGroup(group, shared_ptr<ItemInstance>(new ItemInstance(itemId, 1, 0)));
    printf("[AxoLoader] Added item id=%d to creative group %d\n", itemId, (int)group);
}