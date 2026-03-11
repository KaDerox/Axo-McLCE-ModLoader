
# Axo McLCE ModLoader
[![Discord](https://img.shields.io/badge/Discord-Join%20Server-5865F2?logo=discord&logoColor=white)](https://discord.gg/Rm5fscpnT4)

Axo Minecraft Legacy Console Edition ModLoader is a ModLoader for the McLCE that adds mod compatibility!


![Logo](http://axoloader.eu/api/assets/Logo_Baner.png) 


## Features

- Support for:
    - **[McLCE Version by smartcmd](https://github.com/smartcmd/MinecraftConsoles)**
    - More coming soon
- Make mods using .dll files
- Built-in autocompiler in the installer
- AxoAPI in full development
- You don't need to download a new installer every time there is an update. You will need to download new installer only when there are major changes. The installer uses our API to download the newest ModLoader builds!

## Installation
You will need Visual Studio 2022 for autocompiler. Then clone repository of one of supported version of McLCE and in Visual Studio set Minecraft.Client as a startup project. 
After that get our ModLoader **[here](https://github.com/KaDerox/Axo-McLCE-ModLoader/releases)**! Then launch downloaded exe file and complete setup and you're done!

NOTE: Compilation part of installer opens cmd

## AxoAPI
If you want to create mods for AxoLoader you can read under this section about mod creating using AxoAPI!

**Current AxoAPI features:**
- [x] Custom blocks with custom drop
- [x] Custom items 

**WIP AxoAPI features:**
- [ ] More blocks and item flags
- [ ] Custom recipes
- [ ] Custom entities
- [ ] Custom block generation in the world
- [ ] Custom block models

**AxoAPI features that are in plans:**
- [ ] Custom dimensions
- [ ] Custom World Gen
- [ ] Custom armor
- [ ] And many more!

## Making Mods
Here I will provide examples of using AxoAPI.

### Mod setup
Every Axo mod needs 3 files:
- mod.dll
- manifest.json (currently used as a base for later development)
- textures:
  - items (only if you add items)
  - terrain (only if you add blocks)

### Creating mod.dll
1. Create blank project in VS 2022. Configure the project for the dll export. Open the created project and delete all auto generated files. Download AxoAPI.h from **[here](https://github.com/KaDerox/Axo-McLCE-ModLoader/releases)** and place it in the source files
2. Create YourModName.cpp in the source files. This is the main code for your mod:
   ```
    #define AXO_MOD 
    #define MOD_ID "test_mod"
    #include "AxoAPI.h"

    extern "C" __declspec(dllexport)
    void ModEntry(AxoMod* /*mod*/, AxoAPITable* api) {
        AxoMod_SetAPI(api);
       // Here you register things like blocks, items, etc.
    }
   
    extern "C" __declspec(dllexport)
    void OnTick() {
       // Currently not used for anything.
    }
   
    extern "C" __declspec(dllexport)
    void OnShutdown() {
       // Method called on shutdown
    }
    ```
   
### Adding new blocks
Registering new blocks in Axo is simple! All registrations go in the ```void ModEntry``` function.
Here is an example of adding a block:
```
AxoBlockDef exampleBlock;
exampleBlock.dropItemName = "Example Item"; // "" = drop itself 
exampleBlock.dropCount = 1;
exampleBlock.iconName = L"example_block"; // From textures/terrain
exampleBlock.name = "Example Block";
exampleBlock.hardness = 3.0f;
exampleBlock.resistance = 5.0f;
exampleBlock.creativeTab = AxoTab_BuildingBlocks; // You can find AxoTab references in AxoAPI.h

if AxoAPI_RegisterBlock(&exampleBlock)
    AxoAPI_Log("Example Block DONE.");
else
    AxoAPI_Log("Example Block ERROR.");
```
And that's all you need to register a block! Pretty simple!

### Adding new items
Registering new items in Axo is also very simple! All registrations also go in the ```void ModEntry``` function.
Here is an example of adding an item:
```
AxoItemDef exampleItem;
exampleItem.iconName = L"example_item"; // From textures/items
exampleItem.name = "Example Item";
exampleItem.maxStackSize = 16; // Stack Size

if (AxoAPI_RegisterItem(&exampleItem))
    AxoAPI_Log("Example Item DONE.");
else
    AxoAPI_Log("Example Item ERROR.");
```
Done! Pretty simple aswell!

### Logging in Axo
To log in Axo all you need to use is:
```
AxoAPI_Log("Example Log");
```
And done! Now it logs in debug console!

### Adding textures
In the folder with manifest.json and mod.dll you should have textures folder and there terrain and items. In terrain you put block textures and in items you put item textures.

### Exporting done mod
It is pretty simple! Just compress manifest.json, mod.dll and textures folder into zip file and done! You can put it into mods folder and it should work!

## FAQ
- Q: Was AI used to create Axo?
- A: Yes, it was used for bug fixes, code optimization and to speed up its development. Every line of code is understandable to humans. If you're not happy about it, you don't have to use Axo Loader.
- Q: My compilation takes a long time. Is that normal?
- A: Yes, it's compiling the whole project so it can take a while. From my tests, it takes 2 to 5 minutes. And if it takes a very long time you can cancel compilation in installer and compile it in Visual Studio 2022.

## More about
I started working on it a bit later than everyone else so I'm a little behind but I believe I can catch up to other mod loaders and give players good experience! Also I know that this code could be better but I'm not the greatest at c++ and that's why there is **[CONTRIBUTING.md](https://github.com/KaDerox/Axo-McLCE-ModLoader?tab=contributing-ov-file)**!

## Authors

- [@KaDerox](https://www.github.com/kaDerox)

