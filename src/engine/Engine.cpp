#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "engine/PolishLuaBindings.hpp"
#include "engine/SeamlessnessLuaBindings.hpp"
#include "engine/SystemSupportLuaBindings.hpp"
#include "engine/ConfigPersistenceLuaBindings.hpp"
#include "rendering/RaylibRenderer.hpp"
#include "ecs/CoreSystems.hpp"
#include "physics/PhysicsSystem.hpp"
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
#include <cstdlib>
#include <csignal>
#include <filesystem>

namespace gloaming {

// Stage 19C: Seamlessness — signal handling
std::atomic<bool> Engine::s_signalReceived{false};

void Engine::signalHandler(int signum) {
    // Signal-safe: only set an atomic flag. Logging and cleanup
    // happen in the main loop when it checks this flag.
    (void)signum;
    s_signalReceived.store(true, std::memory_order_relaxed);
}

void Engine::requestShutdown() {
    m_running = false;
}

void Engine::emitShutdownOnce() {
    if (!m_shutdownEmitted) {
        m_shutdownEmitted = true;
        getEventBus().emit("engine.shutdown");
    }
}

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

    // Stage 19E: Merge per-device local config on top of the base config.
    // config.local.json is device-specific (gitignored, not synced via Steam Cloud)
    // and overrides display/input settings for the current hardware.
    {
        // Derive local config path: "config.json" -> "config.local.json"
        namespace fs = std::filesystem;
        fs::path base(configPath);
        fs::path localFile = base.parent_path()
            / (base.stem().string() + ".local" + base.extension().string());
        m_localConfigPath = localFile.string();

        if (m_config.mergeFromFile(m_localConfigPath)) {
            LOG_INFO("Per-device config merged from '{}'", m_localConfigPath);
        }
    }

    LOG_INFO("Gloaming Engine v{} starting...", kEngineVersion);

    // Platform-aware defaults for Steam Deck
    bool onDeck = SteamIntegration::isSteamDeck();
    int defaultWidth  = 1280;
    int defaultHeight = onDeck ? 800 : 720;
    bool defaultFS    = onDeck;

    // Create window
    WindowConfig winCfg;
    winCfg.width      = m_config.getInt("window.width", defaultWidth);
    winCfg.height     = m_config.getInt("window.height", defaultHeight);
    winCfg.title      = m_config.getString("window.title", "Gloaming");
    winCfg.fullscreen = m_config.getBool("window.fullscreen", defaultFS);
    winCfg.vsync      = m_config.getBool("window.vsync", true);

    // Parse fullscreen mode from config
    std::string fsMode = m_config.getString("window.fullscreen_mode", "borderless");
    if (fsMode == "exclusive")       winCfg.fullscreenMode = FullscreenMode::Fullscreen;
    else if (fsMode == "windowed")   winCfg.fullscreenMode = FullscreenMode::Windowed;
    else                             winCfg.fullscreenMode = FullscreenMode::BorderlessFullscreen;

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
        // Start at mid-day so the default scene isn't pitch-black
        m_lightingSystem->getDayNightCycle().setNormalizedTime(0.50f);

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

        // Physics system — gravity, velocity integration, tile collision
        {
            auto* physics = m_systemScheduler.addSystem<PhysicsSystem>(SystemPhase::Update);
            if (m_tileMap.isWorldLoaded()) {
                physics->setTileMap(&m_tileMap);
            }
        }

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

        // Sprite render system — draws entity sprites during the Render phase
        m_systemScheduler.addSystem<SpriteRenderSystem>(SystemPhase::Render);

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

        // Register Seamlessness Lua APIs (Stage 19C)
        bindSeamlessnessAPI(
            m_modLoader.getLuaBindings().getState(), *this);

        // Register System Support Lua APIs (Stage 19D)
        bindSystemSupportAPI(
            m_modLoader.getLuaBindings().getState(), *this);

        // Register Config Persistence Lua APIs (Stage 19E)
        bindConfigPersistenceAPI(
            m_modLoader.getLuaBindings().getState(), *this);

        LOG_INFO("Gameplay, entity, worldgen, gameplay loop, enemy AI, NPC, scene/timer/save, "
                 "particle/tween/debug, profiler/resource/diagnostics, seamlessness, "
                 "system support, and config persistence Lua APIs registered");

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

        std::string glyphStyle = m_config.getString("input.glyph_style", "auto");
        if (glyphStyle == "auto") {
            // Auto-detect: Xbox glyphs on Steam Deck (ABXY labeling), Keyboard elsewhere
            glyphStyle = onDeck ? "xbox" : "keyboard";
        }
        if (glyphStyle == "playstation")       m_inputGlyphProvider.setGlyphStyle(GlyphStyle::PlayStation);
        else if (glyphStyle == "nintendo")     m_inputGlyphProvider.setGlyphStyle(GlyphStyle::Nintendo);
        else if (glyphStyle == "keyboard")     m_inputGlyphProvider.setGlyphStyle(GlyphStyle::Keyboard);
        else if (glyphStyle == "deck")         m_inputGlyphProvider.setGlyphStyle(GlyphStyle::SteamDeck);
        else                                   m_inputGlyphProvider.setGlyphStyle(GlyphStyle::Xbox);

        LOG_INFO("Input systems initialized (gamepad deadzone={:.2f}, rumble={}, glyph_style={})",
                 deadzone, rumbleEnabled, glyphStyle);
    }

    // Initialize display systems (Stage 19B)
    {
        // ViewportScaler configuration
        ViewportConfig vpCfg;
        vpCfg.designWidth  = m_config.getInt("display.design_width", 1280);
        vpCfg.designHeight = m_config.getInt("display.design_height", 720);

        std::string scaleMode = m_config.getString("display.scale_mode", "expand");
        if (scaleMode == "fill_crop")        vpCfg.scaleMode = ScaleMode::FillCrop;
        else if (scaleMode == "fit_letterbox") vpCfg.scaleMode = ScaleMode::FitLetterbox;
        else if (scaleMode == "stretch")     vpCfg.scaleMode = ScaleMode::Stretch;
        else                                  vpCfg.scaleMode = ScaleMode::Expand;

        m_viewportScaler.configure(vpCfg);
        m_viewportScaler.update(m_window.getWidth(), m_window.getHeight());

        // Update camera to use the effective resolution from ViewportScaler
        m_camera.setScreenSize(
            static_cast<float>(m_viewportScaler.getEffectiveWidth()),
            static_cast<float>(m_viewportScaler.getEffectiveHeight()));

        // UIScaling configuration
        UIScalingConfig uiCfg;
        uiCfg.baseScale   = m_config.getFloat("display.ui_scale", 1.0f);
        uiCfg.minFontSize = m_config.getInt("display.min_font_size", 12);
        uiCfg.dpiScale    = m_config.getFloat("display.dpi_scale", 0.0f);
        bool autoDpi = (uiCfg.dpiScale <= 0.0f);
        if (autoDpi) uiCfg.dpiScale = 1.0f;  // Reset to default before configure
        m_uiScaling.configure(uiCfg);
        if (autoDpi) {
            m_uiScaling.autoDetect(m_window.getWidth(), m_window.getHeight());
        }

        // Target FPS (for battery management on Deck)
        int targetFPS = m_config.getInt("performance.target_fps", onDeck ? 60 : 0);
        if (targetFPS > 0) {
            m_time.setTargetFPS(targetFPS);
        }

        LOG_INFO("Display system initialized (design={}x{}, scale_mode={}, effective={}x{}, "
                 "ui_scale={:.2f}, min_font={}px, target_fps={})",
                 vpCfg.designWidth, vpCfg.designHeight, scaleMode,
                 m_viewportScaler.getEffectiveWidth(), m_viewportScaler.getEffectiveHeight(),
                 m_uiScaling.getScale(), uiCfg.minFontSize, targetFPS);

        if (onDeck) {
            LOG_INFO("Steam Deck detected — using 1280x800 defaults, borderless fullscreen, 60 FPS cap");
        }
    }

    // Stage 19C: Seamlessness — install signal handlers for graceful exit
    {
        std::signal(SIGTERM, Engine::signalHandler);
        std::signal(SIGINT, Engine::signalHandler);
        LOG_INFO("Signal handlers installed (SIGTERM, SIGINT)");
    }

    // Stage 19D: System Support — optional Steamworks SDK initialization
    {
        uint32_t steamAppId = static_cast<uint32_t>(m_config.getInt("steam.app_id", 0));
        if (m_steamIntegration.init(steamAppId)) {
            LOG_INFO("Steam integration active (appId={})", steamAppId);
        } else {
            LOG_INFO("Steam integration inactive — engine runs without Steam features");
        }
    }

    m_running = true;
    LOG_INFO("Engine initialized successfully — Stage 19E: Configuration and Persistence");
    return true;
}

void Engine::run() {
    LOG_INFO("Entering main loop");

    while (m_running && !m_window.shouldClose()) {
        // Stage 19C: Check for OS termination signals (SIGTERM/SIGINT)
        if (s_signalReceived.load(std::memory_order_relaxed)) {
            LOG_INFO("Termination signal received — initiating graceful shutdown");
            emitShutdownOnce();
            if (m_saveSystem.isDirty()) {
                LOG_INFO("Auto-saving before signal exit...");
                m_saveSystem.saveAll();
            }
            m_running = false;
            break;
        }

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
    m_inputDeviceTracker.update(m_input, m_gamepad);

    // Stage 19D: Process Steam callbacks (overlay, keyboard, etc.)
    m_steamIntegration.update();

    // Update viewport scaler only when window size actually changes
    if (m_window.pollSizeChanged()) {
        m_viewportScaler.update(m_window.getWidth(), m_window.getHeight());
        m_camera.setScreenSize(
            static_cast<float>(m_viewportScaler.getEffectiveWidth()),
            static_cast<float>(m_viewportScaler.getEffectiveHeight()));
    }

    // Suspend/resume detection (Stage 19C: Seamlessness)
    //
    // Two independent signals:
    //  1. OS-level suspend (Steam Deck sleep): the process is frozen by the OS,
    //     so no frames tick.  On wake, a single frame arrives with a very large
    //     raw delta (seconds/minutes).  We detect this via rawDeltaTime() and
    //     immediately clamp the next delta + pause/unpause audio.
    //  2. Desktop extended unfocus (alt-tab, overlay): the process keeps running
    //     but the window loses focus.  We use a timer so brief focus losses
    //     (< 1s) don't interrupt audio.
    //
    // Signal 1: large raw delta indicates OS-level suspend/resume
    if (m_time.rawDeltaTime() > SUSPEND_THRESHOLD) {
        // Process was frozen — clamp the next frame's delta to prevent
        // physics explosions.  (The current frame's delta is already clamped
        // by Time::MAX_DELTA, but chain a tighter clamp for safety.)
        m_time.clampNextDelta(0.1);
    }

    // Signal 2: focus-based suspend for extended unfocus (also triggers on Steam overlay)
    bool effectivelyUnfocused = !m_window.isFocused() || m_steamIntegration.isOverlayActive();
    if (effectivelyUnfocused) {
        m_unfocusedTimer += static_cast<float>(m_time.deltaTime());
        if (!m_wasSuspended && m_unfocusedTimer >= SUSPEND_THRESHOLD) {
            // --- Enter suspended state ---
            if (m_audioSystem) {
                m_audioSystem->setMusicPaused(true);
                m_audioSystem->stopAllSounds();
            }
            m_haptics.stop();

            // Auto-save on suspend so data survives if the process is killed
            if (m_saveSystem.isDirty()) {
                LOG_INFO("Auto-saving on suspend...");
                m_saveSystem.saveAll();
            }

            // Notify mods via EventBus
            EventData suspendData;
            suspendData.setString("reason", "focus_lost");
            getEventBus().emit("engine.suspend", suspendData);

            LOG_INFO("Engine suspended (focus lost)");
            m_wasSuspended = true;
        }
    } else {
        if (m_wasSuspended) {
            // --- Resume from suspended state ---
            if (m_audioSystem) {
                m_audioSystem->setMusicPaused(false);
            }
            m_time.clampNextDelta(0.1);

            // Notify mods via EventBus
            getEventBus().emit("engine.resume");

            LOG_INFO("Engine resumed");
        }
        m_wasSuspended = false;
        m_unfocusedTimer = 0.0f;
    }

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

    // Latch axis state at end of frame so m_prevAxisValues holds this frame's
    // values when next frame's checkBinding compares current vs previous.
    m_inputActions.latchAxisState(m_gamepad);
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

    // Render letterbox/pillarbox bars if using FitLetterbox mode
    m_viewportScaler.renderBars(m_renderer.get());

    // Render on-screen keyboard (above everything else)
    m_onScreenKeyboard.render(m_renderer.get());

    // Render diagnostic overlay (Stage 18)
    if (m_diagnosticOverlay.isVisible()) {
        m_diagnosticOverlay.render(m_renderer.get(), m_profiler, m_resourceManager, *this);
    }

    m_renderer->endFrame();
    // Camera shake offset is automatically undone by ShakeGuard destructor
}

void Engine::shutdown() {
    LOG_INFO("Shutting down...");

    // Stage 19C: Notify mods of impending shutdown (guarded against double emit
    // if the signal-exit path in run() already fired the event)
    emitShutdownOnce();

    // Restore default signal handlers
    std::signal(SIGTERM, SIG_DFL);
    std::signal(SIGINT, SIG_DFL);

    // Shutdown mods first (they may reference engine resources including Steam)
    m_modLoader.shutdown();

    // Shutdown Steam integration after mods have released their references
    m_steamIntegration.shutdown();

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
