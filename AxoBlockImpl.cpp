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
#include "..\..\Minecraft.World\Item.h"
#include "..\..\Minecraft.World\Player.h"
#include "..\..\Minecraft.World\Inventory.h"
#include "..\..\Minecraft.World\AABB.h"

#include "..\Common\UI\IUIScene_CreativeMenu.h"
#include "..\..\Minecraft.World\ItemInstance.h"
#include "..\..\Minecraft.World\TileItem.h"

#include "AxoAPI.h"
#include "AxoModLoader.h"
#include "AxoModelLoader.h"

bool AxoModLoader_IsTerrainIconQueued(const std::wstring& name);
int  AxoAPI_ResolveItemName(const std::string& name);

class AxoBlock : public Tile {
    std::string    mDisplayName;
    int            mDropItemId;
    int            mDropCount;
    AxoRenderShape mRenderShape;
    bool           mNoCollision;
    bool           mCanBeBrokenByHand;
    int            mPlaceOnTileId;
    std::string    mCustomModel;
    bool           mHasDifferentSides;
    std::wstring   mIconNameFaces[6];
    Icon*          mFaceIcons[6];
    std::function<void(int, int, int, Level*, Player*, ItemInstance*)> mOnDestroyed;

    static Player*                  sLastPlayer;
    static shared_ptr<ItemInstance> sLastItem;

public:
    explicit AxoBlock(const AxoBlockDefInternal& def)
        : Tile(def.id, Material::stone)
        , mDisplayName(def.name)
        , mDropItemId(def.dropItemId)
        , mDropCount(def.dropCount)
        , mRenderShape(def.renderShape)
        , mNoCollision(def.noCollision)
        , mCanBeBrokenByHand(def.canBeBrokenByHand)
        , mPlaceOnTileId(def.placeOnTileId)
        , mCustomModel(def.customModel)
        , mHasDifferentSides(def.hasDifferentSides)
        , mOnDestroyed(def.onDestroyed)
    {
        printf("[AxoBlock] CTOR id=%d name=%s def.onDestroyed=%d mOnDestroyed=%d\n",
               def.id, def.name.c_str(), (bool)def.onDestroyed, (bool)mOnDestroyed);
        fflush(stdout);
        mFaceIcons[0] = nullptr; mFaceIcons[1] = nullptr;
        mFaceIcons[2] = nullptr; mFaceIcons[3] = nullptr;
        mFaceIcons[4] = nullptr; mFaceIcons[5] = nullptr;
        mIconNameFaces[0] = def.iconBottom;
        mIconNameFaces[1] = def.iconTop;
        mIconNameFaces[2] = def.iconNorth;
        mIconNameFaces[3] = def.iconSouth;
        mIconNameFaces[4] = def.iconWest;
        mIconNameFaces[5] = def.iconEast;
        setIconName(def.iconName);
        setDestroyTime(def.hardness);
        setExplodeable(def.resistance);
        if (def.renderShape == AxoShape_Cross)
            setLightBlock(0);
    }

    void registerIcons(IconRegister* iconRegister) override {
        if (AxoModLoader_IsTerrainIconQueued(iconName)) {
            icon = iconRegister->registerIcon(iconName);
            AxoModLoader_CacheTerrainIcon(iconName, icon);
        } else {
            printf("[AxoLoader] WARNING: No terrain texture for \"%ls\", using stone fallback.\n", iconName.c_str());
            icon = iconRegister->registerIcon(L"stone");
        }
        if (mHasDifferentSides) {
            for (int f = 0; f < 6; f++) {
                if (!mIconNameFaces[f].empty()) {
                    if (AxoModLoader_IsTerrainIconQueued(mIconNameFaces[f])) {
                        mFaceIcons[f] = iconRegister->registerIcon(mIconNameFaces[f]);
                        AxoModLoader_CacheTerrainIcon(mIconNameFaces[f], mFaceIcons[f]);
                    } else {
                        mFaceIcons[f] = icon;
                    }
                } else {
                    mFaceIcons[f] = icon;
                }
            }
        }
        if (!mCustomModel.empty())
            AxoModelLoader_ResolveIcons(id, iconRegister);
    }

    float getDestroyProgress(shared_ptr<Player> player, Level* level, int x, int y, int z) override {
        sLastPlayer = player.get();
        sLastItem   = player ? player->inventory->getSelected() : nullptr;
        return Tile::getDestroyProgress(player, level, x, y, z);
    }

    bool mayPlace(Level* level, int x, int y, int z) override {
        if (mPlaceOnTileId > 0) {
            int below = level->getTile(x, y - 1, z);
            return below == mPlaceOnTileId;
        }
        return Tile::mayPlace(level, x, y, z);
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

    int getRenderShape() override {
        if (!mCustomModel.empty()) {
            const AxoBlockModel* model = AxoModelLoader_GetModel(id);
            if (model && !model->elements.empty())
                return SHAPE_AXO_CUSTOM_MODEL;
        }
        if (mRenderShape == AxoShape_Cross)
            return Tile::SHAPE_CROSS_TEXTURE;
        return Tile::SHAPE_BLOCK;
    }

    void updateShape(LevelSource* level, int x, int y, int z, int forceData = -1, shared_ptr<TileEntity> forceEntity = shared_ptr<TileEntity>()) override {
        if (!mCustomModel.empty()) {
            const AxoBlockModel* model = AxoModelLoader_GetModel(id);
            if (model && model->elements.size() == 1) {
                const auto& e = model->elements[0];
                setShape(e.from[0], e.from[1], e.from[2], e.to[0], e.to[1], e.to[2]);
                return;
            }
            setShape(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
            return;
        }
        Tile::updateShape(level, x, y, z, forceData, forceEntity);
    }

    Icon* getTexture(int face, int data) override {
        if (!mCustomModel.empty()) {
            const AxoBlockModel* model = AxoModelLoader_GetModel(id);
            if (model && model->elements.size() == 1 && face >= 0 && face < 6) {
                if (model->elements[0].faces[face].icon)
                    return model->elements[0].faces[face].icon;
            }
        }
        if (mHasDifferentSides && face >= 0 && face < 6 && mFaceIcons[face])
            return mFaceIcons[face];
        return icon;
    }

    Icon* getTexture(int face) override {
        return getTexture(face, 0);
    }

    bool isSolidRender(bool isServerLevel) override {
        if (!mCustomModel.empty()) return false;
        return mRenderShape == AxoShape_Cube && !mNoCollision;
    }

    bool isCubeShaped() override {
        if (!mCustomModel.empty()) return false;
        return mRenderShape == AxoShape_Cube && !mNoCollision;
    }

    bool axoCanBeBrokenByHand() { return mCanBeBrokenByHand; }
    bool isAxoCanBeBrokenByHand() override { return mCanBeBrokenByHand; }

    AABB* getTileAABB(Level* level, int x, int y, int z) override {
        if (!mCustomModel.empty()) {
            const AxoBlockModel* model = AxoModelLoader_GetModel(id);
            if (model && !model->elements.empty()) {
                float x0 = 1, y0 = 1, z0 = 1, x1 = 0, y1 = 0, z1 = 0;
                for (const auto& e : model->elements) {
                    if (e.from[0] < x0) x0 = e.from[0];
                    if (e.from[1] < y0) y0 = e.from[1];
                    if (e.from[2] < z0) z0 = e.from[2];
                    if (e.to[0]   > x1) x1 = e.to[0];
                    if (e.to[1]   > y1) y1 = e.to[1];
                    if (e.to[2]   > z1) z1 = e.to[2];
                }
                return AABB::newTemp(x + x0, y + y0, z + z0, x + x1, y + y1, z + z1);
            }
        }
        return Tile::getTileAABB(level, x, y, z);
    }

    AABB* getAABB(Level* level, int x, int y, int z) override {
        if (mNoCollision) return nullptr;
        if (!mCustomModel.empty()) {
            const AxoBlockModel* model = AxoModelLoader_GetModel(id);
            if (model && !model->elements.empty()) {
                float x0 = 1, y0 = 1, z0 = 1, x1 = 0, y1 = 0, z1 = 0;
                for (const auto& e : model->elements) {
                    if (e.from[0] < x0) x0 = e.from[0];
                    if (e.from[1] < y0) y0 = e.from[1];
                    if (e.from[2] < z0) z0 = e.from[2];
                    if (e.to[0]   > x1) x1 = e.to[0];
                    if (e.to[1]   > y1) y1 = e.to[1];
                    if (e.to[2]   > z1) z1 = e.to[2];
                }
                return AABB::newTemp(x + x0, y + y0, z + z0, x + x1, y + y1, z + z1);
            }
        }
        return Tile::getAABB(level, x, y, z);
    }

    void addAABBs(Level* level, int x, int y, int z, AABB* box, AABBList* boxes, shared_ptr<Entity> source) override {
        if (mNoCollision) return;
        if (!mCustomModel.empty()) {
            const AxoBlockModel* model = AxoModelLoader_GetModel(id);
            if (model && model->elements.size() > 1) {
                for (const auto& e : model->elements) {
                    AABB* aabb = AABB::newTemp(
                        x + e.from[0], y + e.from[1], z + e.from[2],
                        x + e.to[0],   y + e.to[1],   z + e.to[2]
                    );
                    if (box->intersects(aabb)) boxes->push_back(aabb);
                }
                return;
            }
        }
        Tile::addAABBs(level, x, y, z, box, boxes, source);
    }

    void playerDestroy(Level* level, shared_ptr<Player> player, int x, int y, int z, int data) override {
        printf("[AxoBlock] playerDestroy id=%d at %d,%d,%d mOnDestroyed=%d\n", id, x, y, z, (bool)mOnDestroyed);
        fflush(stdout);
        sLastPlayer = player.get();
        sLastItem   = player ? player->inventory->getSelected() : nullptr;
        Tile::playerDestroy(level, player, x, y, z, data);
        if (mOnDestroyed) {
            printf("[AxoBlock] CALLING mOnDestroyed...\n"); fflush(stdout);
            mOnDestroyed(x, y, z, level, sLastPlayer, sLastItem.get());
            printf("[AxoBlock] mOnDestroyed RETURNED\n"); fflush(stdout);
        } else {
            printf("[AxoBlock] mOnDestroyed is EMPTY, skipping\n"); fflush(stdout);
        }
    }

    void destroy(Level* level, int x, int y, int z, int data) override {
        printf("[AxoBlock] destroy id=%d at %d,%d,%d mOnDestroyed=%d\n", id, x, y, z, (bool)mOnDestroyed); fflush(stdout);
        Tile::destroy(level, x, y, z, data);
        if (mCanBeBrokenByHand) {
            Tile::spawnResources(level, x, y, z, data, 1.0f, 0);
        }
        if (mOnDestroyed) {
            printf("[AxoBlock] destroy: CALLING mOnDestroyed...\n"); fflush(stdout);
            mOnDestroyed(x, y, z, level, sLastPlayer, sLastItem.get());
            printf("[AxoBlock] destroy: mOnDestroyed RETURNED\n"); fflush(stdout);
        }
    }
};

Player*                  AxoBlock::sLastPlayer = nullptr;
shared_ptr<ItemInstance> AxoBlock::sLastItem   = nullptr;

class AxoTileItem : public TileItem {
    std::wstring mDisplayName;
public:
    AxoTileItem(int id, const std::string& name)
        : TileItem(id)
        , mDisplayName(name.begin(), name.end())
    {}

    std::wstring getName() override {
        return mDisplayName;
    }

    std::wstring getHoverName(shared_ptr<ItemInstance>) override {
        return mDisplayName;
    }
};

bool AxoBlock_CreateFromDef(const AxoBlockDefInternal& def) {
    new AxoBlock(def);

    if (!def.customModel.empty()) {
        Tile::solid[def.id]      = false;
        Tile::lightBlock[def.id] = 0;
    }

    if (Item::items[def.id] == nullptr)
        Item::items[def.id] = new AxoTileItem(def.id - 256, def.name);

    Tile::propagate[def.id]    = false;
    Tile::mipmapEnable[def.id] = false;

    printf("[AxoLoader] Created AxoBlock id=%d \"%s\"\n", def.id, def.name.c_str());
    return true;
}

void AxoBlock_AddToCreativeMenu(int blockId, int creativeTab) {
    IUIScene_CreativeMenu::ECreative_Inventory_Groups group = IUIScene_CreativeMenu::eCreativeInventory_BuildingBlocks;
    if (creativeTab >= 0 && creativeTab < IUIScene_CreativeMenu::totalGroupCount())
        group = (IUIScene_CreativeMenu::ECreative_Inventory_Groups)creativeTab;
    IUIScene_CreativeMenu::AxoAddToGroup(group, shared_ptr<ItemInstance>(new ItemInstance(blockId, 1, 0)));
    printf("[AxoLoader] Added block id=%d to creative group %d\n", blockId, (int)group);
}