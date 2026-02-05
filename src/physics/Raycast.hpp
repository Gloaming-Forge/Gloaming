#pragma once

#include "physics/AABB.hpp"
#include "physics/Collision.hpp"
#include "world/TileMap.hpp"
#include "ecs/Components.hpp"
#include "ecs/Registry.hpp"
#include <cmath>
#include <limits>
#include <vector>

namespace gloaming {

// Math constants for raycasting (duplicated from IRenderer.hpp to avoid circular deps)
constexpr float RAYCAST_PI = 3.14159265358979323846f;
constexpr float RAYCAST_DEG_TO_RAD = RAYCAST_PI / 180.0f;

/// Result of a raycast
struct RaycastHit {
    bool hit = false;
    float distance = 0.0f;          // Distance to hit point
    Vec2 point{0.0f, 0.0f};        // Hit point in world coordinates
    Vec2 normal{0.0f, 0.0f};       // Surface normal at hit point

    // For tile hits
    int tileX = 0;
    int tileY = 0;
    bool hitTile = false;

    // For entity hits
    uint32_t entity = 0;
    bool hitEntity = false;

    operator bool() const { return hit; }
};

/// Ray structure for raycasting
struct Ray {
    Vec2 origin{0.0f, 0.0f};
    Vec2 direction{1.0f, 0.0f};     // Should be normalized

    Ray() = default;
    Ray(Vec2 origin, Vec2 direction)
        : origin(origin), direction(direction.normalized()) {}

    /// Get point along ray at distance t
    Vec2 getPoint(float t) const {
        return Vec2(origin.x + direction.x * t, origin.y + direction.y * t);
    }
};

/// Raycasting utilities
class Raycast {
public:
    /// Raycast against an AABB
    /// Returns distance to hit, or negative if no hit
    static float raycastAABB(const Ray& ray, const AABB& aabb, Vec2* outNormal = nullptr) {
        Vec2 min = aabb.getMin();
        Vec2 max = aabb.getMax();

        // Slab intersection
        float tMin = -std::numeric_limits<float>::max();
        float tMax = std::numeric_limits<float>::max();
        Vec2 normal{0.0f, 0.0f};

        // X axis
        if (std::abs(ray.direction.x) < 1e-8f) {
            // Ray parallel to slab
            if (ray.origin.x < min.x || ray.origin.x > max.x) {
                return -1.0f;
            }
        } else {
            float invD = 1.0f / ray.direction.x;
            float t1 = (min.x - ray.origin.x) * invD;
            float t2 = (max.x - ray.origin.x) * invD;

            Vec2 n1(-1.0f, 0.0f);
            Vec2 n2(1.0f, 0.0f);

            if (t1 > t2) {
                std::swap(t1, t2);
                std::swap(n1, n2);
            }

            if (t1 > tMin) {
                tMin = t1;
                normal = n1;
            }
            tMax = std::min(tMax, t2);

            if (tMin > tMax) return -1.0f;
        }

        // Y axis
        if (std::abs(ray.direction.y) < 1e-8f) {
            // Ray parallel to slab
            if (ray.origin.y < min.y || ray.origin.y > max.y) {
                return -1.0f;
            }
        } else {
            float invD = 1.0f / ray.direction.y;
            float t1 = (min.y - ray.origin.y) * invD;
            float t2 = (max.y - ray.origin.y) * invD;

            Vec2 n1(0.0f, -1.0f);
            Vec2 n2(0.0f, 1.0f);

            if (t1 > t2) {
                std::swap(t1, t2);
                std::swap(n1, n2);
            }

            if (t1 > tMin) {
                tMin = t1;
                normal = n1;
            }
            tMax = std::min(tMax, t2);

            if (tMin > tMax) return -1.0f;
        }

        // Check if intersection is behind the ray origin
        if (tMax < 0.0f) return -1.0f;

        if (tMin >= 0.0f) {
            // Normal hit from outside
            if (outNormal) *outNormal = normal;
            return tMin;
        } else {
            // Ray origin is inside the box - return distance 0 with zero normal
            // (no meaningful surface normal when starting inside)
            if (outNormal) *outNormal = Vec2(0.0f, 0.0f);
            return 0.0f;
        }
    }

    /// Raycast against tiles using DDA (Digital Differential Analyzer) algorithm
    static RaycastHit raycastTiles(const Ray& ray, float maxDistance, TileMap* tileMap, int tileSize) {
        RaycastHit result;
        if (!tileMap) return result;

        // Starting tile
        int tileX, tileY;
        tileMap->worldToTile(ray.origin.x, ray.origin.y, tileX, tileY);

        // Direction of step
        int stepX = ray.direction.x >= 0 ? 1 : -1;
        int stepY = ray.direction.y >= 0 ? 1 : -1;

        // Distance to move along ray to cross one tile
        float tDeltaX = std::abs(ray.direction.x) > 1e-8f
            ? std::abs(static_cast<float>(tileSize) / ray.direction.x)
            : std::numeric_limits<float>::max();
        float tDeltaY = std::abs(ray.direction.y) > 1e-8f
            ? std::abs(static_cast<float>(tileSize) / ray.direction.y)
            : std::numeric_limits<float>::max();

        // Distance to first tile boundary
        float tileWorldX = static_cast<float>(tileX * tileSize);
        float tileWorldY = static_cast<float>(tileY * tileSize);

        float tMaxX, tMaxY;
        if (ray.direction.x >= 0) {
            tMaxX = std::abs(ray.direction.x) > 1e-8f
                ? ((tileWorldX + tileSize) - ray.origin.x) / ray.direction.x
                : std::numeric_limits<float>::max();
        } else {
            tMaxX = std::abs(ray.direction.x) > 1e-8f
                ? (tileWorldX - ray.origin.x) / ray.direction.x
                : std::numeric_limits<float>::max();
        }

        if (ray.direction.y >= 0) {
            tMaxY = std::abs(ray.direction.y) > 1e-8f
                ? ((tileWorldY + tileSize) - ray.origin.y) / ray.direction.y
                : std::numeric_limits<float>::max();
        } else {
            tMaxY = std::abs(ray.direction.y) > 1e-8f
                ? (tileWorldY - ray.origin.y) / ray.direction.y
                : std::numeric_limits<float>::max();
        }

        // DDA traversal
        float distance = 0.0f;
        Vec2 lastNormal{0.0f, 0.0f};

        while (distance < maxDistance) {
            // Check current tile
            Tile tile = tileMap->getTile(tileX, tileY);
            if (tile.isSolid()) {
                result.hit = true;
                result.hitTile = true;
                result.distance = distance;
                result.point = ray.getPoint(distance);
                result.normal = lastNormal;
                result.tileX = tileX;
                result.tileY = tileY;
                return result;
            }

            // Move to next tile
            if (tMaxX < tMaxY) {
                distance = tMaxX;
                tMaxX += tDeltaX;
                tileX += stepX;
                lastNormal = Vec2(-static_cast<float>(stepX), 0.0f);
            } else {
                distance = tMaxY;
                tMaxY += tDeltaY;
                tileY += stepY;
                lastNormal = Vec2(0.0f, -static_cast<float>(stepY));
            }
        }

        return result;
    }

    /// Raycast against all entities in the registry
    static RaycastHit raycastEntities(const Ray& ray, float maxDistance, Registry& registry,
                                       uint32_t ignoreMask = 0, uint32_t ignoreEntity = 0) {
        RaycastHit result;
        float closestDistance = maxDistance;

        auto view = registry.view<Transform, Collider>();
        for (auto entity : view) {
            uint32_t entityId = static_cast<uint32_t>(entity);
            if (entityId == ignoreEntity) continue;

            auto& collider = view.get<Collider>(entity);
            if (!collider.enabled) continue;
            if ((collider.layer & ignoreMask) != 0) continue;
            if (collider.isTrigger) continue; // Skip triggers for physics raycast

            auto& transform = view.get<Transform>(entity);
            AABB aabb = Collision::getEntityAABB(transform, collider);

            Vec2 normal;
            float distance = raycastAABB(ray, aabb, &normal);

            if (distance >= 0.0f && distance < closestDistance) {
                closestDistance = distance;
                result.hit = true;
                result.hitEntity = true;
                result.distance = distance;
                result.point = ray.getPoint(distance);
                result.normal = normal;
                result.entity = entityId;
            }
        }

        return result;
    }

    /// Raycast against both tiles and entities
    /// Returns the closest hit
    static RaycastHit raycast(const Ray& ray, float maxDistance, TileMap* tileMap, int tileSize,
                               Registry& registry, uint32_t ignoreMask = 0, uint32_t ignoreEntity = 0) {
        RaycastHit tileHit = raycastTiles(ray, maxDistance, tileMap, tileSize);
        RaycastHit entityHit = raycastEntities(ray, maxDistance, registry, ignoreMask, ignoreEntity);

        // Return the closer hit
        if (tileHit.hit && entityHit.hit) {
            return tileHit.distance <= entityHit.distance ? tileHit : entityHit;
        } else if (tileHit.hit) {
            return tileHit;
        } else {
            return entityHit;
        }
    }

    /// Cast multiple rays in a cone pattern
    /// Useful for shotgun-style attacks or area detection
    static std::vector<RaycastHit> raycastCone(
        Vec2 origin, Vec2 direction, float maxDistance,
        int rayCount, float coneAngle,
        TileMap* tileMap, int tileSize,
        Registry& registry, uint32_t ignoreMask = 0, uint32_t ignoreEntity = 0
    ) {
        std::vector<RaycastHit> results;
        results.reserve(rayCount);

        float halfAngle = coneAngle * 0.5f * RAYCAST_DEG_TO_RAD;
        float angleStep = rayCount > 1 ? coneAngle * RAYCAST_DEG_TO_RAD / (rayCount - 1) : 0.0f;
        float startAngle = std::atan2(direction.y, direction.x) - halfAngle;

        for (int i = 0; i < rayCount; ++i) {
            float angle = startAngle + angleStep * i;
            Vec2 rayDir(std::cos(angle), std::sin(angle));
            Ray ray(origin, rayDir);

            results.push_back(raycast(ray, maxDistance, tileMap, tileSize,
                                       registry, ignoreMask, ignoreEntity));
        }

        return results;
    }

    /// Line-of-sight check between two points
    static bool hasLineOfSight(Vec2 from, Vec2 to, TileMap* tileMap, int tileSize) {
        Vec2 direction = to - from;
        float distance = direction.length();
        if (distance < 0.001f) return true;

        Ray ray(from, direction);
        RaycastHit hit = raycastTiles(ray, distance, tileMap, tileSize);

        return !hit.hit;
    }

    /// Check line-of-sight including entity obstacles
    static bool hasLineOfSight(Vec2 from, Vec2 to, TileMap* tileMap, int tileSize,
                                Registry& registry, uint32_t ignoreEntity = 0) {
        Vec2 direction = to - from;
        float distance = direction.length();
        if (distance < 0.001f) return true;

        Ray ray(from, direction);
        RaycastHit hit = raycast(ray, distance, tileMap, tileSize, registry, 0, ignoreEntity);

        return !hit.hit;
    }
};

} // namespace gloaming
