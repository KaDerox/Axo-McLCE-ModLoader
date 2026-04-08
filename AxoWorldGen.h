#pragma once
#include <vector>
#include <string>

class Level;
class Random;
class Biome;
struct AxoBlockSpawnDefI;

void AxoWorldGen_RegisterSpawn(int blockId, const AxoBlockSpawnDefI& spawn);
void AxoWorldGen_Decorate(Level* level, Random* random, Biome* biome, int xo, int zo);

struct AxoBiomeSpawnEntry { int biomeId; int weight; };
void AxoWorldGen_RegisterBiomeSpawn(int biomeId, int weight);
const std::vector<AxoBiomeSpawnEntry>& AxoWorldGen_GetCustomBiomes();
