#pragma once

#include "physics/AABB.hpp"
#include "world/TileMap.hpp"
#include "rendering/TileRenderer.hpp"
#include <cmath>
#include <vector>
#include <algorithm>
#include <functional>

namespace gloaming {

/// Additional tile flags for physics (extends Tile flags)
/// Slope naming convention (Y-down coordinates, visual perspective):
///   SLOPE_LEFT:  Surface is high on LEFT side, slopes DOWN to the right  /
///   SLOPE_RIGHT: Surface is high on RIGHT side, slopes DOWN to the left   \
namespace TilePhysicsFlags {
    constexpr uint8_t SLOPE_LEFT  = 1 << 3;   // 45° slope: high left, low right (descends rightward)
    constexpr uint8_t SLOPE_RIGHT = 1 << 4;   // 45° slope: low left, high right (descends leftward)
    constexpr uint8_t SLOPE_MASK  = SLOPE_LEFT | SLOPE_RIGHT;
}

/// Result of a tile collision check
struct TileCollisionResult {
    bool collided = false;
    Vec2 normal{0.0f, 0.0f};
    float penetration = 0.0f;
    int tileX = 0;
    int tileY = 0;
    bool isSlope = false;
    bool isPlatform = false;

    operator bool() const { return collided; }
};

/// Result of moving through tiles with collision response
struct TileMoveResult {
    Vec2 newPosition{0.0f, 0.0f};       // Final position after collision
    Vec2 remainingVelocity{0.0f, 0.0f}; // Velocity remaining after collision
    bool hitHorizontal = false;          // Hit a wall on X axis
    bool hitVertical = false;            // Hit floor/ceiling on Y axis
    bool onGround = false;               // Standing on solid ground
    bool onSlope = false;                // Standing on a slope
    bool onPlatform = false;             // Standing on a platform
    std::vector<TileCollisionResult> collisions; // All collisions that occurred
};

/// Tile collision detection and response
class TileCollision {
public:
    /// Configuration for tile collision
    struct Config {
        float skinWidth = 0.01f;       // Small buffer to prevent sticking
        int maxIterations = 4;          // Max collision resolution iterations
        float slopeLimit = 46.0f;       // Max slope angle in degrees (45 + 1 for tolerance)
        float groundCheckDistance = 2.0f; // Distance below feet to check for ground
    };

    TileCollision() = default;
    explicit TileCollision(const Config& config) : m_config(config) {}

    /// Set the tile map to use for collision detection
    void setTileMap(TileMap* tileMap) { m_tileMap = tileMap; }

    /// Set tile size (pixels)
    void setTileSize(int size) { m_tileSize = size; }

    /// Set a tile callback for testing (used when m_tileMap is null)
    /// @note The callback is stored by value, but if it captures references/pointers
    ///       (e.g., [this] or [&obj]), ensure the captured objects outlive this TileCollision instance.
    ///       Typically used in tests where both the provider and TileCollision are stack-local.
    void setTileCallback(std::function<Tile(int, int)> callback) {
        m_tileCallback = std::move(callback);
    }

    /// Get configuration
    Config& getConfig() { return m_config; }
    const Config& getConfig() const { return m_config; }

    /// Check if a point is inside a solid tile
    bool isPointSolid(float worldX, float worldY) const {
        if (!hasTileSource()) return false;

        int tileX, tileY;
        worldToTileCoords(worldX, worldY, tileX, tileY);
        return getTileAt(tileX, tileY).isSolid();
    }

    /// Check if an AABB overlaps any solid tiles
    bool doesAABBOverlapSolid(const AABB& aabb) const {
        if (!hasTileSource()) return false;

        Vec2 min = aabb.getMin();
        Vec2 max = aabb.getMax();

        int minTileX, minTileY, maxTileX, maxTileY;
        worldToTileCoords(min.x, min.y, minTileX, minTileY);
        worldToTileCoords(max.x - 0.01f, max.y - 0.01f, maxTileX, maxTileY);

        for (int ty = minTileY; ty <= maxTileY; ++ty) {
            for (int tx = minTileX; tx <= maxTileX; ++tx) {
                Tile tile = getTileAt(tx, ty);
                if (tile.isSolid()) {
                    return true;
                }
            }
        }
        return false;
    }

    /// Get tiles that an AABB overlaps
    std::vector<std::pair<int, int>> getTilesInAABB(const AABB& aabb) const {
        std::vector<std::pair<int, int>> tiles;
        if (!hasTileSource()) return tiles;

        Vec2 min = aabb.getMin();
        Vec2 max = aabb.getMax();

        int minTileX, minTileY, maxTileX, maxTileY;
        worldToTileCoords(min.x, min.y, minTileX, minTileY);
        worldToTileCoords(max.x - 0.01f, max.y - 0.01f, maxTileX, maxTileY);

        for (int ty = minTileY; ty <= maxTileY; ++ty) {
            for (int tx = minTileX; tx <= maxTileX; ++tx) {
                tiles.emplace_back(tx, ty);
            }
        }
        return tiles;
    }

    /// Test collision between an AABB and tiles
    /// Returns the collision with smallest penetration distance
    TileCollisionResult testAABBTileCollision(const AABB& aabb) const {
        TileCollisionResult result;
        if (!hasTileSource()) return result;

        float smallestPenetration = std::numeric_limits<float>::max();

        auto tiles = getTilesInAABB(aabb.expanded(1.0f));
        for (const auto& [tx, ty] : tiles) {
            auto collision = testSingleTileCollision(aabb, tx, ty);
            if (collision && collision.penetration < smallestPenetration) {
                result = collision;
                smallestPenetration = collision.penetration;
            }
        }

        return result;
    }

    /// Test collision with a single tile
    TileCollisionResult testSingleTileCollision(const AABB& aabb, int tileX, int tileY) const {
        TileCollisionResult result;
        if (!hasTileSource()) return result;

        Tile tile = getTileAt(tileX, tileY);
        if (tile.isEmpty()) return result;

        result.tileX = tileX;
        result.tileY = tileY;
        result.isPlatform = (tile.flags & Tile::FLAG_PLATFORM) != 0;
        result.isSlope = (tile.flags & TilePhysicsFlags::SLOPE_MASK) != 0;

        // Get tile AABB
        float tileWorldX = static_cast<float>(tileX * m_tileSize);
        float tileWorldY = static_cast<float>(tileY * m_tileSize);
        AABB tileAABB = AABB(
            Vec2(tileWorldX + m_tileSize * 0.5f, tileWorldY + m_tileSize * 0.5f),
            Vec2(m_tileSize * 0.5f, m_tileSize * 0.5f)
        );

        // Handle slopes
        if (result.isSlope) {
            return testSlopeCollision(aabb, tileX, tileY, tile.flags);
        }

        // Standard solid tile
        if (tile.isSolid()) {
            auto aabbResult = testAABBCollision(aabb, tileAABB);
            if (aabbResult.collided) {
                result.collided = true;
                result.normal = aabbResult.normal;
                result.penetration = aabbResult.penetration;
            }
        }

        return result;
    }

    /// Test collision with a slope tile
    TileCollisionResult testSlopeCollision(const AABB& aabb, int tileX, int tileY, uint8_t flags) const {
        TileCollisionResult result;
        result.tileX = tileX;
        result.tileY = tileY;
        result.isSlope = true;

        float tileWorldX = static_cast<float>(tileX * m_tileSize);
        float tileWorldY = static_cast<float>(tileY * m_tileSize);

        Vec2 aabbMin = aabb.getMin();
        Vec2 aabbMax = aabb.getMax();

        // Check if AABB is in tile's X range
        if (aabbMax.x <= tileWorldX || aabbMin.x >= tileWorldX + m_tileSize) {
            return result;
        }

        // Calculate slope height at entity's position
        float relativeX;
        bool slopeLeft = (flags & TilePhysicsFlags::SLOPE_LEFT) != 0;

        if (slopeLeft) {
            // High on left, low on right
            relativeX = std::clamp((aabbMin.x + aabbMax.x) * 0.5f - tileWorldX,
                                    0.0f, static_cast<float>(m_tileSize));
            // Height decreases as X increases
        } else {
            // High on right, low on left
            relativeX = std::clamp((aabbMin.x + aabbMax.x) * 0.5f - tileWorldX,
                                    0.0f, static_cast<float>(m_tileSize));
        }

        float slopeRatio = relativeX / static_cast<float>(m_tileSize);
        float slopeHeight;

        // In Y-down coordinates:
        // - SLOPE_LEFT: high on left (smaller Y at x=0), low on right (larger Y at x=tileSize)
        // - SLOPE_RIGHT: low on left (larger Y at x=0), high on right (smaller Y at x=tileSize)
        if (slopeLeft) {
            // At slopeRatio=0 (left): slopeHeight = tileWorldY (top of tile)
            // At slopeRatio=1 (right): slopeHeight = tileWorldY + tileSize (bottom of tile)
            slopeHeight = tileWorldY + m_tileSize * slopeRatio;
        } else {
            // At slopeRatio=0 (left): slopeHeight = tileWorldY + tileSize (bottom of tile)
            // At slopeRatio=1 (right): slopeHeight = tileWorldY (top of tile)
            slopeHeight = tileWorldY + m_tileSize * (1.0f - slopeRatio);
        }

        // Check if entity bottom is below slope surface
        float entityBottom = aabbMax.y;
        if (entityBottom > slopeHeight) {
            result.collided = true;
            result.penetration = entityBottom - slopeHeight;

            // Slope normal (45 degrees) - 0.7071f is 1/sqrt(2), already normalized
            constexpr float SQRT2_INV = 0.7071067811865476f;
            if (slopeLeft) {
                result.normal = Vec2(SQRT2_INV, -SQRT2_INV);
            } else {
                result.normal = Vec2(-SQRT2_INV, -SQRT2_INV);
            }
        }

        return result;
    }

    /// Move an AABB through the tile world with collision response
    /// This is the main method for physics integration
    TileMoveResult moveAABB(AABB aabb, Vec2 velocity, bool checkPlatforms = true, bool wasOnGround = false) const {
        TileMoveResult result;
        result.newPosition = aabb.center;

        if (!hasTileSource()) {
            result.newPosition = aabb.center + velocity;
            result.remainingVelocity = velocity;
            return result;
        }

        Vec2 remainingVelocity = velocity;

        for (int iteration = 0; iteration < m_config.maxIterations; ++iteration) {
            if (std::abs(remainingVelocity.x) < 0.001f && std::abs(remainingVelocity.y) < 0.001f) {
                break;
            }

            // Try X movement first
            if (std::abs(remainingVelocity.x) > 0.001f) {
                AABB testAABB = aabb.translated(Vec2(remainingVelocity.x, 0.0f));
                auto collision = resolveHorizontalCollision(testAABB, remainingVelocity.x);

                if (collision.collided) {
                    result.hitHorizontal = true;
                    result.collisions.push_back(collision);

                    // Move to contact point with skin gap
                    // We moved by velocity and penetrated by 'penetration', so back up by that amount plus skin
                    // Clamp to prevent overshooting backwards (can happen if already slightly overlapping)
                    if (remainingVelocity.x > 0.0f) {
                        // Moving right: subtract penetration and skin from the movement
                        float movement = remainingVelocity.x - collision.penetration - m_config.skinWidth;
                        aabb.center.x += std::max(0.0f, movement);  // Don't move backwards
                    } else {
                        // Moving left: add penetration and skin (penetration is always positive)
                        float movement = remainingVelocity.x + collision.penetration + m_config.skinWidth;
                        aabb.center.x += std::min(0.0f, movement);  // Don't move backwards
                    }
                    remainingVelocity.x = 0.0f;
                } else {
                    aabb.center.x += remainingVelocity.x;
                    remainingVelocity.x = 0.0f;
                }
            }

            // Then Y movement
            if (std::abs(remainingVelocity.y) > 0.001f) {
                AABB testAABB = aabb.translated(Vec2(0.0f, remainingVelocity.y));
                auto collision = resolveVerticalCollision(testAABB, remainingVelocity.y,
                                                           checkPlatforms, wasOnGround);

                if (collision.collided) {
                    result.hitVertical = true;
                    result.collisions.push_back(collision);

                    if (collision.isSlope) {
                        // For slopes, snap to slope surface
                        // Clamp to prevent overshooting backwards (consistent with regular tile handling)
                        float movement = remainingVelocity.y - collision.penetration;
                        if (remainingVelocity.y > 0.0f) {
                            aabb.center.y += std::max(0.0f, movement);  // Moving down, don't go up
                        } else {
                            aabb.center.y += std::min(0.0f, movement);  // Moving up, don't go down
                        }
                        result.onSlope = true;
                        result.onGround = true;
                    } else {
                        // Regular tile - move to contact point with skin gap
                        // Clamp to prevent overshooting backwards (can happen if already slightly overlapping)
                        if (remainingVelocity.y > 0.0f) {
                            // Moving down: subtract penetration and skin from the movement
                            float movement = remainingVelocity.y - collision.penetration - m_config.skinWidth;
                            aabb.center.y += std::max(0.0f, movement);  // Don't move backwards
                        } else {
                            // Moving up: add penetration and skin (penetration is always positive)
                            float movement = remainingVelocity.y + collision.penetration + m_config.skinWidth;
                            aabb.center.y += std::min(0.0f, movement);  // Don't move backwards
                        }

                        // Check if we landed on ground (moving down and hit something below)
                        if (remainingVelocity.y > 0.0f && collision.normal.y < 0.0f) {
                            result.onGround = true;
                            result.onPlatform = collision.isPlatform;
                        }
                    }
                    remainingVelocity.y = 0.0f;
                } else {
                    aabb.center.y += remainingVelocity.y;
                    remainingVelocity.y = 0.0f;
                }
            }
        }

        result.newPosition = aabb.center;
        result.remainingVelocity = remainingVelocity;

        // Additional ground check if we didn't detect ground during movement
        if (!result.onGround && velocity.y >= 0.0f) {
            result.onGround = checkGroundBelow(aabb);
        }

        return result;
    }

    /// Check if there's ground directly below an AABB
    bool checkGroundBelow(const AABB& aabb, float distance = 2.0f) const {
        if (!hasTileSource()) return false;

        Vec2 checkPoint(aabb.center.x, aabb.getMax().y + distance);
        int tileX, tileY;
        worldToTileCoords(checkPoint.x, checkPoint.y, tileX, tileY);

        Tile tile = getTileAt(tileX, tileY);
        return tile.isSolid() || (tile.flags & Tile::FLAG_PLATFORM);
    }

    /// Swept collision against tiles
    /// Returns the first tile hit when moving the AABB by velocity
    SweepResult sweepAABBTiles(const AABB& aabb, Vec2 velocity) const {
        SweepResult result;
        result.time = 1.0f;

        if (!hasTileSource()) return result;
        if (velocity.x == 0.0f && velocity.y == 0.0f) return result;

        // Calculate swept AABB bounds
        AABB sweptBounds = AABB::merge(aabb, aabb.translated(velocity));
        auto tiles = getTilesInAABB(sweptBounds.expanded(static_cast<float>(m_tileSize)));

        for (const auto& [tx, ty] : tiles) {
            Tile tile = getTileAt(tx, ty);
            if (!tile.isSolid()) continue;

            // Skip slopes for swept test (handle separately)
            if (tile.flags & TilePhysicsFlags::SLOPE_MASK) continue;

            float tileWorldX = static_cast<float>(tx * m_tileSize);
            float tileWorldY = static_cast<float>(ty * m_tileSize);
            AABB tileAABB = AABB(
                Vec2(tileWorldX + m_tileSize * 0.5f, tileWorldY + m_tileSize * 0.5f),
                Vec2(m_tileSize * 0.5f, m_tileSize * 0.5f)
            );

            auto sweep = sweepAABB(aabb, velocity, tileAABB);
            if (sweep.hit && sweep.time < result.time) {
                result = sweep;
            }
        }

        return result;
    }

private:
    /// Resolve horizontal collision
    TileCollisionResult resolveHorizontalCollision(const AABB& testAABB, float velocityX) const {
        TileCollisionResult result;

        auto tiles = getTilesInAABB(testAABB);
        float closestPenetration = 0.0f;

        for (const auto& [tx, ty] : tiles) {
            Tile tile = getTileAt(tx, ty);
            if (!tile.isSolid()) continue;

            // Skip platforms for horizontal collision
            if ((tile.flags & Tile::FLAG_PLATFORM) && !(tile.flags & Tile::FLAG_SOLID)) continue;

            // Skip slopes for horizontal (they use Y collision)
            if (tile.flags & TilePhysicsFlags::SLOPE_MASK) continue;

            float tileWorldX = static_cast<float>(tx * m_tileSize);
            float tileWorldY = static_cast<float>(ty * m_tileSize);
            AABB tileAABB = AABB(
                Vec2(tileWorldX + m_tileSize * 0.5f, tileWorldY + m_tileSize * 0.5f),
                Vec2(m_tileSize * 0.5f, m_tileSize * 0.5f)
            );

            auto collision = testAABBCollision(testAABB, tileAABB);
            if (collision.collided && collision.penetration > closestPenetration) {
                result.collided = true;
                result.tileX = tx;
                result.tileY = ty;
                result.penetration = collision.penetration;
                result.normal = Vec2(velocityX > 0.0f ? -1.0f : 1.0f, 0.0f);
                closestPenetration = collision.penetration;
            }
        }

        return result;
    }

    /// Resolve vertical collision
    TileCollisionResult resolveVerticalCollision(const AABB& testAABB, float velocityY,
                                                  bool checkPlatforms, bool wasOnGround) const {
        TileCollisionResult result;

        auto tiles = getTilesInAABB(testAABB);
        float closestPenetration = 0.0f;

        for (const auto& [tx, ty] : tiles) {
            Tile tile = getTileAt(tx, ty);
            if (tile.isEmpty()) continue;

            bool isSolid = tile.isSolid();
            bool isPlatform = (tile.flags & Tile::FLAG_PLATFORM) != 0;
            bool isSlope = (tile.flags & TilePhysicsFlags::SLOPE_MASK) != 0;

            // Handle one-way platforms
            if (isPlatform && !isSolid) {
                if (!checkPlatforms) continue;

                // Only collide when falling (moving down)
                if (velocityY <= 0.0f) continue;

                // Check if entity was above platform before movement
                float platformTop = static_cast<float>(ty * m_tileSize);
                float entityBottom = testAABB.getMax().y - velocityY; // Previous bottom

                // Only collide if we were above the platform
                if (entityBottom > platformTop + m_config.skinWidth) continue;
            }

            // Handle slopes
            if (isSlope) {
                auto slopeResult = testSlopeCollision(testAABB, tx, ty, tile.flags);
                if (slopeResult.collided && slopeResult.penetration > closestPenetration) {
                    result = slopeResult;
                    closestPenetration = slopeResult.penetration;
                }
                continue;
            }

            if (!isSolid && !isPlatform) continue;

            float tileWorldX = static_cast<float>(tx * m_tileSize);
            float tileWorldY = static_cast<float>(ty * m_tileSize);
            AABB tileAABB = AABB(
                Vec2(tileWorldX + m_tileSize * 0.5f, tileWorldY + m_tileSize * 0.5f),
                Vec2(m_tileSize * 0.5f, m_tileSize * 0.5f)
            );

            auto collision = testAABBCollision(testAABB, tileAABB);
            if (collision.collided && collision.penetration > closestPenetration) {
                result.collided = true;
                result.tileX = tx;
                result.tileY = ty;
                result.penetration = collision.penetration;
                result.normal = Vec2(0.0f, velocityY > 0.0f ? -1.0f : 1.0f);
                result.isPlatform = isPlatform;
                closestPenetration = collision.penetration;
            }
        }

        return result;
    }

    /// Get a tile using either TileMap or the callback
    Tile getTileAt(int tileX, int tileY) const {
        if (m_tileMap) {
            return m_tileMap->getTile(tileX, tileY);
        }
        if (m_tileCallback) {
            return m_tileCallback(tileX, tileY);
        }
        return Tile{};
    }

    /// Check if we have a tile source (map or callback)
    bool hasTileSource() const {
        return m_tileMap != nullptr || m_tileCallback != nullptr;
    }

    /// Convert world position to tile coordinates
    void worldToTileCoords(float worldX, float worldY, int& tileX, int& tileY) const {
        if (m_tileMap) {
            m_tileMap->worldToTile(worldX, worldY, tileX, tileY);
        } else {
            tileX = static_cast<int>(std::floor(worldX / m_tileSize));
            tileY = static_cast<int>(std::floor(worldY / m_tileSize));
        }
    }

    TileMap* m_tileMap = nullptr;
    std::function<Tile(int, int)> m_tileCallback;
    int m_tileSize = 16;
    Config m_config;
};

} // namespace gloaming
