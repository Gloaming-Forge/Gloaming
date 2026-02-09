#pragma once

#include <sol/sol.hpp>

namespace gloaming {

// Forward declarations
class Engine;
class CraftingManager;

/// Registers all Stage 13 Gameplay Loop Lua APIs onto the given Lua state.
///
/// Provides:
///   - inventory.add(), inventory.remove(), inventory.has(), inventory.count()
///   - inventory.get_slot(), inventory.set_slot(), inventory.swap()
///   - inventory.get_selected(), inventory.set_selected()
///   - item_drop.spawn(), item_drop.set_magnet()
///   - tool.start_mining(), tool.stop_mining(), tool.get_progress()
///   - weapon.melee_swing(), weapon.set_aim(), weapon.is_swinging()
///   - crafting.can_craft(), crafting.craft(), crafting.get_available()
///   - crafting.get_recipes(), crafting.get_recipes_by_category()
///   - combat.take_damage(), combat.heal(), combat.kill(), combat.respawn()
///   - combat.set_spawn(), combat.get_spawn(), combat.is_dead()
void bindGameplayLoopAPI(sol::state& lua, Engine& engine,
                          CraftingManager& crafting);

} // namespace gloaming
