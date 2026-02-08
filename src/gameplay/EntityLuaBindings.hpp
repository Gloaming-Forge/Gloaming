#pragma once

#include <sol/sol.hpp>
#include <string>

namespace gloaming {

// Forward declarations
class Engine;
class EntitySpawning;
class ProjectileSystem;
class CollisionLayerRegistry;

/// Registers the entity and projectile Lua APIs onto the given Lua state.
/// Call this from LuaBindings::init() after the core bindings are set up.
///
/// Provides:
///   - entity.create(), entity.spawn(), entity.destroy()
///   - entity.set_position(), entity.get_position()
///   - entity.set_velocity(), entity.get_velocity()
///   - entity.set_component(), entity.get_component()
///   - entity.find_in_radius(), entity.count()
///   - projectile.spawn()
void bindEntityAPI(sol::state& lua, Engine& engine,
                   EntitySpawning& spawning,
                   ProjectileSystem& projectiles,
                   CollisionLayerRegistry& collisionLayers);

} // namespace gloaming
