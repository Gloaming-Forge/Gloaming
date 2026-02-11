#include "gameplay/NPCLuaBindings.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "gameplay/NPCSystem.hpp"
#include "gameplay/HousingSystem.hpp"
#include "gameplay/ShopSystem.hpp"
#include "gameplay/GameplayLoop.hpp"

namespace gloaming {

void bindNPCAPI(sol::state& lua, Engine& engine,
                NPCSystem& npcSystem,
                HousingSystem& housingSystem,
                ShopManager& shopManager) {

    // =========================================================================
    // npc API — NPC spawning, behavior, dialogue, interaction
    // =========================================================================
    auto npcApi = lua.create_named_table("npc");

    // npc.spawn(npc_id, x, y) -> entityId
    npcApi["spawn"] = [&npcSystem](const std::string& npcId,
                                     float x, float y) -> uint32_t {
        Entity e = npcSystem.spawnNPC(npcId, x, y);
        return static_cast<uint32_t>(e);
    };

    // npc.set_behavior(entityId, behavior)
    npcApi["set_behavior"] = [&engine](uint32_t entityId, const std::string& behavior) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<NPCAI>(entity)) {
            MOD_LOG_WARN("npc.set_behavior: entity {} has no NPCAI", entityId);
            return;
        }
        registry.get<NPCAI>(entity).behavior = behavior;
    };

    // npc.get_behavior(entityId) -> string
    npcApi["get_behavior"] = [&engine](uint32_t entityId) -> std::string {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<NPCAI>(entity)) return "";
        return registry.get<NPCAI>(entity).behavior;
    };

    // npc.set_dialogue(entityId, dialogue_id, greeting_node)
    npcApi["set_dialogue"] = [&engine](uint32_t entityId, const std::string& dialogueId,
                                        sol::optional<std::string> greetingNode) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) {
            MOD_LOG_WARN("npc.set_dialogue: invalid entity {}", entityId);
            return;
        }

        NPCDialogue dlg;
        dlg.dialogueId = dialogueId;
        if (greetingNode) dlg.greetingNodeId = *greetingNode;

        if (registry.has<NPCDialogue>(entity)) {
            registry.get<NPCDialogue>(entity) = std::move(dlg);
        } else {
            registry.add<NPCDialogue>(entity, std::move(dlg));
        }
    };

    // npc.talk(npcEntityId, playerEntityId) -> bool
    npcApi["talk"] = [&npcSystem](uint32_t npcId, uint32_t playerId) -> bool {
        return npcSystem.startDialogue(static_cast<Entity>(npcId),
                                        static_cast<Entity>(playerId));
    };

    // npc.set_home(entityId, x, y)
    npcApi["set_home"] = [&engine](uint32_t entityId, float x, float y) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<NPCAI>(entity)) return;
        registry.get<NPCAI>(entity).homePosition = Vec2(x, y);
    };

    // npc.get_home(entityId) -> x, y
    npcApi["get_home"] = [&engine](uint32_t entityId) -> std::tuple<float, float> {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<NPCAI>(entity)) return {0.0f, 0.0f};
        auto& home = registry.get<NPCAI>(entity).homePosition;
        return {home.x, home.y};
    };

    // npc.set_shop(entityId, shop_id)
    npcApi["set_shop"] = [&engine](uint32_t entityId, const std::string& shopId) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) {
            MOD_LOG_WARN("npc.set_shop: invalid entity {}", entityId);
            return;
        }
        ShopKeeper sk;
        sk.shopId = shopId;
        if (registry.has<ShopKeeper>(entity)) {
            registry.get<ShopKeeper>(entity) = std::move(sk);
        } else {
            registry.add<ShopKeeper>(entity, std::move(sk));
        }
    };

    // npc.set_schedule(entityId, schedule_table)
    // schedule_table = { {hour=6, behavior="wander", x=100, y=200}, ... }
    npcApi["set_schedule"] = [&engine](uint32_t entityId, sol::table scheduleTable) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<NPCAI>(entity)) {
            MOD_LOG_WARN("npc.set_schedule: entity {} has no NPCAI", entityId);
            return;
        }
        auto& ai = registry.get<NPCAI>(entity);
        ai.schedule.clear();

        size_t len = scheduleTable.size();
        for (size_t i = 1; i <= len; ++i) {
            sol::optional<sol::table> entryOpt = scheduleTable.get<sol::optional<sol::table>>(i);
            if (!entryOpt) continue;
            sol::table entry = *entryOpt;

            NPCAI::ScheduleEntry se;
            se.hour = entry.get_or("hour", 0);
            se.behavior = entry.get_or<std::string>("behavior", "idle");
            se.targetPosition.x = entry.get_or("x", 0.0f);
            se.targetPosition.y = entry.get_or("y", 0.0f);
            ai.schedule.push_back(std::move(se));
        }
    };

    // npc.register_behavior(name, callback)
    // callback = function(entityId, dt) ... end
    npcApi["register_behavior"] = [&npcSystem](const std::string& name, sol::function callback) {
        sol::function fn = callback;
        npcSystem.registerBehavior(name,
            [fn](Entity entity, NPCAI& ai, float dt) {
                try {
                    fn(static_cast<uint32_t>(entity), dt);
                } catch (const std::exception& ex) {
                    MOD_LOG_ERROR("npc behavior '{}' error: {}", ai.behavior, ex.what());
                }
            });
    };

    // npc.get_count() -> int
    npcApi["get_count"] = [&npcSystem]() -> int {
        return npcSystem.getActiveNPCCount();
    };

    // npc.is_in_range(entityId) -> bool
    npcApi["is_in_range"] = [&engine](uint32_t entityId) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<NPCAI>(entity)) return false;
        return registry.get<NPCAI>(entity).playerInRange;
    };

    // npc.add(entityId, opts)
    npcApi["add"] = [&engine](uint32_t entityId, sol::optional<sol::table> opts) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) {
            MOD_LOG_WARN("npc.add: invalid entity {}", entityId);
            return;
        }
        if (registry.has<NPCAI>(entity)) return;

        NPCAI ai;
        if (opts) {
            ai.behavior = opts->get_or<std::string>("behavior", "idle");
            ai.defaultBehavior = opts->get_or<std::string>("default_behavior", ai.behavior);
            ai.moveSpeed = opts->get_or("move_speed", 40.0f);
            ai.wanderRadius = opts->get_or("wander_radius", 80.0f);
            ai.interactionRange = opts->get_or("interaction_range", 48.0f);
        }

        if (registry.has<Transform>(entity)) {
            ai.homePosition = registry.get<Transform>(entity).position;
        }

        registry.add<NPCAI>(entity, std::move(ai));

        // Also add NPCTag if missing
        if (!registry.has<NPCTag>(entity)) {
            registry.add<NPCTag>(entity, NPCTag{""});
        }
    };

    // npc.remove(entityId)
    npcApi["remove"] = [&engine](uint32_t entityId) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (registry.valid(entity) && registry.has<NPCAI>(entity)) {
            registry.remove<NPCAI>(entity);
        }
    };

    // =========================================================================
    // housing API — Room validation and NPC housing
    // =========================================================================
    auto housingApi = lua.create_named_table("housing");

    // housing.validate(tileX, tileY) -> table with room info
    housingApi["validate"] = [&housingSystem, &engine](int tileX, int tileY) -> sol::table {
        ValidatedRoom room = housingSystem.validateRoom(tileX, tileY);
        sol::state_view luaView = engine.getModLoader().getLuaBindings().getState();
        sol::table t = luaView.create_table();
        t["is_valid"] = room.isValid;
        t["has_door"] = room.hasDoor;
        t["has_light"] = room.hasLight;
        t["has_furniture"] = room.hasFurniture;
        t["tile_count"] = room.tileCount;
        t["top_left_x"] = room.topLeft.x;
        t["top_left_y"] = room.topLeft.y;
        t["bottom_right_x"] = room.bottomRight.x;
        t["bottom_right_y"] = room.bottomRight.y;
        return t;
    };

    // housing.scan(centerX, centerY, radius) -> array of room tables
    housingApi["scan"] = [&housingSystem, &engine](float cx, float cy,
                                                     float radius) -> sol::table {
        auto rooms = housingSystem.scanForRooms(cx, cy, radius);
        sol::state_view luaView = engine.getModLoader().getLuaBindings().getState();
        sol::table result = luaView.create_table();
        int idx = 1;
        for (const auto& room : rooms) {
            sol::table t = luaView.create_table();
            t["id"] = room.id;
            t["is_valid"] = room.isValid;
            t["has_door"] = room.hasDoor;
            t["has_light"] = room.hasLight;
            t["has_furniture"] = room.hasFurniture;
            t["tile_count"] = room.tileCount;
            t["top_left_x"] = room.topLeft.x;
            t["top_left_y"] = room.topLeft.y;
            t["bottom_right_x"] = room.bottomRight.x;
            t["bottom_right_y"] = room.bottomRight.y;
            result[idx++] = t;
        }
        return result;
    };

    // housing.get_rooms() -> array of all cached rooms
    housingApi["get_rooms"] = [&housingSystem, &engine]() -> sol::table {
        sol::state_view luaView = engine.getModLoader().getLuaBindings().getState();
        sol::table result = luaView.create_table();
        int idx = 1;
        for (const auto& room : housingSystem.getValidRooms()) {
            sol::table t = luaView.create_table();
            t["id"] = room.id;
            t["is_valid"] = room.isValid;
            t["assigned_npc"] = static_cast<uint32_t>(room.assignedNPC);
            t["top_left_x"] = room.topLeft.x;
            t["top_left_y"] = room.topLeft.y;
            t["bottom_right_x"] = room.bottomRight.x;
            t["bottom_right_y"] = room.bottomRight.y;
            result[idx++] = t;
        }
        return result;
    };

    // housing.assign_npc(room_id, entityId) -> bool
    housingApi["assign_npc"] = [&housingSystem](int roomId, uint32_t entityId) -> bool {
        return housingSystem.assignNPCToRoom(static_cast<Entity>(entityId), roomId);
    };

    // housing.get_available() -> array of room tables with no NPC
    housingApi["get_available"] = [&housingSystem, &engine]() -> sol::table {
        sol::state_view luaView = engine.getModLoader().getLuaBindings().getState();
        sol::table result = luaView.create_table();
        auto rooms = housingSystem.getAvailableRooms();
        int idx = 1;
        for (const auto* room : rooms) {
            sol::table t = luaView.create_table();
            t["id"] = room->id;
            t["top_left_x"] = room->topLeft.x;
            t["top_left_y"] = room->topLeft.y;
            t["bottom_right_x"] = room->bottomRight.x;
            t["bottom_right_y"] = room->bottomRight.y;
            result[idx++] = t;
        }
        return result;
    };

    // housing.set_requirements(opts)
    housingApi["set_requirements"] = [&housingSystem](sol::table opts) {
        HousingRequirements reqs = housingSystem.getRequirements();
        reqs.minWidth = opts.get_or("min_width", reqs.minWidth);
        reqs.minHeight = opts.get_or("min_height", reqs.minHeight);
        reqs.maxWidth = opts.get_or("max_width", reqs.maxWidth);
        reqs.maxHeight = opts.get_or("max_height", reqs.maxHeight);
        reqs.requireDoor = opts.get_or("require_door", reqs.requireDoor);
        reqs.requireLightSource = opts.get_or("require_light", reqs.requireLightSource);
        reqs.requireFurniture = opts.get_or("require_furniture", reqs.requireFurniture);

        // Parse tile ID lists
        auto parseTileList = [](sol::optional<sol::table> tbl) -> std::vector<std::string> {
            std::vector<std::string> result;
            if (!tbl) return result;
            size_t len = tbl->size();
            for (size_t i = 1; i <= len; ++i) {
                sol::optional<std::string> s = tbl->get<sol::optional<std::string>>(i);
                if (s) result.push_back(*s);
            }
            return result;
        };

        reqs.doorTiles = parseTileList(opts.get<sol::optional<sol::table>>("door_tiles"));
        reqs.lightTiles = parseTileList(opts.get<sol::optional<sol::table>>("light_tiles"));
        reqs.furnitureTiles = parseTileList(
            opts.get<sol::optional<sol::table>>("furniture_tiles"));

        housingSystem.setRequirements(reqs);
    };

    // =========================================================================
    // shop API — Buy/sell trade operations
    // =========================================================================
    auto shopApi = lua.create_named_table("shop");

    // shop.buy(playerEntityId, shopId, itemId, count) -> { success, reason, price }
    shopApi["buy"] = [&engine, &shopManager](uint32_t playerId, const std::string& shopId,
                                               const std::string& itemId,
                                               int count) -> sol::table {
        auto& registry = engine.getRegistry();
        sol::state_view luaView = engine.getModLoader().getLuaBindings().getState();
        sol::table result = luaView.create_table();

        Entity player = static_cast<Entity>(playerId);
        if (!registry.valid(player) || !registry.has<Inventory>(player)) {
            result["success"] = false;
            result["reason"] = "player has no inventory";
            result["price"] = 0;
            return result;
        }

        auto& inv = registry.get<Inventory>(player);
        TradeResult trade = shopManager.buyItem(shopId, itemId, count, inv);

        result["success"] = trade.success;
        result["reason"] = trade.failReason;
        result["price"] = trade.finalPrice;
        return result;
    };

    // shop.sell(playerEntityId, shopId, itemId, count) -> { success, reason, price }
    shopApi["sell"] = [&engine, &shopManager](uint32_t playerId, const std::string& shopId,
                                                const std::string& itemId,
                                                int count) -> sol::table {
        auto& registry = engine.getRegistry();
        sol::state_view luaView = engine.getModLoader().getLuaBindings().getState();
        sol::table result = luaView.create_table();

        Entity player = static_cast<Entity>(playerId);
        if (!registry.valid(player) || !registry.has<Inventory>(player)) {
            result["success"] = false;
            result["reason"] = "player has no inventory";
            result["price"] = 0;
            return result;
        }

        auto& inv = registry.get<Inventory>(player);
        TradeResult trade = shopManager.sellItem(shopId, itemId, count, inv);

        result["success"] = trade.success;
        result["reason"] = trade.failReason;
        result["price"] = trade.finalPrice;
        return result;
    };

    // shop.get_items(shopId) -> array of item tables
    shopApi["get_items"] = [&engine, &shopManager](const std::string& shopId) -> sol::table {
        sol::state_view luaView = engine.getModLoader().getLuaBindings().getState();
        sol::table result = luaView.create_table();

        const ShopDefinition* shop = shopManager.getShop(shopId);
        if (!shop) return result;

        int idx = 1;
        for (const auto& entry : shop->items) {
            sol::table t = luaView.create_table();
            t["item"] = entry.itemId;
            t["buy_price"] = entry.buyPrice;
            t["sell_price"] = entry.sellPrice;
            t["stock"] = entry.stock;
            t["available"] = entry.available;
            result[idx++] = t;
        }
        return result;
    };

    // shop.get_buy_price(shopId, itemId) -> int
    shopApi["get_buy_price"] = [&shopManager](const std::string& shopId,
                                                const std::string& itemId) -> int {
        return shopManager.getBuyPrice(shopId, itemId);
    };

    // shop.get_sell_price(shopId, itemId) -> int
    shopApi["get_sell_price"] = [&shopManager](const std::string& shopId,
                                                 const std::string& itemId) -> int {
        return shopManager.getSellPrice(shopId, itemId);
    };

    // shop.open(npcEntityId) — marks shop as open, emits event
    shopApi["open"] = [&engine](uint32_t entityId) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<ShopKeeper>(entity)) {
            MOD_LOG_WARN("shop.open: entity {} has no ShopKeeper", entityId);
            return;
        }
        registry.get<ShopKeeper>(entity).shopOpen = true;

        EventData data;
        data.setInt("npc_entity", static_cast<int>(entityId));
        data.setString("shop_id", registry.get<ShopKeeper>(entity).shopId);
        engine.getEventBus().emit("shop_opened", data);
    };

    // shop.close(npcEntityId) — marks shop as closed, emits event
    shopApi["close"] = [&engine](uint32_t entityId) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<ShopKeeper>(entity)) return;
        registry.get<ShopKeeper>(entity).shopOpen = false;

        EventData data;
        data.setInt("npc_entity", static_cast<int>(entityId));
        data.setString("shop_id", registry.get<ShopKeeper>(entity).shopId);
        engine.getEventBus().emit("shop_closed", data);
    };
}

} // namespace gloaming
