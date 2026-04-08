#pragma once

#include <string>

class Icon;
class IconRegister;

void AxoModLoader_PreInit(const char* modsDir);
void AxoModLoader_MidInit();
bool AxoModLoader_IsTerrainIconQueued(const std::wstring& name);
void AxoModLoader_Init(const char* modsDir);
void AxoModLoader_Tick();
void AxoModLoader_Shutdown();
void AxoModLoader_CacheTerrainIcon(const std::wstring& name, Icon* icon);
Icon* AxoModLoader_GetTerrainIcon(const std::wstring& name);

struct AxoAPITable;
AxoAPITable* AxoAPI_GetTable();
AxoAPITable* AxoAPI_GetTableV1();
void         AxoAPI_FlushRegistrations();
void         AxoAPI_FlushBlockRegistrations();
void         AxoAPI_FlushCreativeMenu();

struct AxoRecipeDefI;
bool AxoRecipe_CreateFromDef(const AxoRecipeDefI& def);
void AxoAPI_FlushRecipeRegistrations();
void AxoAPI_FlushBiomeRegistrations();
void AxoAPI_FlushCropRegistrations();
void AxoAPI_RegisterCropSeedForCreative(int seedItemId, int creativeTab);
int  AxoAPI_ResolveItemName(const std::string& name);
int  AxoAPI_ResolveBlockName(const std::string& name);