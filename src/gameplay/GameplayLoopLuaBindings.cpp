#include "gameplay/GameplayLoopLuaBindings.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "gameplay/GameplayLoop.hpp"
#include "gameplay/GameplayLoopSystems.hpp"
#include "gameplay/CraftingSystem.hpp"
#include "mod/ContentRegistry.hpp"

namespace gloaming {

void bindGameplayLoopAPI(sol::state& lua, Engine& engine,
                          CraftingManager& crafting) {

    // =========================================================================
    // inventory API — item management for entities with Inventory component
    // =========================================================================
    auto invApi = lua.create_named_table("inventory");

    // inventory.add(entityId, itemId, count [, maxStack]) -> leftover
    invApi["add"] = [&engine](uint32_t entityId, const std::string& itemId,
                               int count, sol::optional<int> maxStack) -> int {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return count;

        if (!registry.has<Inventory>(entity)) {
            registry.add<Inventory>(entity);
        }
        auto& inv = registry.get<Inventory>(entity);
        return inv.addItem(itemId, count, maxStack.value_or(999));
    };

    // inventory.remove(entityId, itemId, count) -> removedCount
    invApi["remove"] = [&engine](uint32_t entityId, const std::string& itemId,
                                  int count) -> int {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Inventory>(entity)) return 0;
        return registry.get<Inventory>(entity).removeItem(itemId, count);
    };

    // inventory.has(entityId, itemId [, count]) -> bool
    invApi["has"] = [&engine](uint32_t entityId, const std::string& itemId,
                               sol::optional<int> count) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Inventory>(entity)) return false;
        return registry.get<Inventory>(entity).hasItem(itemId, count.value_or(1));
    };

    // inventory.count(entityId, itemId) -> int
    invApi["count"] = [&engine](uint32_t entityId, const std::string& itemId) -> int {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Inventory>(entity)) return 0;
        return registry.get<Inventory>(entity).countItem(itemId);
    };

    // inventory.get_slot(entityId, slotIndex) -> { item = "id", count = N } or nil
    invApi["get_slot"] = [&engine, &lua](uint32_t entityId, int slotIndex) -> sol::object {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Inventory>(entity)) return sol::nil;

        auto& inv = registry.get<Inventory>(entity);
        if (slotIndex < 0 || slotIndex >= Inventory::MaxSlots) return sol::nil;

        const auto& slot = inv.slots[static_cast<size_t>(slotIndex)];
        if (slot.isEmpty()) return sol::nil;

        sol::table t = lua.create_table();
        t["item"] = slot.itemId;
        t["count"] = slot.count;
        return t;
    };

    // inventory.set_slot(entityId, slotIndex, itemId, count)
    invApi["set_slot"] = [&engine](uint32_t entityId, int slotIndex,
                                    const std::string& itemId, int count) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return;

        if (!registry.has<Inventory>(entity)) {
            registry.add<Inventory>(entity);
        }
        auto& inv = registry.get<Inventory>(entity);
        if (slotIndex < 0 || slotIndex >= Inventory::MaxSlots) return;

        auto& slot = inv.slots[static_cast<size_t>(slotIndex)];
        if (itemId.empty() || count <= 0) {
            slot.clear();
        } else {
            slot.itemId = itemId;
            slot.count = count;
        }
    };

    // inventory.clear_slot(entityId, slotIndex)
    invApi["clear_slot"] = [&engine](uint32_t entityId, int slotIndex) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Inventory>(entity)) return;
        registry.get<Inventory>(entity).clearSlot(slotIndex);
    };

    // inventory.swap(entityId, slotA, slotB)
    invApi["swap"] = [&engine](uint32_t entityId, int a, int b) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Inventory>(entity)) return;
        registry.get<Inventory>(entity).swapSlots(a, b);
    };

    // inventory.get_selected(entityId) -> slotIndex
    invApi["get_selected"] = [&engine](uint32_t entityId) -> int {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Inventory>(entity)) return 0;
        return registry.get<Inventory>(entity).selectedSlot;
    };

    // inventory.set_selected(entityId, slotIndex)
    invApi["set_selected"] = [&engine](uint32_t entityId, int slot) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Inventory>(entity)) return;
        auto& inv = registry.get<Inventory>(entity);
        inv.selectedSlot = std::clamp(slot, 0, Inventory::HotbarSlots - 1);
    };

    // inventory.get_selected_item(entityId) -> { item = "id", count = N } or nil
    invApi["get_selected_item"] = [&engine, &lua](uint32_t entityId) -> sol::object {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Inventory>(entity)) return sol::nil;

        const auto& slot = registry.get<Inventory>(entity).getSelected();
        if (slot.isEmpty()) return sol::nil;

        sol::table t = lua.create_table();
        t["item"] = slot.itemId;
        t["count"] = slot.count;
        return t;
    };

    // inventory.find_item(entityId, itemId) -> slotIndex or -1
    invApi["find_item"] = [&engine](uint32_t entityId, const std::string& itemId) -> int {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Inventory>(entity)) return -1;
        return registry.get<Inventory>(entity).findItem(itemId);
    };

    // inventory.slot_count() -> int (MaxSlots constant)
    invApi["slot_count"] = []() -> int { return Inventory::MaxSlots; };

    // inventory.hotbar_count() -> int (HotbarSlots constant)
    invApi["hotbar_count"] = []() -> int { return Inventory::HotbarSlots; };

    // =========================================================================
    // item_drop API — spawn and configure item drops in the world
    // =========================================================================
    auto dropApi = lua.create_named_table("item_drop");

    // item_drop.spawn(itemId, count, x, y [, opts]) -> entityId
    dropApi["spawn"] = [&engine](const std::string& itemId, int count,
                                  float x, float y,
                                  sol::optional<sol::table> opts) -> uint32_t {
        auto& registry = engine.getRegistry();

        Entity entity = registry.create(
            Transform{Vec2(x, y)},
            Name{itemId, "item_drop"}
        );

        ItemDrop drop(itemId, count);
        if (opts) {
            drop.magnetRadius = opts->get_or("magnet_radius", 48.0f);
            drop.pickupRadius = opts->get_or("pickup_radius", 16.0f);
            drop.pickupDelay = opts->get_or("pickup_delay", 0.5f);
            drop.despawnTime = opts->get_or("despawn_time", 300.0f);
            drop.magnetic = opts->get_or("magnetic", true);
            drop.magnetSpeed = opts->get_or("magnet_speed", 200.0f);
        }
        registry.add<ItemDrop>(entity, std::move(drop));

        // Add collider for physics (item collision layer)
        Collider col;
        col.size = Vec2(8.0f, 8.0f);
        col.layer = CollisionLayer::Item;
        col.mask = CollisionLayer::Tile;
        registry.add<Collider>(entity, col);

        return static_cast<uint32_t>(entity);
    };

    // item_drop.spawn_from_tile(tileId, x, y) -> entityId or 0
    // Convenience: looks up the tile's drop item and spawns it
    dropApi["spawn_from_tile"] = [&engine](const std::string& tileId,
                                            float x, float y) -> uint32_t {
        auto& contentRegistry = engine.getContentRegistry();
        const TileContentDef* def = contentRegistry.getTile(tileId);
        if (!def || def->dropItem.empty()) return 0;

        auto& registry = engine.getRegistry();
        Entity entity = registry.create(
            Transform{Vec2(x, y)},
            Name{def->dropItem, "item_drop"}
        );

        ItemDrop drop(def->dropItem, def->dropCount);
        registry.add<ItemDrop>(entity, std::move(drop));

        Collider col;
        col.size = Vec2(8.0f, 8.0f);
        col.layer = CollisionLayer::Item;
        col.mask = CollisionLayer::Tile;
        registry.add<Collider>(entity, col);

        return static_cast<uint32_t>(entity);
    };

    // =========================================================================
    // tool API — tile mining/chopping
    // =========================================================================
    auto toolApi = lua.create_named_table("tool");

    // tool.start_mining(entityId, tileX, tileY [, breakTime])
    toolApi["start_mining"] = [&engine](uint32_t entityId, int tileX, int tileY,
                                         sol::optional<float> breakTimeOpt) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return;

        if (!registry.has<ToolUse>(entity)) {
            registry.add<ToolUse>(entity);
        }
        auto& tool = registry.get<ToolUse>(entity);

        // Calculate break time based on tile hardness and tool power
        float breakTime = breakTimeOpt.value_or(1.0f);

        if (!breakTimeOpt) {
            // Auto-calculate from tile and held item
            auto& tileMap = engine.getTileMap();
            Tile tile = tileMap.getTile(tileX, tileY);

            const TileContentDef* tileDef = engine.getContentRegistry().getTileByRuntime(tile.id);
            if (tileDef) {
                float hardness = tileDef->hardness;

                // Check held tool power
                float toolPower = 1.0f;
                if (registry.has<Inventory>(entity)) {
                    const auto& selected = registry.get<Inventory>(entity).getSelected();
                    if (!selected.isEmpty()) {
                        const ItemDefinition* itemDef = engine.getContentRegistry().getItem(
                            selected.itemId);
                        if (itemDef) {
                            toolPower = std::max(1.0f,
                                std::max(itemDef->pickaxePower, itemDef->axePower));
                        }
                    }
                }

                // Break time = hardness / tool power (minimum 0.1s)
                breakTime = std::max(0.1f, hardness / toolPower);

                // Check if tool meets minimum power requirement
                if (tileDef->requiredPickaxePower > 0.0f) {
                    float pickPower = 0.0f;
                    if (registry.has<Inventory>(entity)) {
                        const auto& sel = registry.get<Inventory>(entity).getSelected();
                        if (!sel.isEmpty()) {
                            const ItemDefinition* iDef = engine.getContentRegistry().getItem(
                                sel.itemId);
                            if (iDef) pickPower = iDef->pickaxePower;
                        }
                    }
                    if (pickPower < tileDef->requiredPickaxePower) {
                        return; // Tool not powerful enough
                    }
                }
            }
        }

        tool.targetTileX = tileX;
        tool.targetTileY = tileY;
        tool.breakTime = breakTime;
        tool.progress = 0.0f;
        tool.active = true;
    };

    // tool.stop_mining(entityId)
    toolApi["stop_mining"] = [&engine](uint32_t entityId) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<ToolUse>(entity)) return;
        registry.get<ToolUse>(entity).reset();
    };

    // tool.get_progress(entityId) -> float (0-1) or 0 if not mining
    toolApi["get_progress"] = [&engine](uint32_t entityId) -> float {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<ToolUse>(entity)) return 0.0f;
        auto& tool = registry.get<ToolUse>(entity);
        if (!tool.active) return 0.0f;
        return tool.getProgressPercent();
    };

    // tool.is_mining(entityId) -> bool
    toolApi["is_mining"] = [&engine](uint32_t entityId) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<ToolUse>(entity)) return false;
        return registry.get<ToolUse>(entity).active;
    };

    // tool.place_tile(entityId, tileId, tileX, tileY) -> bool
    toolApi["place_tile"] = [&engine](uint32_t entityId, const std::string& tileId,
                                       int tileX, int tileY) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return false;

        auto& tileMap = engine.getTileMap();
        auto& contentRegistry = engine.getContentRegistry();

        // Check if the target position is empty
        Tile existingTile = tileMap.getTile(tileX, tileY);
        if (!existingTile.isEmpty()) return false;

        // Look up tile definition
        const TileContentDef* tileDef = contentRegistry.getTile(tileId);
        if (!tileDef) return false;

        // Check if player has the placing item
        if (registry.has<Inventory>(entity)) {
            auto& inv = registry.get<Inventory>(entity);
            // Find an item that places this tile
            auto itemIds = contentRegistry.getItemIds();
            for (const auto& itemQId : itemIds) {
                const ItemDefinition* itemDef = contentRegistry.getItem(itemQId);
                if (itemDef && itemDef->placesTile == tileId) {
                    if (inv.hasItem(itemQId, 1)) {
                        inv.removeItem(itemQId, 1);

                        Tile newTile;
                        newTile.id = tileDef->runtimeId;
                        tileMap.setTile(tileX, tileY, newTile);

                        EventData data;
                        data.setInt("tile_x", tileX);
                        data.setInt("tile_y", tileY);
                        data.setString("tile_type", tileId);
                        data.setInt("entity", static_cast<int>(entity));
                        engine.getEventBus().emit("tile_placed", data);

                        return true;
                    }
                }
            }
        }

        return false;
    };

    // =========================================================================
    // weapon API — melee and ranged weapon actions
    // =========================================================================
    auto weaponApi = lua.create_named_table("weapon");

    // weapon.melee_swing(entityId [, opts]) -> bool
    weaponApi["melee_swing"] = [&engine](uint32_t entityId,
                                          sol::optional<sol::table> opts) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return false;

        if (!registry.has<MeleeAttack>(entity)) {
            registry.add<MeleeAttack>(entity);
        }
        auto& melee = registry.get<MeleeAttack>(entity);

        if (!melee.canAttack()) return false;

        // Get weapon stats from held item or opts
        float damage = 10.0f;
        float knockback = 5.0f;
        float arc = 120.0f;
        float range = 32.0f;
        float useTime = 0.3f;

        if (opts) {
            damage = opts->get_or("damage", damage);
            knockback = opts->get_or("knockback", knockback);
            arc = opts->get_or("arc", arc);
            range = opts->get_or("range", range);
            useTime = opts->get_or("use_time", useTime);
        } else if (registry.has<Inventory>(entity)) {
            // Auto-detect from held weapon
            const auto& selected = registry.get<Inventory>(entity).getSelected();
            if (!selected.isEmpty()) {
                const ItemDefinition* itemDef =
                    engine.getContentRegistry().getItem(selected.itemId);
                if (itemDef && itemDef->type == "weapon" && itemDef->weaponType == "melee") {
                    damage = static_cast<float>(itemDef->damage);
                    knockback = itemDef->knockback;
                    arc = itemDef->swingArc > 0.0f ? itemDef->swingArc : 120.0f;
                    useTime = itemDef->useTime > 0 ? itemDef->useTime / 60.0f : 0.3f;
                }
            }
        }

        melee.startSwing(damage, knockback, arc, range, useTime);
        return true;
    };

    // weapon.set_aim(entityId, dirX, dirY)
    weaponApi["set_aim"] = [&engine](uint32_t entityId, float dirX, float dirY) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<MeleeAttack>(entity)) return;

        auto& melee = registry.get<MeleeAttack>(entity);
        float len = std::sqrt(dirX * dirX + dirY * dirY);
        if (len > 0.001f) {
            melee.aimDirection = Vec2(dirX / len, dirY / len);
        }
    };

    // weapon.is_swinging(entityId) -> bool
    weaponApi["is_swinging"] = [&engine](uint32_t entityId) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<MeleeAttack>(entity)) return false;
        return registry.get<MeleeAttack>(entity).swinging;
    };

    // weapon.get_cooldown(entityId) -> float
    weaponApi["get_cooldown"] = [&engine](uint32_t entityId) -> float {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<MeleeAttack>(entity)) return 0.0f;
        return registry.get<MeleeAttack>(entity).cooldownRemaining;
    };

    // =========================================================================
    // crafting API — recipe checking and crafting
    // =========================================================================
    auto craftApi = lua.create_named_table("crafting");

    // crafting.can_craft(entityId, recipeId) -> bool
    craftApi["can_craft"] = [&engine, &crafting](uint32_t entityId,
                                                   const std::string& recipeId) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Inventory>(entity)) return false;
        if (!registry.has<Transform>(entity)) return false;

        const auto& inv = registry.get<Inventory>(entity);
        Vec2 pos = registry.get<Transform>(entity).position;
        return crafting.canCraft(recipeId, inv, pos);
    };

    // crafting.craft(entityId, recipeId) -> { success, item, count, reason }
    craftApi["craft"] = [&engine, &crafting, &lua](uint32_t entityId,
                                                     const std::string& recipeId) -> sol::table {
        auto& registry = engine.getRegistry();
        sol::table result = lua.create_table();

        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Inventory>(entity) ||
            !registry.has<Transform>(entity)) {
            result["success"] = false;
            result["reason"] = "invalid entity";
            return result;
        }

        auto& inv = registry.get<Inventory>(entity);
        Vec2 pos = registry.get<Transform>(entity).position;

        CraftResult craftResult = crafting.craft(recipeId, inv, pos);
        result["success"] = craftResult.success;
        result["item"] = craftResult.resultItem;
        result["count"] = craftResult.resultCount;
        result["reason"] = craftResult.failReason;

        if (craftResult.success) {
            EventData data;
            data.setString("recipe", recipeId);
            data.setString("item", craftResult.resultItem);
            data.setInt("count", craftResult.resultCount);
            data.setInt("entity", static_cast<int>(entity));
            engine.getEventBus().emit("item_crafted", data);
        }

        return result;
    };

    // crafting.get_available(entityId) -> { "recipe1", "recipe2", ... }
    craftApi["get_available"] = [&engine, &crafting, &lua](uint32_t entityId) -> sol::table {
        sol::table result = lua.create_table();
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Inventory>(entity) ||
            !registry.has<Transform>(entity)) return result;

        const auto& inv = registry.get<Inventory>(entity);
        Vec2 pos = registry.get<Transform>(entity).position;
        auto available = crafting.getAvailableRecipes(inv, pos);

        for (size_t i = 0; i < available.size(); ++i) {
            result[i + 1] = available[i];
        }
        return result;
    };

    // crafting.get_recipes() -> table of all recipe IDs
    craftApi["get_recipes"] = [&crafting, &lua]() -> sol::table {
        sol::table result = lua.create_table();
        auto all = crafting.getAllRecipes();
        for (size_t i = 0; i < all.size(); ++i) {
            result[i + 1] = all[i];
        }
        return result;
    };

    // crafting.get_recipes_by_category(category) -> table of recipe IDs
    craftApi["get_recipes_by_category"] = [&crafting, &lua](
            const std::string& category) -> sol::table {
        sol::table result = lua.create_table();
        auto recipes = crafting.getRecipesByCategory(category);
        for (size_t i = 0; i < recipes.size(); ++i) {
            result[i + 1] = recipes[i];
        }
        return result;
    };

    // crafting.set_station_radius(radius)
    craftApi["set_station_radius"] = [&crafting](float radius) {
        crafting.setStationSearchRadius(radius);
    };

    // crafting.is_station_nearby(entityId, stationTileId) -> bool
    craftApi["is_station_nearby"] = [&engine, &crafting](
            uint32_t entityId, const std::string& stationTileId) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Transform>(entity)) return false;
        Vec2 pos = registry.get<Transform>(entity).position;
        return crafting.isStationNearby(stationTileId, pos);
    };

    // =========================================================================
    // combat API — health, damage, death, respawn
    // =========================================================================
    auto combatApi = lua.create_named_table("combat");

    // combat.take_damage(entityId, amount) -> actualDamage
    combatApi["take_damage"] = [&engine](uint32_t entityId, float amount) -> float {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Health>(entity)) return 0.0f;

        float dealt = registry.get<Health>(entity).takeDamage(amount);
        if (dealt > 0.0f) {
            EventData data;
            data.setInt("entity", static_cast<int>(entity));
            data.setFloat("damage", dealt);
            data.setFloat("remaining", registry.get<Health>(entity).current);
            engine.getEventBus().emit("entity_damaged", data);
        }
        return dealt;
    };

    // combat.heal(entityId, amount) -> actualHealed
    combatApi["heal"] = [&engine](uint32_t entityId, float amount) -> float {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Health>(entity)) return 0.0f;
        return registry.get<Health>(entity).heal(amount);
    };

    // combat.kill(entityId)
    combatApi["kill"] = [&engine](uint32_t entityId) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Health>(entity)) return;
        auto& health = registry.get<Health>(entity);
        health.current = 0.0f;
    };

    // combat.set_health(entityId, current, max)
    combatApi["set_health"] = [&engine](uint32_t entityId, float current, float max) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return;

        if (!registry.has<Health>(entity)) {
            registry.add<Health>(entity, current, max);
        } else {
            auto& health = registry.get<Health>(entity);
            health.current = current;
            health.max = max;
        }
    };

    // combat.get_health(entityId) -> { current, max, percentage, is_dead }
    combatApi["get_health"] = [&engine, &lua](uint32_t entityId) -> sol::object {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Health>(entity)) return sol::nil;

        auto& health = registry.get<Health>(entity);
        sol::table t = lua.create_table();
        t["current"] = health.current;
        t["max"] = health.max;
        t["percentage"] = health.getPercentage();
        t["is_dead"] = health.isDead();
        t["is_invincible"] = health.isInvincible();
        return t;
    };

    // combat.set_spawn(entityId, x, y)
    combatApi["set_spawn"] = [&engine](uint32_t entityId, float x, float y) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return;

        if (!registry.has<PlayerCombat>(entity)) {
            registry.add<PlayerCombat>(entity);
        }
        registry.get<PlayerCombat>(entity).spawnPoint = Vec2(x, y);
    };

    // combat.get_spawn(entityId) -> x, y
    combatApi["get_spawn"] = [&engine](uint32_t entityId) -> std::tuple<float, float> {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<PlayerCombat>(entity)) {
            return {0.0f, 0.0f};
        }
        auto& spawn = registry.get<PlayerCombat>(entity).spawnPoint;
        return {spawn.x, spawn.y};
    };

    // combat.is_dead(entityId) -> bool
    combatApi["is_dead"] = [&engine](uint32_t entityId) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return false;

        // Check PlayerCombat first (tracks death state)
        if (registry.has<PlayerCombat>(entity)) {
            return registry.get<PlayerCombat>(entity).dead;
        }
        // Fallback to Health component
        if (registry.has<Health>(entity)) {
            return registry.get<Health>(entity).isDead();
        }
        return false;
    };

    // combat.set_respawn_delay(entityId, seconds)
    combatApi["set_respawn_delay"] = [&engine](uint32_t entityId, float seconds) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return;

        if (!registry.has<PlayerCombat>(entity)) {
            registry.add<PlayerCombat>(entity);
        }
        registry.get<PlayerCombat>(entity).respawnDelay = seconds;
    };

    // combat.get_death_count(entityId) -> int
    combatApi["get_death_count"] = [&engine](uint32_t entityId) -> int {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<PlayerCombat>(entity)) return 0;
        return registry.get<PlayerCombat>(entity).deathCount;
    };

    // combat.respawn(entityId) — force immediate respawn
    combatApi["respawn"] = [&engine](uint32_t entityId) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return;

        if (registry.has<PlayerCombat>(entity)) {
            auto& combat = registry.get<PlayerCombat>(entity);
            combat.dead = false;
            combat.respawnTimer = 0.0f;

            if (registry.has<Health>(entity)) {
                auto& health = registry.get<Health>(entity);
                health.current = health.max;
                health.invincibilityTime = 2.0f;
            }
            if (registry.has<Transform>(entity)) {
                registry.get<Transform>(entity).position = combat.spawnPoint;
            }
            if (registry.has<Velocity>(entity)) {
                registry.get<Velocity>(entity).linear = Vec2(0.0f, 0.0f);
            }

            EventData data;
            data.setInt("entity", static_cast<int>(entity));
            data.setFloat("x", combat.spawnPoint.x);
            data.setFloat("y", combat.spawnPoint.y);
            engine.getEventBus().emit("player_respawn", data);
        }
    };

    // combat.set_invincible(entityId, seconds)
    combatApi["set_invincible"] = [&engine](uint32_t entityId, float seconds) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Health>(entity)) return;
        registry.get<Health>(entity).invincibilityTime = seconds;
    };
}

} // namespace gloaming
