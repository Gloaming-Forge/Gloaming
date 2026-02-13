#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "engine/PolishLuaBindings.hpp"
#include "rendering/RaylibRenderer.hpp"
#include "ecs/CoreSystems.hpp"
#include "gameplay/Gameplay.hpp"
#include "gameplay/GameplayLuaBindings.hpp"
#include "gameplay/EntityLuaBindings.hpp"
#include "gameplay/GameplayLoopSystems.hpp"
#include "gameplay/GameplayLoopLuaBindings.hpp"
#include "gameplay/EnemyLuaBindings.hpp"
#include "gameplay/NPCLuaBindings.hpp"
#include "gameplay/SceneTimerSaveLuaBindings.hpp"
#include "gameplay/ParticlePolishLuaBindings.hpp"
#include "world/WorldGenLuaBindings.hpp"

#include <raylib.h>

namespace gloaming {

bool Engine::init(const std::string& configPath) {
    // Load configuration
    if (!m_config.loadFromFile(configPath)) {
        // Initialize logging with defaults before reporting the error
        Log::init("", "debug");
        LOG_WARN("Could not load config from '{}', using defaults", configPath);
    } else {
        Log::init(
            m_config.getString("logging.file", ""),
            m_config.getString("logging.level", "debug")
        );
        LOG_INFO("Configuration loaded from '{}'", configPath);
    }

    LOG_INFO("Gloaming Engine v{} starting...", kEngineVersion);

    // Create window
    WindowConfig winCfg;
    winCfg.width      = m_config.getInt("window.width", 1280);
    winCfg.height     = m_config.getInt("window.height", 720);
    winCfg.title      = m_config.getString("window.title", "Gloaming");
    winCfg.fullscreen = m_config.getBool("window.fullscreen", false);
    winCfg.vsync      = m_config.getBool("window.vsync", true);

    if (!m_window.init(winCfg)) {
        LOG_CRITICAL("Failed to create window");
        return false;
    }

    LOG_INFO("Window created: {}x{} ({})",
             winCfg.width, winCfg.height,
             winCfg.fullscreen ? "fullscreen" : "windowed");

    // Initialize renderer
    m_renderer = std::make_unique<RaylibRenderer>();
    if (!m_renderer->init(winCfg.width, winCfg.height)) {
        LOG_CRITICAL("Failed to initialize renderer");
        return false;
    }

    // Initialize camera
    m_camera = Camera(static_cast<float>(winCfg.width), static_cast<float>(winCfg.height));

    // Initialize texture manager with renderer
    m_textureManager.setRenderer(m_renderer.get());

    // Initialize sprite batch
    m_spriteBatch.setRenderer(m_renderer.get());
    m_spriteBatch.setCamera(&m_camera);

    // Initialize tile renderer
    m_tileRenderer.setRenderer(m_renderer.get());
    m_tileRenderer.setCamera(&m_camera);

    // Initialize parallax background
    m_parallaxBg.setRenderer(m_renderer.get());
    m_parallaxBg.setCamera(&m_camera);

    LOG_INFO("Rendering systems initialized");

    // Initialize ECS
    m_systemScheduler.init(m_registry, *this);
    m_entityFactory.setTextureManager(&m_textureManager);

    LOG_INFO("ECS initialized");

    // Initialize world system with default config
    TileMapConfig tileMapConfig;
    tileMapConfig.tileSize = m_tileRenderer.getTileSize();
    tileMapConfig.chunkManager.loadRadiusChunks = 3;
    tileMapConfig.chunkManager.unloadRadiusChunks = 5;
    m_tileMap.setConfig(tileMapConfig);

    // Initialize world generator (Stage 12)
    uint64_t worldSeed = 42; // Demo seed, will be overridden if loading
    m_worldGenerator.init(worldSeed);

    // For demo: create a test world
    std::string worldPath = "worlds/test_world";
    if (!m_tileMap.loadWorld(worldPath)) {
        // World doesn't exist, create it
        if (m_tileMap.createWorld(worldPath, "Test World", worldSeed)) {
            LOG_INFO("Created new test world with seed {}", worldSeed);
            // Set spawn point at surface level
            m_tileMap.setSpawnPoint(0.0f, 80.0f * m_tileRenderer.getTileSize());
        } else {
            LOG_WARN("Failed to create test world (directory may be read-only)");
        }
    } else {
        LOG_INFO("Loaded existing test world");
        worldSeed = m_tileMap.getSeed();
        m_worldGenerator.setSeed(worldSeed);
    }

    // Wire the WorldGenerator as the chunk generation callback
    m_tileMap.setGeneratorCallback(m_worldGenerator.asCallback());

    // Position camera at spawn point if world is loaded
    if (m_tileMap.isWorldLoaded()) {
        Vec2 spawn = m_tileMap.getSpawnPoint();
        m_camera.setPosition(spawn.x, spawn.y);
    }

    LOG_INFO("World system initialized");

    // Initialize lighting system (Stage 6)
    {
        LightingSystemConfig lightCfg;
        lightCfg.lightMap.lightFalloff = m_config.getInt("lighting.falloff", 16);
        lightCfg.lightMap.skylightFalloff = m_config.getInt("lighting.skylight_falloff", 10);
        lightCfg.lightMap.maxLightRadius = m_config.getInt("lighting.max_radius", 16);
        lightCfg.lightMap.enableSkylight = m_config.getBool("lighting.skylight", true);
        lightCfg.lightMap.enableSmoothLighting = m_config.getBool("lighting.smooth", true);
        lightCfg.dayNight.dayDurationSeconds = m_config.getFloat("lighting.day_duration", 600.0f);
        lightCfg.recalcInterval = m_config.getFloat("lighting.recalc_interval", 0.1f);
        lightCfg.enabled = m_config.getBool("lighting.enabled", true);

        m_lightingSystem = m_systemScheduler.addSystem<LightingSystem>(
            SystemPhase::PostUpdate, lightCfg);

        LOG_INFO("Lighting system initialized (smooth={}, skylight={}, day_duration={}s)",
                 lightCfg.lightMap.enableSmoothLighting,
                 lightCfg.lightMap.enableSkylight,
                 lightCfg.dayNight.dayDurationSeconds);
    }

    // Initialize audio system (Stage 7)
    {
        AudioConfig audioCfg;
        audioCfg.enabled = m_config.getBool("audio.enabled", true);
        audioCfg.masterVolume = m_config.getFloat("audio.master_volume", 1.0f);
        audioCfg.sfxVolume = m_config.getFloat("audio.sfx_volume", 0.8f);
        audioCfg.musicVolume = m_config.getFloat("audio.music_volume", 0.7f);
        audioCfg.ambientVolume = m_config.getFloat("audio.ambient_volume", 0.8f);
        audioCfg.maxConcurrentSounds = m_config.getInt("audio.max_sounds", 32);
        audioCfg.positionalRange = m_config.getFloat("audio.positional_range", 1000.0f);
        audioCfg.minCrossfade = m_config.getFloat("audio.min_crossfade", 0.5f);

        m_audioSystem = m_systemScheduler.addSystem<AudioSystem>(
            SystemPhase::PostUpdate, audioCfg);

        LOG_INFO("Audio system initialized (enabled={}, master={:.0f}%)",
                 audioCfg.enabled, audioCfg.masterVolume * 100.0f);
    }

    // Initialize UI system (Stage 8)
    {
        m_uiSystem.init(*this);
        LOG_INFO("UI system initialized");
    }

    // Initialize gameplay systems (Stage 9+)
    {
        // Grid movement system (for Pokemon-style games)
        auto* gridSys = m_systemScheduler.addSystem<GridMovementSystem>(SystemPhase::PreUpdate);
        // Set default walkability callback using the tile map
        gridSys->setWalkabilityCallback([this](int tileX, int tileY) -> bool {
            Tile tile = m_tileMap.getTile(tileX, tileY);
            return !tile.isSolid();
        });

        // State machine system (for entity AI)
        m_systemScheduler.addSystem<StateMachineSystem>(SystemPhase::Update);

        // Animation controller system (Stage 10)
        m_systemScheduler.addSystem<AnimationControllerSystem>(SystemPhase::Update);

        // Camera controller system
        m_systemScheduler.addSystem<CameraControllerSystem>(SystemPhase::PostUpdate);

        // Projectile system (Stage 11) — runs after physics in Update phase
        m_systemScheduler.addSystem<ProjectileSystem>(SystemPhase::Update);

        // Initialize entity spawning helpers (Stage 11)
        m_entitySpawning.setRegistry(&m_registry);
        m_entitySpawning.setEntityFactory(&m_entityFactory);

        // Wire dialogue system to use InputActions for key rebinding support
        m_dialogueSystem.setInputActions(&m_inputActions);

        // Initialize tile layer manager — deferred: tile size will be read lazily
        // from the renderer when tiles are registered. For now, use the renderer default.
        m_tileLayers.setTileSize(m_tileRenderer.getTileSize());

        // Gameplay loop systems (Stage 13)
        m_systemScheduler.addSystem<ItemDropSystem>(SystemPhase::Update);
        m_systemScheduler.addSystem<ToolUseSystem>(SystemPhase::Update);
        m_systemScheduler.addSystem<MeleeAttackSystem>(SystemPhase::Update);
        m_systemScheduler.addSystem<CombatSystem>(SystemPhase::Update);

        // Initialize crafting manager
        m_craftingManager.setContentRegistry(&m_modLoader.getContentRegistry());
        m_craftingManager.setTileMap(&m_tileMap);

        // Enemy & AI systems (Stage 14)
        m_enemyAISystem = m_systemScheduler.addSystem<EnemyAISystem>(SystemPhase::Update);
        m_enemySpawnSystem = m_systemScheduler.addSystem<EnemySpawnSystem>(SystemPhase::Update);
        m_systemScheduler.addSystem<LootDropSystem>(SystemPhase::PostUpdate);

        // NPC, Housing & Shop systems (Stage 15)
        m_npcSystem = m_systemScheduler.addSystem<NPCSystem>(SystemPhase::Update);
        m_housingSystem = m_systemScheduler.addSystem<HousingSystem>(SystemPhase::PostUpdate);
        m_shopManager.setContentRegistry(&m_modLoader.getContentRegistry());
        m_shopManager.setEventBus(&m_modLoader.getEventBus());

        // Particle system (Stage 17) — runs in Update phase; rendering is called separately
        m_particleSystem = m_systemScheduler.addSystem<ParticleSystem>(SystemPhase::Update);

        // Scene, Timer & Save systems (Stage 16)
        m_sceneManager.init(*this);
        // TimerSystem and SaveSystem don't need engine-level init() — they're configured later

        // Wire save system to world path if a world is loaded
        if (m_tileMap.isWorldLoaded()) {
            m_saveSystem.setWorldPath(m_tileMap.getWorldFile().getWorldPath());
            m_saveSystem.loadAll();
        }

        LOG_INFO("Gameplay systems initialized (grid movement, state machine, camera controller, "
                 "pathfinding, dialogue, input actions, tile layers, animation controller, "
                 "collision layers, entity spawning, projectile system, "
                 "item drops, tool use, melee attack, combat, crafting, "
                 "enemy AI, enemy spawning, loot drops, "
                 "NPCs, housing, shops, "
                 "scenes, timers, save state, "
                 "particles, tweens, debug drawing)");
    }

    // Initialize mod system
    ModLoaderConfig modConfig;
    modConfig.modsDirectory = m_config.getString("mods.directory", "mods");
    modConfig.configFile = m_config.getString("mods.config", "config/mods.json");

    if (m_modLoader.init(*this, modConfig)) {
        // Register gameplay Lua APIs (available to all mods)
        bindGameplayAPI(
            m_modLoader.getLuaBindings().getState(),
            *this, m_inputActions, m_pathfinder, m_dialogueSystem, m_tileLayers,
            m_collisionLayers);

        // Register entity and projectile Lua APIs (Stage 11)
        auto* projSys = m_systemScheduler.getSystem<ProjectileSystem>();
        if (projSys) {
            bindEntityAPI(
                m_modLoader.getLuaBindings().getState(),
                *this, m_entitySpawning, *projSys, m_collisionLayers);
        } else {
            LOG_WARN("ProjectileSystem not found — entity/projectile Lua APIs will be unavailable");
        }
        // Register world generation Lua APIs (Stage 12)
        bindWorldGenAPI(
            m_modLoader.getLuaBindings().getState(),
            *this, m_worldGenerator);

        // Register gameplay loop Lua APIs (Stage 13)
        bindGameplayLoopAPI(
            m_modLoader.getLuaBindings().getState(),
            *this, m_craftingManager);

        // Register enemy & AI Lua APIs (Stage 14)
        if (m_enemySpawnSystem && m_enemyAISystem) {
            bindEnemyAPI(
                m_modLoader.getLuaBindings().getState(),
                *this, *m_enemySpawnSystem, *m_enemyAISystem);
        }

        // Register NPC, Housing & Shop Lua APIs (Stage 15)
        if (m_npcSystem && m_housingSystem) {
            bindNPCAPI(
                m_modLoader.getLuaBindings().getState(),
                *this, *m_npcSystem, *m_housingSystem, m_shopManager);
        }

        // Register Scene, Timer & Save Lua APIs (Stage 16)
        bindSceneTimerSaveAPI(
            m_modLoader.getLuaBindings().getState(),
            *this, m_sceneManager, m_timerSystem, m_saveSystem);

        // Register Particle, Tween & Debug Lua APIs (Stage 17)
        if (m_particleSystem) {
            bindParticlePolishAPI(
                m_modLoader.getLuaBindings().getState(),
                *this, *m_particleSystem, m_tweenSystem, m_debugDrawSystem);
        }

        // Register Profiler, Resource Manager & Diagnostics Lua APIs (Stage 18)
        bindPolishAPI(
            m_modLoader.getLuaBindings().getState(),
            *this, m_profiler, m_resourceManager, m_diagnosticOverlay);

        LOG_INFO("Gameplay, entity, worldgen, gameplay loop, enemy AI, NPC, scene/timer/save, particle/tween/debug, and profiler/resource/diagnostics Lua APIs registered");

        int discovered = m_modLoader.discoverMods();
        if (discovered > 0) {
            if (m_modLoader.resolveDependencies()) {
                int loaded = m_modLoader.loadMods();
                m_modLoader.postInitMods();
                m_modLoader.getContentRegistry().validateNPCReferences();
                LOG_INFO("Mod system: {}/{} mods loaded successfully", loaded, discovered);
            }
        } else {
            LOG_INFO("No mods found in '{}'", modConfig.modsDirectory);
        }
    } else {
        LOG_WARN("Mod system failed to initialize (non-fatal, continuing without mods)");
    }

    // Initialize profiler and diagnostics (Stage 18)
    {
        int targetFPS = m_config.getInt("profiler.target_fps", 60);
        m_profiler.setTargetFPS(targetFPS);
        m_profiler.setEnabled(m_config.getBool("profiler.enabled", true));

        LOG_INFO("Profiler initialized (target={}fps, budget={:.2f}ms, enabled={})",
                 targetFPS, m_profiler.frameBudgetMs(), m_profiler.isEnabled());
    }

    // Initialize gamepad and input systems (Stage 19A)
    {
        float deadzone = m_config.getFloat("input.gamepad_deadzone", 0.15f);
        m_gamepad.setDeadzone(deadzone);

        bool rumbleEnabled = m_config.getBool("input.rumble_enabled", true);
        float rumbleIntensity = m_config.getFloat("input.rumble_intensity", 1.0f);
        m_haptics.setEnabled(rumbleEnabled);
        m_haptics.setIntensity(rumbleIntensity);

        std::string glyphStyle = m_config.getString("input.glyph_style", "xbox");
        if (glyphStyle == "playstation")       m_inputGlyphProvider.setGlyphStyle(GlyphStyle::PlayStation);
        else if (glyphStyle == "nintendo")     m_inputGlyphProvider.setGlyphStyle(GlyphStyle::Nintendo);
        else if (glyphStyle == "keyboard")     m_inputGlyphProvider.setGlyphStyle(GlyphStyle::Keyboard);
        else if (glyphStyle == "deck")         m_inputGlyphProvider.setGlyphStyle(GlyphStyle::SteamDeck);
        else                                   m_inputGlyphProvider.setGlyphStyle(GlyphStyle::Xbox);

        LOG_INFO("Input systems initialized (gamepad deadzone={:.2f}, rumble={}, glyph_style={})",
                 deadzone, rumbleEnabled, glyphStyle);
    }

    m_running = true;
    LOG_INFO("Engine initialized successfully — Stage 19A: Input System");
    return true;
}

void Engine::run() {
    LOG_INFO("Entering main loop");

    while (m_running && !m_window.shouldClose()) {
        m_profiler.beginFrame();

        double dt = static_cast<double>(GetFrameTime());
        m_time.update(dt);

        {
            auto z = m_profiler.scopedZone("Input");
            processInput();
        }
        {
            auto z = m_profiler.scopedZone("Update");
            update(m_time.deltaTime());
        }
        {
            auto z = m_profiler.scopedZone("Render");
            render();
        }

        m_profiler.endFrame();
    }

    LOG_INFO("Main loop exited");
}

void Engine::processInput() {
    m_input.update();
    m_gamepad.update();
    m_inputActions.latchAxisState(m_gamepad);
    m_inputDeviceTracker.update(m_input, m_gamepad);

    if (m_input.isKeyPressed(KEY_F11)) {
        m_window.toggleFullscreen();
    }

    // Cycle diagnostic overlay (F2)
    if (m_input.isKeyPressed(KEY_F2)) {
        m_diagnosticOverlay.cycle();
        const char* modeStr = "off";
        if (m_diagnosticOverlay.getMode() == DiagnosticMode::Minimal) modeStr = "minimal";
        else if (m_diagnosticOverlay.getMode() == DiagnosticMode::Full) modeStr = "full";
        LOG_INFO("Diagnostic overlay: {}", modeStr);
    }

    // Toggle debug drawing (F3)
    if (m_input.isKeyPressed(KEY_F3)) {
        m_debugDrawSystem.toggle();
        LOG_INFO("Debug drawing {}", m_debugDrawSystem.isEnabled() ? "enabled" : "disabled");
    }

    // Toggle profiler (F4)
    if (m_input.isKeyPressed(KEY_F4)) {
        m_profiler.toggle();
        LOG_INFO("Profiler {}", m_profiler.isEnabled() ? "enabled" : "disabled");
    }

    // Toggle lighting system
    if (m_input.isKeyPressed(KEY_L) && m_lightingSystem) {
        bool wasEnabled = m_lightingSystem->getConfig().enabled;
        m_lightingSystem->setLightingEnabled(!wasEnabled);
        LOG_INFO("Lighting system {}", !wasEnabled ? "enabled" : "disabled");
    }
}

void Engine::update(double dt) {
    float dtFloat = static_cast<float>(dt);

    // Run ECS update systems
    m_systemScheduler.update(dtFloat);

    // Update UI system (processes input, rebuilds dynamic UIs, computes layout)
    m_uiSystem.update(dtFloat);

    // Update parallax background auto-scrolling
    m_parallaxBg.update(dtFloat);

    // Update dialogue system
    m_dialogueSystem.update(dtFloat, m_input);

    // Update scene manager (processes transitions)
    m_sceneManager.update(dtFloat);

    // Update timers (paused when overlay scenes are active)
    m_timerSystem.update(dtFloat, m_registry, m_sceneManager.isPausedByOverlay());

    // Update tweens
    m_tweenSystem.update(dtFloat, m_registry);

    // Update haptics (tick down vibrations)
    m_haptics.update(dtFloat);

    // Update on-screen keyboard
    m_onScreenKeyboard.update(m_input, m_gamepad, dtFloat);

    // Handle camera controls for testing (Stage 1 demo)
    // Only active when no mod has assigned a CameraTarget to any entity.
    // The CameraControllerSystem is always registered, but it does nothing without
    // a target entity — so we check entity count, not system existence.
    {
        bool hasCameraTarget = (m_registry.count<CameraTarget>() > 0);

        if (!hasCameraTarget && !m_uiSystem.isBlockingInput() && !m_dialogueSystem.isBlocking()) {
            float cameraSpeed = 300.0f * dtFloat;
            if (m_input.isKeyDown(KEY_W) || m_input.isKeyDown(KEY_UP)) {
                m_camera.move(0, -cameraSpeed);
            }
            if (m_input.isKeyDown(KEY_S) || m_input.isKeyDown(KEY_DOWN)) {
                m_camera.move(0, cameraSpeed);
            }
            if (m_input.isKeyDown(KEY_A) || m_input.isKeyDown(KEY_LEFT)) {
                m_camera.move(-cameraSpeed, 0);
            }
            if (m_input.isKeyDown(KEY_D) || m_input.isKeyDown(KEY_RIGHT)) {
                m_camera.move(cameraSpeed, 0);
            }

            // Zoom controls
            float zoomSpeed = 1.0f * dtFloat;
            if (m_input.isKeyDown(KEY_Q)) {
                m_camera.zoom(-zoomSpeed);
            }
            if (m_input.isKeyDown(KEY_E)) {
                m_camera.zoom(zoomSpeed);
            }
        }
    }

    // Update world chunk loading based on camera position
    if (m_tileMap.isWorldLoaded()) {
        m_tileMap.update(m_camera);
    }
}

void Engine::render() {
    // Apply camera shake offset before rendering, with RAII guard to ensure undo
    Vec2 shakeOffset = m_tweenSystem.getShakeOffset();
    bool hasShake = (shakeOffset.x != 0.0f || shakeOffset.y != 0.0f);
    if (hasShake) {
        m_camera.move(shakeOffset.x, shakeOffset.y);
    }
    // RAII guard: undo camera shake on scope exit (even if an exception occurs)
    struct ShakeGuard {
        Camera& camera;
        Vec2 offset;
        bool active;
        ~ShakeGuard() {
            if (active) camera.move(-offset.x, -offset.y);
        }
    } shakeGuard{m_camera, shakeOffset, hasShake};

    m_renderer->beginFrame();
    m_renderer->clear(Color(20, 20, 30, 255));

    // Render parallax background layers
    m_parallaxBg.render();

    // Render tile layers: background and decoration (behind entities)
    m_tileLayers.renderLayer(m_tileRenderer, m_camera,
                             static_cast<int>(TileLayerIndex::Background));
    m_tileLayers.renderLayer(m_tileRenderer, m_camera,
                             static_cast<int>(TileLayerIndex::Decoration));

    // Render world tiles (main ground layer)
    if (m_tileMap.isWorldLoaded()) {
        m_tileMap.render(m_tileRenderer, m_camera);
    }

    // Run ECS render systems (dt=0 for render phase, timing not needed)
    m_systemScheduler.render(0.0f);

    // Render tile foreground layer (above entities)
    m_tileLayers.renderLayer(m_tileRenderer, m_camera,
                             static_cast<int>(TileLayerIndex::Foreground));

    // Render particles (after entities, before lighting)
    if (m_particleSystem) {
        m_particleSystem->render(m_renderer.get(), m_camera);
    }

    // Render lighting overlay (after tiles and sprites, before UI)
    if (m_lightingSystem) {
        m_lightingSystem->renderLightOverlay(m_renderer.get(), m_camera);
    }

    // Render debug draw overlay (after lighting, visible over everything in world)
    m_debugDrawSystem.render(m_renderer.get(), m_camera);

    // Render UI screens (after lighting overlay, on top of everything)
    m_uiSystem.render();

    // Render dialogue box (on top of UI)
    m_dialogueSystem.render(m_renderer.get(),
                            m_renderer->getScreenWidth(),
                            m_renderer->getScreenHeight());

    // Render scene transition overlay (on top of everything)
    m_sceneManager.renderTransition(m_renderer.get());

    // Render on-screen keyboard (above everything else)
    m_onScreenKeyboard.render(m_renderer.get());

    // Render diagnostic overlay (Stage 18) — replaces old HUD when active
    if (m_diagnosticOverlay.isVisible()) {
        m_diagnosticOverlay.render(m_renderer.get(), m_profiler, m_resourceManager, *this);
    } else {
        // Default HUD (when diagnostics overlay is off)
        char bannerBuf[128];
        snprintf(bannerBuf, sizeof(bannerBuf), "Gloaming Engine v%s - Stage 19A: Input System", kEngineVersion);
        m_renderer->drawText(bannerBuf, {20, 20}, 20, Color::White());

        char fpsText[64];
        snprintf(fpsText, sizeof(fpsText), "FPS: %d  (%.2f ms)", GetFPS(), m_profiler.frameTimeMs());
        m_renderer->drawText(fpsText, {20, 50}, 16, Color::Green());

        // Camera info
        Vec2 camPos = m_camera.getPosition();
        char camText[128];
        snprintf(camText, sizeof(camText), "Camera: (%.1f, %.1f) Zoom: %.2f",
                 camPos.x, camPos.y, m_camera.getZoom());
        m_renderer->drawText(camText, {20, 80}, 16, Color(100, 200, 255, 255));

        // Chunk info
        if (m_tileMap.isWorldLoaded()) {
            const auto& stats = m_tileMap.getStats();
            char chunkText[128];
            snprintf(chunkText, sizeof(chunkText), "Chunks: %zu loaded | %zu dirty | Rendered: %zu tiles",
                     stats.loadedChunks, stats.dirtyChunks, m_tileRenderer.getTilesRendered());
            m_renderer->drawText(chunkText, {20, 110}, 16, Color(200, 200, 100, 255));
        }

        // Mod info
        char modText[192];
        snprintf(modText, sizeof(modText),
                 "Mods: %zu loaded | Content: %zu tiles, %zu items, %zu enemies, %zu npcs, %zu shops",
                 m_modLoader.loadedCount(),
                 m_modLoader.getContentRegistry().tileCount(),
                 m_modLoader.getContentRegistry().itemCount(),
                 m_modLoader.getContentRegistry().enemyCount(),
                 m_modLoader.getContentRegistry().npcCount(),
                 m_modLoader.getContentRegistry().shopCount());
        m_renderer->drawText(modText, {20, 140}, 16, Color(200, 150, 255, 255));

        // Lighting info
        if (m_lightingSystem) {
            const auto& lStats = m_lightingSystem->getStats();
            const auto& dnc = m_lightingSystem->getDayNightCycle();
            const char* todStr = "?";
            switch (dnc.getTimeOfDay()) {
                case TimeOfDay::Dawn:  todStr = "Dawn";  break;
                case TimeOfDay::Day:   todStr = "Day";   break;
                case TimeOfDay::Dusk:  todStr = "Dusk";  break;
                case TimeOfDay::Night: todStr = "Night"; break;
            }
            char lightText[192];
            snprintf(lightText, sizeof(lightText),
                     "Light: %zu sources | %zu tiles | %.1fms | %s (%.0f%% bright) | Day %d",
                     lStats.pointLightCount, lStats.tilesLit, lStats.lastRecalcTimeMs,
                     todStr, lStats.skyBrightness * 100.0f, dnc.getDayCount());
            m_renderer->drawText(lightText, {20, 170}, 16, Color(255, 220, 100, 255));
        }

        // Audio info
        if (m_audioSystem) {
            auto aStats = m_audioSystem->getStats();
            std::string audioText = "Audio: ";
            audioText += aStats.deviceInitialized ? "ready" : "no device";
            audioText += " | " + std::to_string(aStats.registeredSounds) + " sounds registered";
            audioText += " | " + std::to_string(aStats.activeSounds) + " playing";
            audioText += " | Music: ";
            audioText += aStats.musicPlaying ? aStats.currentMusic : "none";
            m_renderer->drawText(audioText.c_str(), {20, 200}, 16, Color(150, 255, 150, 255));
        }

        // UI info
        {
            auto uiStats = m_uiSystem.getStats();
            char uiText[128];
            snprintf(uiText, sizeof(uiText), "UI: %zu screens (%zu visible) | %zu elements",
                     uiStats.screenCount, uiStats.visibleScreenCount, uiStats.totalElements);
            m_renderer->drawText(uiText, {20, 230}, 16, Color(220, 180, 255, 255));
        }

        // Enemy info (Stage 14)
        if (m_enemySpawnSystem) {
            const auto& eStats = m_enemySpawnSystem->getStats();
            char enemyText[192];
            snprintf(enemyText, sizeof(enemyText),
                     "Enemies: %d active | %d spawned | %d killed | Spawning: %s",
                     eStats.activeEnemies, eStats.totalSpawned, eStats.totalKilled,
                     m_enemySpawnSystem->getConfig().enabled ? "on" : "off");
            m_renderer->drawText(enemyText, {20, 260}, 16, Color(255, 150, 150, 255));
        }

        // NPC info (Stage 15)
        {
            int npcCount = 0;
            m_registry.each<NPCTag>([&npcCount](Entity, const NPCTag&) { ++npcCount; });
            char npcText[128];
            snprintf(npcText, sizeof(npcText), "NPCs: %d active | Rooms: %zu validated",
                     npcCount, m_housingSystem ? m_housingSystem->getValidRoomCount() : 0u);
            m_renderer->drawText(npcText, {20, 290}, 16, Color(150, 200, 255, 255));
        }

        // Scene, Timer & Save info (Stage 16)
        {
            char stageText[192];
            const char* sceneName = m_sceneManager.currentScene().empty() ? "none" : m_sceneManager.currentScene().c_str();
            snprintf(stageText, sizeof(stageText),
                     "Scene: %s | Timers: %zu active | Save: %zu mods%s",
                     sceneName,
                     m_timerSystem.activeCount(),
                     m_saveSystem.modCount(),
                     m_saveSystem.isDirty() ? " (dirty)" : "");
            m_renderer->drawText(stageText, {20, 320}, 16, Color(200, 255, 200, 255));
        }

        // Particle, Tween & Debug info (Stage 17)
        {
            char polishText[256];
            auto pStats = m_particleSystem ? m_particleSystem->getStats()
                                           : ParticleSystem::Stats{};
            snprintf(polishText, sizeof(polishText),
                     "Particles: %zu emitters, %zu alive | Tweens: %zu active | Debug: %s",
                     pStats.activeEmitters, pStats.activeParticles,
                     m_tweenSystem.activeCount(),
                     m_debugDrawSystem.isEnabled() ? "on" : "off");
            m_renderer->drawText(polishText, {20, 350}, 16, Color(255, 200, 150, 255));
        }

        // Profiler & Resources (Stage 18)
        {
            auto rStats = m_resourceManager.getStats();
            char profText[256];
            snprintf(profText, sizeof(profText),
                     "Profiler: %s | Budget: %.0f%% | Resources: %zu tracked",
                     m_profiler.isEnabled() ? "on" : "off",
                     m_profiler.frameBudgetUsage() * 100.0,
                     rStats.totalCount);
            m_renderer->drawText(profText, {20, 380}, 16, Color(200, 220, 255, 255));
        }

        // Input device info (Stage 19A)
        {
            const char* deviceStr = (m_inputDeviceTracker.getActiveDevice() == InputDevice::Gamepad) ? "Gamepad" : "Keyboard/Mouse";
            int gpCount = m_gamepad.getConnectedCount();
            char inputText[128];
            snprintf(inputText, sizeof(inputText), "Input: %s | Gamepads: %d connected",
                     deviceStr, gpCount);
            m_renderer->drawText(inputText, {20, 410}, 16, Color(180, 220, 255, 255));
        }
    }

    m_renderer->drawText("WASD/Arrows: Move | Q/E: Zoom | F2: Diagnostics | F3: Debug | F4: Profiler | L: Light | F11: FS",
                         {20, 440}, 16, Color::Gray());

    m_renderer->endFrame();
    // Camera shake offset is automatically undone by ShakeGuard destructor
}

void Engine::shutdown() {
    LOG_INFO("Shutting down...");

    // Shutdown mods first (they may reference engine resources)
    m_modLoader.shutdown();

    // Shutdown UI system (after mods, before renderer)
    m_uiSystem.shutdown();

    // Flush save system data to disk before closing world
    if (m_saveSystem.isDirty()) {
        LOG_INFO("Saving mod data...");
        m_saveSystem.saveAll();
    }

    // Clear timers and tweens
    m_timerSystem.clear();
    m_tweenSystem.clear();

    // Close world (auto-saves if enabled)
    if (m_tileMap.isWorldLoaded()) {
        LOG_INFO("Closing world...");
        m_tileMap.closeWorld();
    }

    // Shutdown ECS
    m_systemScheduler.shutdown();
    m_registry.clear();

    // Unload all textures
    m_textureManager.unloadAll();

    // Shutdown renderer
    if (m_renderer) {
        m_renderer->shutdown();
        m_renderer.reset();
    }

    m_window.shutdown();
    Log::shutdown();
}

} // namespace gloaming
