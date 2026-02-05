#include "rendering/Camera.hpp"
#include <cmath>
#include <algorithm>

namespace gloaming {

Camera::Camera(float screenWidth, float screenHeight)
    : m_screenSize(screenWidth, screenHeight) {}

void Camera::setPosition(Vec2 position) {
    m_position = position;
    clampToBounds();
}

void Camera::move(Vec2 delta) {
    m_position += delta;
    clampToBounds();
}

void Camera::setZoom(float zoom) {
    m_zoom = std::clamp(zoom, MIN_ZOOM, MAX_ZOOM);
    clampToBounds();  // Visible area changes with zoom
}

void Camera::zoom(float delta) {
    setZoom(m_zoom + delta);
}

void Camera::setRotation(float degrees) {
    m_rotation = degrees;
    // Normalize to 0-360 range
    while (m_rotation < 0.0f) m_rotation += 360.0f;
    while (m_rotation >= 360.0f) m_rotation -= 360.0f;
}

void Camera::rotate(float deltaDegrees) {
    setRotation(m_rotation + deltaDegrees);
}

void Camera::setScreenSize(float width, float height) {
    m_screenSize = {width, height};
    clampToBounds();
}

void Camera::setWorldBounds(const Rect& bounds) {
    m_worldBounds = bounds;
    m_hasBounds = true;
    clampToBounds();
}

void Camera::clearWorldBounds() {
    m_hasBounds = false;
}

Vec2 Camera::screenToWorld(Vec2 screenPos) const {
    // Account for camera position and zoom
    // Screen center is at camera position
    Vec2 center = m_screenSize * 0.5f;
    Vec2 offset = (screenPos - center) / m_zoom;

    // Apply rotation (inverse)
    if (m_rotation != 0.0f) {
        float radians = -m_rotation * DEG_TO_RAD;
        float cosR = std::cos(radians);
        float sinR = std::sin(radians);
        Vec2 rotated = {
            offset.x * cosR - offset.y * sinR,
            offset.x * sinR + offset.y * cosR
        };
        offset = rotated;
    }

    return m_position + offset;
}

Vec2 Camera::worldToScreen(Vec2 worldPos) const {
    Vec2 offset = worldPos - m_position;

    // Apply rotation
    if (m_rotation != 0.0f) {
        float radians = m_rotation * DEG_TO_RAD;
        float cosR = std::cos(radians);
        float sinR = std::sin(radians);
        Vec2 rotated = {
            offset.x * cosR - offset.y * sinR,
            offset.x * sinR + offset.y * cosR
        };
        offset = rotated;
    }

    // Apply zoom and center
    Vec2 center = m_screenSize * 0.5f;
    return center + (offset * m_zoom);
}

Rect Camera::getVisibleArea() const {
    // Calculate visible area accounting for zoom
    float visibleWidth = m_screenSize.x / m_zoom;
    float visibleHeight = m_screenSize.y / m_zoom;

    // When rotated, we need a larger bounding box
    if (m_rotation != 0.0f) {
        float radians = std::abs(m_rotation) * DEG_TO_RAD;
        float cosR = std::abs(std::cos(radians));
        float sinR = std::abs(std::sin(radians));
        float newWidth = visibleWidth * cosR + visibleHeight * sinR;
        float newHeight = visibleWidth * sinR + visibleHeight * cosR;
        visibleWidth = newWidth;
        visibleHeight = newHeight;
    }

    return Rect(
        m_position.x - visibleWidth * 0.5f,
        m_position.y - visibleHeight * 0.5f,
        visibleWidth,
        visibleHeight
    );
}

bool Camera::isVisible(const Rect& worldRect) const {
    Rect visible = getVisibleArea();
    return visible.intersects(worldRect);
}

bool Camera::isVisible(Vec2 worldPoint) const {
    Rect visible = getVisibleArea();
    return visible.contains(worldPoint);
}

void Camera::follow(Vec2 target, float smoothness, float dt) {
    if (smoothness <= 0.0f) {
        setPosition(target);
        return;
    }

    // Exponential smoothing for frame-rate independent movement
    float t = 1.0f - std::exp(-dt / smoothness);
    Vec2 newPos = {
        m_position.x + (target.x - m_position.x) * t,
        m_position.y + (target.y - m_position.y) * t
    };
    setPosition(newPos);
}

Vec2 Camera::getOffset() const {
    return m_screenSize * 0.5f;
}

void Camera::clampToBounds() {
    if (!m_hasBounds) return;

    Rect visible = getVisibleArea();
    float halfWidth = visible.width * 0.5f;
    float halfHeight = visible.height * 0.5f;

    // If visible area is larger than bounds, center on bounds
    if (visible.width >= m_worldBounds.width) {
        m_position.x = m_worldBounds.x + m_worldBounds.width * 0.5f;
    } else {
        // Clamp position so visible area stays within bounds
        float minX = m_worldBounds.x + halfWidth;
        float maxX = m_worldBounds.x + m_worldBounds.width - halfWidth;
        m_position.x = std::clamp(m_position.x, minX, maxX);
    }

    if (visible.height >= m_worldBounds.height) {
        m_position.y = m_worldBounds.y + m_worldBounds.height * 0.5f;
    } else {
        float minY = m_worldBounds.y + halfHeight;
        float maxY = m_worldBounds.y + m_worldBounds.height - halfHeight;
        m_position.y = std::clamp(m_position.y, minY, maxY);
    }
}

} // namespace gloaming
