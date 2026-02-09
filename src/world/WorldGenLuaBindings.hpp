#pragma once

#include <sol/sol.hpp>
#include <string>

namespace gloaming {

// Forward declarations
class Engine;
class WorldGenerator;

/// Registers the worldgen Lua API onto the given Lua state.
/// Provides: worldgen.registerTerrainGenerator(), worldgen.registerBiome(),
///           worldgen.registerOre(), worldgen.registerStructure(),
///           worldgen.setSurfaceLevel(), worldgen.setSeaLevel(), etc.
void bindWorldGenAPI(sol::state& lua, Engine& engine, WorldGenerator& worldGen);

} // namespace gloaming
