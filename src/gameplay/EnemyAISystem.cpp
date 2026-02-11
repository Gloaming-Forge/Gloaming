#include "gameplay/EnemyAISystem.hpp"
#include "gameplay/EnemySpawnSystem.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "world/TileMap.hpp"
#include "mod/EventBus.hpp"

#include <cmath>

namespace gloaming {

void EnemyAISystem::init(Registry& registry, Engine& engine) {
    System::init(registry, engine);
    m_tileMap = &engine.getTileMap();
    m_eventBus = &engine.getEventBus();
    m_enemySpawnSystem = engine.getEnemySpawnSystem();
    m_viewMode = engine.getGameModeConfig().viewMode;
}

void EnemyAISystem::shutdown() {
    m_customBehaviors.clear();
}

void EnemyAISystem::registerBehavior(const std::string& name, CustomAIBehavior behavior) {
    m_customBehaviors[name] = std::move(behavior);
}

bool EnemyAISystem::hasBehavior(const std::string& name) const {
    return m_customBehaviors.find(name) != m_customBehaviors.end();
}

void EnemyAISystem::update(float dt) {
    auto& registry = getRegistry();
    m_viewMode = getEngine().getGameModeConfig().viewMode;

    // Collect entities to despawn (can't destroy during iteration)
    std::vector<Entity> toDespawn;

    registry.each<EnemyAI, Transform>([&](Entity entity, EnemyAI& ai, Transform& transform) {
        // Skip "custom" behavior — those use the FSM system directly
        if (ai.behavior == "custom") return;

        // 1. Despawn check
        if (checkDespawn(entity, transform, ai, dt)) {
            toDespawn.push_back(entity);
            return;
        }

        // 2. Contact damage (with cooldown)
        checkContactDamage(entity, transform, ai);

        // 3. Target acquisition (periodic)
        ai.targetCheckTimer -= dt;
        if (ai.targetCheckTimer <= 0.0f) {
            ai.targetCheckTimer = ai.targetCheckInterval;
            ai.target = findNearestPlayer(transform.position, ai.detectionRange);
        }

        // 4. Health-based behavior transitions
        if (registry.has<Health>(entity)) {
            auto& health = registry.get<Health>(entity);
            if (!health.isDead() && health.getPercentage() < ai.fleeHealthThreshold &&
                ai.behavior != AIBehavior::Flee) {
                ai.behavior = AIBehavior::Flee;
            }
        }

        // 5. Update attack timer
        if (ai.attackTimer > 0.0f) {
            ai.attackTimer -= dt;
        }

        // 6. Execute current behavior
        // Check custom behaviors first
        auto customIt = m_customBehaviors.find(ai.behavior);
        if (customIt != m_customBehaviors.end()) {
            customIt->second(entity, ai, dt);
            return;
        }

        // Built-in behaviors
        if (ai.behavior == AIBehavior::Idle)          behaviorIdle(entity, ai, dt);
        else if (ai.behavior == AIBehavior::PatrolWalk)  behaviorPatrolWalk(entity, ai, dt);
        else if (ai.behavior == AIBehavior::PatrolFly)   behaviorPatrolFly(entity, ai, dt);
        else if (ai.behavior == AIBehavior::PatrolPath)  behaviorPatrolPath(entity, ai, dt);
        else if (ai.behavior == AIBehavior::Chase)       behaviorChase(entity, ai, dt);
        else if (ai.behavior == AIBehavior::Flee)        behaviorFlee(entity, ai, dt);
        else if (ai.behavior == AIBehavior::Guard)       behaviorGuard(entity, ai, dt);
        else if (ai.behavior == AIBehavior::Orbit)       behaviorOrbit(entity, ai, dt);
        else if (ai.behavior == AIBehavior::StrafeRun)   behaviorStrafeRun(entity, ai, dt);
    });

    // Destroy despawned entities
    for (Entity entity : toDespawn) {
        if (registry.valid(entity)) {
            if (m_eventBus) {
                EventData data;
                data.setInt("entity", static_cast<int>(entity));
                data.setString("reason", "despawn");
                m_eventBus->emit("enemy_removed", data);
            }
            if (m_enemySpawnSystem) {
                m_enemySpawnSystem->incrementDespawned();
            }
            registry.destroy(entity);
        }
    }
}

Entity EnemyAISystem::findNearestPlayer(const Vec2& position, float maxRange) {
    auto& registry = getRegistry();
    Entity nearest = NullEntity;
    float nearestDistSq = maxRange * maxRange;

    registry.each<PlayerTag, Transform>([&](Entity player, const PlayerTag&,
                                              const Transform& playerTransform) {
        float dx = playerTransform.position.x - position.x;
        float dy = playerTransform.position.y - position.y;
        float distSq = dx * dx + dy * dy;
        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearest = player;
        }
    });

    return nearest;
}

void EnemyAISystem::checkContactDamage(Entity enemy, const Transform& enemyTransform,
                                        EnemyAI& ai) {
    if (ai.contactDamage <= 0) return;
    if (ai.attackTimer > 0.0f) return; // Cooldown not elapsed

    auto& registry = getRegistry();
    if (!registry.has<Collider>(enemy)) return;
    auto& enemyCollider = registry.get<Collider>(enemy);

    Rect enemyBounds = enemyCollider.getBounds(enemyTransform);

    registry.each<PlayerTag, Transform, Health, Collider>([&](
            Entity player, const PlayerTag&, const Transform& playerTransform,
            Health& playerHealth, const Collider& playerCollider) {

        if (playerHealth.isDead() || playerHealth.isInvincible()) return;

        Rect playerBounds = playerCollider.getBounds(playerTransform);

        // Simple AABB overlap
        bool overlapping =
            enemyBounds.x < playerBounds.x + playerBounds.width &&
            enemyBounds.x + enemyBounds.width > playerBounds.x &&
            enemyBounds.y < playerBounds.y + playerBounds.height &&
            enemyBounds.y + enemyBounds.height > playerBounds.y;

        if (overlapping) {
            float dealt = playerHealth.takeDamage(static_cast<float>(ai.contactDamage));
            if (dealt > 0.0f) {
                // Start cooldown after dealing damage
                ai.attackTimer = ai.attackCooldown;

                // Apply knockback away from enemy
                if (registry.has<Velocity>(player)) {
                    auto& vel = registry.get<Velocity>(player);
                    float dx = playerTransform.position.x - enemyTransform.position.x;
                    float dy = playerTransform.position.y - enemyTransform.position.y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist > 0.01f) {
                        vel.linear.x += (dx / dist) * 200.0f;
                        vel.linear.y += (dy / dist) * 150.0f;
                    }
                }

                if (m_eventBus) {
                    EventData data;
                    data.setInt("enemy", static_cast<int>(enemy));
                    data.setInt("player", static_cast<int>(player));
                    data.setFloat("damage", dealt);
                    m_eventBus->emit("enemy_contact_damage", data);
                }
            }
        }
    });
}

bool EnemyAISystem::checkDespawn(Entity enemy, const Transform& transform,
                                  EnemyAI& ai, float dt) {
    if (ai.despawnDistance <= 0.0f) return false;

    auto& registry = getRegistry();

    // Track whether any player was found
    bool foundPlayer = false;
    float closestDistSq = ai.despawnDistance * ai.despawnDistance * 4.0f;

    registry.each<PlayerTag, Transform>([&](Entity, const PlayerTag&,
                                              const Transform& playerTransform) {
        foundPlayer = true;
        float dx = playerTransform.position.x - transform.position.x;
        float dy = playerTransform.position.y - transform.position.y;
        float distSq = dx * dx + dy * dy;
        if (distSq < closestDistSq) closestDistSq = distSq;
    });

    // If no players exist, despawn immediately
    if (!foundPlayer) return true;

    float despawnDistSq = ai.despawnDistance * ai.despawnDistance;
    if (closestDistSq > despawnDistSq) {
        ai.despawnTimer += dt;
        if (ai.despawnTimer >= ai.despawnDelay) {
            return true;
        }
    } else {
        ai.despawnTimer = 0.0f;
    }

    return false;
}

// =============================================================================
// Built-in behavior implementations
// =============================================================================

void EnemyAISystem::behaviorIdle(Entity /*entity*/, EnemyAI& /*ai*/, float /*dt*/) {
    // Do nothing — enemies with "idle" just stand there
    // Useful for turrets or enemies waiting for triggers
}

void EnemyAISystem::behaviorPatrolWalk(Entity entity, EnemyAI& ai, float dt) {
    auto& registry = getRegistry();
    if (!registry.has<Velocity>(entity)) return;
    auto& velocity = registry.get<Velocity>(entity);
    auto& transform = registry.get<Transform>(entity);

    // If a player is detected, switch to chase
    if (ai.target != NullEntity && registry.valid(ai.target)) {
        ai.behavior = AIBehavior::Chase;
        return;
    }

    // Walk horizontally, reversing at patrol bounds or walls
    float targetX = ai.homePosition.x + ai.patrolRadius * static_cast<float>(ai.patrolDirection);
    float dx = targetX - transform.position.x;

    if (std::abs(dx) < 8.0f) {
        // Reached patrol endpoint, reverse
        ai.patrolDirection = -ai.patrolDirection;
    }

    velocity.linear.x = ai.moveSpeed * static_cast<float>(ai.patrolDirection);

    // Check for wall ahead using tile map
    if (m_tileMap) {
        float tileSize = static_cast<float>(m_tileMap->getTileSize());
        int tileX = static_cast<int>(std::floor(
            (transform.position.x + static_cast<float>(ai.patrolDirection) * tileSize) / tileSize));
        int tileY = static_cast<int>(std::floor(transform.position.y / tileSize));
        Tile ahead = m_tileMap->getTile(tileX, tileY);
        if (ahead.isSolid()) {
            ai.patrolDirection = -ai.patrolDirection;
            velocity.linear.x = ai.moveSpeed * static_cast<float>(ai.patrolDirection);
        }

        // Check for ledge (no ground ahead) — only in side-view
        if (m_viewMode == ViewMode::SideView) {
            Tile ground = m_tileMap->getTile(tileX, tileY + 1);
            if (!ground.isSolid()) {
                ai.patrolDirection = -ai.patrolDirection;
                velocity.linear.x = ai.moveSpeed * static_cast<float>(ai.patrolDirection);
            }
        }
    }
}

void EnemyAISystem::behaviorPatrolFly(Entity entity, EnemyAI& ai, float dt) {
    auto& registry = getRegistry();
    if (!registry.has<Velocity>(entity)) return;
    auto& velocity = registry.get<Velocity>(entity);
    auto& transform = registry.get<Transform>(entity);

    // If a player is detected, switch to chase
    if (ai.target != NullEntity && registry.valid(ai.target)) {
        ai.behavior = AIBehavior::Chase;
        return;
    }

    // Fly in a sine-wave pattern around home position
    ai.patrolTimer += dt;
    float xTarget = ai.homePosition.x + std::cos(ai.patrolTimer * 0.5f) * ai.patrolRadius;
    float yTarget = ai.homePosition.y + std::sin(ai.patrolTimer * 1.0f) * ai.patrolRadius * 0.5f;

    float dx = xTarget - transform.position.x;
    float dy = yTarget - transform.position.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist > 1.0f) {
        velocity.linear.x = (dx / dist) * ai.moveSpeed;
        velocity.linear.y = (dy / dist) * ai.moveSpeed;
    } else {
        velocity.linear = Vec2(0.0f, 0.0f);
    }
}

void EnemyAISystem::behaviorPatrolPath(Entity entity, EnemyAI& ai, float dt) {
    auto& registry = getRegistry();
    if (!registry.has<Velocity>(entity)) return;
    auto& velocity = registry.get<Velocity>(entity);
    auto& transform = registry.get<Transform>(entity);

    // If a player is detected, switch to chase
    if (ai.target != NullEntity && registry.valid(ai.target)) {
        ai.behavior = AIBehavior::Chase;
        return;
    }

    // For top-down games: wander around home using simple direction changes
    ai.patrolTimer -= dt;
    if (ai.patrolTimer <= 0.0f) {
        // Pick a new random direction using engine RNG
        std::uniform_real_distribution<float> timerDist(2.0f, 5.0f);
        ai.patrolTimer = timerDist(m_rng);
        ai.patrolDirection = (ai.patrolDirection + 1) % 4; // Cycle 0-3

        // Random chance to idle for a moment
        std::uniform_int_distribution<int> idleDist(0, 2);
        if (idleDist(m_rng) == 0) {
            velocity.linear = Vec2(0.0f, 0.0f);
            return;
        }
    }

    // Move in current direction (4-directional for top-down)
    switch (ai.patrolDirection % 4) {
        case 0: velocity.linear = Vec2(ai.moveSpeed, 0.0f); break;
        case 1: velocity.linear = Vec2(0.0f, ai.moveSpeed); break;
        case 2: velocity.linear = Vec2(-ai.moveSpeed, 0.0f); break;
        case 3: velocity.linear = Vec2(0.0f, -ai.moveSpeed); break;
    }

    // Enforce patrol radius from home
    float dx = transform.position.x - ai.homePosition.x;
    float dy = transform.position.y - ai.homePosition.y;
    float distSq = dx * dx + dy * dy;
    if (distSq > ai.patrolRadius * ai.patrolRadius) {
        // Move back toward home
        float dist = std::sqrt(distSq);
        velocity.linear.x = -(dx / dist) * ai.moveSpeed;
        velocity.linear.y = -(dy / dist) * ai.moveSpeed;
    }
}

void EnemyAISystem::behaviorChase(Entity entity, EnemyAI& ai, float dt) {
    auto& registry = getRegistry();
    if (!registry.has<Velocity>(entity)) return;
    auto& velocity = registry.get<Velocity>(entity);
    auto& transform = registry.get<Transform>(entity);

    // If target is invalid or out of range, return to default
    if (ai.target == NullEntity || !registry.valid(ai.target) ||
        !registry.has<Transform>(ai.target)) {
        ai.behavior = ai.defaultBehavior;
        ai.target = NullEntity;
        velocity.linear = Vec2(0.0f, 0.0f);
        return;
    }

    auto& targetPos = registry.get<Transform>(ai.target).position;
    float dx = targetPos.x - transform.position.x;
    float dy = targetPos.y - transform.position.y;
    float distSq = dx * dx + dy * dy;
    float dist = std::sqrt(distSq);

    // If target is too far, give up
    float giveUpRange = ai.detectionRange * 1.5f;
    if (dist > giveUpRange) {
        ai.behavior = ai.defaultBehavior;
        ai.target = NullEntity;
        velocity.linear = Vec2(0.0f, 0.0f);
        return;
    }

    // Move toward target
    if (dist > ai.attackRange) {
        float chaseSpeed = ai.moveSpeed * 1.2f; // Chase slightly faster than patrol
        if (m_viewMode == ViewMode::SideView) {
            // Side-view: only horizontal chase (vertical handled by gravity/jumping)
            velocity.linear.x = (dx > 0.0f ? 1.0f : -1.0f) * chaseSpeed;
        } else {
            // Top-down / flight: full 2D chase
            velocity.linear.x = (dx / dist) * chaseSpeed;
            velocity.linear.y = (dy / dist) * chaseSpeed;
        }
    } else {
        // In attack range: slow down and deal damage via contact
        velocity.linear.x *= 0.5f;
        if (m_viewMode != ViewMode::SideView) {
            velocity.linear.y *= 0.5f;
        }
    }
}

void EnemyAISystem::behaviorFlee(Entity entity, EnemyAI& ai, float dt) {
    auto& registry = getRegistry();
    if (!registry.has<Velocity>(entity)) return;
    auto& velocity = registry.get<Velocity>(entity);
    auto& transform = registry.get<Transform>(entity);

    // If health recovered or no target, return to default
    if (registry.has<Health>(entity)) {
        auto& health = registry.get<Health>(entity);
        if (health.getPercentage() > ai.fleeHealthThreshold * 1.5f) {
            ai.behavior = ai.defaultBehavior;
            return;
        }
    }

    if (ai.target == NullEntity || !registry.valid(ai.target) ||
        !registry.has<Transform>(ai.target)) {
        ai.behavior = ai.defaultBehavior;
        return;
    }

    auto& targetPos = registry.get<Transform>(ai.target).position;
    float dx = transform.position.x - targetPos.x;
    float dy = transform.position.y - targetPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist > 0.01f) {
        float fleeSpeed = ai.moveSpeed * 1.5f;
        if (m_viewMode == ViewMode::SideView) {
            velocity.linear.x = (dx > 0.0f ? 1.0f : -1.0f) * fleeSpeed;
        } else {
            velocity.linear.x = (dx / dist) * fleeSpeed;
            velocity.linear.y = (dy / dist) * fleeSpeed;
        }
    }
}

void EnemyAISystem::behaviorGuard(Entity entity, EnemyAI& ai, float dt) {
    auto& registry = getRegistry();
    if (!registry.has<Velocity>(entity)) return;
    auto& velocity = registry.get<Velocity>(entity);
    auto& transform = registry.get<Transform>(entity);

    // If target is in detection range, chase it
    if (ai.target != NullEntity && registry.valid(ai.target) &&
        registry.has<Transform>(ai.target)) {
        auto& targetPos = registry.get<Transform>(ai.target).position;
        float dx = targetPos.x - transform.position.x;
        float dy = targetPos.y - transform.position.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < ai.detectionRange) {
            // Chase, but don't go too far from home
            float homeDistX = transform.position.x - ai.homePosition.x;
            float homeDistY = transform.position.y - ai.homePosition.y;
            float homeDist = std::sqrt(homeDistX * homeDistX + homeDistY * homeDistY);

            if (homeDist < ai.patrolRadius * 2.0f) {
                // Chase within leash range
                if (dist > ai.attackRange) {
                    if (m_viewMode == ViewMode::SideView) {
                        velocity.linear.x = (dx > 0.0f ? 1.0f : -1.0f) * ai.moveSpeed;
                    } else {
                        velocity.linear.x = (dx / dist) * ai.moveSpeed;
                        velocity.linear.y = (dy / dist) * ai.moveSpeed;
                    }
                }
                return;
            }
        }
    }

    // Return to home position
    float dx = ai.homePosition.x - transform.position.x;
    float dy = ai.homePosition.y - transform.position.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist > 8.0f) {
        if (m_viewMode == ViewMode::SideView) {
            velocity.linear.x = (dx > 0.0f ? 1.0f : -1.0f) * ai.moveSpeed * 0.5f;
        } else {
            velocity.linear.x = (dx / dist) * ai.moveSpeed * 0.5f;
            velocity.linear.y = (dy / dist) * ai.moveSpeed * 0.5f;
        }
    } else {
        velocity.linear = Vec2(0.0f, 0.0f);
    }
}

void EnemyAISystem::behaviorOrbit(Entity entity, EnemyAI& ai, float dt) {
    auto& registry = getRegistry();
    if (!registry.has<Velocity>(entity)) return;
    auto& velocity = registry.get<Velocity>(entity);
    auto& transform = registry.get<Transform>(entity);

    Vec2 center = ai.homePosition;

    // If target exists, orbit around target instead of home
    if (ai.target != NullEntity && registry.valid(ai.target) &&
        registry.has<Transform>(ai.target)) {
        center = registry.get<Transform>(ai.target).position;
    }

    // Update orbit angle
    ai.orbitAngle += ai.orbitSpeed * dt;
    if (ai.orbitAngle > 2.0f * PI) ai.orbitAngle -= 2.0f * PI;

    // Calculate desired position on orbit circle
    float desiredX = center.x + std::cos(ai.orbitAngle) * ai.orbitDistance;
    float desiredY = center.y + std::sin(ai.orbitAngle) * ai.orbitDistance;

    // Smoothly move toward desired position
    float dx = desiredX - transform.position.x;
    float dy = desiredY - transform.position.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist > 1.0f) {
        float speed = std::min(ai.moveSpeed * 2.0f, dist * 5.0f);
        velocity.linear.x = (dx / dist) * speed;
        velocity.linear.y = (dy / dist) * speed;
    }
}

void EnemyAISystem::behaviorStrafeRun(Entity entity, EnemyAI& ai, float dt) {
    auto& registry = getRegistry();
    if (!registry.has<Velocity>(entity)) return;
    auto& velocity = registry.get<Velocity>(entity);
    auto& transform = registry.get<Transform>(entity);

    if (ai.target == NullEntity || !registry.valid(ai.target) ||
        !registry.has<Transform>(ai.target)) {
        // No target — fly back toward home
        float dx = ai.homePosition.x - transform.position.x;
        float dy = ai.homePosition.y - transform.position.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > 1.0f) {
            velocity.linear.x = (dx / dist) * ai.moveSpeed;
            velocity.linear.y = (dy / dist) * ai.moveSpeed;
        }
        return;
    }

    auto& targetPos = registry.get<Transform>(ai.target).position;
    float dx = targetPos.x - transform.position.x;
    float dy = targetPos.y - transform.position.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    // Strafe run: approach, then retreat after getting close
    if (ai.patrolDirection > 0) {
        // Approach phase
        if (dist > ai.attackRange) {
            velocity.linear.x = (dx / dist) * ai.moveSpeed * 1.5f;
            velocity.linear.y = (dy / dist) * ai.moveSpeed * 1.5f;
        } else {
            // Close enough — switch to retreat
            ai.patrolDirection = -1;
            ai.patrolTimer = 3.0f; // Retreat for 3 seconds
        }
    } else {
        // Retreat phase
        ai.patrolTimer -= dt;
        if (ai.patrolTimer <= 0.0f) {
            ai.patrolDirection = 1; // Start approach again
        } else {
            // Move away
            if (dist > 0.01f) {
                velocity.linear.x = -(dx / dist) * ai.moveSpeed;
                velocity.linear.y = -(dy / dist) * ai.moveSpeed;
            }
        }
    }
}

} // namespace gloaming
