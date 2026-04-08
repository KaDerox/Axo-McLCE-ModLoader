cmake_minimum_required(VERSION 3.20)
project(HelloMod LANGUAGES CXX)

# Point at the SDK — adjust this path if needed
set(AXO_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../" CACHE PATH "AxoSDK root")

add_library(mod SHARED HelloMod.cpp)

target_include_directories(mod PRIVATE "${AXO_SDK_DIR}/include")
target_compile_definitions(mod PRIVATE AXO_MOD)
set_target_properties(mod PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    MSVC_RUNTIME_LIBRARY "MultiThreaded"
    OUTPUT_NAME "mod"
    PREFIX ""
    SUFFIX ".dll"
)

if(MSVC)
    target_compile_options(mod PRIVATE /W3 /O2)
endif()
