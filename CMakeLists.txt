cmake_minimum_required(VERSION 3.16)
project(SunnyHollowFarm VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(DEFINED ENV{HOME} AND EXISTS "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu/pkgconfig")
    set(ENV{PKG_CONFIG_PATH} "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu/pkgconfig:$ENV{PKG_CONFIG_PATH}")
endif()

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
    src/systems/TimeSystem.cpp
    src/systems/SaveSystem.cpp
    src/systems/AudioSystem.cpp
    src/systems/FishingSystem.cpp
    src/entities/Chicken.cpp
    src/systems/ChickenSystem.cpp
    src/systems/StatsSystem.cpp
    src/ui/StatsUI.cpp
    src/ui/PauseUI.cpp
    src/ui/TitleUI.cpp
)

add_executable(sunny_hollow_farm ${SOURCES})

target_include_directories(sunny_hollow_farm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(MSVC)
    target_compile_options(sunny_hollow_farm PRIVATE /W3 /permissive- /utf-8)
else()
    target_compile_options(sunny_hollow_farm PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL2-2.32.10")
    set(SDL2_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL2-2.32.10")
    set(SDL2_IMAGE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL2_image-2.8.5")
    set(SDL2_TTF_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL2_ttf-2.24.0")
    set(SDL2_MIXER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL2_mixer-2.8.0")

    target_include_directories(sunny_hollow_farm PRIVATE
        ${SDL2_ROOT}/include
        ${SDL2_IMAGE_ROOT}/include
        ${SDL2_TTF_ROOT}/include
        ${SDL2_MIXER_ROOT}/include
    )

    target_link_directories(sunny_hollow_farm PRIVATE
        ${SDL2_ROOT}/lib/x64
        ${SDL2_IMAGE_ROOT}/lib/x64
        ${SDL2_TTF_ROOT}/lib/x64
        ${SDL2_MIXER_ROOT}/lib/x64
    )

    target_link_libraries(sunny_hollow_farm PRIVATE
        SDL2main
        SDL2
        SDL2_image
        SDL2_ttf
        SDL2_mixer
    )

    # Copy DLLs to target binary directory and project root after build
    add_custom_command(TARGET sunny_hollow_farm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL2_ROOT}/lib/x64/SDL2.dll
            ${SDL2_IMAGE_ROOT}/lib/x64/SDL2_image.dll
            ${SDL2_TTF_ROOT}/lib/x64/SDL2_ttf.dll
            ${SDL2_MIXER_ROOT}/lib/x64/SDL2_mixer.dll
            $<TARGET_FILE_DIR:sunny_hollow_farm>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL2_ROOT}/lib/x64/SDL2.dll
            ${SDL2_IMAGE_ROOT}/lib/x64/SDL2_image.dll
            ${SDL2_TTF_ROOT}/lib/x64/SDL2_ttf.dll
            ${SDL2_MIXER_ROOT}/lib/x64/SDL2_mixer.dll
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
    pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)
    pkg_check_modules(SDL2_MIXER REQUIRED IMPORTED_TARGET SDL2_mixer)

    if(DEFINED ENV{HOME} AND EXISTS "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu")
        target_link_directories(sunny_hollow_farm PRIVATE "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu")
        target_link_options(sunny_hollow_farm PRIVATE "-Wl,--disable-new-dtags,-rpath,$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu")
    endif()

    target_link_libraries(sunny_hollow_farm PRIVATE
        PkgConfig::SDL2
        PkgConfig::SDL2_IMAGE
        PkgConfig::SDL2_TTF
        PkgConfig::SDL2_MIXER
    )
endif()
