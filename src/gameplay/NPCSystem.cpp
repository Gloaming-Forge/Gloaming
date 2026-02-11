#include "gameplay/NPCSystem.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "gameplay/DialogueSystem.hpp"
#include "gameplay/Pathfinding.hpp"
#include "gameplay/CollisionLayers.hpp"
#include "mod/ContentRegistry.hpp"
#include "mod/EventBus.hpp"
#include "world/TileMap.hpp"

#include <cmath>

namespace gloaming {

void NPCSystem::init(Registry& registry, Engine& engine) {
    System::init(registry, engine);
    m_tileMap = &engine.getTileMap();
    m_eventBus = &engine.getEventBus();
    m_pathfinder = &engine.getPathfinder();
    m_dialogueSystem = &engine.getDialogueSystem();
    m_contentRegistry = &engine.getContentRegistry();
    m_viewMode = engine.getGameModeConfig().viewMode;

    LOG_INFO("NPCSystem initialized");
}

void NPCSystem::update(float dt) {
    auto& registry = getRegistry();

    registry.each<NPCAI, Transform>([&](Entity entity, NPCAI& ai, Transform& transform) {
        // Skip FSM-driven NPCs
        if (ai.behavior == NPCBehavior::Custom) return;

        // Check player proximity for interaction
        checkPlayerInteraction(entity, transform, ai);

        // Execute custom behavior if registered
        auto customIt = m_customBehaviors.find(ai.behavior);
        if (customIt != m_customBehaviors.end()) {
            customIt->second(entity, ai, dt);
            return;
        }

        // Execute built-in behavior
        if (ai.behavior == NPCBehavior::Idle) {
            behaviorIdle(entity, ai, dt);
        } else if (ai.behavior == NPCBehavior::Wander) {
            behaviorWander(entity, ai, dt);
        } else if (ai.behavior == NPCBehavior::Stationed) {
            behaviorStationed(entity, ai, dt);
        }
        // "schedule" is handled by Lua via custom behavior or FSM
    });
}

void NPCSystem::shutdown() {
    m_customBehaviors.clear();
    LOG_INFO("NPCSystem shut down");
}

void NPCSystem::registerBehavior(const std::string& name, NPCBehaviorCallback callback) {
    m_customBehaviors[name] = std::move(callback);
    LOG_DEBUG("NPCSystem: registered custom behavior '{}'", name);
}

bool NPCSystem::hasBehavior(const std::string& name) const {
    return m_customBehaviors.count(name) > 0;
}

Entity NPCSystem::spawnNPC(const std::string& npcId, float x, float y) {
    if (!m_contentRegistry) return NullEntity;

    const NPCDefinition* def = m_contentRegistry->getNPC(npcId);
    if (!def) {
        LOG_WARN("NPCSystem: unknown NPC type '{}'", npcId);
        return NullEntity;
    }

    auto& registry = getRegistry();

    // Create entity with core components
    Entity npc = registry.create(
        Transform{Vec2(x, y)},
        Name{def->name, def->qualifiedId},
        NPCTag{def->qualifiedId},
        Velocity{}
    );

    // Add collider on NPC layer
    Collider col;
    col.size = Vec2(def->colliderWidth, def->colliderHeight);
    col.layer = CollisionLayer::NPC;
    col.mask = CollisionLayer::Player | CollisionLayer::Tile;
    registry.add<Collider>(npc, std::move(col));

    // Add NPC AI component
    NPCAI ai(def->aiBehavior);
    ai.moveSpeed = def->moveSpeed;
    ai.wanderRadius = def->wanderRadius;
    ai.interactionRange = def->interactionRange;
    ai.homePosition = Vec2(x, y);
    registry.add<NPCAI>(npc, std::move(ai));

    // Add dialogue if referenced
    if (!def->dialogueId.empty()) {
        NPCDialogue dlg;
        dlg.dialogueId = def->dialogueId;
        // Look up the greeting node from the dialogue tree
        const DialogueTreeDef* tree = m_contentRegistry->getDialogueTree(def->dialogueId);
        if (tree) {
            dlg.greetingNodeId = tree->greetingNodeId;
        }
        registry.add<NPCDialogue>(npc, std::move(dlg));
    }

    // Add shop keeper component if referenced
    if (!def->shopId.empty()) {
        ShopKeeper sk;
        sk.shopId = def->shopId;
        registry.add<ShopKeeper>(npc, std::move(sk));
    }

    // Emit event
    if (m_eventBus) {
        EventData data;
        data.setString("npc_id", npcId);
        data.setInt("entity", static_cast<int>(npc));
        data.setFloat("x", x);
        data.setFloat("y", y);
        m_eventBus->emit("npc_spawned", data);
    }

    LOG_DEBUG("NPCSystem: spawned '{}' at ({}, {})", npcId, x, y);
    return npc;
}

bool NPCSystem::startDialogue(Entity npc, Entity player) {
    if (!m_dialogueSystem || !m_contentRegistry) return false;

    auto& registry = getRegistry();
    if (!registry.valid(npc) || !registry.has<NPCDialogue>(npc)) return false;

    auto& dlg = registry.get<NPCDialogue>(npc);
    if (dlg.dialogueId.empty()) return false;

    const DialogueTreeDef* tree = m_contentRegistry->getDialogueTree(dlg.dialogueId);
    if (!tree || tree->nodes.empty()) return false;

    // Convert content dialogue nodes to DialogueSystem nodes
    std::vector<DialogueNode> nodes;
    for (const auto& nodeDef : tree->nodes) {
        DialogueNode node;
        node.id = nodeDef.id;
        node.speaker = nodeDef.speaker;
        node.text = nodeDef.text;
        node.portraitId = nodeDef.portraitId;
        node.nextNodeId = nodeDef.nextNodeId;

        for (const auto& choiceDef : nodeDef.choices) {
            DialogueChoice choice;
            choice.text = choiceDef.text;
            choice.nextNodeId = choiceDef.nextNodeId;
            node.choices.push_back(std::move(choice));
        }

        nodes.push_back(std::move(node));
    }

    // Start the dialogue
    std::string startId = dlg.greetingNodeId.empty() ? "" : dlg.greetingNodeId;
    if (startId.empty() && !nodes.empty()) {
        startId = nodes[0].id;
    }
    m_dialogueSystem->startDialogue(nodes, startId);

    dlg.hasBeenTalkedTo = true;

    // Emit event
    if (m_eventBus) {
        EventData data;
        data.setInt("npc_entity", static_cast<int>(npc));
        data.setInt("player_entity", static_cast<int>(player));
        data.setString("dialogue_id", dlg.dialogueId);
        m_eventBus->emit("npc_dialogue_started", data);
    }

    return true;
}

int NPCSystem::getActiveNPCCount() const {
    int count = 0;
    auto& registry = const_cast<Registry&>(getRegistry());
    registry.each<NPCTag>([&count](Entity, const NPCTag&) { ++count; });
    return count;
}

void NPCSystem::checkPlayerInteraction(Entity npc, const Transform& npcTransform, NPCAI& ai) {
    auto& registry = getRegistry();
    ai.playerInRange = false;
    ai.interactingPlayer = NullEntity;

    registry.each<PlayerTag, Transform>([&](Entity player, const PlayerTag&,
                                             const Transform& pt) {
        float dx = pt.position.x - npcTransform.position.x;
        float dy = pt.position.y - npcTransform.position.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= ai.interactionRange) {
            ai.playerInRange = true;
            ai.interactingPlayer = player;
        }
    });
}

void NPCSystem::behaviorIdle(Entity /*npc*/, NPCAI& /*ai*/, float /*dt*/) {
    // Do nothing — NPC stands still at current position
}

void NPCSystem::behaviorWander(Entity npc, NPCAI& ai, float dt) {
    auto& registry = getRegistry();
    if (!registry.has<Velocity>(npc)) return;

    auto& transform = registry.get<Transform>(npc);
    auto& velocity = registry.get<Velocity>(npc);

    // Pause between wander movements
    if (ai.wanderPauseTimer > 0.0f) {
        ai.wanderPauseTimer -= dt;
        velocity.linear = Vec2(0.0f, 0.0f);
        return;
    }

    // Update wander timer
    ai.wanderTimer -= dt;
    if (ai.wanderTimer <= 0.0f) {
        // Pick new direction or pause
        if (ai.wanderDirection != 0) {
            // Was moving, now pause
            ai.wanderDirection = 0;
            ai.wanderPauseTimer = 1.0f + static_cast<float>(std::rand() % 30) / 10.0f;
            velocity.linear = Vec2(0.0f, 0.0f);
        } else {
            // Was paused, now move
            ai.wanderDirection = (std::rand() % 2 == 0) ? -1 : 1;
            ai.wanderTimer = 1.0f + static_cast<float>(std::rand() % 30) / 10.0f;
        }
        return;
    }

    // Check if we're outside wander radius
    float dx = transform.position.x - ai.homePosition.x;
    if (std::abs(dx) > ai.wanderRadius) {
        // Turn back toward home
        ai.wanderDirection = (dx > 0) ? -1 : 1;
    }

    if (m_viewMode == ViewMode::TopDown) {
        // Top-down: wander in both axes
        float dy = transform.position.y - ai.homePosition.y;
        if (std::abs(dy) > ai.wanderRadius) {
            // For simplicity, only handle X-axis wander for now
        }
        velocity.linear.x = ai.wanderDirection * ai.moveSpeed;
    } else {
        // Side-view: horizontal wander only
        velocity.linear.x = ai.wanderDirection * ai.moveSpeed;
    }
}

void NPCSystem::behaviorStationed(Entity npc, NPCAI& ai, float /*dt*/) {
    // Stay at home position
    auto& registry = getRegistry();
    if (!registry.has<Velocity>(npc)) return;
    auto& velocity = registry.get<Velocity>(npc);
    velocity.linear = Vec2(0.0f, 0.0f);
}

Entity NPCSystem::findNearestPlayer(const Vec2& position, float maxRange) const {
    Entity nearest = NullEntity;
    float nearestDist = maxRange * maxRange;

    auto& registry = const_cast<Registry&>(getRegistry());
    registry.each<PlayerTag, Transform>([&](Entity player, const PlayerTag&,
                                             const Transform& pt) {
        float dx = pt.position.x - position.x;
        float dy = pt.position.y - position.y;
        float distSq = dx * dx + dy * dy;
        if (distSq < nearestDist) {
            nearestDist = distSq;
            nearest = player;
        }
    });

    return nearest;
}

} // namespace gloaming
