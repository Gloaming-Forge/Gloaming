#pragma once

#include "ecs/Systems.hpp"
#include "ecs/Components.hpp"
#include "world/TileMap.hpp"

#include <cmath>
#include <functional>

namespace gloaming {

/// Cardinal facing direction for grid-based games
enum class FacingDirection : uint8_t {
    Down  = 0,
    Left  = 1,
    Up    = 2,
    Right = 3
};

/// Component for grid-snapped movement (Pokemon / Zelda-style)
/// Entities with this component move tile-by-tile with smooth interpolation.
///
/// Position is tracked as integer tile coordinates (tileX, tileY) to prevent
/// floating-point drift. The visual position is derived from tile coords during
/// movement interpolation.
struct GridMovement {
    int gridSize = 16;                  // Pixels per grid cell
    float moveSpeed = 4.0f;            // Grid cells per second
    FacingDirection facing = FacingDirection::Down;

    // Authoritative position in tile coordinates (prevents floating-point drift).
    // Must be initialized via snapToGrid() before first use — the system auto-initializes
    // from Transform on the first update if tileInitialized is false.
    int tileX = 0;
    int tileY = 0;
    bool tileInitialized = false;

    // Movement state (managed by system)
    bool isMoving = false;
    Vec2 startPos{0.0f, 0.0f};         // World position at start of move
    Vec2 targetPos{0.0f, 0.0f};        // World position at end of move
    float moveProgress = 0.0f;         // 0.0 = start, 1.0 = arrived

    // Input buffering — stores the next requested direction
    bool hasPendingInput = false;
    FacingDirection pendingDirection = FacingDirection::Down;

    GridMovement() = default;
    explicit GridMovement(int grid, float speed = 4.0f)
        : gridSize(grid), moveSpeed(speed) {}

    /// Snap position to nearest grid cell and update tile coords
    Vec2 snapToGrid(Vec2 pos) {
        float gs = static_cast<float>(gridSize);
        tileX = static_cast<int>(std::round(pos.x / gs));
        tileY = static_cast<int>(std::round(pos.y / gs));
        tileInitialized = true;
        return tileToWorldPos();
    }

    /// Get world position from current tile coordinates
    Vec2 tileToWorldPos() const {
        return {
            static_cast<float>(tileX * gridSize),
            static_cast<float>(tileY * gridSize)
        };
    }

    /// Get the tile coordinate for a world position
    int worldToTile(float worldCoord) const {
        return static_cast<int>(std::floor(worldCoord / static_cast<float>(gridSize)));
    }

    /// Get world position for a tile coordinate
    float tileToWorld(int tileCoord) const {
        return static_cast<float>(tileCoord * gridSize);
    }
};

/// Walkability callback type — return true if the tile at (x,y) can be walked on.
/// Set on the system (not per-entity) to avoid std::function overhead in components.
using WalkabilityCallback = std::function<bool(int tileX, int tileY)>;

/// System that processes grid movement for all entities with GridMovement + Transform.
/// This replaces the continuous physics movement for grid-based entities.
///
/// Note: Entities with GridMovement should not also have Velocity, as the physics
/// system would apply gravity/velocity on top of grid-interpolated positions.
class GridMovementSystem : public System {
public:
    GridMovementSystem() : System("GridMovementSystem", -10) {}  // Run before physics

    void init(Registry& registry, Engine& engine) override {
        System::init(registry, engine);
    }

    /// Set the walkability callback (shared across all grid-movement entities).
    /// If not set, all tiles are considered walkable.
    void setWalkabilityCallback(WalkabilityCallback callback) {
        m_isWalkable = std::move(callback);
    }

    void update(float dt) override {
        getRegistry().each<Transform, GridMovement>(
            [dt, this](Entity /*entity*/, Transform& transform, GridMovement& grid) {
                // Auto-initialize tile coords from Transform on first update
                if (!grid.tileInitialized) {
                    grid.snapToGrid(transform.position);
                    transform.position = grid.tileToWorldPos();
                }

                if (grid.isMoving) {
                    // Advance movement progress
                    float moveDuration = 1.0f / grid.moveSpeed;
                    grid.moveProgress += dt / moveDuration;

                    if (grid.moveProgress >= 1.0f) {
                        // Arrived at target — set position from authoritative tile coords
                        grid.moveProgress = 1.0f;
                        transform.position = grid.targetPos;
                        grid.isMoving = false;

                        // Process buffered input
                        if (grid.hasPendingInput) {
                            grid.hasPendingInput = false;
                            tryMove(transform, grid, grid.pendingDirection);
                        }
                    } else {
                        // Interpolate position (smooth step for nicer feel)
                        float t = grid.moveProgress;
                        float smooth = t * t * (3.0f - 2.0f * t);
                        transform.position = {
                            grid.startPos.x + (grid.targetPos.x - grid.startPos.x) * smooth,
                            grid.startPos.y + (grid.targetPos.y - grid.startPos.y) * smooth
                        };
                    }
                }
            }
        );
    }

    /// Request a move in the given direction. Call from input handling.
    bool requestMove(Transform& transform, GridMovement& grid, FacingDirection direction) {
        // Always update facing direction
        grid.facing = direction;

        if (grid.isMoving) {
            // Buffer the input for when current move completes
            grid.hasPendingInput = true;
            grid.pendingDirection = direction;
            return false;
        }

        return tryMove(transform, grid, direction);
    }

private:
    bool tryMove(Transform& transform, GridMovement& grid, FacingDirection direction) {
        // Compute target tile from authoritative tile coordinates
        int targetTileX = grid.tileX;
        int targetTileY = grid.tileY;
        switch (direction) {
            case FacingDirection::Up:    targetTileY -= 1; break;
            case FacingDirection::Down:  targetTileY += 1; break;
            case FacingDirection::Left:  targetTileX -= 1; break;
            case FacingDirection::Right: targetTileX += 1; break;
        }

        // Check walkability via system-level callback
        if (m_isWalkable && !m_isWalkable(targetTileX, targetTileY)) {
            return false;  // Blocked
        }

        // Start the move using integer tile coords as source of truth
        grid.startPos = grid.tileToWorldPos();
        grid.tileX = targetTileX;
        grid.tileY = targetTileY;
        grid.targetPos = grid.tileToWorldPos();
        grid.isMoving = true;
        grid.moveProgress = 0.0f;
        grid.facing = direction;

        // Snap visual position to grid start
        transform.position = grid.startPos;
        return true;
    }

    WalkabilityCallback m_isWalkable;
};

} // namespace gloaming
