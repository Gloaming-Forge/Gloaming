#pragma once

#include "engine/Config.hpp"
#include "engine/Window.hpp"
#include "engine/Input.hpp"
#include "engine/Time.hpp"
#include "rendering/IRenderer.hpp"
#include "rendering/Camera.hpp"
#include "rendering/Texture.hpp"
#include "rendering/SpriteBatch.hpp"
#include "rendering/TileRenderer.hpp"
#include "rendering/ParallaxBackground.hpp"

#include <string>
#include <memory>

namespace gloaming {

class Engine {
public:
    bool init(const std::string& configPath = "config.json");
    void run();
    void shutdown();

    Config& getConfig() { return m_config; }
    Window& getWindow() { return m_window; }
    Input&  getInput()  { return m_input; }
    Time&   getTime()   { return m_time; }

    // Rendering accessors
    IRenderer* getRenderer() { return m_renderer.get(); }
    Camera& getCamera() { return m_camera; }
    TextureManager& getTextureManager() { return m_textureManager; }
    SpriteBatch& getSpriteBatch() { return m_spriteBatch; }
    TileRenderer& getTileRenderer() { return m_tileRenderer; }
    ParallaxBackground& getParallaxBackground() { return m_parallaxBg; }

private:
    void processInput();
    void update(double dt);
    void render();

    Config m_config;
    Window m_window;
    Input  m_input;
    Time   m_time;

    // Rendering systems
    std::unique_ptr<IRenderer> m_renderer;
    Camera m_camera;
    TextureManager m_textureManager;
    SpriteBatch m_spriteBatch;
    TileRenderer m_tileRenderer;
    ParallaxBackground m_parallaxBg;

    bool m_running = false;
};

} // namespace gloaming
