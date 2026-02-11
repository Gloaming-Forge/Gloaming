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
#include "gameplay/GameMode.hpp"
#include "gameplay/InputActions.hpp"
#include "gameplay/Pathfinding.hpp"
#include "gameplay/DialogueSystem.hpp"
#include "gameplay/TileLayers.hpp"
#include "gameplay/CollisionLayers.hpp"
#include "gameplay/EntitySpawning.hpp"
#include "gameplay/ProjectileSystem.hpp"
#include "world/WorldGenerator.hpp"
#include "gameplay/CraftingSystem.hpp"
#include "gameplay/EnemySpawnSystem.hpp"
#include "gameplay/EnemyAISystem.hpp"
#include "gameplay/LootDropSystem.hpp"
#include "gameplay/NPCSystem.hpp"
#include "gameplay/HousingSystem.hpp"
#include "gameplay/ShopSystem.hpp"
#include "gameplay/SceneManager.hpp"
#include "gameplay/TimerSystem.hpp"
#include "gameplay/SaveSystem.hpp"

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
    GameModeConfig& getGameModeConfig() { return m_gameModeConfig; }
    InputActionMap& getInputActions() { return m_inputActions; }
    Pathfinder& getPathfinder() { return m_pathfinder; }
    DialogueSystem& getDialogueSystem() { return m_dialogueSystem; }
    TileLayerManager& getTileLayerManager() { return m_tileLayers; }
    CollisionLayerRegistry& getCollisionLayers() { return m_collisionLayers; }
    EntitySpawning& getEntitySpawning() { return m_entitySpawning; }

    // World generation accessors (Stage 12)
    WorldGenerator& getWorldGenerator() { return m_worldGenerator; }

    // Gameplay loop accessors (Stage 13)
    CraftingManager& getCraftingManager() { return m_craftingManager; }

    // Enemy & AI accessors (Stage 14)
    EnemySpawnSystem* getEnemySpawnSystem() { return m_enemySpawnSystem; }
    EnemyAISystem* getEnemyAISystem() { return m_enemyAISystem; }

    // NPC, Housing & Shop accessors (Stage 15)
    NPCSystem* getNPCSystem() { return m_npcSystem; }
    HousingSystem* getHousingSystem() { return m_housingSystem; }
    ShopManager& getShopManager() { return m_shopManager; }

    // Scene, Timer & Save accessors (Stage 16)
    SceneManager& getSceneManager() { return m_sceneManager; }
    TimerSystem& getTimerSystem() { return m_timerSystem; }
    SaveSystem& getSaveSystem() { return m_saveSystem; }

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
    GameModeConfig m_gameModeConfig;
    InputActionMap m_inputActions;
    Pathfinder m_pathfinder;
    DialogueSystem m_dialogueSystem;
    TileLayerManager m_tileLayers;
    CollisionLayerRegistry m_collisionLayers;
    EntitySpawning m_entitySpawning;

    // World generation (Stage 12)
    WorldGenerator m_worldGenerator;

    // Gameplay loop (Stage 13)
    CraftingManager m_craftingManager;

    // Enemy & AI (Stage 14) — managed by SystemScheduler, raw pointers for access
    EnemySpawnSystem* m_enemySpawnSystem = nullptr;
    EnemyAISystem* m_enemyAISystem = nullptr;

    // NPC, Housing & Shops (Stage 15) — Systems managed by SystemScheduler, raw pointers for access
    NPCSystem* m_npcSystem = nullptr;
    HousingSystem* m_housingSystem = nullptr;
    ShopManager m_shopManager;

    // Scene, Timer & Save (Stage 16)
    SceneManager m_sceneManager;
    TimerSystem m_timerSystem;
    SaveSystem m_saveSystem;

    bool m_running = false;
};

} // namespace gloaming
