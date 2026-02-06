#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "rendering/RaylibRenderer.hpp"
#include "ecs/CoreSystems.hpp"

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

    LOG_INFO("Gloaming Engine v0.1.0 starting...");

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

    // For demo: create a test world
    std::string worldPath = "worlds/test_world";
    if (!m_tileMap.loadWorld(worldPath)) {
        // World doesn't exist, create it
        uint64_t seed = 42;  // Demo seed
        if (m_tileMap.createWorld(worldPath, "Test World", seed)) {
            LOG_INFO("Created new test world with seed {}", seed);
            // Set spawn point at surface level
            m_tileMap.setSpawnPoint(0.0f, 80.0f * m_tileRenderer.getTileSize());
        } else {
            LOG_WARN("Failed to create test world (directory may be read-only)");
        }
    } else {
        LOG_INFO("Loaded existing test world");
    }

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

    // Initialize mod system
    ModLoaderConfig modConfig;
    modConfig.modsDirectory = m_config.getString("mods.directory", "mods");
    modConfig.configFile = m_config.getString("mods.config", "config/mods.json");

    if (m_modLoader.init(*this, modConfig)) {
        int discovered = m_modLoader.discoverMods();
        if (discovered > 0) {
            if (m_modLoader.resolveDependencies()) {
                int loaded = m_modLoader.loadMods();
                m_modLoader.postInitMods();
                LOG_INFO("Mod system: {}/{} mods loaded successfully", loaded, discovered);
            }
        } else {
            LOG_INFO("No mods found in '{}'", modConfig.modsDirectory);
        }
    } else {
        LOG_WARN("Mod system failed to initialize (non-fatal, continuing without mods)");
    }

    m_running = true;
    LOG_INFO("Engine initialized successfully");
    return true;
}

void Engine::run() {
    LOG_INFO("Entering main loop");

    while (m_running && !m_window.shouldClose()) {
        double dt = static_cast<double>(GetFrameTime());
        m_time.update(dt);

        processInput();
        update(m_time.deltaTime());
        render();
    }

    LOG_INFO("Main loop exited");
}

void Engine::processInput() {
    m_input.update();

    if (m_input.isKeyPressed(KEY_F11)) {
        m_window.toggleFullscreen();
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

    // Update parallax background auto-scrolling
    m_parallaxBg.update(dtFloat);

    // Handle camera controls for testing (Stage 1 demo)
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

    // Update world chunk loading based on camera position
    if (m_tileMap.isWorldLoaded()) {
        m_tileMap.update(m_camera);
    }
}

void Engine::render() {
    m_renderer->beginFrame();
    m_renderer->clear(Color(20, 20, 30, 255));

    // Render parallax background layers
    m_parallaxBg.render();

    // Render world tiles
    if (m_tileMap.isWorldLoaded()) {
        m_tileMap.render(m_tileRenderer, m_camera);
    }

    // Run ECS render systems (dt=0 for render phase, timing not needed)
    m_systemScheduler.render(0.0f);

    // Render lighting overlay (after tiles and sprites, before UI)
    if (m_lightingSystem) {
        m_lightingSystem->renderLightOverlay(m_renderer.get(), m_camera);
    }

    // Draw basic info text using renderer
    m_renderer->drawText("Gloaming Engine v0.1.0 - Stage 6: Lighting System", {20, 20}, 20, Color::White());

    char fpsText[64];
    snprintf(fpsText, sizeof(fpsText), "FPS: %d", GetFPS());
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
    char modText[128];
    snprintf(modText, sizeof(modText), "Mods: %zu loaded | Content: %zu tiles, %zu items, %zu enemies",
             m_modLoader.loadedCount(),
             m_modLoader.getContentRegistry().tileCount(),
             m_modLoader.getContentRegistry().itemCount(),
             m_modLoader.getContentRegistry().enemyCount());
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

    m_renderer->drawText("WASD/Arrows: Move camera | Q/E: Zoom | L: Toggle light | F11: Fullscreen",
                         {20, 200}, 16, Color::Gray());

    m_renderer->endFrame();
}

void Engine::shutdown() {
    LOG_INFO("Shutting down...");

    // Shutdown mods first (they may reference engine resources)
    m_modLoader.shutdown();

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
