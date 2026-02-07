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
struct GridMovement {
    int gridSize = 16;                  // Pixels per grid cell
    float moveSpeed = 4.0f;            // Grid cells per second
    FacingDirection facing = FacingDirection::Down;

    // Movement state (managed by system)
    bool isMoving = false;
    Vec2 startPos{0.0f, 0.0f};         // World position at start of move
    Vec2 targetPos{0.0f, 0.0f};        // World position at end of move
    float moveProgress = 0.0f;         // 0.0 = start, 1.0 = arrived

    // Input buffering — stores the next requested direction
    bool hasPendingInput = false;
    FacingDirection pendingDirection = FacingDirection::Down;

    // Collision check callback — return true if the target tile is walkable.
    // If not set, all tiles are walkable.
    std::function<bool(int tileX, int tileY)> isWalkable;

    GridMovement() = default;
    explicit GridMovement(int grid, float speed = 4.0f)
        : gridSize(grid), moveSpeed(speed) {}

    /// Snap position to nearest grid cell
    Vec2 snapToGrid(Vec2 pos) const {
        float gs = static_cast<float>(gridSize);
        return {
            std::round(pos.x / gs) * gs,
            std::round(pos.y / gs) * gs
        };
    }

    /// Get the tile coordinate for a world position
    int worldToTile(float worldCoord) const {
        return static_cast<int>(std::floor(worldCoord / static_cast<float>(gridSize)));
    }

    /// Get world position for a tile coordinate (center of tile)
    float tileToWorld(int tileCoord) const {
        return static_cast<float>(tileCoord * gridSize);
    }
};

/// System that processes grid movement for all entities with GridMovement + Transform.
/// This replaces the continuous physics movement for grid-based entities.
class GridMovementSystem : public System {
public:
    GridMovementSystem() : System("GridMovementSystem", -10) {}  // Run before physics

    void init(Registry& registry, Engine& engine) override {
        System::init(registry, engine);
    }

    void update(float dt) override {
        getRegistry().each<Transform, GridMovement>(
            [dt](Entity /*entity*/, Transform& transform, GridMovement& grid) {
                if (grid.isMoving) {
                    // Advance movement progress
                    float moveDuration = 1.0f / grid.moveSpeed;
                    grid.moveProgress += dt / moveDuration;

                    if (grid.moveProgress >= 1.0f) {
                        // Arrived at target
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
    static bool requestMove(Transform& transform, GridMovement& grid, FacingDirection direction) {
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
    static bool tryMove(Transform& transform, GridMovement& grid, FacingDirection direction) {
        // Calculate target tile
        Vec2 currentSnapped = grid.snapToGrid(transform.position);
        float gs = static_cast<float>(grid.gridSize);

        Vec2 target = currentSnapped;
        switch (direction) {
            case FacingDirection::Up:    target.y -= gs; break;
            case FacingDirection::Down:  target.y += gs; break;
            case FacingDirection::Left:  target.x -= gs; break;
            case FacingDirection::Right: target.x += gs; break;
        }

        // Check walkability
        int targetTileX = grid.worldToTile(target.x);
        int targetTileY = grid.worldToTile(target.y);

        if (grid.isWalkable && !grid.isWalkable(targetTileX, targetTileY)) {
            return false;  // Blocked
        }

        // Start the move
        grid.isMoving = true;
        grid.startPos = currentSnapped;
        grid.targetPos = target;
        grid.moveProgress = 0.0f;
        grid.facing = direction;

        // Snap current position to grid start (prevents drift)
        transform.position = currentSnapped;
        return true;
    }
};

} // namespace gloaming
