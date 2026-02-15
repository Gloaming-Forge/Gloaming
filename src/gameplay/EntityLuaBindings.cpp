#include "gameplay/EntityLuaBindings.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "gameplay/EntitySpawning.hpp"
#include "gameplay/ProjectileSystem.hpp"
#include "gameplay/CollisionLayers.hpp"
#include "gameplay/SpriteAnimation.hpp"

#include <cmath>

namespace gloaming {

void bindEntityAPI(sol::state& lua, Engine& engine,
                   EntitySpawning& spawning,
                   ProjectileSystem& projectileSystem,
                   CollisionLayerRegistry& collisionLayers) {

    // =========================================================================
    // entity API — dynamic entity creation, destruction, queries
    // =========================================================================
    auto entityApi = lua.create_named_table("entity");

    // entity.create() -> entityId
    entityApi["create"] = [&engine]() -> uint32_t {
        auto& registry = engine.getRegistry();
        Entity e = registry.create(Transform{}, Name{"entity"});
        return static_cast<uint32_t>(e);
    };

    // entity.spawn(type, x, y) -> entityId (0 if unknown type)
    entityApi["spawn"] = [&spawning](const std::string& type, float x, float y) -> uint32_t {
        Entity e = spawning.spawn(type, x, y);
        return static_cast<uint32_t>(e);
    };

    // entity.destroy(entityId)
    entityApi["destroy"] = [&spawning](uint32_t entityId) {
        spawning.destroy(static_cast<Entity>(entityId));
    };

    // entity.is_valid(entityId) -> bool
    entityApi["is_valid"] = [&spawning](uint32_t entityId) -> bool {
        return spawning.isValid(static_cast<Entity>(entityId));
    };

    // entity.count() -> int
    entityApi["count"] = [&spawning]() -> int {
        return static_cast<int>(spawning.entityCount());
    };

    // entity.count_by_type(type) -> int
    entityApi["count_by_type"] = [&spawning](const std::string& type) -> int {
        return static_cast<int>(spawning.countByType(type));
    };

    // entity.set_position(entityId, x, y)
    entityApi["set_position"] = [&spawning](uint32_t entityId, float x, float y) {
        spawning.setPosition(static_cast<Entity>(entityId), x, y);
    };

    // entity.get_position(entityId) -> x, y (as two return values)
    entityApi["get_position"] = [&engine](uint32_t entityId)
            -> std::tuple<float, float> {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Transform>(entity)) {
            return {0.0f, 0.0f};
        }
        auto& pos = registry.get<Transform>(entity).position;
        return {pos.x, pos.y};
    };

    // entity.set_velocity(entityId, vx, vy)
    entityApi["set_velocity"] = [&spawning](uint32_t entityId, float vx, float vy) {
        spawning.setVelocity(static_cast<Entity>(entityId), vx, vy);
    };

    // entity.get_velocity(entityId) -> vx, vy
    entityApi["get_velocity"] = [&engine](uint32_t entityId)
            -> std::tuple<float, float> {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Velocity>(entity)) {
            return {0.0f, 0.0f};
        }
        auto& vel = registry.get<Velocity>(entity).linear;
        return {vel.x, vel.y};
    };

    // entity.set_sprite(entityId, texturePath)
    entityApi["set_sprite"] = [&engine](uint32_t entityId, const std::string& texturePath) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) {
            MOD_LOG_WARN("entity.set_sprite: invalid entity {}", entityId);
            return;
        }

        const Texture* texture = engine.getTextureManager().loadTexture(texturePath);
        if (!texture) {
            MOD_LOG_WARN("entity.set_sprite: failed to load texture '{}'", texturePath);
            return;
        }

        if (registry.has<Sprite>(entity)) {
            auto& sprite = registry.get<Sprite>(entity);
            sprite.texture = texture;
            sprite.sourceRect = Rect(0, 0,
                static_cast<float>(texture->getWidth()),
                static_cast<float>(texture->getHeight()));
        } else {
            registry.add<Sprite>(entity, texture);
        }
    };

    // entity.set_source_rect(entityId, x, y, w, h)
    // Sets the sprite's source rectangle for atlas-based rendering.
    entityApi["set_source_rect"] = [&engine](uint32_t entityId,
            float x, float y, float w, float h) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) {
            MOD_LOG_WARN("entity.set_source_rect: invalid entity {}", entityId);
            return;
        }
        if (!registry.has<Sprite>(entity)) {
            MOD_LOG_WARN("entity.set_source_rect: entity {} has no sprite", entityId);
            return;
        }
        auto& sprite = registry.get<Sprite>(entity);
        sprite.sourceRect = Rect(x, y, w, h);
    };

    // entity.set_component(entityId, componentName, data)
    entityApi["set_component"] = [&engine, &collisionLayers](
            uint32_t entityId, const std::string& name, sol::table data) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) {
            MOD_LOG_WARN("entity.set_component: invalid entity {}", entityId);
            return;
        }

        if (name == "health") {
            float current = data.get_or("current", 100.0f);
            float max = data.get_or("max", current);
            if (registry.has<Health>(entity)) {
                auto& h = registry.get<Health>(entity);
                h.current = current;
                h.max = max;
            } else {
                registry.add<Health>(entity, current, max);
            }
        } else if (name == "collider") {
            float w = data.get_or("width", 16.0f);
            float h = data.get_or("height", 16.0f);
            float ox = data.get_or("offset_x", 0.0f);
            float oy = data.get_or("offset_y", 0.0f);
            bool trigger = data.get_or("trigger", false);

            Collider col;
            col.offset = Vec2(ox, oy);
            col.size = Vec2(w, h);
            col.isTrigger = trigger;

            // Optional layer/mask from named strings
            sol::optional<std::string> layerName = data.get<sol::optional<std::string>>("layer");
            if (layerName) {
                collisionLayers.setLayer(col, *layerName);
            }
            sol::optional<sol::table> maskTable = data.get<sol::optional<sol::table>>("mask");
            if (maskTable) {
                std::vector<std::string> names;
                size_t len = maskTable->size();
                for (size_t i = 1; i <= len; ++i) {
                    sol::optional<std::string> n = maskTable->get<sol::optional<std::string>>(i);
                    if (n) names.push_back(*n);
                }
                collisionLayers.setMask(col, names);
            }

            registry.addOrReplace<Collider>(entity, col);
        } else if (name == "gravity") {
            float scale = data.get_or("scale", 1.0f);
            registry.addOrReplace<Gravity>(entity, scale);
        } else if (name == "lifetime") {
            float duration = data.get_or("duration", 5.0f);
            registry.addOrReplace<Lifetime>(entity, duration);
        } else if (name == "light") {
            LightSource light;
            light.radius = data.get_or("radius", 100.0f);
            light.intensity = data.get_or("intensity", 1.0f);
            sol::optional<sol::table> colorT = data.get<sol::optional<sol::table>>("color");
            if (colorT) {
                light.color = Color(
                    colorT->get_or("r", 255),
                    colorT->get_or("g", 255),
                    colorT->get_or("b", 255),
                    colorT->get_or("a", 255));
            }
            light.flicker = data.get_or("flicker", false);
            registry.addOrReplace<LightSource>(entity, light);
        } else if (name == "name") {
            std::string n = data.get_or<std::string>("name", "");
            std::string t = data.get_or<std::string>("type", "");
            registry.addOrReplace<Name>(entity, n, t);
        } else if (name == "velocity") {
            float vx = data.get_or("x", 0.0f);
            float vy = data.get_or("y", 0.0f);
            float angular = data.get_or("angular", 0.0f);
            if (registry.has<Velocity>(entity)) {
                auto& v = registry.get<Velocity>(entity);
                v.linear = Vec2(vx, vy);
                v.angular = angular;
            } else {
                registry.add<Velocity>(entity, Vec2(vx, vy), angular);
            }
        } else if (name == "transform") {
            float x = data.get_or("x", 0.0f);
            float y = data.get_or("y", 0.0f);
            float rotation = data.get_or("rotation", 0.0f);
            float scaleX = data.get_or("scale_x", 1.0f);
            float scaleY = data.get_or("scale_y", 1.0f);
            if (registry.has<Transform>(entity)) {
                auto& tr = registry.get<Transform>(entity);
                tr.position = Vec2(x, y);
                tr.rotation = rotation;
                tr.scale = Vec2(scaleX, scaleY);
            } else {
                registry.add<Transform>(entity, Vec2(x, y), rotation, Vec2(scaleX, scaleY));
            }
        } else if (name == "sprite") {
            if (registry.has<Sprite>(entity)) {
                auto& s = registry.get<Sprite>(entity);
                sol::optional<bool> vis = data.get<sol::optional<bool>>("visible");
                if (vis) s.visible = *vis;
                sol::optional<int> layer = data.get<sol::optional<int>>("layer");
                if (layer) s.layer = *layer;
                sol::optional<bool> fx = data.get<sol::optional<bool>>("flip_x");
                if (fx) s.flipX = *fx;
                sol::optional<bool> fy = data.get<sol::optional<bool>>("flip_y");
                if (fy) s.flipY = *fy;
                sol::optional<sol::table> rect = data.get<sol::optional<sol::table>>("source_rect");
                if (rect) {
                    s.sourceRect = Rect(
                        rect->get_or("x", 0.0f),
                        rect->get_or("y", 0.0f),
                        rect->get_or("w", 0.0f),
                        rect->get_or("h", 0.0f));
                }
            } else {
                MOD_LOG_WARN("entity.set_component('sprite'): entity {} has no sprite - call entity.set_sprite() first", entityId);
            }
        } else {
            MOD_LOG_WARN("entity.set_component: unknown component '{}'", name);
        }
    };

    // entity.get_component(entityId, componentName) -> table or nil
    entityApi["get_component"] = [&engine, &lua](
            uint32_t entityId, const std::string& name) -> sol::object {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return sol::nil;

        if (name == "health") {
            if (!registry.has<Health>(entity)) return sol::nil;
            auto& h = registry.get<Health>(entity);
            sol::table t = lua.create_table();
            t["current"] = h.current;
            t["max"] = h.max;
            t["percentage"] = h.getPercentage();
            t["is_dead"] = h.isDead();
            t["is_invincible"] = h.isInvincible();
            return t;
        } else if (name == "transform") {
            if (!registry.has<Transform>(entity)) return sol::nil;
            auto& tr = registry.get<Transform>(entity);
            sol::table t = lua.create_table();
            t["x"] = tr.position.x;
            t["y"] = tr.position.y;
            t["rotation"] = tr.rotation;
            t["scale_x"] = tr.scale.x;
            t["scale_y"] = tr.scale.y;
            return t;
        } else if (name == "velocity") {
            if (!registry.has<Velocity>(entity)) return sol::nil;
            auto& v = registry.get<Velocity>(entity);
            sol::table t = lua.create_table();
            t["x"] = v.linear.x;
            t["y"] = v.linear.y;
            t["angular"] = v.angular;
            return t;
        } else if (name == "collider") {
            if (!registry.has<Collider>(entity)) return sol::nil;
            auto& c = registry.get<Collider>(entity);
            sol::table t = lua.create_table();
            t["width"] = c.size.x;
            t["height"] = c.size.y;
            t["offset_x"] = c.offset.x;
            t["offset_y"] = c.offset.y;
            t["layer"] = c.layer;
            t["mask"] = c.mask;
            t["enabled"] = c.enabled;
            t["is_trigger"] = c.isTrigger;
            return t;
        } else if (name == "gravity") {
            if (!registry.has<Gravity>(entity)) return sol::nil;
            auto& g = registry.get<Gravity>(entity);
            sol::table t = lua.create_table();
            t["scale"] = g.scale;
            t["grounded"] = g.grounded;
            return t;
        } else if (name == "name") {
            if (!registry.has<Name>(entity)) return sol::nil;
            auto& n = registry.get<Name>(entity);
            sol::table t = lua.create_table();
            t["name"] = n.name;
            t["type"] = n.type;
            return t;
        } else if (name == "projectile") {
            if (!registry.has<Projectile>(entity)) return sol::nil;
            auto& p = registry.get<Projectile>(entity);
            sol::table t = lua.create_table();
            t["owner"] = p.ownerEntity;
            t["damage"] = p.damage;
            t["speed"] = p.speed;
            t["lifetime"] = p.lifetime;
            t["age"] = p.age;
            t["pierce"] = p.pierce;
            t["alive"] = p.alive;
            return t;
        } else if (name == "lifetime") {
            if (!registry.has<Lifetime>(entity)) return sol::nil;
            auto& l = registry.get<Lifetime>(entity);
            sol::table t = lua.create_table();
            t["duration"] = l.duration;
            t["elapsed"] = l.elapsed;
            t["remaining"] = l.getRemaining();
            t["progress"] = l.getProgress();
            t["expired"] = l.isExpired();
            return t;
        } else if (name == "light") {
            if (!registry.has<LightSource>(entity)) return sol::nil;
            auto& l = registry.get<LightSource>(entity);
            sol::table t = lua.create_table();
            t["radius"] = l.radius;
            t["intensity"] = l.intensity;
            t["enabled"] = l.enabled;
            t["flicker"] = l.flicker;
            sol::table color = lua.create_table();
            color["r"] = l.color.r;
            color["g"] = l.color.g;
            color["b"] = l.color.b;
            color["a"] = l.color.a;
            t["color"] = color;
            return t;
        } else if (name == "sprite") {
            if (!registry.has<Sprite>(entity)) return sol::nil;
            auto& s = registry.get<Sprite>(entity);
            sol::table t = lua.create_table();
            t["visible"] = s.visible;
            t["layer"] = s.layer;
            t["flip_x"] = s.flipX;
            t["flip_y"] = s.flipY;
            return t;
        }

        return sol::nil;
    };

    // entity.has_component(entityId, componentName) -> bool
    entityApi["has_component"] = [&engine](uint32_t entityId, const std::string& name) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return false;

        if (name == "health")     return registry.has<Health>(entity);
        if (name == "transform")  return registry.has<Transform>(entity);
        if (name == "velocity")   return registry.has<Velocity>(entity);
        if (name == "collider")   return registry.has<Collider>(entity);
        if (name == "gravity")    return registry.has<Gravity>(entity);
        if (name == "sprite")     return registry.has<Sprite>(entity);
        if (name == "light")      return registry.has<LightSource>(entity);
        if (name == "name")       return registry.has<Name>(entity);
        if (name == "projectile") return registry.has<Projectile>(entity);
        if (name == "lifetime")   return registry.has<Lifetime>(entity);
        if (name == "animation")  return registry.has<AnimationController>(entity);
        if (name == "player")     return registry.has<PlayerTag>(entity);
        if (name == "enemy")      return registry.has<EnemyTag>(entity);
        if (name == "npc")        return registry.has<NPCTag>(entity);
        return false;
    };

    // entity.remove_component(entityId, componentName)
    entityApi["remove_component"] = [&engine](uint32_t entityId, const std::string& name) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return;

        if (name == "health")      registry.remove<Health>(entity);
        else if (name == "transform")  registry.remove<Transform>(entity);
        else if (name == "velocity")   registry.remove<Velocity>(entity);
        else if (name == "collider")   registry.remove<Collider>(entity);
        else if (name == "gravity")    registry.remove<Gravity>(entity);
        else if (name == "sprite")     registry.remove<Sprite>(entity);
        else if (name == "light")      registry.remove<LightSource>(entity);
        else if (name == "name")       registry.remove<Name>(entity);
        else if (name == "lifetime")   registry.remove<Lifetime>(entity);
        else if (name == "projectile") registry.remove<Projectile>(entity);
        else if (name == "animation")  registry.remove<AnimationController>(entity);
        else MOD_LOG_WARN("entity.remove_component: unknown component '{}'", name);
    };

    // entity.find_in_radius(x, y, radius [, filter]) -> table of results
    entityApi["find_in_radius"] = [&engine, &spawning, &collisionLayers](
            float x, float y, float radius,
            sol::optional<sol::table> filterOpts) -> sol::table {

        EntityQueryFilter filter;
        if (filterOpts) {
            filter.type = filterOpts->get_or<std::string>("type", "");
            filter.excludeDead = filterOpts->get_or("exclude_dead", true);

            sol::optional<std::string> layerName = filterOpts->get<sol::optional<std::string>>("layer");
            if (layerName) {
                filter.requiredLayer = collisionLayers.getLayerBit(*layerName);
            }
        }

        auto results = spawning.findInRadius(x, y, radius, filter);

        sol::state_view luaView = engine.getModLoader().getLuaBindings().getState();
        sol::table resultTable = luaView.create_table();

        for (size_t i = 0; i < results.size(); ++i) {
            sol::table entry = luaView.create_table();
            entry["entity"] = static_cast<uint32_t>(results[i].entity);
            entry["distance"] = results[i].distance;
            entry["x"] = results[i].position.x;
            entry["y"] = results[i].position.y;
            resultTable[i + 1] = entry;
        }

        return resultTable;
    };

    // =========================================================================
    // projectile API — spawning and configuring projectiles
    // =========================================================================
    auto projectileApi = lua.create_named_table("projectile");

    // projectile.spawn(opts) -> entityId
    //
    // opts = {
    //     owner = playerId,
    //     x = 100, y = 200,
    //     speed = 400,
    //     angle = 0,             -- degrees (0 = right, 90 = down)
    //     damage = 10,
    //     sprite = "textures/arrow.png",
    //     gravity = true,
    //     lifetime = 3.0,
    //     pierce = 0,            -- 0 = destroy on first hit, N = N additional hits after the first, -1 = unlimited (up to 8 unique targets)
    //     max_distance = 0,      -- 0 = unlimited
    //     layer = "projectile",
    //     hits = { "enemy", "tile" },
    //     auto_rotate = true,
    //     collider_width = 8,
    //     collider_height = 8,
    //     on_hit = function(proj, target) end
    // }
    projectileApi["spawn"] = [&engine, &projectileSystem, &collisionLayers](
            sol::table opts) -> uint32_t {
        auto& registry = engine.getRegistry();

        float x = opts.get_or("x", 0.0f);
        float y = opts.get_or("y", 0.0f);
        float speed = opts.get_or("speed", 400.0f);
        float angleDeg = opts.get_or("angle", 0.0f);
        float damage = opts.get_or("damage", 10.0f);
        float lifetime = opts.get_or("lifetime", 5.0f);
        int pierce = opts.get_or("pierce", 0);
        float maxDistance = opts.get_or("max_distance", 0.0f);
        bool gravity = opts.get_or("gravity", false);
        bool autoRotate = opts.get_or("auto_rotate", true);
        uint32_t owner = opts.get_or<uint32_t>("owner", 0);
        float colliderW = opts.get_or("collider_width", 8.0f);
        float colliderH = opts.get_or("collider_height", 8.0f);

        // Convert angle to velocity
        float angleRad = angleDeg * (PI / 180.0f);
        float vx = std::cos(angleRad) * speed;
        float vy = std::sin(angleRad) * speed;

        // Create the entity
        Entity entity = registry.create(
            Transform{Vec2(x, y)},
            Velocity{Vec2(vx, vy)},
            Name{"projectile", "projectile"}
        );

        // Sprite
        sol::optional<std::string> spritePath = opts.get<sol::optional<std::string>>("sprite");
        if (spritePath && !spritePath->empty()) {
            const Texture* tex = engine.getTextureManager().loadTexture(*spritePath);
            if (tex) {
                registry.add<Sprite>(entity, tex);
            }
        }

        // Parse hits table to determine collision behavior
        uint32_t hitMask = 0;
        bool hitsTiles = false;
        sol::optional<sol::table> hitsTable = opts.get<sol::optional<sol::table>>("hits");
        if (hitsTable) {
            size_t len = hitsTable->size();
            for (size_t i = 1; i <= len; ++i) {
                sol::optional<std::string> name = hitsTable->get<sol::optional<std::string>>(i);
                if (name) {
                    if (*name == "tile") {
                        hitsTiles = true;
                    } else {
                        hitMask |= collisionLayers.getLayerBit(*name);
                    }
                }
            }
        }

        // Collider — mask controls physics collision (tile pass-through vs stop)
        Collider collider;
        collider.size = Vec2(colliderW, colliderH);
        collider.layer = CollisionLayer::Projectile;
        // Only collide with tiles in physics if "tile" is in hits
        collider.mask = hitsTiles ? CollisionLayer::Tile : CollisionLayer::None;
        registry.add<Collider>(entity, collider);

        // Projectile component
        Projectile proj;
        proj.ownerEntity = owner;
        proj.damage = damage;
        proj.speed = speed;
        proj.lifetime = lifetime;
        proj.pierce = pierce;
        proj.gravityAffected = gravity;
        proj.autoRotate = autoRotate;
        proj.maxDistance = maxDistance;
        proj.startPosition = Vec2(x, y);
        proj.hitMask = hitMask;
        registry.add<Projectile>(entity, std::move(proj));

        // Gravity component (if enabled)
        if (gravity) {
            registry.add<Gravity>(entity, 1.0f);
        }

        // On-hit callback
        sol::optional<sol::function> onHit = opts.get<sol::optional<sol::function>>("on_hit");
        if (onHit) {
            sol::function fn = *onHit;
            projectileSystem.getCallbacks().registerOnHit(entity,
                [fn](const ProjectileHitInfo& info) {
                    try {
                        fn(static_cast<uint32_t>(info.projectile),
                           static_cast<uint32_t>(info.target));
                    } catch (const std::exception& ex) {
                        MOD_LOG_ERROR("projectile on_hit callback error: {}", ex.what());
                    }
                });
        }

        return static_cast<uint32_t>(entity);
    };

    // projectile.destroy(entityId)
    projectileApi["destroy"] = [&engine, &projectileSystem](uint32_t entityId) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (registry.valid(entity)) {
            projectileSystem.getCallbacks().removeOnHit(entity);
            registry.destroy(entity);
        }
    };

    // projectile.get_owner(entityId) -> ownerId
    projectileApi["get_owner"] = [&engine](uint32_t entityId) -> uint32_t {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Projectile>(entity)) return 0;
        return registry.get<Projectile>(entity).ownerEntity;
    };

    // projectile.set_damage(entityId, damage)
    projectileApi["set_damage"] = [&engine](uint32_t entityId, float damage) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (registry.valid(entity) && registry.has<Projectile>(entity)) {
            registry.get<Projectile>(entity).damage = damage;
        }
    };

    // projectile.count() -> int
    projectileApi["count"] = [&engine]() -> int {
        return static_cast<int>(engine.getRegistry().count<Projectile>());
    };
}

} // namespace gloaming
