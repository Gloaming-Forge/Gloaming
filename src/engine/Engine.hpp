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
#include "ecs/Registry.hpp"
#include "ecs/Systems.hpp"
#include "ecs/EntityFactory.hpp"
#include "world/TileMap.hpp"
#include "lighting/LightingSystem.hpp"
#include "audio/AudioSystem.hpp"
#include "ui/UISystem.hpp"
#include "mod/ModLoader.hpp"
#include "gameplay/InputActions.hpp"
#include "gameplay/Pathfinding.hpp"
#include "gameplay/DialogueSystem.hpp"
#include "gameplay/TileLayers.hpp"

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

    // ECS accessors
    Registry& getRegistry() { return m_registry; }
    SystemScheduler& getSystemScheduler() { return m_systemScheduler; }
    EntityFactory& getEntityFactory() { return m_entityFactory; }

    // World accessors
    TileMap& getTileMap() { return m_tileMap; }

    // Lighting accessors
    LightingSystem* getLightingSystem() { return m_lightingSystem; }

    // Audio accessors
    AudioSystem* getAudioSystem() { return m_audioSystem; }

    // UI accessors
    UISystem* getUISystem() { return &m_uiSystem; }

    // Mod system accessors
    ModLoader& getModLoader() { return m_modLoader; }
    ContentRegistry& getContentRegistry() { return m_modLoader.getContentRegistry(); }
    EventBus& getEventBus() { return m_modLoader.getEventBus(); }

    // Gameplay system accessors
    InputActionMap& getInputActions() { return m_inputActions; }
    Pathfinder& getPathfinder() { return m_pathfinder; }
    DialogueSystem& getDialogueSystem() { return m_dialogueSystem; }
    TileLayerManager& getTileLayerManager() { return m_tileLayers; }

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

    // ECS systems
    Registry m_registry;
    SystemScheduler m_systemScheduler;
    EntityFactory m_entityFactory;

    // World systems
    TileMap m_tileMap;

    // Lighting system (managed by SystemScheduler, raw pointer for access)
    LightingSystem* m_lightingSystem = nullptr;

    // Audio system (managed by SystemScheduler, raw pointer for access)
    AudioSystem* m_audioSystem = nullptr;

    // UI system
    UISystem m_uiSystem;

    // Mod system
    ModLoader m_modLoader;

    // Gameplay systems (Stage 9+)
    InputActionMap m_inputActions;
    Pathfinder m_pathfinder;
    DialogueSystem m_dialogueSystem;
    TileLayerManager m_tileLayers;

    bool m_running = false;
};

} // namespace gloaming
