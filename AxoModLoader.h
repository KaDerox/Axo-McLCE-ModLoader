#pragma once

#include <string>

void AxoModLoader_PreInit(const char* modsDir);
void AxoModLoader_MidInit();
bool AxoModLoader_IsTerrainIconQueued(const std::wstring& name);
void AxoModLoader_Init(const char* modsDir);
void AxoModLoader_Tick();
void AxoModLoader_Shutdown();

struct AxoAPITable;
AxoAPITable* AxoAPI_GetTable();
void         AxoAPI_FlushRegistrations();
void         AxoAPI_FlushBlockRegistrations();
void         AxoAPI_FlushCreativeMenu();