#include "stdafx.h"

#include <memory>
#include <string>
#include <functional>
#include <cstdio>

#include "..\..\Minecraft.World\net.minecraft.world.item.h"
#include "..\..\Minecraft.World\net.minecraft.world.entity.h"
#include "..\..\Minecraft.World\net.minecraft.world.entity.ai.attributes.h"
#include "..\..\Minecraft.World\net.minecraft.world.entity.player.h"
#include "..\..\Minecraft.World\net.minecraft.world.entity.monster.h"
#include "..\..\Minecraft.World\net.minecraft.world.level.h"
#include "..\..\Minecraft.World\net.minecraft.world.level.tile.h"
#include "..\..\Minecraft.World\net.minecraft.locale.h"
#include "..\..\Minecraft.World\net.minecraft.world.h"
#include "..\..\Minecraft.World\net.minecraft.world.entity.item.h"
#include "..\..\Minecraft.World\HtmlString.h"

#include "..\..\Minecraft.World\Item.h"
#include "..\..\Minecraft.World\FoodItem.h"
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
    int                    mAttackDamage;
    float                  mMiningSpeed;
    bool                   mIsPickaxe;
    bool                   mIsAxe;
    bool                   mIsShovel;

    explicit AxoItem(const AxoItemDef& def)
        : Item(def.id - 256)
        , mIconName(def.iconName)
        , mDisplayName(def.name)
        , mOnUse(def.onUse)
        , mOnUseOn(def.onUseOn)
        , mAttackDamage(def.attackDamage)
        , mMiningSpeed(def.miningSpeed)
        , mIsPickaxe(def.isPickaxe)
        , mIsAxe(def.isAxe)
        , mIsShovel(def.isShovel)
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

    attrAttrModMap* getDefaultAttributeModifiers() override {
        attrAttrModMap* result = Item::getDefaultAttributeModifiers();
        if (mAttackDamage > 1) {
            result->insert(attrAttrModMap::value_type(
                SharedMonsterAttributes::ATTACK_DAMAGE->getId(),
                new AttributeModifier(eModifierId_ITEM_BASEDAMAGE, (float)mAttackDamage, AttributeModifier::OPERATION_ADDITION)
            ));
        }
        return result;
    }

    float getDestroySpeed(shared_ptr<ItemInstance> itemInstance, Tile* tile) override {
        if (tile == nullptr) return 1.0f;
        Material* mat = tile->material;
        if (mIsPickaxe && (mat == Material::stone || mat == Material::metal || mat == Material::heavyMetal))
            return mMiningSpeed;
        if (mIsAxe && (mat == Material::wood || mat == Material::plant || mat == Material::leaves))
            return mMiningSpeed;
        if (mIsShovel && (mat == Material::dirt || mat == Material::sand || mat == Material::clay || mat == Material::snow))
            return mMiningSpeed;
        if (!mIsPickaxe && !mIsAxe && !mIsShovel && mMiningSpeed != 1.0f)
            return mMiningSpeed;
        return 1.0f;
    }
};

class AxoFoodItem : public FoodItem {
public:
    std::wstring           mIconName;
    std::string            mDisplayName;
    std::function<void()>  mOnUse;
    std::function<void()>  mOnUseOn;
    int                    mAttackDamage;
    float                  mMiningSpeed;
    bool                   mIsPickaxe;
    bool                   mIsAxe;
    bool                   mIsShovel;

    explicit AxoFoodItem(const AxoItemDef& def)
        : FoodItem(def.id - 256, def.food.nutrition, def.food.saturation, def.food.isMeat)
        , mIconName(def.iconName)
        , mDisplayName(def.name)
        , mOnUse(def.onUse)
        , mOnUseOn(def.onUseOn)
        , mAttackDamage(def.attackDamage)
        , mMiningSpeed(def.miningSpeed)
        , mIsPickaxe(def.isPickaxe)
        , mIsAxe(def.isAxe)
        , mIsShovel(def.isShovel)
    {
        maxStackSize = def.maxStackSize;
        setIconName(mIconName);
        if (def.food.canAlwaysEat)
            setCanAlwaysEat();
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

    attrAttrModMap* getDefaultAttributeModifiers() override {
        attrAttrModMap* result = Item::getDefaultAttributeModifiers();
        if (mAttackDamage > 1) {
            result->insert(attrAttrModMap::value_type(
                SharedMonsterAttributes::ATTACK_DAMAGE->getId(),
                new AttributeModifier(eModifierId_ITEM_BASEDAMAGE, (float)mAttackDamage, AttributeModifier::OPERATION_ADDITION)
            ));
        }
        return result;
    }

    float getDestroySpeed(shared_ptr<ItemInstance> itemInstance, Tile* tile) override {
        if (tile == nullptr) return 1.0f;
        Material* mat = tile->material;
        if (mIsPickaxe && (mat == Material::stone || mat == Material::metal || mat == Material::heavyMetal))
            return mMiningSpeed;
        if (mIsAxe && (mat == Material::wood || mat == Material::plant || mat == Material::leaves))
            return mMiningSpeed;
        if (mIsShovel && (mat == Material::dirt || mat == Material::sand || mat == Material::clay || mat == Material::snow))
            return mMiningSpeed;
        if (!mIsPickaxe && !mIsAxe && !mIsShovel && mMiningSpeed != 1.0f)
            return mMiningSpeed;
        return 1.0f;
    }
};

bool AxoItem_CreateFromDef(const AxoItemDef& def) {
    if (Item::items[def.id] != NULL) {
        printf("[AxoLoader] id %d already taken, skipping \"%s\".\n", def.id, def.name.c_str());
        return false;
    }
    if (def.isEdible)
        new AxoFoodItem(def);
    else
        new AxoItem(def);
    printf("[AxoLoader] Created AxoItem id=%d \"%s\"%s\n",
           def.id, def.name.c_str(), def.isEdible ? " (edible)" : "");
    return true;
}

void AxoItem_AddToCreativeMenu(int itemId, int creativeTab) {
    IUIScene_CreativeMenu::ECreative_Inventory_Groups group = IUIScene_CreativeMenu::eCreativeInventory_Misc;
    if (creativeTab >= 0 && creativeTab < IUIScene_CreativeMenu::eCreativeInventoryGroupsCount)
        group = (IUIScene_CreativeMenu::ECreative_Inventory_Groups)creativeTab;
    IUIScene_CreativeMenu::AxoAddToGroup(group, shared_ptr<ItemInstance>(new ItemInstance(itemId, 1, 0)));
    printf("[AxoLoader] Added item id=%d to creative group %d\n", itemId, (int)group);
}
