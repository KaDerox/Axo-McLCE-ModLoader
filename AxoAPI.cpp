#include "AxoAPI.h"
#include "AxoModLoader.h"

#include <vector>
#include <cstdio>

extern bool AxoItem_CreateFromDef(const AxoItemDef& def);
extern void AxoItem_AddToCreativeMenu(int itemId, int creativeTab);
extern bool AxoBlock_CreateFromDef(const AxoBlockDef& def);
extern void AxoBlock_AddToCreativeMenu(int blockId, int creativeTab);

AxoAPITable* gAxoAPI = nullptr;

static std::vector<AxoItemDef>  sPendingItems;
static std::vector<AxoBlockDef> sPendingBlocks;

static void Impl_Log(const char* modId, const char* msg) {
    printf("[AxoLoader][%s] %s\n", modId, msg);
    fflush(stdout);
}

static bool Impl_RegisterItem(const AxoItemDef* def) {
    if (!def) return false;
    if (def->id < 422 || def->id > 31999) {
        printf("[AxoLoader] RegisterItem: id %d out of range (422-31999).\n", def->id);
        return false;
    }
    sPendingItems.push_back(*def);
    printf("[AxoLoader] Queued item id=%d \"%s\"\n", def->id, def->name.c_str());
    return true;
}

static bool Impl_RegisterBlock(const AxoBlockDef* def) {
    if (!def) return false;
    if (def->id < 174 || def->id > 255) {
        printf("[AxoLoader] RegisterBlock: id %d out of range (174-255).\n", def->id);
        return false;
    }
    sPendingBlocks.push_back(*def);
    printf("[AxoLoader] Queued block id=%d \"%s\"\n", def->id, def->name.c_str());
    return true;
}

static AxoAPITable sAPITable = {
    Impl_Log,
    Impl_RegisterItem,
    Impl_RegisterBlock
};

AxoAPITable* AxoAPI_GetTable() {
    return &sAPITable;
}

struct RegisteredItem  { int id; int creativeTab; };
struct RegisteredBlock { int id; int creativeTab; };
static std::vector<RegisteredItem>  sRegisteredItems;
static std::vector<RegisteredBlock> sRegisteredBlocks;

void AxoAPI_FlushRegistrations() {
    printf("[AxoLoader] FlushRegistrations: %u item(s)...\n", (unsigned)sPendingItems.size());
    for (unsigned i = 0; i < (unsigned)sPendingItems.size(); ++i) {
        const AxoItemDef& def = sPendingItems[i];
        if (AxoItem_CreateFromDef(def))
            sRegisteredItems.push_back({def.id, def.creativeTab});
    }
    sPendingItems.clear();
}

void AxoAPI_FlushBlockRegistrations() {
    printf("[AxoLoader] FlushBlockRegistrations: %u block(s)...\n", (unsigned)sPendingBlocks.size());
    for (unsigned i = 0; i < (unsigned)sPendingBlocks.size(); ++i) {
        const AxoBlockDef& def = sPendingBlocks[i];
        if (AxoBlock_CreateFromDef(def))
            sRegisteredBlocks.push_back({def.id, def.creativeTab});
    }
    sPendingBlocks.clear();
}

void AxoAPI_FlushCreativeMenu() {
    printf("[AxoLoader] FlushCreativeMenu: %u item(s), %u block(s)...\n",
           (unsigned)sRegisteredItems.size(), (unsigned)sRegisteredBlocks.size());
    for (unsigned i = 0; i < (unsigned)sRegisteredItems.size(); ++i)
        AxoItem_AddToCreativeMenu(sRegisteredItems[i].id, sRegisteredItems[i].creativeTab);
    for (unsigned i = 0; i < (unsigned)sRegisteredBlocks.size(); ++i)
        AxoBlock_AddToCreativeMenu(sRegisteredBlocks[i].id, sRegisteredBlocks[i].creativeTab);
}