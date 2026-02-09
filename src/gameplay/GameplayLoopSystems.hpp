#pragma once

#include "ecs/Systems.hpp"
#include "ecs/Components.hpp"
#include "gameplay/GameplayLoop.hpp"
#include "mod/EventBus.hpp"

#include <functional>

namespace gloaming {

// Forward declarations
class TileMap;
class ContentRegistry;

// ============================================================================
// ItemDropSystem — handles magnet pull, pickup, and despawn of dropped items
// ============================================================================

class ItemDropSystem : public System {
public:
    ItemDropSystem() : System("ItemDropSystem", 0) {}

    void init(Registry& registry, Engine& engine) override;
    void update(float dt) override;

private:
    TileMap* m_tileMap = nullptr;
    EventBus* m_eventBus = nullptr;
};

// ============================================================================
// ToolUseSystem — handles tile mining/chopping progress
// ============================================================================

class ToolUseSystem : public System {
public:
    ToolUseSystem() : System("ToolUseSystem", 0) {}

    void init(Registry& registry, Engine& engine) override;
    void update(float dt) override;

private:
    TileMap* m_tileMap = nullptr;
    ContentRegistry* m_contentRegistry = nullptr;
    EventBus* m_eventBus = nullptr;
};

// ============================================================================
// MeleeAttackSystem — handles melee swing updates and hit detection
// ============================================================================

class MeleeAttackSystem : public System {
public:
    MeleeAttackSystem() : System("MeleeAttackSystem", 0) {}

    void init(Registry& registry, Engine& engine) override;
    void update(float dt) override;

private:
    EventBus* m_eventBus = nullptr;
};

// ============================================================================
// CombatSystem — handles health updates, death detection, and respawn
// ============================================================================

class CombatSystem : public System {
public:
    CombatSystem() : System("CombatSystem", 0) {}

    void init(Registry& registry, Engine& engine) override;
    void update(float dt) override;

private:
    EventBus* m_eventBus = nullptr;
};

} // namespace gloaming
