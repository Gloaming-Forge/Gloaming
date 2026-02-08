#pragma once

#include <sol/sol.hpp>
#include <string>

namespace gloaming {

// Forward declarations
class Engine;
class InputActionMap;
class Pathfinder;
class DialogueSystem;
class TileLayerManager;
class CollisionLayerRegistry;

/// Registers all gameplay Lua APIs onto the given Lua state.
/// Call this from LuaBindings::init() after the core bindings are set up.
void bindGameplayAPI(sol::state& lua, Engine& engine,
                     InputActionMap& actions,
                     Pathfinder& pathfinder,
                     DialogueSystem& dialogue,
                     TileLayerManager& tileLayers,
                     CollisionLayerRegistry& collisionLayers);

} // namespace gloaming
