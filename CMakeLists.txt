cmake_minimum_required(VERSION 3.16)
project(SunnyHollowFarm VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)

set(SOURCES
    src/main.cpp
    src/core/Time.cpp
    src/core/Input.cpp
    src/core/Renderer.cpp
    src/core/AssetManager.cpp
    src/core/Game.cpp
    src/world/Map.cpp
    src/entities/Player.cpp
    src/entities/Crop.cpp
    src/entities/Item.cpp
    src/entities/Inventory.cpp
    src/systems/FarmSystem.cpp
    src/ui/BitmapFont.cpp
    src/ui/HUD.cpp
    src/ui/InventoryUI.cpp
    src/systems/CookingSystem.cpp
    src/ui/CookingUI.cpp
    src/entities/NPC.cpp
    src/systems/ShopSystem.cpp
    src/ui/ShopUI.cpp
)

add_executable(sunny_hollow_farm ${SOURCES})

target_include_directories(sunny_hollow_farm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_options(sunny_hollow_farm PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(sunny_hollow_farm PRIVATE
    PkgConfig::SDL2
    PkgConfig::SDL2_IMAGE
    PkgConfig::SDL2_TTF
)
