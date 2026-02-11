#pragma once

#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "rendering/IRenderer.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <cstdint>

namespace gloaming {

// Forward declarations
class Engine;
class TileMap;

// ============================================================================
// Scene Transition Types
// ============================================================================

/// Transition effect type for scene changes
enum class TransitionType {
    Instant,    // No transition effect
    Fade,       // Fade to black and back
    SlideLeft,  // Slide current scene left, new scene in from right
    SlideRight, // Slide current scene right, new scene in from left
    SlideUp,    // Slide current scene up, new scene in from bottom
    SlideDown   // Slide current scene down, new scene in from top
};

/// Convert string to TransitionType
inline TransitionType parseTransitionType(const std::string& str) {
    if (str == "fade")        return TransitionType::Fade;
    if (str == "slide_left")  return TransitionType::SlideLeft;
    if (str == "slide_right") return TransitionType::SlideRight;
    if (str == "slide_up")    return TransitionType::SlideUp;
    if (str == "slide_down")  return TransitionType::SlideDown;
    return TransitionType::Instant;
}

/// Convert TransitionType to string
inline const char* transitionTypeToString(TransitionType type) {
    switch (type) {
        case TransitionType::Instant:    return "instant";
        case TransitionType::Fade:       return "fade";
        case TransitionType::SlideLeft:  return "slide_left";
        case TransitionType::SlideRight: return "slide_right";
        case TransitionType::SlideUp:    return "slide_up";
        case TransitionType::SlideDown:  return "slide_down";
    }
    return "instant";
}

// ============================================================================
// Scene Camera Config
// ============================================================================

/// Camera configuration for a scene
struct SceneCameraConfig {
    std::string mode = "free_follow"; // Camera mode to set on enter
    float x = 0.0f;                   // Camera position X (for locked mode)
    float y = 0.0f;                   // Camera position Y (for locked mode)
    float zoom = 1.0f;                // Camera zoom
    bool configured = false;          // Whether camera config was explicitly set
};

// ============================================================================
// Scene Definition
// ============================================================================

/// Defines a scene's configuration and callbacks
struct SceneDefinition {
    std::string name;                  // Scene name/identifier
    std::string tilesPath;             // Path to binary tile data (empty = no tile data)
    int width = 0;                     // Scene width in tiles
    int height = 0;                    // Scene height in tiles
    SceneCameraConfig camera;          // Camera configuration
    std::function<void()> onEnter;     // Called when scene is entered
    std::function<void()> onExit;      // Called when scene is exited
    bool isOverlay = false;            // Overlay scenes don't unload tile data
};

// ============================================================================
// Transition State
// ============================================================================

/// Internal state for active transition effects
struct TransitionState {
    bool active = false;
    TransitionType type = TransitionType::Instant;
    float duration = 0.0f;
    float elapsed = 0.0f;
    std::string targetScene;           // Scene we're transitioning TO
    bool halfwayReached = false;       // For fade: have we hit the midpoint?

    float getProgress() const {
        return duration > 0.0f ? std::min(elapsed / duration, 1.0f) : 1.0f;
    }

    bool isComplete() const {
        return elapsed >= duration;
    }
};

// PersistentEntity and SceneLocalEntity tag components are defined in
// ecs/Components.hpp alongside other entity tags (PlayerTag, EnemyTag, etc.)

// ============================================================================
// Scene Manager
// ============================================================================

/// Manages named scenes, transitions, and a scene stack for overlays.
///
/// Scenes and the infinite world are mutually exclusive approaches:
/// - Infinite world (default): single TileMap with chunk streaming
/// - Scene-based: each scene has its own fixed-size tile grid
///
/// When go_to() is called:
/// 1. Current scene's on_exit callback fires
/// 2. Scene-local entities are destroyed
/// 3. Transition effect plays
/// 4. New scene's tile data is loaded into the TileMap
/// 5. on_enter fires — mod spawns entities for the new scene
/// 6. Camera is repositioned per scene's camera config
class SceneManager {
public:
    SceneManager() = default;

    /// Initialize with engine reference
    void init(Engine& engine);

    /// Register a named scene
    void registerScene(const std::string& name, SceneDefinition definition);

    /// Check if a scene is registered
    bool hasScene(const std::string& name) const;

    /// Get the current active scene name (empty if none)
    const std::string& currentScene() const { return m_currentScene; }

    /// Transition to a new scene (replaces current)
    /// @param name Scene name to transition to
    /// @param transition Transition type
    /// @param duration Transition duration in seconds
    /// @return true if transition was started
    bool goTo(const std::string& name,
              TransitionType transition = TransitionType::Instant,
              float duration = 0.0f);

    /// Push a scene onto the overlay stack (does NOT unload tile data)
    /// The underlying scene remains active but paused.
    /// @param name Scene name to push
    /// @return true if scene was pushed
    bool push(const std::string& name);

    /// Pop the top scene from the overlay stack
    /// @return true if a scene was popped
    bool pop();

    /// Get the overlay stack depth
    size_t stackDepth() const { return m_sceneStack.size(); }

    /// Mark an entity as persistent (survives scene transitions)
    void setPersistent(Entity entity);

    /// Check if an entity is persistent
    bool isPersistent(Entity entity) const;

    /// Remove persistent status from an entity
    void clearPersistent(Entity entity);

    /// Update the scene manager (processes transitions)
    void update(float dt);

    /// Render transition effects (call after all other rendering)
    void renderTransition(IRenderer* renderer);

    /// Check if a transition is currently active
    bool isTransitioning() const { return m_transition.active; }

    /// Check if the game is paused due to an overlay scene
    bool isPausedByOverlay() const { return !m_sceneStack.empty(); }

    /// Get the transition state (for debug display)
    const TransitionState& getTransitionState() const { return m_transition; }

    /// Get registered scene count
    size_t sceneCount() const { return m_scenes.size(); }

private:
    /// Execute the actual scene switch (called during transition or immediately)
    void executeSceneSwitch(const std::string& newScene);

    /// Destroy all scene-local entities for the current scene
    void destroySceneLocalEntities();

    /// Apply camera config from a scene definition
    void applyCameraConfig(const SceneCameraConfig& config);

    Engine* m_engine = nullptr;
    std::unordered_map<std::string, SceneDefinition> m_scenes;
    std::string m_currentScene;
    std::vector<std::string> m_sceneStack; // Overlay stack
    TransitionState m_transition;
};

} // namespace gloaming
