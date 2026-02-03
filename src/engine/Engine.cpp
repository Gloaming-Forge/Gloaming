#include "engine/Engine.hpp"
#include "engine/Log.hpp"

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

void Engine::update([[maybe_unused]] double dt) {
    // Stage 0: nothing to update yet.
    // Future stages add ECS systems, physics, etc.
}

void Engine::render() {
    m_window.beginFrame();

    // Stage 0: draw basic info text
    DrawText("Gloaming Engine v0.1.0", 20, 20, 20, RAYWHITE);

    char fpsText[64];
    snprintf(fpsText, sizeof(fpsText), "FPS: %d", GetFPS());
    DrawText(fpsText, 20, 50, 16, GREEN);

    DrawText("Press F11 to toggle fullscreen", 20, 80, 16, GRAY);
    DrawText("Press ESC to exit", 20, 104, 16, GRAY);

    m_window.endFrame();
}

void Engine::shutdown() {
    LOG_INFO("Shutting down...");
    m_window.shutdown();
    Log::shutdown();
}

} // namespace gloaming
