#pragma once

#include <sol/sol.hpp>

namespace gloaming {

class Engine;
class NPCSystem;
class HousingSystem;
class ShopManager;

/// Registers all Stage 15 NPC, Housing, and Shop Lua APIs.
///
/// Provides:
///   npc.*       — NPC spawning, behavior, dialogue, interaction
///   housing.*   — Room validation and NPC housing assignment
///   shop.*      — Buy/sell trade operations
void bindNPCAPI(sol::state& lua, Engine& engine,
                NPCSystem& npcSystem,
                HousingSystem& housingSystem,
                ShopManager& shopManager);

} // namespace gloaming
