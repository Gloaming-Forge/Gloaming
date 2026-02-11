#pragma once

#include "ecs/Systems.hpp"
#include "ecs/Components.hpp"
#include "gameplay/GameMode.hpp"

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace gloaming {

// Forward declarations
class TileMap;
class EventBus;
class Pathfinder;
class DialogueSystem;
class ContentRegistry;

// ============================================================================
// NPC Behavior constants
// ============================================================================

namespace NPCBehavior {
    constexpr const char* Idle       = "idle";
    constexpr const char* Wander     = "wander";
    constexpr const char* Schedule   = "schedule";
    constexpr const char* Stationed  = "stationed";
    constexpr const char* Custom     = "custom";
}

// ============================================================================
// NPCAI — behavior and interaction component for NPC entities
// ============================================================================

struct NPCAI {
    std::string behavior = "idle";
    std::string defaultBehavior = "idle";

    float moveSpeed = 40.0f;
    Vec2 homePosition{0.0f, 0.0f};
    float wanderRadius = 80.0f;
    float interactionRange = 48.0f;

    // Schedule entries — time-of-day behaviors
    struct ScheduleEntry {
        int hour = 0;                     // 0-23
        std::string behavior;
        Vec2 targetPosition{0.0f, 0.0f};
    };
    std::vector<ScheduleEntry> schedule;

    // Wander state
    float wanderTimer = 0.0f;
    float wanderPauseTimer = 0.0f;
    int wanderDirection = 0;             // -1 = left, 0 = still, 1 = right
    int wanderDirectionY = 0;            // -1 = up, 0 = still, 1 = down (top-down only)

    // Interaction state
    bool playerInRange = false;
    Entity interactingPlayer = NullEntity;

    NPCAI() = default;
    explicit NPCAI(const std::string& behav)
        : behavior(behav), defaultBehavior(behav) {}
};

// ============================================================================
// NPCDialogue — stores dialogue tree reference for conversation
// ============================================================================

struct NPCDialogue {
    std::string dialogueId;              // Content-registry qualified ID
    std::string greetingNodeId;          // Starting node
    bool hasBeenTalkedTo = false;
    std::string currentMood = "neutral";
};

// ============================================================================
// ShopKeeper — marks an NPC as a shop vendor
// ============================================================================

struct ShopKeeper {
    std::string shopId;                  // References ShopDefinition in ContentRegistry
    bool shopOpen = false;
};

// ============================================================================
// NPCSystem — processes NPC behaviors, interaction detection, dialogue
// ============================================================================

/// Callback type for custom NPC behaviors registered from Lua
using NPCBehaviorCallback = std::function<void(Entity, NPCAI&, float)>;

class NPCSystem : public System {
public:
    NPCSystem() : System("NPCSystem", 13) {}

    void init(Registry& registry, Engine& engine) override;
    void update(float dt) override;
    void shutdown() override;

    /// Register a custom behavior callable from Lua mods
    void registerBehavior(const std::string& name, NPCBehaviorCallback callback);

    /// Check if a custom behavior is registered
    bool hasBehavior(const std::string& name) const;

    /// Spawn an NPC from a ContentRegistry definition
    Entity spawnNPC(const std::string& npcId, float x, float y);

    /// Trigger dialogue between an NPC and a player
    bool startDialogue(Entity npc, Entity player);

    /// Get NPC count
    int getActiveNPCCount() const;

private:
    void checkPlayerInteraction(Entity npc, const Transform& transform, NPCAI& ai);

    // Built-in behaviors
    void behaviorIdle(Entity npc, NPCAI& ai, float dt);
    void behaviorWander(Entity npc, NPCAI& ai, float dt);
    void behaviorStationed(Entity npc, NPCAI& ai, float dt);

    Entity findNearestPlayer(const Vec2& position, float maxRange) const;

    TileMap* m_tileMap = nullptr;
    EventBus* m_eventBus = nullptr;
    Pathfinder* m_pathfinder = nullptr;
    DialogueSystem* m_dialogueSystem = nullptr;
    ContentRegistry* m_contentRegistry = nullptr;
    ViewMode m_viewMode = ViewMode::SideView;

    std::unordered_map<std::string, NPCBehaviorCallback> m_customBehaviors;
};

} // namespace gloaming
