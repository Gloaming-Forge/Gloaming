#include "gameplay/EnemyLuaBindings.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "gameplay/EnemySpawnSystem.hpp"
#include "gameplay/EnemyAISystem.hpp"

namespace gloaming {

void bindEnemyAPI(sol::state& lua, Engine& engine,
                  EnemySpawnSystem& spawnSystem,
                  EnemyAISystem& aiSystem) {

    // =========================================================================
    // enemy_spawns API — control enemy spawning rules and behavior
    // =========================================================================
    auto spawnApi = lua.create_named_table("enemy_spawns");

    // enemy_spawns.add_rule({ enemy_id = "base:slime", weight = 1.0, ... })
    spawnApi["add_rule"] = [&spawnSystem](sol::table opts) {
        SpawnRule rule;
        rule.enemyId = opts.get_or<std::string>("enemy_id", "");
        if (rule.enemyId.empty()) {
            MOD_LOG_WARN("enemy_spawns.add_rule: missing enemy_id");
            return;
        }

        rule.weight = opts.get_or("weight", 1.0f);
        rule.maxAlive = opts.get_or("max_alive", 10);
        rule.depthMin = opts.get_or("depth_min", -1e6f);
        rule.depthMax = opts.get_or("depth_max", 1e6f);
        rule.lightLevelMax = opts.get_or("light_max", 1.0f);
        rule.nightOnly = opts.get_or("night_only", false);
        rule.dayOnly = opts.get_or("day_only", false);

        // Parse biomes table
        sol::optional<sol::table> biomes = opts.get<sol::optional<sol::table>>("biomes");
        if (biomes) {
            size_t len = biomes->size();
            for (size_t i = 1; i <= len; ++i) {
                sol::optional<std::string> name = biomes->get<sol::optional<std::string>>(i);
                if (name) rule.biomes.push_back(*name);
            }
        }

        spawnSystem.addSpawnRule(rule);
    };

    // enemy_spawns.clear_rules()
    spawnApi["clear_rules"] = [&spawnSystem]() {
        spawnSystem.clearSpawnRules();
    };

    // enemy_spawns.spawn_at(enemy_id, x, y) -> entityId
    spawnApi["spawn_at"] = [&spawnSystem](const std::string& enemyId,
                                           float x, float y) -> uint32_t {
        Entity e = spawnSystem.spawnEnemy(enemyId, x, y);
        return static_cast<uint32_t>(e);
    };

    // enemy_spawns.set_enabled(enabled)
    spawnApi["set_enabled"] = [&spawnSystem](bool enabled) {
        spawnSystem.getConfig().enabled = enabled;
    };

    // enemy_spawns.set_max(maxEnemies)
    spawnApi["set_max"] = [&spawnSystem](int max) {
        spawnSystem.getConfig().maxEnemies = max;
    };

    // enemy_spawns.set_interval(seconds)
    spawnApi["set_interval"] = [&spawnSystem](float interval) {
        spawnSystem.getConfig().spawnCheckInterval = interval;
    };

    // enemy_spawns.set_range(min, max)
    spawnApi["set_range"] = [&spawnSystem](float minDist, float maxDist) {
        spawnSystem.getConfig().spawnRangeMin = minDist;
        spawnSystem.getConfig().spawnRangeMax = maxDist;
    };

    // enemy_spawns.get_count() -> int
    spawnApi["get_count"] = [&spawnSystem]() -> int {
        return spawnSystem.getActiveEnemyCount();
    };

    // enemy_spawns.get_count_type(enemy_id) -> int
    spawnApi["get_count_type"] = [&spawnSystem](const std::string& type) -> int {
        return spawnSystem.getEnemyCountByType(type);
    };

    // enemy_spawns.get_stats() -> table
    spawnApi["get_stats"] = [&spawnSystem, &engine]() -> sol::table {
        auto& stats = spawnSystem.getStats();
        sol::state_view luaView = engine.getModLoader().getLuaBindings().getState();
        sol::table t = luaView.create_table();
        t["active"] = stats.activeEnemies;
        t["total_spawned"] = stats.totalSpawned;
        t["total_despawned"] = stats.totalDespawned;
        t["total_killed"] = stats.totalKilled;
        t["time_since_spawn"] = stats.timeSinceLastSpawn;
        return t;
    };

    // =========================================================================
    // enemy_ai API — configure AI behaviors on individual enemies
    // =========================================================================
    auto aiApi = lua.create_named_table("enemy_ai");

    // enemy_ai.set_behavior(entityId, behaviorName)
    aiApi["set_behavior"] = [&engine](uint32_t entityId, const std::string& behavior) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<EnemyAI>(entity)) {
            MOD_LOG_WARN("enemy_ai.set_behavior: entity {} has no EnemyAI", entityId);
            return;
        }
        registry.get<EnemyAI>(entity).behavior = behavior;
    };

    // enemy_ai.get_behavior(entityId) -> string
    aiApi["get_behavior"] = [&engine](uint32_t entityId) -> std::string {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<EnemyAI>(entity)) return "";
        return registry.get<EnemyAI>(entity).behavior;
    };

    // enemy_ai.set_detection(entityId, range)
    aiApi["set_detection"] = [&engine](uint32_t entityId, float range) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<EnemyAI>(entity)) return;
        registry.get<EnemyAI>(entity).detectionRange = range;
    };

    // enemy_ai.set_attack(entityId, { range = 32, cooldown = 1.0, damage = 10 })
    aiApi["set_attack"] = [&engine](uint32_t entityId, sol::table opts) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<EnemyAI>(entity)) return;
        auto& ai = registry.get<EnemyAI>(entity);

        ai.attackRange = opts.get_or("range", ai.attackRange);
        ai.attackCooldown = opts.get_or("cooldown", ai.attackCooldown);
        ai.contactDamage = opts.get_or("damage", ai.contactDamage);
    };

    // enemy_ai.set_patrol(entityId, { radius = 100, speed = 60 })
    aiApi["set_patrol"] = [&engine](uint32_t entityId, sol::table opts) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<EnemyAI>(entity)) return;
        auto& ai = registry.get<EnemyAI>(entity);

        ai.patrolRadius = opts.get_or("radius", ai.patrolRadius);
        ai.moveSpeed = opts.get_or("speed", ai.moveSpeed);
    };

    // enemy_ai.set_flee(entityId, healthThreshold)
    aiApi["set_flee"] = [&engine](uint32_t entityId, float threshold) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<EnemyAI>(entity)) return;
        registry.get<EnemyAI>(entity).fleeHealthThreshold = threshold;
    };

    // enemy_ai.set_despawn(entityId, { distance = 1500, delay = 5.0 })
    aiApi["set_despawn"] = [&engine](uint32_t entityId, sol::table opts) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<EnemyAI>(entity)) return;
        auto& ai = registry.get<EnemyAI>(entity);

        ai.despawnDistance = opts.get_or("distance", ai.despawnDistance);
        ai.despawnDelay = opts.get_or("delay", ai.despawnDelay);
    };

    // enemy_ai.set_orbit(entityId, { distance = 100, speed = 2.0 })
    aiApi["set_orbit"] = [&engine](uint32_t entityId, sol::table opts) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<EnemyAI>(entity)) return;
        auto& ai = registry.get<EnemyAI>(entity);

        ai.orbitDistance = opts.get_or("distance", ai.orbitDistance);
        ai.orbitSpeed = opts.get_or("speed", ai.orbitSpeed);
    };

    // enemy_ai.set_target(entityId, targetEntityId)
    aiApi["set_target"] = [&engine](uint32_t entityId, uint32_t targetId) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<EnemyAI>(entity)) return;
        registry.get<EnemyAI>(entity).target = static_cast<Entity>(targetId);
    };

    // enemy_ai.get_target(entityId) -> targetEntityId (0 if none)
    aiApi["get_target"] = [&engine](uint32_t entityId) -> uint32_t {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<EnemyAI>(entity)) return 0;
        return static_cast<uint32_t>(registry.get<EnemyAI>(entity).target);
    };

    // enemy_ai.get_home(entityId) -> x, y
    aiApi["get_home"] = [&engine](uint32_t entityId) -> std::tuple<float, float> {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<EnemyAI>(entity)) return {0.0f, 0.0f};
        auto& home = registry.get<EnemyAI>(entity).homePosition;
        return {home.x, home.y};
    };

    // enemy_ai.set_home(entityId, x, y)
    aiApi["set_home"] = [&engine](uint32_t entityId, float x, float y) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<EnemyAI>(entity)) return;
        registry.get<EnemyAI>(entity).homePosition = Vec2(x, y);
    };

    // enemy_ai.register_behavior(name, callback)
    // callback = function(entityId, dt) ... end
    aiApi["register_behavior"] = [&aiSystem](const std::string& name, sol::function callback) {
        sol::function fn = callback;
        aiSystem.registerBehavior(name,
            [fn](Entity entity, EnemyAI& ai, float dt) {
                try {
                    fn(static_cast<uint32_t>(entity), dt);
                } catch (const std::exception& ex) {
                    MOD_LOG_ERROR("enemy_ai behavior '{}' error: {}", ai.behavior, ex.what());
                }
            });
    };

    // enemy_ai.add(entityId [, opts])
    // Adds an EnemyAI component to an entity (for manual setup from Lua)
    aiApi["add"] = [&engine](uint32_t entityId, sol::optional<sol::table> opts) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) {
            MOD_LOG_WARN("enemy_ai.add: invalid entity {}", entityId);
            return;
        }

        if (registry.has<EnemyAI>(entity)) return; // Already has AI

        EnemyAI ai;
        if (opts) {
            ai.behavior = opts->get_or<std::string>("behavior", "idle");
            ai.defaultBehavior = opts->get_or<std::string>("default_behavior", ai.behavior);
            ai.detectionRange = opts->get_or("detection_range", 200.0f);
            ai.attackRange = opts->get_or("attack_range", 32.0f);
            ai.moveSpeed = opts->get_or("move_speed", 60.0f);
            ai.contactDamage = opts->get_or("contact_damage", 10);
            ai.patrolRadius = opts->get_or("patrol_radius", 100.0f);
            ai.despawnDistance = opts->get_or("despawn_distance", 1500.0f);
            ai.fleeHealthThreshold = opts->get_or("flee_threshold", 0.2f);
            ai.orbitDistance = opts->get_or("orbit_distance", 100.0f);
            ai.orbitSpeed = opts->get_or("orbit_speed", 2.0f);
        }

        // Set home to current position
        if (registry.has<Transform>(entity)) {
            ai.homePosition = registry.get<Transform>(entity).position;
        }

        registry.add<EnemyAI>(entity, std::move(ai));
    };

    // enemy_ai.remove(entityId)
    aiApi["remove"] = [&engine](uint32_t entityId) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (registry.valid(entity) && registry.has<EnemyAI>(entity)) {
            registry.remove<EnemyAI>(entity);
        }
    };
}

} // namespace gloaming
