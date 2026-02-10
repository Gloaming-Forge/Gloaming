#include "gameplay/GameplayLoopSystems.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "world/TileMap.hpp"
#include "mod/ContentRegistry.hpp"

#include <cmath>

namespace gloaming {

// ============================================================================
// ItemDropSystem
// ============================================================================

void ItemDropSystem::init(Registry& registry, Engine& engine) {
    System::init(registry, engine);
    m_tileMap = &engine.getTileMap();
    m_contentRegistry = &engine.getContentRegistry();
    m_eventBus = &engine.getEventBus();
}

void ItemDropSystem::update(float dt) {
    auto& registry = getRegistry();

    // Collect player entities (entities with Inventory + PlayerTag + Transform)
    struct PlayerInfo {
        Entity entity;
        Vec2 position;
    };
    std::vector<PlayerInfo> players;
    registry.each<PlayerTag, Transform, Inventory>([&](Entity e, const PlayerTag&,
                                                         const Transform& t,
                                                         const Inventory&) {
        players.push_back({e, t.position});
    });

    // Collect drops to destroy after iteration
    std::vector<Entity> toDestroy;

    registry.each<ItemDrop, Transform>([&](Entity dropEntity, ItemDrop& drop,
                                            Transform& dropTransform) {
        drop.age += dt;

        // Despawn expired items
        if (drop.isExpired()) {
            toDestroy.push_back(dropEntity);
            return;
        }

        // Skip if not yet pickupable
        if (!drop.canPickup()) return;

        // Find nearest player
        for (const auto& player : players) {
            float dx = player.position.x - dropTransform.position.x;
            float dy = player.position.y - dropTransform.position.y;
            float distSq = dx * dx + dy * dy;

            // Pickup range check
            if (distSq <= drop.pickupRadius * drop.pickupRadius) {
                // Try to add to player inventory, respecting item maxStack
                auto& inventory = registry.get<Inventory>(player.entity);
                int maxStack = 999;
                if (m_contentRegistry) {
                    const ItemDefinition* itemDef = m_contentRegistry->getItem(drop.itemId);
                    if (itemDef) maxStack = itemDef->maxStack;
                }
                int leftover = inventory.addItem(drop.itemId, drop.count, maxStack);

                if (leftover < drop.count) {
                    // At least some items were picked up
                    EventData data;
                    data.setString("item", drop.itemId);
                    data.setInt("count", drop.count - leftover);
                    data.setInt("player", static_cast<int>(player.entity));
                    m_eventBus->emit("item_pickup", data);

                    if (leftover <= 0) {
                        toDestroy.push_back(dropEntity);
                    } else {
                        drop.count = leftover;
                    }
                }
                return;
            }

            // Magnet pull
            if (drop.magnetic && distSq <= drop.magnetRadius * drop.magnetRadius &&
                distSq > 0.01f) {
                float dist = std::sqrt(distSq);
                float pullSpeed = drop.magnetSpeed * dt;
                float moveAmount = std::min(pullSpeed, dist);
                dropTransform.position.x += (dx / dist) * moveAmount;
                dropTransform.position.y += (dy / dist) * moveAmount;
            }
        }
    });

    for (Entity e : toDestroy) {
        registry.destroy(e);
    }
}

// ============================================================================
// ToolUseSystem
// ============================================================================

void ToolUseSystem::init(Registry& registry, Engine& engine) {
    System::init(registry, engine);
    m_tileMap = &engine.getTileMap();
    m_contentRegistry = &engine.getContentRegistry();
    m_eventBus = &engine.getEventBus();
}

void ToolUseSystem::update(float dt) {
    auto& registry = getRegistry();

    registry.each<ToolUse, Transform, Inventory>([&](Entity entity, ToolUse& tool,
                                                       const Transform& transform,
                                                       const Inventory& inventory) {
        if (!tool.active) return;

        // Advance mining progress
        tool.progress += dt;

        if (tool.isComplete()) {
            // Tile is fully broken
            Tile tile = m_tileMap->getTile(tool.targetTileX, tool.targetTileY);

            if (!tile.isEmpty()) {
                // Look up tile definition for drops
                const TileContentDef* tileDef = m_contentRegistry->getTileByRuntime(tile.id);

                // Clear the tile
                m_tileMap->setTile(tool.targetTileX, tool.targetTileY, Tile{});

                // Emit event
                EventData data;
                data.setInt("tile_x", tool.targetTileX);
                data.setInt("tile_y", tool.targetTileY);
                data.setInt("tile_id", tile.id);
                data.setInt("entity", static_cast<int>(entity));
                if (tileDef) {
                    data.setString("tile_type", tileDef->qualifiedId);
                    data.setString("drop_item", tileDef->dropItem);
                    data.setInt("drop_count", tileDef->dropCount);
                }
                m_eventBus->emit("tile_broken", data);
            }

            tool.reset();
        }
    });
}

// ============================================================================
// MeleeAttackSystem
// ============================================================================

void MeleeAttackSystem::init(Registry& registry, Engine& engine) {
    System::init(registry, engine);
    m_eventBus = &engine.getEventBus();
}

void MeleeAttackSystem::update(float dt) {
    auto& registry = getRegistry();

    // Collect active swingers and their data
    struct SwingInfo {
        Entity attacker;
        Vec2 position;
        MeleeAttack* melee;
    };
    std::vector<SwingInfo> swingers;

    registry.each<MeleeAttack, Transform>([&](Entity entity, MeleeAttack& melee,
                                                const Transform& transform) {
        melee.update(dt);

        // Check for hits once per swing using a flag
        if (melee.swinging && !melee.hitChecked) {
            melee.hitChecked = true;
            swingers.push_back({entity, transform.position, &melee});
        }
    });

    // For each active swing, check against damageable entities
    for (auto& swing : swingers) {
        float aimAngle = std::atan2(swing.melee->aimDirection.y,
                                     swing.melee->aimDirection.x) * RAD_TO_DEG;
        float halfArc = swing.melee->arc * 0.5f;
        float rangeSq = swing.melee->range * swing.melee->range;

        registry.each<Health, Transform, Collider>([&](Entity target, Health& health,
                                                         const Transform& targetTransform,
                                                         const Collider& collider) {
            if (target == swing.attacker) return;
            if (health.isDead() || health.isInvincible()) return;

            // Distance check
            float dx = targetTransform.position.x - swing.position.x;
            float dy = targetTransform.position.y - swing.position.y;
            float distSq = dx * dx + dy * dy;

            if (distSq > rangeSq || distSq < 0.001f) return;

            // Arc check: is the target within the swing arc?
            float angleToTarget = std::atan2(dy, dx) * RAD_TO_DEG;
            float angleDiff = angleToTarget - aimAngle;

            // Normalize angle difference to [-180, 180]
            angleDiff = std::fmod(angleDiff + 180.0f, 360.0f);
            if (angleDiff < 0.0f) angleDiff += 360.0f;
            angleDiff -= 180.0f;

            if (std::abs(angleDiff) > halfArc) return;

            // Hit! Apply damage
            float dealt = health.takeDamage(swing.melee->damage);
            if (dealt > 0.0f) {
                // Apply knockback
                if (registry.has<Velocity>(target)) {
                    float dist = std::sqrt(distSq);
                    Vec2 knockDir(dx / dist, dy / dist);
                    auto& vel = registry.get<Velocity>(target);
                    vel.linear.x += knockDir.x * swing.melee->knockback * 60.0f;
                    vel.linear.y += knockDir.y * swing.melee->knockback * 60.0f;
                }

                EventData data;
                data.setInt("attacker", static_cast<int>(swing.attacker));
                data.setInt("target", static_cast<int>(target));
                data.setFloat("damage", dealt);
                m_eventBus->emit("melee_hit", data);
            }
        });
    }
}

// ============================================================================
// CombatSystem
// ============================================================================

void CombatSystem::init(Registry& registry, Engine& engine) {
    System::init(registry, engine);
    m_eventBus = &engine.getEventBus();
}

void CombatSystem::update(float dt) {
    auto& registry = getRegistry();

    // Update invincibility timers on all Health components
    registry.each<Health>([&](Entity entity, Health& health) {
        health.update(dt);
    });

    // Handle player death and respawn
    registry.each<PlayerCombat, Health, Transform>([&](Entity entity,
                                                        PlayerCombat& combat,
                                                        Health& health,
                                                        Transform& transform) {
        // Detect death
        if (health.isDead() && !combat.dead) {
            combat.die();

            EventData data;
            data.setInt("entity", static_cast<int>(entity));
            m_eventBus->emit("player_death", data);
        }

        // Handle respawn timer
        if (combat.dead) {
            if (combat.updateRespawn(dt)) {
                performRespawn(combat, health, transform, registry, entity);

                EventData data;
                data.setInt("entity", static_cast<int>(entity));
                data.setFloat("x", combat.spawnPoint.x);
                data.setFloat("y", combat.spawnPoint.y);
                m_eventBus->emit("player_respawn", data);
            }
        }
    });
}

void performRespawn(PlayerCombat& combat, Health& health,
                    Transform& transform, Registry& registry, Entity entity) {
    combat.dead = false;
    combat.respawnTimer = 0.0f;
    health.current = health.max;
    health.invincibilityTime = 2.0f;
    transform.position = combat.spawnPoint;
    if (registry.has<Velocity>(entity)) {
        registry.get<Velocity>(entity).linear = Vec2(0.0f, 0.0f);
    }
}

} // namespace gloaming
