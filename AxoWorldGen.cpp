#include "stdafx.h"

#include <vector>
#include <string>
#include <cstdio>

#include "..\..\Minecraft.World\net.minecraft.world.level.h"
#include "..\..\Minecraft.World\net.minecraft.world.level.tile.h"
#include "..\..\Minecraft.World\net.minecraft.world.level.levelgen.feature.h"
#include "..\..\Minecraft.World\net.minecraft.world.level.biome.h"
#include "..\..\Minecraft.World\OreFeature.h"
#include "..\..\Minecraft.World\Level.h"
#include "..\..\Minecraft.World\Biome.h"

#include "AxoAPI.h"
#include "AxoWorldGen.h"

struct AxoSpawnEntry {
    int               blockId;
    AxoBlockSpawnDefI spawn;
};

static std::vector<AxoSpawnEntry> sSpawnEntries;
static std::vector<AxoBiomeSpawnEntry> sBiomeSpawnEntries;

void AxoWorldGen_RegisterBiomeSpawn(int biomeId, int weight) {
    sBiomeSpawnEntries.push_back({biomeId, weight});
    printf("[AxoLoader] Registered biome spawn id=%d weight=%d\n", biomeId, weight);
}

const std::vector<AxoBiomeSpawnEntry>& AxoWorldGen_GetCustomBiomes() {
    return sBiomeSpawnEntries;
}

void AxoWorldGen_RegisterSpawn(int blockId, const AxoBlockSpawnDefI& spawn) {
    if (!spawn.enabled) return;
    sSpawnEntries.push_back({blockId, spawn});
    printf("[AxoLoader] Registered spawn for block id=%d freq=%d vein=%d y=%d-%d\n",
        blockId, spawn.frequency, spawn.veinSize, spawn.yLevelMin, spawn.yLevelMax);
}

static bool IsWaterBlock(int tileId) {
    return tileId == Tile::water_Id || tileId == Tile::calmWater_Id;
}

void AxoWorldGen_Decorate(Level* level, Random* random, Biome* biome, int xo, int zo) {
    if (sSpawnEntries.empty()) return;

    bool isNether = (biome != nullptr && biome == Biome::hell);

    std::string biomeName = "";
    if (biome && !biome->m_name.empty())
        biomeName = std::string(biome->m_name.begin(), biome->m_name.end());

    level->setInstaTick(true);
    for (const auto& entry : sSpawnEntries) {
        const AxoBlockSpawnDefI& s = entry.spawn;

        if (isNether  && !s.inNether)    continue;
        if (!isNether && !s.inOverworld) continue;

        if (!s.inBiome.empty() && biomeName != s.inBiome)
            continue;

        if (s.likeGrass) {
            for (int i = 0; i < s.frequency; i++) {
                int x = xo + random->nextInt(16) + 8;
                int z = zo + random->nextInt(16) + 8;
                int y = level->getHeightmap(x, z);

                if (y < s.yLevelMin || y > s.yLevelMax)
                    continue;

                int below = level->getTile(x, y - 1, z);
                bool isWater = IsWaterBlock(below);

                if (isWater  && !s.onWater)   continue;
                if (!isWater && !s.onTerrain) continue;

                level->setTileAndUpdate(x, y, z, entry.blockId);
            }
        } else {
            int targetTile = isNether ? Tile::netherRack_Id : Tile::stone_Id;
            OreFeature feature(entry.blockId, s.veinSize, targetTile);
            int range = s.yLevelMax - s.yLevelMin;
            if (range <= 0) range = 1;
            for (int i = 0; i < s.frequency; i++) {
                int x = xo + random->nextInt(16);
                int y = s.yLevelMin + random->nextInt(range);
                int z = zo + random->nextInt(16);
                feature.place(level, random, x, y, z);
            }
        }
    }
    level->setInstaTick(false);
}