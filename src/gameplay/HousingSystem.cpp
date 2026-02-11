#include "gameplay/HousingSystem.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "mod/ContentRegistry.hpp"
#include "mod/EventBus.hpp"
#include "world/TileMap.hpp"

#include <algorithm>
#include <queue>
#include <unordered_set>

namespace gloaming {

void HousingSystem::init(Registry& registry, Engine& engine) {
    System::init(registry, engine);
    m_tileMap = &engine.getTileMap();
    m_eventBus = &engine.getEventBus();
    m_contentRegistry = &engine.getContentRegistry();

    LOG_INFO("HousingSystem initialized");
}

void HousingSystem::update(float dt) {
    // Periodically re-validate cached rooms
    m_timeSinceValidation += dt;
    if (m_timeSinceValidation < m_validationInterval) return;
    m_timeSinceValidation = 0.0f;

    if (!m_tileMap || !m_tileMap->isWorldLoaded()) return;

    // Re-validate existing rooms and track consecutive failures
    for (auto& room : m_rooms) {
        int cx = (room.topLeft.x + room.bottomRight.x) / 2;
        int cy = (room.topLeft.y + room.bottomRight.y) / 2;
        ValidatedRoom recheck = validateRoom(cx, cy);
        room.hasDoor = recheck.hasDoor;
        room.hasLight = recheck.hasLight;
        room.hasFurniture = recheck.hasFurniture;
        room.isValid = recheck.isValid;

        if (room.isValid) {
            room.consecutiveInvalidChecks = 0;
        } else {
            ++room.consecutiveInvalidChecks;
        }
    }

    // Evict rooms that have been invalid for 3+ consecutive checks (grace period
    // prevents eviction from transient changes like a player briefly mining a wall)
    static constexpr int EvictionThreshold = 3;
    m_rooms.erase(
        std::remove_if(m_rooms.begin(), m_rooms.end(),
            [this](const ValidatedRoom& room) {
                if (room.consecutiveInvalidChecks >= EvictionThreshold) {
                    if (m_eventBus && room.assignedNPC != NullEntity) {
                        EventData data;
                        data.setInt("room_id", room.id);
                        data.setInt("npc_entity", static_cast<int>(room.assignedNPC));
                        m_eventBus->emit("housing_room_invalidated", data);
                    }
                    return true;
                }
                return false;
            }),
        m_rooms.end());
}

void HousingSystem::shutdown() {
    m_rooms.clear();
    LOG_INFO("HousingSystem shut down");
}

ValidatedRoom HousingSystem::validateRoom(int tileX, int tileY) {
    ValidatedRoom result;

    if (!m_tileMap || !m_tileMap->isWorldLoaded()) return result;

    // Seed tile must be non-solid (interior)
    if (m_tileMap->isSolid(tileX, tileY)) return result;

    // BFS flood-fill from seed tile through non-solid tiles
    std::queue<TilePos> frontier;
    std::unordered_set<TilePos, TilePosHash> visited;
    frontier.push({tileX, tileY});
    visited.insert({tileX, tileY});

    int maxTiles = m_requirements.maxWidth * m_requirements.maxHeight;
    int minX = tileX, maxX = tileX;
    int minY = tileY, maxY = tileY;

    bool hasDoor = false;
    bool hasLight = false;
    bool hasFurniture = false;

    while (!frontier.empty()) {
        if (static_cast<int>(visited.size()) > maxTiles) {
            // Area too large — not an enclosed room
            return result;
        }

        TilePos pos = frontier.front();
        frontier.pop();

        // Track bounds
        if (pos.x < minX) minX = pos.x;
        if (pos.x > maxX) maxX = pos.x;
        if (pos.y < minY) minY = pos.y;
        if (pos.y > maxY) maxY = pos.y;

        // Check this tile for special properties
        if (isTileDoor(pos.x, pos.y)) hasDoor = true;
        if (isTileLight(pos.x, pos.y)) hasLight = true;
        if (isTileFurniture(pos.x, pos.y)) hasFurniture = true;

        // Expand to 4-connected neighbors
        const TilePos neighbors[] = {
            {pos.x - 1, pos.y}, {pos.x + 1, pos.y},
            {pos.x, pos.y - 1}, {pos.x, pos.y + 1}
        };

        for (const auto& next : neighbors) {
            if (visited.count(next)) continue;
            visited.insert(next);

            if (!m_tileMap->isSolid(next.x, next.y)) {
                frontier.push(next);
            }
            // If the neighbor is solid, it's part of the wall — that's fine.
            // We also check border tiles for door/light/furniture.
            if (m_tileMap->isSolid(next.x, next.y)) {
                if (isTileDoor(next.x, next.y)) hasDoor = true;
                if (isTileLight(next.x, next.y)) hasLight = true;
                if (isTileFurniture(next.x, next.y)) hasFurniture = true;
            }
        }
    }

    int width = maxX - minX + 1;
    int height = maxY - minY + 1;

    // Check minimum size
    if (width < m_requirements.minWidth || height < m_requirements.minHeight) {
        return result;
    }

    // Verify enclosure: every non-solid interior tile on the bounding-box perimeter
    // must have a solid neighbor just outside the bounding box in the outward direction.
    // This catches gaps where the BFS terminated within maxTiles but walls are incomplete.
    for (const auto& pos : visited) {
        if (m_tileMap->isSolid(pos.x, pos.y)) continue;
        if (pos.x == minX && !m_tileMap->isSolid(pos.x - 1, pos.y)) return result;
        if (pos.x == maxX && !m_tileMap->isSolid(pos.x + 1, pos.y)) return result;
        if (pos.y == minY && !m_tileMap->isSolid(pos.x, pos.y - 1)) return result;
        if (pos.y == maxY && !m_tileMap->isSolid(pos.x, pos.y + 1)) return result;
    }

    result.topLeft = {minX, minY};
    result.bottomRight = {maxX, maxY};
    result.tileCount = static_cast<int>(visited.size());
    result.hasDoor = hasDoor;
    result.hasLight = hasLight;
    result.hasFurniture = hasFurniture;

    // Check all requirements
    bool valid = true;
    if (m_requirements.requireDoor && !hasDoor) valid = false;
    if (m_requirements.requireLightSource && !hasLight) valid = false;
    if (m_requirements.requireFurniture && !hasFurniture) valid = false;

    result.isValid = valid;
    return result;
}

std::vector<ValidatedRoom> HousingSystem::scanForRooms(float centerX, float centerY,
                                                         float radius) {
    std::vector<ValidatedRoom> found;
    if (!m_tileMap || !m_tileMap->isWorldLoaded()) return found;

    int tileSize = m_tileMap->getTileSize();
    if (tileSize <= 0) tileSize = 16;

    int startTX = static_cast<int>((centerX - radius) / tileSize);
    int endTX = static_cast<int>((centerX + radius) / tileSize);
    int startTY = static_cast<int>((centerY - radius) / tileSize);
    int endTY = static_cast<int>((centerY + radius) / tileSize);

    // Track which tiles we've already scanned to avoid duplicates
    std::unordered_set<TilePos, TilePosHash> scanned;

    // Sample every few tiles (skip interval for performance)
    int step = std::max(m_requirements.minWidth / 2, 2);
    for (int ty = startTY; ty <= endTY; ty += step) {
        for (int tx = startTX; tx <= endTX; tx += step) {
            if (m_tileMap->isSolid(tx, ty)) continue;

            TilePos pos{tx, ty};
            if (scanned.count(pos)) continue;
            scanned.insert(pos);

            ValidatedRoom room = validateRoom(tx, ty);
            if (room.isValid) {
                room.id = m_nextRoomId++;

                // Mark all tiles in this room as scanned
                for (int ry = room.topLeft.y; ry <= room.bottomRight.y; ++ry) {
                    for (int rx = room.topLeft.x; rx <= room.bottomRight.x; ++rx) {
                        scanned.insert({rx, ry});
                    }
                }

                // Check if this room overlaps with an existing cached room
                bool duplicate = false;
                for (const auto& existing : m_rooms) {
                    if (room.topLeft.x <= existing.bottomRight.x &&
                        room.bottomRight.x >= existing.topLeft.x &&
                        room.topLeft.y <= existing.bottomRight.y &&
                        room.bottomRight.y >= existing.topLeft.y) {
                        duplicate = true;
                        break;
                    }
                }

                if (!duplicate) {
                    m_rooms.push_back(room);
                    found.push_back(room);

                    if (m_eventBus) {
                        EventData data;
                        data.setInt("room_id", room.id);
                        data.setInt("x", room.topLeft.x);
                        data.setInt("y", room.topLeft.y);
                        data.setInt("width", room.bottomRight.x - room.topLeft.x + 1);
                        data.setInt("height", room.bottomRight.y - room.topLeft.y + 1);
                        m_eventBus->emit("housing_room_found", data);
                    }
                }
            }
        }
    }

    return found;
}

size_t HousingSystem::getValidRoomCount() const {
    size_t count = 0;
    for (const auto& room : m_rooms) {
        if (room.isValid) ++count;
    }
    return count;
}

bool HousingSystem::assignNPCToRoom(Entity npc, int roomId) {
    for (auto& room : m_rooms) {
        if (room.id == roomId && room.isValid) {
            // Unassign any existing NPC from this room
            room.assignedNPC = npc;

            if (m_eventBus) {
                EventData data;
                data.setInt("room_id", roomId);
                data.setInt("npc_entity", static_cast<int>(npc));
                m_eventBus->emit("housing_npc_assigned", data);
            }

            return true;
        }
    }
    return false;
}

std::vector<const ValidatedRoom*> HousingSystem::getAvailableRooms() const {
    std::vector<const ValidatedRoom*> available;
    for (const auto& room : m_rooms) {
        if (room.isValid && room.assignedNPC == NullEntity) {
            available.push_back(&room);
        }
    }
    return available;
}

const ValidatedRoom* HousingSystem::getRoomForNPC(Entity npc) const {
    for (const auto& room : m_rooms) {
        if (room.assignedNPC == npc) return &room;
    }
    return nullptr;
}

bool HousingSystem::isTileDoor(int tileX, int tileY) const {
    if (!m_contentRegistry || !m_tileMap) return false;

    Tile tile = m_tileMap->getTile(tileX, tileY);
    if (tile.id == 0) return false;

    // Check explicit door tile list
    if (!m_requirements.doorTiles.empty()) {
        const TileContentDef* def = m_contentRegistry->getTileByRuntime(tile.id);
        if (def) {
            for (const auto& doorId : m_requirements.doorTiles) {
                if (def->qualifiedId == doorId) return true;
            }
        }
        return false;
    }

    // Heuristic: platform tiles act as doors (can pass through)
    const TileContentDef* def = m_contentRegistry->getTileByRuntime(tile.id);
    return def && def->isPlatform;
}

bool HousingSystem::isTileLight(int tileX, int tileY) const {
    if (!m_contentRegistry || !m_tileMap) return false;

    Tile tile = m_tileMap->getTile(tileX, tileY);
    if (tile.id == 0) return false;

    // Check explicit light tile list
    if (!m_requirements.lightTiles.empty()) {
        const TileContentDef* def = m_contentRegistry->getTileByRuntime(tile.id);
        if (def) {
            for (const auto& lightId : m_requirements.lightTiles) {
                if (def->qualifiedId == lightId) return true;
            }
        }
        return false;
    }

    // Heuristic: tiles that emit light
    const TileContentDef* def = m_contentRegistry->getTileByRuntime(tile.id);
    return def && def->emitsLight;
}

bool HousingSystem::isTileFurniture(int tileX, int tileY) const {
    if (!m_contentRegistry || !m_tileMap) return false;

    Tile tile = m_tileMap->getTile(tileX, tileY);
    if (tile.id == 0) return false;

    // Check explicit furniture tile list
    if (!m_requirements.furnitureTiles.empty()) {
        const TileContentDef* def = m_contentRegistry->getTileByRuntime(tile.id);
        if (def) {
            for (const auto& furnId : m_requirements.furnitureTiles) {
                if (def->qualifiedId == furnId) return true;
            }
        }
        return false;
    }

    // Heuristic: non-solid, non-transparent, non-platform tiles in the world
    // (e.g., tables, chairs placed as tiles)
    const TileContentDef* def = m_contentRegistry->getTileByRuntime(tile.id);
    return def && !def->solid && !def->transparent && !def->isPlatform;
}

} // namespace gloaming
