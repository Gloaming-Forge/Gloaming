#pragma once

#include "rendering/IRenderer.hpp"

namespace gloaming {

/// 2D Camera for world space to screen space transformation
/// Supports position, zoom, rotation, and optional world bounds
class Camera {
public:
    Camera() = default;
    Camera(float screenWidth, float screenHeight);

    /// Set the camera position (world coordinates)
    void setPosition(Vec2 position);
    void setPosition(float x, float y) { setPosition({x, y}); }

    /// Get the camera position
    Vec2 getPosition() const { return m_position; }

    /// Move the camera by a delta
    void move(Vec2 delta);
    void move(float dx, float dy) { move({dx, dy}); }

    /// Set the zoom level (1.0 = normal, 2.0 = 2x zoom in)
    void setZoom(float zoom);

    /// Get the current zoom level
    float getZoom() const { return m_zoom; }

    /// Zoom by a delta (positive = zoom in, negative = zoom out)
    void zoom(float delta);

    /// Set the camera rotation in degrees
    void setRotation(float degrees);

    /// Get the camera rotation in degrees
    float getRotation() const { return m_rotation; }

    /// Rotate by a delta in degrees
    void rotate(float deltaDegrees);

    /// Set the screen/viewport size
    void setScreenSize(float width, float height);
    void setScreenSize(Vec2 size) { setScreenSize(size.x, size.y); }

    /// Get the screen size
    Vec2 getScreenSize() const { return m_screenSize; }

    /// Set world bounds for camera movement (optional)
    /// When enabled, the camera will not show areas outside these bounds
    void setWorldBounds(const Rect& bounds);

    /// Clear world bounds (allow camera to move freely)
    void clearWorldBounds();

    /// Check if world bounds are set
    bool hasWorldBounds() const { return m_hasBounds; }

    /// Get world bounds
    Rect getWorldBounds() const { return m_worldBounds; }

    /// Convert screen coordinates to world coordinates
    Vec2 screenToWorld(Vec2 screenPos) const;

    /// Convert world coordinates to screen coordinates
    Vec2 worldToScreen(Vec2 worldPos) const;

    /// Get the visible area in world coordinates
    Rect getVisibleArea() const;

    /// Check if a world rectangle is visible on screen
    bool isVisible(const Rect& worldRect) const;

    /// Check if a world point is visible on screen
    bool isVisible(Vec2 worldPoint) const;

    /// Smoothly follow a target position
    /// @param target The target position to follow
    /// @param smoothness Smoothing factor (0 = instant, higher = slower)
    /// @param dt Delta time for frame-independent movement
    void follow(Vec2 target, float smoothness, float dt);

    /// Get the camera offset (center of screen in world space)
    Vec2 getOffset() const;

private:
    void clampToBounds();

    Vec2 m_position = {0.0f, 0.0f};
    Vec2 m_screenSize = {1280.0f, 720.0f};
    float m_zoom = 1.0f;
    float m_rotation = 0.0f;

    bool m_hasBounds = false;
    Rect m_worldBounds = {0.0f, 0.0f, 0.0f, 0.0f};

    // Zoom limits
    static constexpr float MIN_ZOOM = 0.1f;
    static constexpr float MAX_ZOOM = 10.0f;
};

} // namespace gloaming
