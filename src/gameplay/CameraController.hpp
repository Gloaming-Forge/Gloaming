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

/// Axis lock for constraining camera movement (bitmask)
enum class AxisLock : uint8_t {
    None = 0,
    LockX = 1,     // Camera cannot move horizontally (vertical scroller)
    LockY = 2,     // Camera cannot move vertically (horizontal side-scroller)
};

/// Bitwise operators for AxisLock so bitmask operations are clean
inline AxisLock operator|(AxisLock a, AxisLock b) {
    return static_cast<AxisLock>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline bool operator&(AxisLock a, AxisLock b) {
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

/// Camera controller configuration
struct CameraControllerConfig {
    CameraMode mode = CameraMode::FreeFollow;

    // -- FreeFollow settings --
    float smoothness = 5.0f;            // Lerp speed (higher = snappier)
    Vec2 deadzone{32.0f, 32.0f};        // Pixels of deadzone before camera starts following
    Vec2 lookAhead{0.0f, 0.0f};         // Look-ahead offset (scaled by target velocity direction)

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
    /// Find the highest-priority camera target entity and its velocity
    struct TargetInfo {
        Vec2 position{0, 0};
        Vec2 velocity{0, 0};
        bool found = false;
    };

    TargetInfo getTargetInfo() {
        TargetInfo info;
        int bestPriority = -9999;

        getRegistry().each<Transform, CameraTarget>(
            [&](Entity entity, const Transform& transform, const CameraTarget& target) {
                if (!info.found || target.priority > bestPriority) {
                    info.position = transform.position + target.offset;
                    // Read velocity for look-ahead if available
                    if (getRegistry().has<Velocity>(entity)) {
                        auto& vel = getRegistry().get<Velocity>(entity);
                        info.velocity = vel.linear;
                    } else {
                        info.velocity = {0.0f, 0.0f};
                    }
                    bestPriority = target.priority;
                    info.found = true;
                }
            }
        );

        if (!info.found) info.position = m_camera->getPosition();
        return info;
    }

    Vec2 getTargetPosition() {
        return getTargetInfo().position;
    }

    void updateFreeFollow(float dt) {
        auto targetInfo = getTargetInfo();
        Vec2 current = m_camera->getPosition();
        Vec2 diff = targetInfo.position - current;

        // Apply deadzone
        if (std::abs(diff.x) < m_config.deadzone.x) diff.x = 0.0f;
        else diff.x -= (diff.x > 0 ? m_config.deadzone.x : -m_config.deadzone.x);

        if (std::abs(diff.y) < m_config.deadzone.y) diff.y = 0.0f;
        else diff.y -= (diff.y > 0 ? m_config.deadzone.y : -m_config.deadzone.y);

        // Apply axis lock
        if (m_config.axisLock & AxisLock::LockX) diff.x = 0.0f;
        if (m_config.axisLock & AxisLock::LockY) diff.y = 0.0f;

        // Smooth follow
        Vec2 newPos = current + diff * (m_config.smoothness * dt);

        // Look-ahead: apply offset in the direction the entity is moving
        if (m_config.lookAhead.x != 0.0f || m_config.lookAhead.y != 0.0f) {
            // Normalize velocity to get direction, then scale by lookAhead
            float speed = std::sqrt(targetInfo.velocity.x * targetInfo.velocity.x
                                  + targetInfo.velocity.y * targetInfo.velocity.y);
            if (speed > 1.0f) {
                float dirX = targetInfo.velocity.x / speed;
                float dirY = targetInfo.velocity.y / speed;
                newPos.x += dirX * m_config.lookAhead.x;
                newPos.y += dirY * m_config.lookAhead.y;
            }
        }

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

        // Keep the camera target centered on the non-scrolling axis
        Vec2 target = getTargetPosition();
        if (m_config.axisLock & AxisLock::LockY) {
            newPos.y = target.y;
        } else if (m_config.axisLock & AxisLock::LockX) {
            newPos.x = target.x;
        }

        m_camera->setPosition(newPos);
    }

    void updateRoomBased(float dt) {
        // TODO: Currently identical to GridSnap. Future: add room-transition
        // animations (slide/fade) and discrete boundary detection instead of
        // continuous room-center snapping.
        updateGridSnap(dt);
    }

    Camera* m_camera = nullptr;
    CameraControllerConfig m_config;
};

} // namespace gloaming
