
# Axo McLCE ModLoader
[![Discord](https://img.shields.io/badge/Discord-Join%20Server-5865F2?logo=discord&logoColor=white)](https://discord.gg/Rm5fscpnT4)

Axo Minecraft Legacy Console Edition ModLoader is a ModLoader for the McLCE that adds mod compatibility!

> [!IMPORTANT]
> AxoAPI V2 and project structure update is coming soon.

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
You will need Visual Studio 2022 with Desktop development with C++ for autocompiler. Then clone repository of one of supported version of McLCE or download Nightly Source Code (zip) and in Visual Studio set Minecraft.Client as a startup project. 
After that get our ModLoader **[here](https://github.com/KaDerox/Axo-McLCE-ModLoader/releases)**! Then launch downloaded exe file and complete setup and you're done!

NOTE: Compilation part of installer opens cmd

**WARNING**
Axo currently works only on Windows. Linux and macOS are not supported yet, but support will be added in the future.

## AxoAPI
If you want to create mods for AxoLoader you can read under this section about mod creating using AxoAPI!

**Current AxoAPI features:**
- [x] Custom blocks with custom drop
- [x] Custom items
- [x] Food items
- [x] Custom swords, pickaxes, axes and shovels
- [x] Custom recipes (may be bit bugged)
- [x] Custom block generation in the world and nether
- [x] Custom plants
- [x] Cross texture blocks
- [x] Custom block models
- [x] Custom World Gen (Custom biomes)
- [x] Multi sided texture for blocks
- [x] Custom plants with seeds
- [x] Food effects

**WIP AxoAPI features:**
- [ ] Custom entities
- [ ] Custom item models
- [ ] Custom armor

**AxoAPI features that are in plans:**
- [ ] Custom dimensions
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
- models:
  - blocks (only if you add 3D blocks)

### Creating mod.dll
1. Create blank c++ project in VS 2022. Configure the project for the dll export. Download AxoAPI.h from **[here](https://github.com/KaDerox/Axo-McLCE-ModLoader/releases)** and place it in the source files
2. Create YourModName.cpp in the source files. This is the main code for your mod:
   ```
    #define AXO_MOD 
    #define MOD_ID "your_mod"
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

### Manifest.json
Here is an example of how manifest should look like.
```
{
    "mod_id": "example_mod",
    "name": "Example Mod",
    "version": "1.0.0",
    "author": "you",
    "description": "Simple example mod for AxoLoader"
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

### Custom item in Axo
Creating items in Axo is really simple!
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

### Custom food in Axo
To create food in Axo you use the same method that is used for items but you add food flags!
```
AxoItemDef exampleFood;
exampleFood.iconName = L"example_food";
exampleFood.name = "Example Food";
exampleFood.maxStackSize = 16;
exampleFood.isEdible = true; // Declares if it is Edible
exampleFood.food.nutrition = 6; // Food nutrition 
exampleFood.food.saturation = AXO_SATURATION_GOOD; // Food saturation (you can find refrences in AxoAPI.h)
exampleFood.food.isMeat = true; // Declares if wolfs can eat it
exampleFood.food.canAlwaysEat = true; // Declares if you can eat it even with full hunger bar

if (AxoAPI_RegisterItem(&exampleFood))
    AxoAPI_Log("Example Food DONE.");
else
    AxoAPI_Log("Example Food ERROR.");
```
Done!

### Custom swords in Axo
To create sword in Axo you use the same method that is used for items but you add sword flags!
```
AxoItemDef exampleSword;
exampleSword.iconName = L"example_sword";
exampleSword.name = "Example Sword";
exampleSword.attackDamage = 99; // Attack damage base: 1
exampleSwoed.isHandheld = true; // Declares if it's an handheld or not

if (AxoAPI_RegisterItem(&exampleSword))
    AxoAPI_Log("Example Sword DONE.");
else
    AxoAPI_Log("Example Sword ERROR.");
```
And done!

### Custom pickaxes, axes and shovels in Axo
```
AxoItemDef examplePickaxe;
examplePickaxe.iconName = L"example_pickaxe";
examplePickaxe.name = "Example Pickaxe";
examplePickaxe.isPickaxe = true; // Available options are "isPickaxe, isAxe, isShovel"
examplePickaxe.miningSpeed = 9.0f; // Mining speed, base: 1.0f

if (AxoAPI_RegisterItem(&exampleSword))
    AxoAPI_Log("Example Pickaxe DONE.");
else
    AxoAPI_Log("Example Pickaxe ERROR.");
```

### Logging in Axo
To log in Axo all you need to use is:
```
AxoAPI_Log("Example Log");
```
And done! Now it logs in debug console!

## Adding recipes
Here are examples of creating custom recipes

### Furnace recipe
```
AxoRecipeDef exampleFurnace;
exampleFurnace.isFurnace       = true;
exampleFurnace.furnaceInputName = "Example Item"; // Input item name
exampleFurnace.resultItemName   = "diamond"; // Output item name
exampleFurnace.resultCount      = 1;
exampleFurnace.furnaceXP        = 0.7f;

AxoAPI_RegisterRecipe(&exampleFurnace);
```

### Shapeless crafting recipes
```
AxoRecipeDef exampleShapeless;
exampleShapeless.isShaped  = false;
exampleShapeless.isFurnace = false;
exampleShapeless.resultItemName = "Example Item";
exampleShapeless.resultCount    = 1;
exampleShapeless.ingredients[0] = "diamond";
exampleShapeless.ingredients[1] = "emerald";
exampleShapeless.ingredients[2] = "ender_pearl";
exampleShapeless.ingredientCount = 3;
exampleShapeless.recipeGroup = AxoRecipe_Armor; // Declares which crafting tab is in

AxoAPI_RegisterRecipe(&exampleShapeless);
```

### Shaped crafting recipes
```
AxoRecipeDef exampleShaped;
exampleShaped.isShaped      = true;
exampleShaped.isFurnace     = false;
exampleShaped.resultItemName = "Example Item";
exampleShaped.resultCount    = 1;
exampleShaped.grid[0] = { "emerald" };
exampleShaped.grid[1] = { "diamond" };
exampleShaped.grid[2] = { "diamond" };
exampleShaped.grid[3] = { "" };
exampleShaped.grid[4] = { "stick" };
exampleShaped.grid[5] = { "" };
exampleShaped.grid[6] = { "" };
exampleShaped.grid[7] = { "stick" };
exampleShaped.grid[8] = { "" };
exampleShaped.recipeGroup = AxoRecipe_Armor;

AxoAPI_RegisterRecipe(&exampleShaped);
```
Creating recipes here is a bit harder than with others. Here is a table showing the slot number corresponding to the crafting location:

| 0 | 1 | 2 |
| - | - | - |
| 3 | 4 | 5 |
| 6 | 7 | 8 |

Now you know how to add custom recipes using Axo!

### Custom block generation in the overworld
Here are the basics of making block spawn in overworld! It's normal block registration but with spawn flags!
```
AxoBlockDef exampleOre;
exampleOre.dropItemName = "Ruby";
exampleOre.dropCount = 3;
exampleOre.iconName = L"ruby_ore";
exampleOre.name = "Ruby Ore";
exampleOre.hardness = 0.5f;
exampleOre.resistance = 10.0f;
exampleOre.creativeTab = AxoTab_BuildingBlocks;
exampleOre.spawn.enabled = true; // Does it spawn?
exampleOre.spawn.likeOre = true; // Does it spawns like ore?
exampleOre.spawn.frequency = 20; // How frequent it spawns
exampleOre.spawn.veinSize = 4; // Max Vein size 
exampleOre.spawn.yLevelMin = 0; // Min. y Level of spawning
exampleOre.spawn.yLevelMax = 32; // Max. y Level of spawning 
exampleOre.spawn.inOverworld = true; // Does it spawns in overworld?
if AxoAPI_RegisterBlock(&block2)
        AxoAPI_Log("Example Ore DONE.");
else
        AxoAPI_Log("Example Ore ERROR.");
```
And you're done!

### Custom ore generation in the nether
It's simple!
```
AxoBlockDef exampleNetherOre;
exampleNetherOre.dropItemName = "Ruby";
exampleNetherOre.dropCount = 3;
exampleNetherOre.iconName = L"hell_ore";
exampleNetherOre.name = "Hell Ore";
exampleNetherOre.hardness = 0.5f;
exampleNetherOre.resistance = 10.0f;
exampleNetherOre.creativeTab = AxoTab_BuildingBlocks;
exampleNetherOre.spawn.enabled = true; // Declares if spawning is enabled
exampleNetherOre.spawn.likeOre = true; // Declares if it spawns like ores
block5.spawn.inOverworld = false; // Does it spawn in overworld?
exampleNetherOre.spawn.inNether = true; // Does it spawn in nether?
exampleNetherOre.spawn.frequency = 100; // Frequency of spawning
exampleNetherOre.spawn.veinSize = 4; // Max vein size
exampleNetherOre.spawn.yLevelMin = 0; // Min. y Level of spawning
exampleNetherOre.spawn.yLevelMax = 70; // Max. y Level of spawning
if AxoAPI_RegisterBlock(&exampleNetherOre)
        AxoAPI_Log("Example Nether Ore DONE.");
else
        AxoAPI_Log("Example Nether Ore ERROR.");
```
Done!

### Custom plants in axo
It's the same process that is used in creation of blocks but you add few flags!
```
AxoBlockDef exampleFlower;
exampleFlower.name            = "Glow Flower";
exampleFlower.iconName        = L"flower_vines";
exampleFlower.dropItemName = "";
exampleFlower.hardness        = 0.2f;
exampleFlower.resistance      = 0.5f;
exampleFlower.creativeTab     = AxoTab_Decoration;
exampleFlower.spawn.enabled   = true; // Does it have spawning enabled 
exampleFlower.spawn.likeOre   = false; // Set so it doesn't spawn like ore
exampleFlower.spawn.likeGrass = true; // Does it spawn like grass
exampleFlower.spawn.onTerrain = true; // Does it spawn on terrain?
exampleFlower.spawn.onWater = false; // Can it spawn on water?
exampleFlower.spawn.frequency = 30; // Spawning frequency 
exampleFlower.spawn.yLevelMin = 60; Min. y Level of spawning 
exampleFlower.spawn.yLevelMax = 128; // Max. y Level of spawning 
exampleFlower.spawn.inOverworld = true; // Does it spawn in overworld 
exampleFlower.spawn.inBiome = "Plains"; // In which biome it can generate (leave "" for every)
exampleFlower.noCollision       = true; // Does it have collisions?
exampleFlower.canBePlacedOnlyOn = "grass"; // Where can it be placed on?
exampleFlower.renderShape = AxoShape_Cross; // Does it uses cross texture instead of normal block model
exampleFlower.canBeBrokenByHand = true; // Does it drops when broken by hand
if AxoAPI_RegisterBlock(&exampleFlower)
        AxoAPI_Log("Example Flower DONE.");
else
        AxoAPI_Log("Example Flower ERROR.");
```
And you're done!

### Blocks with custom models
It's the same as block but with some flags
```
AxoBlockDef rubySlab;
rubySlab.name        = "Ruby Slab";
rubySlab.iconName    = L"ruby_slab_top";
rubySlab.hardness    = 1.5f;
rubySlab.resistance  = 10.0f;
rubySlab.creativeTab = AxoTab_BuildingBlocks;
rubySlab.customModel = "ruby_slab"; // JSON model from models/blocks
AxoAPI_RegisterBlock(&rubySlab);
```

### Multi texture blocks
```
AxoBlockDef myBlock;
myBlock.name = "Crystal Log";
myBlock.iconName = L"crystal_log_side"; // Placeholder image
myBlock.hasDifferentSides = true;
myBlock.iconTop = L"crystal_log_top"; // Top texture
myBlock.iconBottom = L"crystal_log_top"; // Bottom texture
myBlock.iconNorth = L"crystal_log_side"; // North texture
myBlock.iconSouth = L"crystal_log_side"; // South texture
myBlock.iconEast = L"crystal_log_side"; // East texture
myBlock.iconWest = L"crystal_log_side"; // West texture
AxoAPI_RegisterBlock(&myBlock);
```

### Custom crops with seeds 
```
AxoCropDef crop;
crop.name = "Crop";
// Crop Stages from textures/terrain
crop.stageTextures[0] = L"crop_stage_0";
crop.stageTextures[1] = L"crop_stage_1";
crop.stageTextures[2] = L"crop_stage_2";
crop.stageTextures[3] = L"crop_stage_3";
crop.stageTextures[4] = L"crop_stage_4";
crop.stageTextures[5] = L"crop_stage_5";
crop.stageTextures[6] = L"crop_stage_6";
crop.stageTextures[7] = L"crop_stage_7";
// Seed creation
crop.seedIconName    = L"crop_seeds";
crop.seedName        = "Crop Seeds";
crop.seedCreativeTab = AxoTab_ToolsArmor;
// Grown crop drop
crop.growDrop.itemName      = "Custom Item";
crop.growDrop.count         = 1;
crop.growDrop.seedDropCount = 1;
crop.growDrop.bonusDropMax  = 2;

AxoAPI_RegisterCrop(&crop);
```

### Food Effects
```
AxoItemDef magicfood;
magicfood.name        = "Magic Food";
magicfood.iconName    = L"magic_food";
magicfood.creativeTab = AxoTab_Food;
magicfood.isEdible    = true;
magicfood.food.nutrition  = 4;
magicfood.food.canAlwaysEat = true;
magicfood.food.saturation = AXO_SATURATION_NORMAL;
magicfood.food.effect.effectName = "nausea"; // Effect you want to give
magicfood.food.effect.duration = 200;
magicfood.food.effect.amplifier = 0;

AxoAPI_RegisterItem(&magicfood);
```

### Custom biomes
Simple guide for adding custom biomes
```
AxoBiomeDef crystalForest;
crystalForest.name         = "Crystal Forest";
crystalForest.temperature  = 0.7f;
crystalForest.downfall     = 0.8f;
crystalForest.depth        = 0.1f;
crystalForest.scale        = 0.3f;
crystalForest.grassColor   = 0x00FF88; // Hex colour but # is 0x
crystalForest.foliageColor = 0x00CC66; // Hex colour but # is 0x
crystalForest.waterColor   = 0x0055FF; // Hex colour but # is 0x
crystalForest.skyColor     = 0x88DDFF; // Hex colour but # is 0x
crystalForest.hasRain      = true;
crystalForest.hasSnow      = false;
crystalForest.spawnWeight  = 8; // How often it spawns
crystalForest.treeCount    = 5;
crystalForest.grassCount   = 3;
crystalForest.flowerCount  = 4;
crystalForest.topMaterial  = "grass";
crystalForest.material     = "dirt";
crystalForest.hilliness = 1.0f;
AxoAPI_RegisterBiome(&crystalForest);
```

### Adding textures
In the folder with manifest.json and mod.dll, you should have a textures folder with two subfolders: terrain and items. In the terrain folder you put block textures, and in the items folder you put item textures.

### Exporting a finished mod
It is pretty simple! Just compress manifest.json, mod.dll, models folder and the textures folder into a zip file and you're done! You can put it into the mods folder and it should work.

## FAQ
- **Q**: Was AI used to create Axo?
- **A:** Yes. It was used to help find solutions to bugs, suggest some code optimizations, and speed up development. Every line of code is understandable to humans. If you're not happy about it, you don't have to use Axo Loader.

NOTE: By saying "to speed up development", I mean analyzing functions made by 4J so it takes less time to read thousands of lines of code.
- **Q:** My compilation takes a long time. Is that normal?
- **A:** Yes, it's compiling the whole project so it can take a while. From my tests, it takes 2 to 5 minutes. And if it takes a very long time you can cancel compilation in installer and compile it in Visual Studio 2022.
- **Q:** Is Axo malware?
- **A:** No, it only modifies the game's code. You can see every file in the repository.

## More about
I started working on it a bit later than everyone else so I'm a little behind but I believe I can catch up to other mod loaders and give players good experience! Also I know that this code could be better but I'm not the greatest at c++ and that's why there is **[CONTRIBUTING.md](https://github.com/KaDerox/Axo-McLCE-ModLoader?tab=contributing-ov-file)**!

## Authors

- [@KaDerox](https://www.github.com/kaDerox)

