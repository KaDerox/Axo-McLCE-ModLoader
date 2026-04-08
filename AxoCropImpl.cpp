#include "stdafx.h"

#include <memory>
#include <string>
#include <cstdio>

#include "..\..\Minecraft.World\net.minecraft.world.level.h"
#include "..\..\Minecraft.World\net.minecraft.world.entity.player.h"
#include "..\..\Minecraft.World\net.minecraft.world.level.tile.h"
#include "..\..\Minecraft.World\net.minecraft.world.item.h"
#include "..\..\Minecraft.World\net.minecraft.world.entity.item.h"
#include "..\..\Minecraft.World\net.minecraft.world.h"
#include "..\..\Minecraft.World\Item.h"
#include "..\..\Minecraft.World\IconRegister.h"
#include "..\..\Minecraft.World\Icon.h"
#include "..\..\Minecraft.World\ItemInstance.h"
#include "..\..\Minecraft.World\Tile.h"

#include "..\..\Minecraft.World\CropTile.h"
#include "..\..\Minecraft.World\SeedItem.h"
#include "AxoAPI.h"
#include "AxoModLoader.h"

class AxoCrop : public CropTile {
    int          mSeedItemId;
    int          mDropItemId;
    int          mDropCount;
    int          mSeedDropCount;
    int          mBonusDropMax;
    std::wstring mStageTextures[8];
    Icon*        mStageIcons[8];

public:
    AxoCrop(int tileId, int seedItemId, int dropItemId, int dropCount, int seedDropCount, int bonusDropMax, const std::wstring stageTextures[8])
        : CropTile(tileId)
        , mSeedItemId(seedItemId)
        , mDropItemId(dropItemId)
        , mDropCount(dropCount)
        , mSeedDropCount(seedDropCount)
        , mBonusDropMax(bonusDropMax)
    {
        for (int i = 0; i < 8; i++) {
            mStageTextures[i] = stageTextures[i];
            mStageIcons[i]    = nullptr;
        }
    }

    void registerIcons(IconRegister* iconRegister) override {
        for (int i = 0; i < 8; i++) {
            if (!mStageTextures[i].empty())
                mStageIcons[i] = iconRegister->registerIcon(mStageTextures[i]);
        }
    }

    int getRenderShape() override {
        return Tile::SHAPE_CROSS_TEXTURE;
    }

    Icon* getTexture(int face, int data) override {
        if (data < 0 || data > 7) data = 7;
        return mStageIcons[data] ? mStageIcons[data] : mStageIcons[7];
    }

protected:
    int getBaseSeedId() override {
        return mSeedItemId;
    }

    int getBasePlantId() override {
        return mDropItemId;
    }

public:
    int getResource(int data, Random* random, int playerBonusLevel) override {
        if (data == 7)
            return mDropItemId;
        return mSeedItemId;
    }

    int getResourceCount(Random* random) override {
        return mDropCount;
    }

    void spawnResources(Level* level, int x, int y, int z, int data, float odds, int playerBonus) override {
        if (level->isClientSide)
            return;

        if (data >= 7) {
            for (int i = 0; i < mDropCount; i++)
                popResource(level, x, y, z, std::make_shared<ItemInstance>(mDropItemId, 1, 0));

            int seedsToDrop = mSeedDropCount;
            if (mBonusDropMax > 0)
                seedsToDrop += level->random->nextInt(mBonusDropMax + 1) + playerBonus;

            for (int i = 0; i < seedsToDrop; i++)
                popResource(level, x, y, z, std::make_shared<ItemInstance>(mSeedItemId, 1, 0));
        } else {
            popResource(level, x, y, z, std::make_shared<ItemInstance>(mSeedItemId, 1, 0));
        }
    }
};

class AxoCropSeedItem : public SeedItem {
    std::wstring mIconName;
    std::string  mDisplayName;

public:
    AxoCropSeedItem(int id, int cropTileId, const std::wstring& iconName, const std::string& displayName)
        : SeedItem(id, cropTileId, Tile::farmland_Id)
        , mIconName(iconName)
        , mDisplayName(displayName)
    {
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

static int sNextCropSeedItemId = 422;

bool AxoCrop_CreateFromDef(const AxoCropDefI& def) {
    if (Tile::tiles[def.id] != nullptr) {
        printf("[AxoLoader] AxoCrop: tile id %d already taken, skipping \"%s\".\n", def.id, def.name.c_str());
        return false;
    }

    int resolvedDropId = AxoAPI_ResolveItemName(def.growDrop.itemName);
    if (resolvedDropId == -1) {
        printf("[AxoLoader] AxoCrop: growDrop item \"%s\" not found for \"%s\".\n",
            def.growDrop.itemName.c_str(), def.name.c_str());
        return false;
    }

    while (sNextCropSeedItemId <= 31999 && Item::items[sNextCropSeedItemId] != nullptr)
        sNextCropSeedItemId++;

    if (sNextCropSeedItemId > 31999) {
        printf("[AxoLoader] AxoCrop: no free item ID for seed of \"%s\".\n", def.name.c_str());
        return false;
    }

    int seedItemId = sNextCropSeedItemId++;

    std::string seedName = def.seedName.empty() ? (def.name + " Seeds") : def.seedName;

    new AxoCropSeedItem(
        seedItemId - 256,
        def.id,
        def.seedIconName,
        seedName
    );

    new AxoCrop(
        def.id,
        seedItemId,
        resolvedDropId,
        def.growDrop.count,
        def.growDrop.seedDropCount,
        def.growDrop.bonusDropMax,
        def.stageTextures
    );

    AxoAPI_RegisterCropSeedForCreative(seedItemId, def.seedCreativeTab);

    printf("[AxoLoader] Created AxoCrop tileId=%d seedItemId=%d drop=\"%s\"x%d seeds=%d bonus=%d\n",
        def.id, seedItemId, def.growDrop.itemName.c_str(), def.growDrop.count,
        def.growDrop.seedDropCount, def.growDrop.bonusDropMax);
    return true;
}