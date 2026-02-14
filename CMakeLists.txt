cmake_minimum_required(VERSION 3.20)
project(Gloaming VERSION 0.5.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(GLOAMING_BUILD_TESTS "Build unit tests" ON)
option(GLOAMING_STEAM "Enable Steamworks SDK integration" OFF)
set(STEAMWORKS_SDK_DIR "" CACHE PATH "Path to Steamworks SDK")

# --------------------------------------------------------------------------
# Dependencies via FetchContent
# --------------------------------------------------------------------------
include(FetchContent)

# Raylib
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# EnTT (header-only ECS)
FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.14.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(entt)

# nlohmann/json (header-only JSON)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# spdlog (logging)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(spdlog)

# sol2 (C++ Lua bindings) + Lua
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/walterschell/Lua.git
    GIT_TAG        v5.4.7
    GIT_SHALLOW    TRUE
)
set(LUA_BUILD_COMPILER OFF CACHE BOOL "" FORCE)
set(LUA_BUILD_BINARY OFF CACHE BOOL "" FORCE)
set(LUA_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(lua)

FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG        v3.3.1
    GIT_SHALLOW    TRUE
)
set(SOL2_LUA_VERSION "5.4" CACHE STRING "" FORCE)
set(SOL2_BUILD_LUA FALSE CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(sol2)

# --------------------------------------------------------------------------
# Engine static library
# --------------------------------------------------------------------------
add_library(gloaming_engine STATIC
    # Core engine
    src/engine/Log.cpp
    src/engine/Config.cpp
    src/engine/Time.cpp
    src/engine/Window.cpp
    src/engine/Input.cpp
    src/engine/Engine.cpp
    # Rendering (Stage 1)
    src/rendering/Texture.cpp
    src/rendering/Camera.cpp
    src/rendering/SpriteBatch.cpp
    src/rendering/TileRenderer.cpp
    src/rendering/ParallaxBackground.cpp
    src/rendering/RaylibRenderer.cpp
    # ECS (Stage 2)
    src/ecs/EntityFactory.cpp
    # World (Stage 3)
    src/world/Chunk.cpp
    src/world/ChunkGenerator.cpp
    src/world/ChunkManager.cpp
    src/world/WorldFile.cpp
    src/world/TileMap.cpp
    # Physics (Stage 4)
    src/physics/PhysicsSystem.cpp
    # Lighting (Stage 6)
    src/lighting/LightMap.cpp
    src/lighting/LightingSystem.cpp
    # Audio (Stage 7)
    src/audio/SoundManager.cpp
    src/audio/MusicManager.cpp
    src/audio/AudioSystem.cpp
    # UI System (Stage 8)
    src/ui/UIElement.cpp
    src/ui/UIWidgets.cpp
    src/ui/UILayout.cpp
    src/ui/UIInput.cpp
    src/ui/UISystem.cpp
    # Mod System (Stage 5)
    src/mod/ModManifest.cpp
    src/mod/ContentRegistry.cpp
    src/mod/LuaBindings.cpp
    src/mod/ModLoader.cpp
    src/mod/HotReload.cpp
    # Gameplay Systems (Stage 9)
    src/gameplay/GameplayLuaBindings.cpp
    # Entity Spawning & Projectiles (Stage 11)
    src/gameplay/EntitySpawning.cpp
    src/gameplay/ProjectileSystem.cpp
    src/gameplay/EntityLuaBindings.cpp
    # World Generation (Stage 12)
    src/world/WorldGenerator.cpp
    src/world/BiomeSystem.cpp
    src/world/OreDistribution.cpp
    src/world/StructurePlacer.cpp
    src/world/WorldGenLuaBindings.cpp
    # Gameplay Loop (Stage 13)
    src/gameplay/GameplayLoopSystems.cpp
    src/gameplay/CraftingSystem.cpp
    src/gameplay/GameplayLoopLuaBindings.cpp
    # Enemies & AI (Stage 14)
    src/gameplay/EnemySpawnSystem.cpp
    src/gameplay/EnemyAISystem.cpp
    src/gameplay/LootDropSystem.cpp
    src/gameplay/EnemyLuaBindings.cpp
    # NPCs, Housing & Shops (Stage 15)
    src/gameplay/NPCSystem.cpp
    src/gameplay/HousingSystem.cpp
    src/gameplay/ShopSystem.cpp
    src/gameplay/NPCLuaBindings.cpp
    # Scenes, Timers & Save State (Stage 16)
    src/gameplay/SceneManager.cpp
    src/gameplay/TimerSystem.cpp
    src/gameplay/SaveSystem.cpp
    src/gameplay/SceneTimerSaveLuaBindings.cpp
    # Particles & Polish (Stage 17)
    src/gameplay/ParticleSystem.cpp
    src/gameplay/TweenSystem.cpp
    src/gameplay/DebugDrawSystem.cpp
    src/gameplay/ParticlePolishLuaBindings.cpp
    # Polish & Release (Stage 18)
    src/engine/Profiler.cpp
    src/engine/ResourceManager.cpp
    src/engine/DiagnosticOverlay.cpp
    src/engine/PolishLuaBindings.cpp
    # Input System — Steam Deck Support (Stage 19A)
    src/gameplay/InputActions.cpp
    src/engine/Gamepad.cpp
    src/engine/InputDeviceTracker.cpp
    src/engine/InputGlyphs.cpp
    src/engine/Haptics.cpp
    src/ui/OnScreenKeyboard.cpp
    # Display System — Steam Deck Support (Stage 19B)
    src/rendering/ViewportScaler.cpp
    src/ui/UIScaling.cpp
    # Seamlessness — Steam Deck Support (Stage 19C)
    src/engine/SeamlessnessLuaBindings.cpp
    # System Support — Steam Deck Support (Stage 19D)
    src/engine/SteamIntegration.cpp
    src/engine/SystemSupportLuaBindings.cpp
    # Configuration & Persistence — Steam Deck Support (Stage 19E)
    src/engine/ConfigPersistenceLuaBindings.cpp
)

target_include_directories(gloaming_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(gloaming_engine PUBLIC
    raylib
    EnTT::EnTT
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    sol2
    lua_static
)

# --------------------------------------------------------------------------
# Optional Steamworks SDK (Stage 19D)
# --------------------------------------------------------------------------
if(GLOAMING_STEAM)
    if(STEAMWORKS_SDK_DIR AND EXISTS "${STEAMWORKS_SDK_DIR}")
        target_compile_definitions(gloaming_engine PUBLIC GLOAMING_STEAM)
        target_include_directories(gloaming_engine PUBLIC
            "${STEAMWORKS_SDK_DIR}/public"
        )
        if(WIN32)
            if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                set(_STEAM_LIB_DIR "${STEAMWORKS_SDK_DIR}/redistributable_bin/win64")
                set(_STEAM_LIB_NAME "steam_api64")
            else()
                set(_STEAM_LIB_DIR "${STEAMWORKS_SDK_DIR}/redistributable_bin")
                set(_STEAM_LIB_NAME "steam_api")
            endif()
        elseif(UNIX)
            if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                set(_STEAM_LIB_DIR "${STEAMWORKS_SDK_DIR}/redistributable_bin/linux64")
            else()
                set(_STEAM_LIB_DIR "${STEAMWORKS_SDK_DIR}/redistributable_bin/linux32")
            endif()
            set(_STEAM_LIB_NAME "steam_api")
        endif()
        find_library(STEAM_API_LIBRARY
            NAMES ${_STEAM_LIB_NAME}
            PATHS "${_STEAM_LIB_DIR}"
            NO_DEFAULT_PATH
        )
        if(STEAM_API_LIBRARY)
            target_link_libraries(gloaming_engine PUBLIC ${STEAM_API_LIBRARY})
            message(STATUS "Steamworks SDK linked: ${STEAM_API_LIBRARY}")
        else()
            message(WARNING "GLOAMING_STEAM is ON but steam_api library not found in ${_STEAM_LIB_DIR}")
        endif()
    else()
        message(WARNING "GLOAMING_STEAM is ON but STEAMWORKS_SDK_DIR is not set or does not exist")
    endif()
endif()

# --------------------------------------------------------------------------
# Main executable
# --------------------------------------------------------------------------
add_executable(gloaming src/main.cpp)
target_link_libraries(gloaming PRIVATE gloaming_engine)

# Copy config.json to build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.json
    ${CMAKE_CURRENT_BINARY_DIR}/config.json
    COPYONLY
)

# --------------------------------------------------------------------------
# Tests
# --------------------------------------------------------------------------
if(GLOAMING_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
