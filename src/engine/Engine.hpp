#pragma once

#include "engine/Config.hpp"
#include "engine/Window.hpp"
#include "engine/Input.hpp"
#include "engine/Time.hpp"
#include "engine/Profiler.hpp"
#include "engine/ResourceManager.hpp"
#include "engine/DiagnosticOverlay.hpp"
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
#include "gameplay/ParticleSystem.hpp"
#include "gameplay/TweenSystem.hpp"
#include "gameplay/DebugDrawSystem.hpp"
#include "engine/Gamepad.hpp"
#include "engine/InputDeviceTracker.hpp"
#include "engine/InputGlyphs.hpp"
#include "engine/Haptics.hpp"
#include "ui/OnScreenKeyboard.hpp"
#include "rendering/ViewportScaler.hpp"
#include "ui/UIScaling.hpp"
#include "engine/SteamIntegration.hpp"

#include <string>
#include <memory>
#include <atomic>

namespace gloaming {

/// Engine version string — single source of truth for all version displays.
inline constexpr const char* kEngineVersion = "0.5.0";

class Engine {
public:
    bool init(const std::string& configPath = "config.json");
    void run();
    void shutdown();

    /// Request a graceful shutdown (used by SIGTERM handler and Lua API).
    /// Sets m_running to false so the main loop exits cleanly.
    void requestShutdown();

    /// Check whether the engine is currently in a suspended state
    /// (extended focus loss or OS-level suspend detected).
    bool isSuspended() const { return m_wasSuspended; }

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

    // Particles, Tween & Debug accessors (Stage 17)
    ParticleSystem* getParticleSystem() { return m_particleSystem; }
    TweenSystem& getTweenSystem() { return m_tweenSystem; }
    DebugDrawSystem& getDebugDrawSystem() { return m_debugDrawSystem; }

    // Profiler, Resource Manager & Diagnostics accessors (Stage 18)
    Profiler& getProfiler() { return m_profiler; }
    const Profiler& getProfiler() const { return m_profiler; }
    ResourceManager& getResourceManager() { return m_resourceManager; }
    const ResourceManager& getResourceManager() const { return m_resourceManager; }
    DiagnosticOverlay& getDiagnosticOverlay() { return m_diagnosticOverlay; }
    const DiagnosticOverlay& getDiagnosticOverlay() const { return m_diagnosticOverlay; }

    // Gamepad, Input Device Tracker, Glyphs, Haptics & On-Screen Keyboard (Stage 19A)
    Gamepad& getGamepad() { return m_gamepad; }
    const Gamepad& getGamepad() const { return m_gamepad; }
    InputDeviceTracker& getInputDeviceTracker() { return m_inputDeviceTracker; }
    const InputDeviceTracker& getInputDeviceTracker() const { return m_inputDeviceTracker; }
    InputGlyphProvider& getInputGlyphProvider() { return m_inputGlyphProvider; }
    const InputGlyphProvider& getInputGlyphProvider() const { return m_inputGlyphProvider; }
    Haptics& getHaptics() { return m_haptics; }
    const Haptics& getHaptics() const { return m_haptics; }
    OnScreenKeyboard& getOnScreenKeyboard() { return m_onScreenKeyboard; }

    // Display System (Stage 19B)
    ViewportScaler& getViewportScaler() { return m_viewportScaler; }
    const ViewportScaler& getViewportScaler() const { return m_viewportScaler; }
    UIScaling& getUIScaling() { return m_uiScaling; }
    const UIScaling& getUIScaling() const { return m_uiScaling; }

    // Steam Integration (Stage 19D)
    SteamIntegration& getSteamIntegration() { return m_steamIntegration; }
    const SteamIntegration& getSteamIntegration() const { return m_steamIntegration; }

    // Configuration Persistence (Stage 19E)
    const std::string& getLocalConfigPath() const { return m_localConfigPath; }

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

    // Particles, Tween & Debug (Stage 17) — ParticleSystem managed by SystemScheduler
    ParticleSystem* m_particleSystem = nullptr;
    TweenSystem m_tweenSystem;
    DebugDrawSystem m_debugDrawSystem;

    // Profiler, Resource Manager & Diagnostics (Stage 18)
    Profiler m_profiler;
    ResourceManager m_resourceManager;
    DiagnosticOverlay m_diagnosticOverlay;

    // Gamepad, Input Device Tracker, Glyphs, Haptics & On-Screen Keyboard (Stage 19A)
    Gamepad m_gamepad;
    InputDeviceTracker m_inputDeviceTracker;
    InputGlyphProvider m_inputGlyphProvider;
    Haptics m_haptics;
    OnScreenKeyboard m_onScreenKeyboard;

    // Display System (Stage 19B)
    ViewportScaler m_viewportScaler;
    UIScaling m_uiScaling;

    // Steam Integration (Stage 19D)
    SteamIntegration m_steamIntegration;

    // Configuration Persistence (Stage 19E)
    std::string m_localConfigPath;  ///< Derived path for per-device config overrides

    bool m_wasSuspended = false;
    float m_unfocusedTimer = 0.0f;
    static constexpr float SUSPEND_THRESHOLD = 1.0f;  // Seconds unfocused before treating as suspend

    bool m_running = false;

    // Stage 19C: Seamlessness — signal handling
    static std::atomic<bool> s_signalReceived;
    static void signalHandler(int signum);
    bool m_shutdownEmitted = false;  // Guards against duplicate engine.shutdown events
    void emitShutdownOnce();         // Emit engine.shutdown at most once per lifecycle
};

} // namespace gloaming
