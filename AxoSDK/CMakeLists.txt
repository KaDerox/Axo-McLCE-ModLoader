cmake_minimum_required(VERSION 3.20)
project(MyAxoMod LANGUAGES CXX)

# ── Point this at the AxoSDK include directory ──────────────────────────────
# If AxoSDK is next to your mod folder:
#   set(AXO_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../AxoSDK")
# Or provide it on the command line:
#   cmake -DAXO_SDK_DIR=C:/path/to/AxoSDK ..
if(NOT DEFINED AXO_SDK_DIR)
    set(AXO_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../AxoSDK"
        CACHE PATH "Path to the AxoSDK directory")
endif()

# ── Mod source files ────────────────────────────────────────────────────────
add_library(mod SHARED
    MyMod.cpp
)

target_include_directories(mod PRIVATE "${AXO_SDK_DIR}/include")

# ── Required compiler settings ──────────────────────────────────────────────
# AXO_MOD  : tells AxoAPI.h to expose mod-side helpers
# /MT      : static Release CRT (matches the game — avoids CRT crashes)
# C++17    : required standard
target_compile_definitions(mod PRIVATE AXO_MOD)
set_target_properties(mod PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    MSVC_RUNTIME_LIBRARY "MultiThreaded"       # /MT
    OUTPUT_NAME "mod"                           # produces mod.dll
    PREFIX ""                                   # no "lib" prefix
    SUFFIX ".dll"
)

# Strip debug info in Release for smaller DLLs
if(MSVC)
    target_compile_options(mod PRIVATE /W3 /O2)
endif()
