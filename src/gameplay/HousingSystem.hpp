#pragma once

#include "ecs/Systems.hpp"
#include "ecs/Components.hpp"
#include "gameplay/Pathfinding.hpp"

#include <string>
#include <vector>

namespace gloaming {

// Forward declarations
class TileMap;
class EventBus;
class ContentRegistry;

// ============================================================================
// HousingRequirements — what makes a valid NPC room
// ============================================================================

struct HousingRequirements {
    int minWidth = 6;                    // Minimum interior width (tiles)
    int minHeight = 4;                   // Minimum interior height (tiles)
    int maxWidth = 50;                   // Maximum room scan size
    int maxHeight = 50;
    bool requireDoor = true;
    bool requireLightSource = true;
    bool requireFurniture = true;

    // Tile IDs that satisfy each requirement.
    // If empty, the system uses heuristics (emitsLight, isPlatform, etc.)
    std::vector<std::string> doorTiles;
    std::vector<std::string> lightTiles;
    std::vector<std::string> furnitureTiles;
};

// ============================================================================
// ValidatedRoom — a cached room that has been verified for NPC housing
// ============================================================================

struct ValidatedRoom {
    int id = 0;
    TilePos topLeft{0, 0};
    TilePos bottomRight{0, 0};
    int tileCount = 0;                   // Number of interior tiles
    bool hasDoor = false;
    bool hasLight = false;
    bool hasFurniture = false;
    bool isValid = false;
    Entity assignedNPC = NullEntity;
    float lastValidationTime = 0.0f;
};

// ============================================================================
// HousingSystem — validates rooms and manages NPC housing assignments
// ============================================================================

class HousingSystem : public System {
public:
    HousingSystem() : System("HousingSystem", 10) {}

    void init(Registry& registry, Engine& engine) override;
    void update(float dt) override;
    void shutdown() override;

    /// Validate a room at a given tile position (flood-fill from interior tile).
    /// Returns validated room info; check .isValid for success.
    ValidatedRoom validateRoom(int tileX, int tileY);

    /// Scan an area for potential rooms around a world position.
    /// Returns all valid rooms found.
    std::vector<ValidatedRoom> scanForRooms(float centerX, float centerY, float radius);

    /// Get all currently validated rooms
    const std::vector<ValidatedRoom>& getValidRooms() const { return m_rooms; }

    /// Get count of valid rooms
    size_t getValidRoomCount() const;

    /// Assign an NPC to a validated room (by room ID)
    bool assignNPCToRoom(Entity npc, int roomId);

    /// Get rooms with no assigned NPC
    std::vector<const ValidatedRoom*> getAvailableRooms() const;

    /// Get the room assigned to an NPC (nullptr if none)
    const ValidatedRoom* getRoomForNPC(Entity npc) const;

    /// Set housing requirements (can be configured from Lua)
    void setRequirements(const HousingRequirements& reqs) { m_requirements = reqs; }
    const HousingRequirements& getRequirements() const { return m_requirements; }

private:
    bool isTileDoor(int tileX, int tileY) const;
    bool isTileLight(int tileX, int tileY) const;
    bool isTileFurniture(int tileX, int tileY) const;

    TileMap* m_tileMap = nullptr;
    EventBus* m_eventBus = nullptr;
    ContentRegistry* m_contentRegistry = nullptr;

    HousingRequirements m_requirements;
    std::vector<ValidatedRoom> m_rooms;
    int m_nextRoomId = 1;

    float m_timeSinceValidation = 0.0f;
    float m_validationInterval = 5.0f;   // Re-validate cached rooms every 5 seconds
};

} // namespace gloaming
