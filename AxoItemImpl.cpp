#include "stdafx.h"

#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
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
#include "..\..\Minecraft.World\MobEffectInstance.h"

#include "..\Common\UI\IUIScene_CreativeMenu.h"

#include "AxoAPI.h"

static int ResolveEffectName(const std::string& name) {
    static const std::unordered_map<std::string, int> kMap = {
        {"speed",           1},
        {"slowness",        2},
        {"haste",           3},
        {"mining_fatigue",  4},
        {"strength",        5},
        {"instant_health",  6},
        {"instant_damage",  7},
        {"jump_boost",      8},
        {"nausea",          9},
        {"regeneration",   10},
        {"resistance",     11},
        {"fire_resistance",12},
        {"water_breathing",13},
        {"invisibility",   14},
        {"blindness",      15},
        {"night_vision",   16},
        {"hunger",         17},
        {"weakness",       18},
        {"poison",         19},
        {"wither",         20},
        {"health_boost",   21},
        {"absorption",     22},
        {"saturation",     23},
    };
    auto it = kMap.find(name);
    if (it != kMap.end()) return it->second;
    printf("[AxoLoader] ResolveEffectName: unknown effect \"%s\"\n", name.c_str());
    return -1;
}

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

    explicit AxoItem(const AxoItemDefI& def)
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

    bool canDestroySpecial(Tile* tile) override {
        if (tile == nullptr) return false;
        Material* mat = tile->material;
        if (mIsPickaxe && (mat == Material::stone || mat == Material::metal || mat == Material::heavyMetal))
            return true;
        if (mIsAxe && mat == Material::wood)
            return true;
        if (mIsShovel && (mat == Material::dirt || mat == Material::sand))
            return true;
        return false;
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
    std::string            mEffectName;
    int                    mEffectDuration;
    int                    mEffectAmplifier;

    explicit AxoFoodItem(const AxoItemDefI& def)
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
        , mEffectName(def.food.effect.effectName)
        , mEffectDuration(def.food.effect.duration)
        , mEffectAmplifier(def.food.effect.amplifier)
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

    shared_ptr<ItemInstance> useTimeDepleted(shared_ptr<ItemInstance> instance, Level* level, shared_ptr<Player> player) override {
        shared_ptr<ItemInstance> result = FoodItem::useTimeDepleted(instance, level, player);
        if (!mEffectName.empty() && mEffectDuration > 0 && !level->isClientSide) {
            int effectId = ResolveEffectName(mEffectName);
            if (effectId >= 0)
                player->addEffect(new MobEffectInstance(effectId, mEffectDuration, mEffectAmplifier));
        }
        return result;
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

    bool canDestroySpecial(Tile* tile) override {
        if (tile == nullptr) return false;
        Material* mat = tile->material;
        if (mIsPickaxe && (mat == Material::stone || mat == Material::metal || mat == Material::heavyMetal))
            return true;
        if (mIsAxe && mat == Material::wood)
            return true;
        if (mIsShovel && (mat == Material::dirt || mat == Material::sand))
            return true;
        return false;
    }
};

bool AxoItem_CreateFromDef(const AxoItemDefI& def) {
    printf("[AxoLoader] CreateFromDef id=%d edible=%d nutrition=%d sat=%.2f isMeat=%d\n",
        def.id, (int)def.isEdible, def.food.nutrition, def.food.saturation, (int)def.food.isMeat);
    fflush(stdout);
    if (Item::items[def.id] != NULL) {
        printf("[AxoLoader] id %d already taken, skipping \"%s\".\n", def.id, def.name.c_str());
        return false;
    }
    if (def.isEdible) {
        printf("[AxoLoader] Creating FoodItem...\n"); fflush(stdout);
        new AxoFoodItem(def);
        printf("[AxoLoader] FoodItem created OK\n"); fflush(stdout);
    } else {
        printf("[AxoLoader] Creating Item...\n"); fflush(stdout);
        new AxoItem(def);
        printf("[AxoLoader] Item created OK\n"); fflush(stdout);
    }
    printf("[AxoLoader] Created AxoItem id=%d \"%s\"%s\n",
           def.id, def.name.c_str(), def.isEdible ? " (edible)" : "");
    return true;
}

void AxoItem_AddToCreativeMenu(int itemId, int creativeTab) {
    IUIScene_CreativeMenu::ECreative_Inventory_Groups group = IUIScene_CreativeMenu::eCreativeInventory_Misc;
    if (creativeTab >= 0 && creativeTab < IUIScene_CreativeMenu::totalGroupCount())
        group = (IUIScene_CreativeMenu::ECreative_Inventory_Groups)creativeTab;
    IUIScene_CreativeMenu::AxoAddToGroup(group, shared_ptr<ItemInstance>(new ItemInstance(itemId, 1, 0)));
    printf("[AxoLoader] Added item id=%d to creative group %d\n", itemId, (int)group);
}