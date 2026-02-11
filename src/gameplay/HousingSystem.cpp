#include "gameplay/HousingSystem.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "mod/ContentRegistry.hpp"
#include "mod/EventBus.hpp"
#include "world/TileMap.hpp"

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

    // Re-validate existing rooms
    for (auto& room : m_rooms) {
        // Pick center tile of room to re-validate
        int cx = (room.topLeft.x + room.bottomRight.x) / 2;
        int cy = (room.topLeft.y + room.bottomRight.y) / 2;
        ValidatedRoom recheck = validateRoom(cx, cy);
        room.hasDoor = recheck.hasDoor;
        room.hasLight = recheck.hasLight;
        room.hasFurniture = recheck.hasFurniture;
        room.isValid = recheck.isValid;
    }
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
    bool enclosed = true;

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

    // Verify enclosure: all border cells of the bounding box must be solid or visited
    // (A simpler check: if we didn't exceed maxTiles, the room is bounded.)
    // The flood fill already handles this — if it didn't overflow, the room is enclosed.

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
                    if (existing.topLeft.x == room.topLeft.x &&
                        existing.topLeft.y == room.topLeft.y &&
                        existing.bottomRight.x == room.bottomRight.x &&
                        existing.bottomRight.y == room.bottomRight.y) {
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
