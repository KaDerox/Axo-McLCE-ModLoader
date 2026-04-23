# AxoSDK — Mod Development Kit for Minecraft LCE (Prologue)

Build mods for the Axo Mod Loader using a **C-stable ABI** that works across
any MSVC version and CRT configuration.

---

## Quick Start

### 1. Copy the SDK

Place this `AxoSDK/` folder anywhere convenient. Your mod project just needs to
point `AXO_SDK_DIR` at it.

### 2. Create your mod

```
my-mod/
├── CMakeLists.txt    ← copy from AxoSDK/CMakeLists.txt and rename source files
├── MyMod.cpp
├── manifest.json
└── textures/         ← item/block textures (PNG)
    ├── items/
    └── terrain/
```

### 3. manifest.json

```json
{
    "mod_id": "my_mod",
    "api_version": 2
}
```

> **`api_version` must be `2`** for mods compiled with this SDK.
> Old mods without this field are loaded through a V1 compatibility layer.

### 4. Mod entry point

```cpp
#define AXO_MOD
#define MOD_ID "my_mod"
#include "AxoAPI.h"

extern "C" __declspec(dllexport)
void ModEntry(AxoMod* mod, AxoAPITable* api) {
    AxoMod_SetAPI(api);
    AxoAPI_Log("Hello from my mod!");

    AxoItemDef item = {};
    item.name = "My Item";
    item.iconName = L"my_item";
    item.maxStackSize = 64;
    AxoAPI_RegisterItem(&item);
}

// Optional lifecycle hooks:
extern "C" __declspec(dllexport) void OnTick() { }
extern "C" __declspec(dllexport) void OnShutdown() { }
```

### 5. Build (Windows)

```
cd my-mod
cmake -B build -G "Visual Studio 17 2022" -A x64 -DAXO_SDK_DIR=C:/path/to/AxoSDK
cmake --build build --config Release
```

Output: `build/Release/mod.dll`

### 6. Package as a .pck

Create a ZIP file (rename `.zip` → `.pck`) containing:

```
manifest.json
mod.dll
textures/
  items/
    my_item.png        (32×32, name matches iconName)
  terrain/
    my_block.png       (16×16, name matches block iconName)
```

Drop the `.pck` into the game's `mods/` folder.

---

## API Reference

### Registration functions

| Macro | Description |
|-------|-------------|
| `AxoAPI_Log(msg)` | Log a message to the console |
| `AxoAPI_RegisterItem(&def)` | Register a custom item |
| `AxoAPI_RegisterBlock(&def)` | Register a custom block |
| `AxoAPI_RegisterRecipe(&def)` | Register a crafting/furnace recipe |
| `AxoAPI_RegisterBiome(&def)` | Register a custom biome |
| `AxoAPI_RegisterCrop(&def)` | Register a custom crop |

### World interaction (v2)

| Macro | Description |
|-------|-------------|
| `AxoAPI_SpawnEntity(lv, id, x, y, z)` | Spawn an entity by ID |
| `AxoAPI_DropItem(lv, id, n, aux, x, y, z)` | Drop an item stack |
| `AxoAPI_StrikeLightning(lv, x, y, z)` | Strike lightning |
| `AxoAPI_SpawnTnt(lv, x, y, z, fuse)` | Spawn primed TNT |
| `AxoAPI_SpawnFallingBlock(lv, x, y, z, tid, d)` | Spawn a falling block |

### Struct fields

See `AxoAPI.h` for all struct definitions with default values.

---

## Critical Build Requirements

| Setting | Value | Why |
|---------|-------|-----|
| `AXO_MOD` define | Required | Enables mod-side helpers in AxoAPI.h |
| CRT | `/MT` (static Release) | Must match the game — avoids CRT heap crashes |
| C++ Standard | C++17 | Matches game build |
| Platform | x64 | Game is 64-bit only |

> **DO NOT** compile with `/MD` or Debug CRT (`/MTd`, `/MDd`).
> The game uses `/MT` (Release static CRT). Mismatched CRT = crash.

---

## Texture Names

- **Item textures**: `textures/items/<name>.png` — 32×32 pixels.
  `iconName` in AxoItemDef is a wide string matching the filename (no extension).
- **Block textures**: `textures/terrain/<name>.png` — 16×16 pixels.
  `iconName` in AxoBlockDef is a wide string matching the filename.

---

## Version History

| Version | Changes |
|---------|---------|
| **v2** (current) | C-stable ABI, world interaction functions, `api_version` manifest field |
| **v1** (legacy) | Original Axo_McLCE_ModLoader API with `std::string`-based structs |
