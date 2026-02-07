#pragma once

#include "ecs/Systems.hpp"
#include "ecs/Components.hpp"
#include "rendering/Camera.hpp"

namespace gloaming {

/// Camera behavior mode
enum class CameraMode {
    FreeFollow,     // Smooth follow with deadzone (general purpose)
    GridSnap,       // Snap camera to grid cells (Pokemon-style room transitions)
    AutoScroll,     // Automatic scrolling in a direction (Sopwith, shmups)
    RoomBased,      // Camera stays within discrete rooms, transitions on boundary
    Locked          // Camera does not move (fixed screen)
};

/// Axis lock for constraining camera movement
enum class AxisLock : uint8_t {
    None = 0,
    LockX = 1,     // Camera cannot move horizontally (vertical scroller)
    LockY = 2,     // Camera cannot move vertically (horizontal side-scroller)
};

/// Camera controller configuration
struct CameraControllerConfig {
    CameraMode mode = CameraMode::FreeFollow;

    // -- FreeFollow settings --
    float smoothness = 5.0f;            // Lerp speed (higher = snappier)
    Vec2 deadzone{32.0f, 32.0f};        // Pixels of deadzone before camera starts following
    Vec2 lookAhead{0.0f, 0.0f};         // Offset in the direction the entity faces

    // -- AutoScroll settings --
    Vec2 scrollSpeed{100.0f, 0.0f};     // Pixels per second (e.g., {100,0} for Sopwith)
    bool wrapHorizontal = false;         // Wrap camera horizontally (for looping backgrounds)

    // -- GridSnap / RoomBased settings --
    float roomWidth = 320.0f;            // Room size in pixels
    float roomHeight = 240.0f;
    float transitionSpeed = 5.0f;        // Speed of room transitions

    // -- Axis constraints --
    AxisLock axisLock = AxisLock::None;

    // -- Zoom --
    float targetZoom = 1.0f;
    float zoomSpeed = 2.0f;             // Zoom lerp speed

    // -- Bounds --
    bool useBounds = false;
    Rect bounds{0, 0, 0, 0};
};

/// Component that marks an entity as the camera target
struct CameraTarget {
    Vec2 offset{0.0f, 0.0f};           // Additional offset from entity position
    int priority = 0;                   // Higher priority targets override lower
};

/// System that controls the camera based on the configured mode and target entity.
class CameraControllerSystem : public System {
public:
    explicit CameraControllerSystem(const CameraControllerConfig& config = {})
        : System("CameraControllerSystem", -100), m_config(config) {}

    void init(Registry& registry, Engine& engine) override {
        System::init(registry, engine);
        m_camera = &engine.getCamera();
    }

    void update(float dt) override {
        if (!m_camera) return;

        // Handle zoom interpolation
        float currentZoom = m_camera->getZoom();
        if (std::abs(currentZoom - m_config.targetZoom) > 0.001f) {
            float newZoom = currentZoom + (m_config.targetZoom - currentZoom) * m_config.zoomSpeed * dt;
            m_camera->setZoom(newZoom);
        }

        switch (m_config.mode) {
            case CameraMode::FreeFollow:   updateFreeFollow(dt); break;
            case CameraMode::GridSnap:     updateGridSnap(dt); break;
            case CameraMode::AutoScroll:   updateAutoScroll(dt); break;
            case CameraMode::RoomBased:    updateRoomBased(dt); break;
            case CameraMode::Locked:       break;  // No movement
        }

        // Apply bounds
        if (m_config.useBounds) {
            m_camera->setWorldBounds(m_config.bounds);
        }
    }

    CameraControllerConfig& getConfig() { return m_config; }
    const CameraControllerConfig& getConfig() const { return m_config; }

private:
    /// Find the highest-priority camera target entity
    Vec2 getTargetPosition() {
        Vec2 best{0, 0};
        int bestPriority = -9999;
        bool found = false;

        getRegistry().each<Transform, CameraTarget>(
            [&](Entity /*entity*/, const Transform& transform, const CameraTarget& target) {
                if (!found || target.priority > bestPriority) {
                    best = transform.position + target.offset;
                    bestPriority = target.priority;
                    found = true;
                }
            }
        );

        if (!found) return m_camera->getPosition();
        return best;
    }

    void updateFreeFollow(float dt) {
        Vec2 target = getTargetPosition();
        Vec2 current = m_camera->getPosition();
        Vec2 diff = target - current;

        // Apply deadzone
        if (std::abs(diff.x) < m_config.deadzone.x) diff.x = 0.0f;
        else diff.x -= (diff.x > 0 ? m_config.deadzone.x : -m_config.deadzone.x);

        if (std::abs(diff.y) < m_config.deadzone.y) diff.y = 0.0f;
        else diff.y -= (diff.y > 0 ? m_config.deadzone.y : -m_config.deadzone.y);

        // Apply axis lock
        if (static_cast<uint8_t>(m_config.axisLock) & static_cast<uint8_t>(AxisLock::LockX)) diff.x = 0.0f;
        if (static_cast<uint8_t>(m_config.axisLock) & static_cast<uint8_t>(AxisLock::LockY)) diff.y = 0.0f;

        // Smooth follow
        Vec2 newPos = current + diff * (m_config.smoothness * dt);

        // Look-ahead offset
        newPos = newPos + m_config.lookAhead;

        m_camera->setPosition(newPos);
    }

    void updateGridSnap(float dt) {
        Vec2 target = getTargetPosition();

        // Calculate which room/grid cell the target is in
        float roomX = std::floor(target.x / m_config.roomWidth) * m_config.roomWidth
                      + m_config.roomWidth * 0.5f;
        float roomY = std::floor(target.y / m_config.roomHeight) * m_config.roomHeight
                      + m_config.roomHeight * 0.5f;

        Vec2 roomCenter{roomX, roomY};
        Vec2 current = m_camera->getPosition();

        // Smooth transition to room center
        Vec2 diff = roomCenter - current;
        m_camera->setPosition(current + diff * (m_config.transitionSpeed * dt));
    }

    void updateAutoScroll(float dt) {
        Vec2 current = m_camera->getPosition();
        Vec2 newPos = current + m_config.scrollSpeed * dt;

        // Keep the camera target centered vertically (or horizontally)
        // if there's a target entity
        Vec2 target = getTargetPosition();
        if (static_cast<uint8_t>(m_config.axisLock) & static_cast<uint8_t>(AxisLock::LockY)) {
            // Auto-scroll horizontal only, follow target on Y
            newPos.y = target.y;
        } else if (static_cast<uint8_t>(m_config.axisLock) & static_cast<uint8_t>(AxisLock::LockX)) {
            // Auto-scroll vertical only, follow target on X
            newPos.x = target.x;
        }

        m_camera->setPosition(newPos);
    }

    void updateRoomBased(float dt) {
        // Same as GridSnap but with discrete room boundaries
        updateGridSnap(dt);
    }

    Camera* m_camera = nullptr;
    CameraControllerConfig m_config;
};

} // namespace gloaming
