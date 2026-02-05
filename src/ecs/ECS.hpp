#pragma once

/// ECS (Entity Component System) module for Gloaming
/// Provides entity management, component storage, and system scheduling

// Core ECS types
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "ecs/Systems.hpp"
#include "ecs/EntityFactory.hpp"
#include "ecs/CoreSystems.hpp"

namespace gloaming {

// Convenient type aliases are already defined in their respective headers:
// - Entity (from Registry.hpp)
// - NullEntity (from Registry.hpp)
// - All component types (from Components.hpp)
// - System, SystemPhase, SystemScheduler (from Systems.hpp)
// - EntityFactory, EntityDefinition (from EntityFactory.hpp)
// - Core systems: MovementSystem, AnimationSystem, LifetimeSystem, etc. (from CoreSystems.hpp)
// - registerCoreSystems() helper function (from CoreSystems.hpp)

} // namespace gloaming
