#include "stdafx.h"

#include <string>
#include <cstdio>

#include "..\..\Minecraft.World\Biome.h"
#include "..\..\Minecraft.World\BiomeDecorator.h"
#include "AxoAPI.h"
#include "AxoModLoader.h"
#include "AxoWorldGen.h"

class AxoBiomeDecorator : public BiomeDecorator {
public:
    explicit AxoBiomeDecorator(Biome* biome, const AxoBiomeDefI& def)
        : BiomeDecorator(biome)
    {
        treeCount   = def.treeCount;
        grassCount  = def.grassCount;
        flowerCount = def.flowerCount;
    }
};

class AxoBiome : public Biome {
    int  mGrassColor;
    int  mFoliageColor;
    int  mWaterColor;
    int  mSkyColor;
    bool mHasSnow;
    bool mHasRain;
public:
    explicit AxoBiome(const AxoBiomeDefI& def) : Biome(def.id) {
        m_name      = std::wstring(def.name.begin(), def.name.end());
        temperature = def.temperature;
        downfall    = def.downfall;
        if (def.hilliness > 0.0f) {
            depth = def.hilliness * 1.8f - 0.2f;
            scale = 0.2f + def.hilliness * 1.3f;
        } else {
            depth = def.depth;
            scale = def.scale;
        }
        topMaterial = (byte)AxoAPI_ResolveBlockName(def.topMaterial);
        material    = (byte)AxoAPI_ResolveBlockName(def.material);
        mGrassColor   = def.grassColor;
        mFoliageColor = def.foliageColor;
        mWaterColor   = def.waterColor;
        mSkyColor     = def.skyColor;
        mHasSnow      = def.hasSnow;
        mHasRain      = def.hasRain;
        if (decorator) { delete decorator; }
        decorator = new AxoBiomeDecorator(this, def);
    }

    int  getGrassColor()    override { return mGrassColor; }
    int  getFolageColor()   override { return mFoliageColor; }
    int  getWaterColor()    override { return mWaterColor; }
    int  getSkyColor(float) override { return mSkyColor; }
    bool hasSnow()          override { return mHasSnow; }
    bool hasRain()          override { return mHasRain && !mHasSnow; }

protected:
    BiomeDecorator* createDecorator() { return nullptr; }
};

bool AxoBiome_CreateFromDef(const AxoBiomeDefI& def) {
    printf("[AxoLoader] AxoBiome_CreateFromDef start id=%d \"%s\"\n", def.id, def.name.c_str());
    printf("[AxoLoader]   topMaterial=\"%s\" material=\"%s\"\n", def.topMaterial.c_str(), def.material.c_str());
    AxoBiome* biome = new AxoBiome(def);
    printf("[AxoLoader]   AxoBiome created ok\n");
    if (def.spawnWeight > 0)
        AxoWorldGen_RegisterBiomeSpawn(def.id, def.spawnWeight);
    printf("[AxoLoader] Created AxoBiome id=%d \"%s\"\n", def.id, def.name.c_str());
    return true;
}