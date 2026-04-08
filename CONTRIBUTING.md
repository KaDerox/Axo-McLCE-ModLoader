# Contributing to Axo Loader

Thank you for your interest in contributing to Axo Loader.

Contributions are very welcome. If you have ideas for improvements, bug fixes, or new features, feel free to open a pull request or submit an issue to discuss your proposal.

As the project is written in C++, any help with improving the code quality, performance, or structure is especially appreciated.

## How to Contribute

1. Get the modloader .cpp and .h files.
2. Make your improvements or fixes.
3. Submit a pull request with a clear description of what you changed and why.

## Additional Technical Info
1. Loader uses the following source files:
    - **AxoAPI.h** — C-stable ABI public structs + game-internal I-suffixed types
    - **AxoAPI.cpp** — API tables (V1 + V2), registration pipeline, entity spawn functions, vanilla item/block maps
    - **AxoModLoader.h** — Loader function declarations
    - **AxoModLoader.cpp** — Mod loading lifecycle, native ZIP extraction, GDI+ atlas patching, SEH crash wrappers, V1/V2 detection
    - **AxoItemImpl.cpp** — Custom item / food item classes
    - **AxoBlockImpl.cpp / .h** — Custom block class with model, collision, multi-face support
    - **AxoRecipeImpl.cpp** — Crafting and furnace recipe registration
    - **AxoBiomeImpl.cpp** — Custom biome class
    - **AxoCropImpl.cpp** — Custom crop and seed item classes
    - **AxoWorldGen.cpp / .h** — Ore and grass-like world generation
    - **AxoModelLoader.cpp / .h** — Custom JSON block model parser and tessellator
    - **AxoSDK/** — Public SDK for mod developers (header, CMake template, examples)
2. You can get those files from decompiled game files or from this repository!
    
## Reporting Issues

If you find a bug or unexpected behavior, please open an issue or report it on discord and include:
- a clear description of the problem
- if you have clues you can give steps to reproduce it
- any relevant logs or screenshots

## Thank You

Every contribution helps improve the project. Thank you to everyone who takes the time to help.
