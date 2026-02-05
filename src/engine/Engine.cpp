#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "rendering/RaylibRenderer.hpp"

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
}

void Engine::update(double dt) {
    // Update parallax background auto-scrolling
    m_parallaxBg.update(static_cast<float>(dt));

    // Handle camera controls for testing (Stage 1 demo)
    float cameraSpeed = 300.0f * static_cast<float>(dt);
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
    float zoomSpeed = 1.0f * static_cast<float>(dt);
    if (m_input.isKeyDown(KEY_Q)) {
        m_camera.zoom(-zoomSpeed);
    }
    if (m_input.isKeyDown(KEY_E)) {
        m_camera.zoom(zoomSpeed);
    }
}

void Engine::render() {
    m_renderer->beginFrame();
    m_renderer->clear(Color(20, 20, 30, 255));

    // Render parallax background layers
    m_parallaxBg.render();

    // Stage 1: draw basic info text using renderer
    m_renderer->drawText("Gloaming Engine v0.1.0", {20, 20}, 20, Color::White());

    char fpsText[64];
    snprintf(fpsText, sizeof(fpsText), "FPS: %d", GetFPS());
    m_renderer->drawText(fpsText, {20, 50}, 16, Color::Green());

    // Camera info
    Vec2 camPos = m_camera.getPosition();
    char camText[128];
    snprintf(camText, sizeof(camText), "Camera: (%.1f, %.1f) Zoom: %.2f",
             camPos.x, camPos.y, m_camera.getZoom());
    m_renderer->drawText(camText, {20, 80}, 16, Color(100, 200, 255, 255));

    m_renderer->drawText("WASD/Arrows: Move camera | Q/E: Zoom | F11: Fullscreen", {20, 110}, 16, Color::Gray());

    m_renderer->endFrame();
}

void Engine::shutdown() {
    LOG_INFO("Shutting down...");

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
