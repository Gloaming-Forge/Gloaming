#pragma once

#include "rendering/IRenderer.hpp"
#include <algorithm>
#include <limits>
#include <cmath>

namespace gloaming {

/// Axis-Aligned Bounding Box for collision detection
/// Uses center + half-extents representation internally for easier math
struct AABB {
    Vec2 center{0.0f, 0.0f};
    Vec2 halfExtents{0.0f, 0.0f};

    constexpr AABB() = default;

    /// Create AABB from center and half-extents
    constexpr AABB(Vec2 center, Vec2 halfExtents)
        : center(center), halfExtents(halfExtents) {}

    /// Create AABB from a Rect (x, y, width, height where x,y is top-left)
    static AABB fromRect(const Rect& rect) {
        return AABB(
            Vec2(rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f),
            Vec2(rect.width * 0.5f, rect.height * 0.5f)
        );
    }

    /// Create AABB from min/max corners
    static AABB fromMinMax(Vec2 min, Vec2 max) {
        Vec2 center = Vec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
        Vec2 halfExtents = Vec2((max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f);
        return AABB(center, halfExtents);
    }

    /// Get minimum corner (top-left)
    Vec2 getMin() const {
        return Vec2(center.x - halfExtents.x, center.y - halfExtents.y);
    }

    /// Get maximum corner (bottom-right)
    Vec2 getMax() const {
        return Vec2(center.x + halfExtents.x, center.y + halfExtents.y);
    }

    /// Get width
    float getWidth() const { return halfExtents.x * 2.0f; }

    /// Get height
    float getHeight() const { return halfExtents.y * 2.0f; }

    /// Convert to Rect (x, y, width, height)
    Rect toRect() const {
        Vec2 min = getMin();
        return Rect(min.x, min.y, halfExtents.x * 2.0f, halfExtents.y * 2.0f);
    }

    /// Check if a point is inside the AABB
    bool contains(Vec2 point) const {
        return point.x >= center.x - halfExtents.x &&
               point.x <= center.x + halfExtents.x &&
               point.y >= center.y - halfExtents.y &&
               point.y <= center.y + halfExtents.y;
    }

    /// Check if two AABBs intersect (overlap)
    bool intersects(const AABB& other) const {
        return std::abs(center.x - other.center.x) <= (halfExtents.x + other.halfExtents.x) &&
               std::abs(center.y - other.center.y) <= (halfExtents.y + other.halfExtents.y);
    }

    /// Get the overlap/penetration depth between two AABBs
    /// Returns (0, 0) if not overlapping
    Vec2 getOverlap(const AABB& other) const {
        float overlapX = (halfExtents.x + other.halfExtents.x) - std::abs(center.x - other.center.x);
        float overlapY = (halfExtents.y + other.halfExtents.y) - std::abs(center.y - other.center.y);

        if (overlapX <= 0.0f || overlapY <= 0.0f) {
            return Vec2(0.0f, 0.0f);
        }

        return Vec2(overlapX, overlapY);
    }

    /// Expand AABB by adding margin on all sides
    AABB expanded(float margin) const {
        return AABB(center, Vec2(halfExtents.x + margin, halfExtents.y + margin));
    }

    /// Expand AABB by different margins for each axis
    AABB expanded(float marginX, float marginY) const {
        return AABB(center, Vec2(halfExtents.x + marginX, halfExtents.y + marginY));
    }

    /// Translate AABB by offset
    AABB translated(Vec2 offset) const {
        return AABB(Vec2(center.x + offset.x, center.y + offset.y), halfExtents);
    }

    /// Get the Minkowski difference of this AABB and another
    /// Useful for collision detection: if the origin is inside the result, the AABBs overlap
    AABB minkowskiDifference(const AABB& other) const {
        Vec2 newCenter = Vec2(center.x - other.center.x, center.y - other.center.y);
        Vec2 newHalfExtents = Vec2(halfExtents.x + other.halfExtents.x,
                                    halfExtents.y + other.halfExtents.y);
        return AABB(newCenter, newHalfExtents);
    }

    /// Compute the closest point on this AABB to the given point
    Vec2 closestPoint(Vec2 point) const {
        Vec2 min = getMin();
        Vec2 max = getMax();
        return Vec2(
            std::clamp(point.x, min.x, max.x),
            std::clamp(point.y, min.y, max.y)
        );
    }

    /// Get the merge of two AABBs (smallest AABB containing both)
    static AABB merge(const AABB& a, const AABB& b) {
        Vec2 minA = a.getMin();
        Vec2 maxA = a.getMax();
        Vec2 minB = b.getMin();
        Vec2 maxB = b.getMax();

        Vec2 min(std::min(minA.x, minB.x), std::min(minA.y, minB.y));
        Vec2 max(std::max(maxA.x, maxB.x), std::max(maxA.y, maxB.y));

        return fromMinMax(min, max);
    }
};

/// Result of a collision test between two AABBs
struct CollisionResult {
    bool collided = false;
    Vec2 normal{0.0f, 0.0f};       // Collision normal (direction to push out)
    float penetration = 0.0f;       // Penetration depth
    Vec2 point{0.0f, 0.0f};        // Contact point (approximate)

    operator bool() const { return collided; }
};

/// Result of a swept collision test
struct SweepResult {
    bool hit = false;
    float time = 1.0f;              // Time of impact (0-1, 1 = no hit)
    Vec2 normal{0.0f, 0.0f};       // Surface normal at hit point
    Vec2 position{0.0f, 0.0f};     // Position at time of impact

    operator bool() const { return hit; }
};

/// Test collision between two AABBs and get detailed result
inline CollisionResult testAABBCollision(const AABB& a, const AABB& b) {
    CollisionResult result;

    Vec2 overlap = a.getOverlap(b);
    if (overlap.x <= 0.0f || overlap.y <= 0.0f) {
        return result; // No collision
    }

    result.collided = true;

    // Find minimum translation vector (MTV)
    if (overlap.x < overlap.y) {
        // Horizontal separation is smaller
        result.penetration = overlap.x;
        result.normal = Vec2(a.center.x < b.center.x ? -1.0f : 1.0f, 0.0f);
    } else {
        // Vertical separation is smaller
        result.penetration = overlap.y;
        result.normal = Vec2(0.0f, a.center.y < b.center.y ? -1.0f : 1.0f);
    }

    // Approximate contact point
    result.point = Vec2(
        std::clamp(a.center.x, b.center.x - b.halfExtents.x, b.center.x + b.halfExtents.x),
        std::clamp(a.center.y, b.center.y - b.halfExtents.y, b.center.y + b.halfExtents.y)
    );

    return result;
}

/// Swept AABB collision detection
/// Returns the time of impact (0-1) when moving 'a' by 'velocity'
inline SweepResult sweepAABB(const AABB& a, Vec2 velocity, const AABB& b) {
    SweepResult result;
    result.time = 1.0f;
    result.position = Vec2(a.center.x + velocity.x, a.center.y + velocity.y);

    // Check if already overlapping
    if (a.intersects(b)) {
        result.hit = true;
        result.time = 0.0f;
        result.position = a.center;
        // Get normal for existing overlap
        Vec2 overlap = a.getOverlap(b);
        if (overlap.x < overlap.y) {
            result.normal = Vec2(a.center.x < b.center.x ? -1.0f : 1.0f, 0.0f);
        } else {
            result.normal = Vec2(0.0f, a.center.y < b.center.y ? -1.0f : 1.0f);
        }
        return result;
    }

    // No movement, no collision
    if (velocity.x == 0.0f && velocity.y == 0.0f) {
        return result;
    }

    // Minkowski sum approach: expand B by A's extents, then raycast
    AABB expanded = AABB(b.center, Vec2(b.halfExtents.x + a.halfExtents.x,
                                         b.halfExtents.y + a.halfExtents.y));

    Vec2 expMin = expanded.getMin();
    Vec2 expMax = expanded.getMax();

    // Slab intersection test with proper zero-velocity handling
    float tMinX, tMaxX, tMinY, tMaxY;

    // X axis
    if (std::abs(velocity.x) < 1e-8f) {
        // No X movement - check if we're within X slab
        if (a.center.x < expMin.x || a.center.x > expMax.x) {
            return result; // Outside X slab, no collision possible
        }
        // Inside X slab - use infinite time range for X
        tMinX = -std::numeric_limits<float>::max();
        tMaxX = std::numeric_limits<float>::max();
    } else {
        float invVelX = 1.0f / velocity.x;
        float t1x = (expMin.x - a.center.x) * invVelX;
        float t2x = (expMax.x - a.center.x) * invVelX;
        tMinX = std::min(t1x, t2x);
        tMaxX = std::max(t1x, t2x);
    }

    // Y axis
    if (std::abs(velocity.y) < 1e-8f) {
        // No Y movement - check if we're within Y slab
        if (a.center.y < expMin.y || a.center.y > expMax.y) {
            return result; // Outside Y slab, no collision possible
        }
        // Inside Y slab - use infinite time range for Y
        tMinY = -std::numeric_limits<float>::max();
        tMaxY = std::numeric_limits<float>::max();
    } else {
        float invVelY = 1.0f / velocity.y;
        float t1y = (expMin.y - a.center.y) * invVelY;
        float t2y = (expMax.y - a.center.y) * invVelY;
        tMinY = std::min(t1y, t2y);
        tMaxY = std::max(t1y, t2y);
    }

    // Find intersection of time ranges
    float tEnter = std::max(tMinX, tMinY);
    float tExit = std::min(tMaxX, tMaxY);

    // Check for valid intersection
    if (tEnter > tExit || tEnter > 1.0f || tExit < 0.0f) {
        return result; // No collision in this frame
    }

    // Collision occurs
    if (tEnter >= 0.0f) {
        result.hit = true;
        result.time = tEnter;
        result.position = Vec2(a.center.x + velocity.x * tEnter,
                               a.center.y + velocity.y * tEnter);

        // Determine collision normal based on which axis was hit first
        if (tMinX > tMinY) {
            result.normal = Vec2(velocity.x > 0.0f ? -1.0f : 1.0f, 0.0f);
        } else {
            result.normal = Vec2(0.0f, velocity.y > 0.0f ? -1.0f : 1.0f);
        }
    }

    return result;
}

/// Calculate the slide velocity after a collision
/// Used for smooth sliding along surfaces
inline Vec2 calculateSlideVelocity(Vec2 velocity, Vec2 normal, float remainingTime) {
    // Project velocity onto surface (perpendicular to normal)
    float dot = Vec2::dot(velocity, normal);
    Vec2 normalComponent = normal * dot;
    Vec2 tangentComponent = velocity - normalComponent;

    return tangentComponent * remainingTime;
}

} // namespace gloaming
