
# Axo McLCE ModLoader
[![Discord](https://img.shields.io/badge/Discord-Join%20Server-5865F2?logo=discord&logoColor=white)](https://discord.gg/Rm5fscpnT4)

Axo Minecraft Legacy Console Edition ModLoader is a ModLoader for the leaked Minecraft Console Edition that adds mod compatibility!


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
After that get our ModLoader **[here]()**! Then launch downloaded exe file and complete setup and you're done!

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
Here I will provide you with samples of using AxoAPI!

### Mod setup
Every mod for Axo needs 3 files:
    - mod.dll
    - manifest.json (currently used as a base for later development)
    - textures
        - items (only if you add items)
        - terrain (only if you add blocks)
### Creating mod.dll
1. Create blank DLL project. Open created project and delete genereted .cpp files. Download AxoAPI.h from [here](a) and put it into source files.
2. Create YourModName.cpp in source files. This is main code for your mod:
   ```
    #define AXO_MOD 
    #define MOD_ID "test_mod"
    #include "AxoAPI.h"

    extern "C" __declspec(dllexport)
    void ModEntry(AxoMod* /*mod*/, AxoAPITable* api) {
        AxoMod_SetAPI(api); 

    }

    extern "C" __declspec(dllexport)
    void OnTick() {
    }

    extern "C" __declspec(dllexport)
    void OnShutdown() {
    
    }
    ```


## More about
I started working on it a bit later than everyone else so I'm a little behind but I believe I can catch up to other mod loaders and give players good experience!

## Authors

- [@KaDerox](https://www.github.com/kaDerox)

