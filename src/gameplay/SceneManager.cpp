#include "gameplay/SceneManager.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "gameplay/CameraController.hpp"

#include <algorithm>

namespace gloaming {

void SceneManager::init(Engine& engine) {
    m_engine = &engine;
    LOG_INFO("SceneManager initialized");
}

void SceneManager::registerScene(const std::string& name, SceneDefinition definition) {
    definition.name = name;
    m_scenes[name] = std::move(definition);
    LOG_INFO("Scene registered: '{}'", name);
}

bool SceneManager::hasScene(const std::string& name) const {
    return m_scenes.find(name) != m_scenes.end();
}

bool SceneManager::goTo(const std::string& name, TransitionType transition, float duration) {
    if (!m_engine) {
        LOG_WARN("SceneManager::goTo: not initialized");
        return false;
    }

    auto it = m_scenes.find(name);
    if (it == m_scenes.end()) {
        LOG_WARN("SceneManager::goTo: scene '{}' not found", name);
        return false;
    }

    if (m_transition.active) {
        LOG_WARN("SceneManager::goTo: transition already in progress");
        return false;
    }

    // If instant or zero duration, switch immediately
    if (transition == TransitionType::Instant || duration <= 0.0f) {
        executeSceneSwitch(name);
        return true;
    }

    // Start transition effect
    m_transition.active = true;
    m_transition.type = transition;
    m_transition.duration = duration;
    m_transition.elapsed = 0.0f;
    m_transition.targetScene = name;
    m_transition.halfwayReached = false;

    LOG_INFO("Scene transition started: '{}' -> '{}' ({}, {:.1f}s)",
             m_currentScene, name, transitionTypeToString(transition), duration);
    return true;
}

bool SceneManager::push(const std::string& name) {
    if (!m_engine) {
        LOG_WARN("SceneManager::push: not initialized");
        return false;
    }

    auto it = m_scenes.find(name);
    if (it == m_scenes.end()) {
        LOG_WARN("SceneManager::push: scene '{}' not found", name);
        return false;
    }

    // Push current scene onto stack
    m_sceneStack.push_back(m_currentScene);

    // Call exit on current scene (if any)
    if (!m_currentScene.empty()) {
        auto currentIt = m_scenes.find(m_currentScene);
        if (currentIt != m_scenes.end() && currentIt->second.onExit) {
            try { currentIt->second.onExit(); }
            catch (const std::exception& ex) {
                LOG_ERROR("Scene '{}' onExit error: {}", m_currentScene, ex.what());
            }
        }
    }

    // Enter the overlay scene (don't destroy entities or unload tiles)
    m_currentScene = name;

    if (it->second.onEnter) {
        try { it->second.onEnter(); }
        catch (const std::exception& ex) {
            LOG_ERROR("Scene '{}' onEnter error: {}", name, ex.what());
        }
    }

    // Apply camera config if specified
    if (it->second.camera.configured) {
        applyCameraConfig(it->second.camera);
    }

    LOG_INFO("Scene pushed: '{}' (stack depth: {})", name, m_sceneStack.size());
    return true;
}

bool SceneManager::pop() {
    if (m_sceneStack.empty()) {
        LOG_WARN("SceneManager::pop: stack is empty");
        return false;
    }

    // Call exit on current overlay scene
    if (!m_currentScene.empty()) {
        auto it = m_scenes.find(m_currentScene);
        if (it != m_scenes.end() && it->second.onExit) {
            try { it->second.onExit(); }
            catch (const std::exception& ex) {
                LOG_ERROR("Scene '{}' onExit error: {}", m_currentScene, ex.what());
            }
        }

        // Destroy scene-local entities for the overlay
        auto& registry = m_engine->getRegistry();
        std::string exitingScene = m_currentScene;
        registry.destroyIf([&registry, &exitingScene](Entity entity) {
            auto* sceneLocal = registry.tryGet<SceneLocalEntity>(entity);
            return sceneLocal && sceneLocal->sceneName == exitingScene;
        });
    }

    // Restore the previous scene
    m_currentScene = m_sceneStack.back();
    m_sceneStack.pop_back();

    // Call enter on restored scene
    if (!m_currentScene.empty()) {
        auto it = m_scenes.find(m_currentScene);
        if (it != m_scenes.end() && it->second.onEnter) {
            try { it->second.onEnter(); }
            catch (const std::exception& ex) {
                LOG_ERROR("Scene '{}' onEnter error: {}", m_currentScene, ex.what());
            }
        }

        if (it != m_scenes.end() && it->second.camera.configured) {
            applyCameraConfig(it->second.camera);
        }
    }

    LOG_INFO("Scene popped, now: '{}' (stack depth: {})", m_currentScene, m_sceneStack.size());
    return true;
}

void SceneManager::setPersistent(Entity entity) {
    if (!m_engine) return;
    auto& registry = m_engine->getRegistry();
    if (registry.valid(entity) && !registry.has<PersistentEntity>(entity)) {
        registry.raw().emplace<PersistentEntity>(entity);
    }
}

bool SceneManager::isPersistent(Entity entity) const {
    if (!m_engine) return false;
    auto& registry = m_engine->getRegistry();
    return registry.valid(entity) && registry.has<PersistentEntity>(entity);
}

void SceneManager::clearPersistent(Entity entity) {
    if (!m_engine) return;
    auto& registry = m_engine->getRegistry();
    if (registry.valid(entity)) {
        registry.remove<PersistentEntity>(entity);
    }
}

void SceneManager::update(float dt) {
    if (!m_transition.active) return;

    m_transition.elapsed += dt;

    // For fade transitions, execute the switch at the halfway point
    if (m_transition.type == TransitionType::Fade && !m_transition.halfwayReached) {
        float halfDuration = m_transition.duration * 0.5f;
        if (m_transition.elapsed >= halfDuration) {
            m_transition.halfwayReached = true;
            executeSceneSwitch(m_transition.targetScene);
        }
    }

    // For slide transitions, execute the switch at the halfway point
    if ((m_transition.type == TransitionType::SlideLeft ||
         m_transition.type == TransitionType::SlideRight ||
         m_transition.type == TransitionType::SlideUp ||
         m_transition.type == TransitionType::SlideDown) &&
        !m_transition.halfwayReached) {
        float halfDuration = m_transition.duration * 0.5f;
        if (m_transition.elapsed >= halfDuration) {
            m_transition.halfwayReached = true;
            executeSceneSwitch(m_transition.targetScene);
        }
    }

    // Complete transition
    if (m_transition.isComplete()) {
        // If instant (shouldn't reach here) or non-fade, execute switch if not done
        if (!m_transition.halfwayReached) {
            executeSceneSwitch(m_transition.targetScene);
        }
        m_transition.active = false;
        LOG_INFO("Scene transition complete, now in '{}'", m_currentScene);
    }
}

void SceneManager::renderTransition(IRenderer* renderer) {
    if (!m_transition.active || !renderer) return;

    float progress = m_transition.getProgress();
    int screenW = renderer->getScreenWidth();
    int screenH = renderer->getScreenHeight();

    switch (m_transition.type) {
        case TransitionType::Fade: {
            // Fade out for first half, fade in for second half
            float alpha;
            if (progress < 0.5f) {
                alpha = progress * 2.0f; // 0 -> 1
            } else {
                alpha = (1.0f - progress) * 2.0f; // 1 -> 0
            }
            uint8_t a = static_cast<uint8_t>(std::min(255.0f, alpha * 255.0f));
            renderer->drawRectangle(
                Rect(0.0f, 0.0f, static_cast<float>(screenW), static_cast<float>(screenH)),
                Color(0, 0, 0, a));
            break;
        }

        case TransitionType::SlideLeft:
        case TransitionType::SlideRight:
        case TransitionType::SlideUp:
        case TransitionType::SlideDown: {
            // Sliding wipe effect — a black bar sweeps across/down/up
            float alpha;
            if (progress < 0.5f) {
                alpha = progress * 2.0f;
            } else {
                alpha = (1.0f - progress) * 2.0f;
            }
            uint8_t a = static_cast<uint8_t>(std::min(255.0f, alpha * 255.0f));
            renderer->drawRectangle(
                Rect(0.0f, 0.0f, static_cast<float>(screenW), static_cast<float>(screenH)),
                Color(0, 0, 0, a));
            break;
        }

        case TransitionType::Instant:
            break;
    }
}

void SceneManager::executeSceneSwitch(const std::string& newScene) {
    if (!m_engine) return;

    // 1. Call exit on current scene
    if (!m_currentScene.empty()) {
        auto it = m_scenes.find(m_currentScene);
        if (it != m_scenes.end() && it->second.onExit) {
            try { it->second.onExit(); }
            catch (const std::exception& ex) {
                LOG_ERROR("Scene '{}' onExit error: {}", m_currentScene, ex.what());
            }
        }
    }

    // 2. Destroy scene-local entities (non-persistent)
    destroySceneLocalEntities();

    // 3. Update current scene name
    std::string previousScene = m_currentScene;
    m_currentScene = newScene;

    // 4. Load new scene's tile data (if applicable)
    auto it = m_scenes.find(newScene);
    if (it != m_scenes.end()) {
        // Apply camera config
        if (it->second.camera.configured) {
            applyCameraConfig(it->second.camera);
        }

        // 5. Call enter on new scene
        if (it->second.onEnter) {
            try { it->second.onEnter(); }
            catch (const std::exception& ex) {
                LOG_ERROR("Scene '{}' onEnter error: {}", newScene, ex.what());
            }
        }
    }

    LOG_INFO("Scene switched: '{}' -> '{}'", previousScene, newScene);
}

void SceneManager::destroySceneLocalEntities() {
    if (!m_engine) return;

    auto& registry = m_engine->getRegistry();
    // Destroy all entities that are NOT persistent and have SceneLocalEntity tag
    // Also destroy entities that have no persistence and no scene-local tag
    // but belong to the current scene context
    registry.destroyIf([&registry](Entity entity) {
        // Keep persistent entities
        if (registry.has<PersistentEntity>(entity)) return false;
        // Destroy scene-local entities
        if (registry.has<SceneLocalEntity>(entity)) return true;
        return false;
    });
}

void SceneManager::applyCameraConfig(const SceneCameraConfig& config) {
    if (!m_engine) return;

    auto& camera = m_engine->getCamera();

    // Set camera position for locked/positioned modes
    if (config.mode == "locked") {
        camera.setPosition(config.x, config.y);
    }

    // Set camera zoom
    if (config.zoom != 1.0f) {
        camera.setZoom(config.zoom);
    }

    // Set camera mode through CameraControllerSystem
    auto* ctrl = m_engine->getSystemScheduler().getSystem<CameraControllerSystem>();
    if (ctrl) {
        auto& cameraConfig = ctrl->getConfig();
        if (config.mode == "free_follow")      cameraConfig.mode = CameraMode::FreeFollow;
        else if (config.mode == "grid_snap")   cameraConfig.mode = CameraMode::GridSnap;
        else if (config.mode == "auto_scroll") cameraConfig.mode = CameraMode::AutoScroll;
        else if (config.mode == "room_based")  cameraConfig.mode = CameraMode::RoomBased;
        else if (config.mode == "locked")      cameraConfig.mode = CameraMode::Locked;
    }
}

} // namespace gloaming
