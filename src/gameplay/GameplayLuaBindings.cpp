#include "gameplay/GameplayLuaBindings.hpp"
#include "gameplay/Gameplay.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"

namespace gloaming {

/// Helper: convert a Lua string to FacingDirection
static FacingDirection parseFacing(const std::string& dir) {
    if (dir == "up")    return FacingDirection::Up;
    if (dir == "down")  return FacingDirection::Down;
    if (dir == "left")  return FacingDirection::Left;
    if (dir == "right") return FacingDirection::Right;
    return FacingDirection::Down;
}

/// Helper: convert FacingDirection to Lua string
static std::string facingToString(FacingDirection dir) {
    switch (dir) {
        case FacingDirection::Up:    return "up";
        case FacingDirection::Down:  return "down";
        case FacingDirection::Left:  return "left";
        case FacingDirection::Right: return "right";
    }
    return "down";
}

/// Helper: convert a Lua Key name string to Key enum
static Key parseKey(const std::string& name) {
    // Letters
    if (name.size() == 1 && name[0] >= 'a' && name[0] <= 'z') {
        return static_cast<Key>(name[0] - 'a' + static_cast<int>(Key::A));
    }
    if (name.size() == 1 && name[0] >= 'A' && name[0] <= 'Z') {
        return static_cast<Key>(name[0] - 'A' + static_cast<int>(Key::A));
    }
    // Common keys
    if (name == "space")      return Key::Space;
    if (name == "enter")      return Key::Enter;
    if (name == "escape")     return Key::Escape;
    if (name == "tab")        return Key::Tab;
    if (name == "backspace")  return Key::Backspace;
    if (name == "up")         return Key::Up;
    if (name == "down")       return Key::Down;
    if (name == "left")       return Key::Left;
    if (name == "right")      return Key::Right;
    if (name == "lshift")     return Key::LeftShift;
    if (name == "rshift")     return Key::RightShift;
    if (name == "lctrl")      return Key::LeftControl;
    if (name == "rctrl")      return Key::RightControl;
    return Key::Space;  // fallback
}

void bindGameplayAPI(sol::state& lua, Engine& engine,
                     InputActionMap& actions,
                     Pathfinder& pathfinder,
                     DialogueSystem& dialogue,
                     TileLayerManager& tileLayers) {

    // =========================================================================
    // game_mode API — configure the game type
    // =========================================================================
    auto gameMode = lua.create_named_table("game_mode");

    gameMode["set_view"] = [&engine](const std::string& mode) {
        // This is informational — mods declare their view mode so other systems
        // can adapt. The actual physics/camera changes happen through the
        // respective APIs.
        LOG_INFO("Game mode set to: {}", mode);
    };

    gameMode["set_physics"] = [&engine](const std::string& preset) {
        auto* physics = engine.getSystemScheduler().getSystem<PhysicsSystem>();
        if (!physics) {
            LOG_WARN("game_mode.set_physics: PhysicsSystem not found");
            return;
        }
        if (preset == "platformer")       physics->getConfig() = PhysicsPresets::Platformer();
        else if (preset == "topdown")     physics->getConfig() = PhysicsPresets::TopDown();
        else if (preset == "flight")      physics->getConfig() = PhysicsPresets::Flight();
        else if (preset == "zero_g")      physics->getConfig() = PhysicsPresets::ZeroG();
        else LOG_WARN("game_mode.set_physics: unknown preset '{}'", preset);
    };

    gameMode["set_gravity"] = [&engine](float x, float y) {
        auto* physics = engine.getSystemScheduler().getSystem<PhysicsSystem>();
        if (physics) {
            physics->getConfig().gravity = {x, y};
        }
    };

    // =========================================================================
    // input_actions API — abstract input binding
    // =========================================================================
    auto inputApi = lua.create_named_table("input_actions");

    inputApi["register"] = [&actions](const std::string& name, const std::string& keyName) {
        actions.registerAction(name, parseKey(keyName));
    };

    inputApi["add_binding"] = [&actions](const std::string& name, const std::string& keyName) {
        actions.addBinding(name, parseKey(keyName));
    };

    inputApi["rebind"] = [&actions](const std::string& name, const std::string& keyName) {
        actions.rebind(name, parseKey(keyName));
    };

    inputApi["is_pressed"] = [&actions, &engine](const std::string& name) -> bool {
        return actions.isActionPressed(name, engine.getInput());
    };

    inputApi["is_down"] = [&actions, &engine](const std::string& name) -> bool {
        return actions.isActionDown(name, engine.getInput());
    };

    inputApi["is_released"] = [&actions, &engine](const std::string& name) -> bool {
        return actions.isActionReleased(name, engine.getInput());
    };

    inputApi["register_platformer_defaults"] = [&actions]() {
        actions.registerPlatformerDefaults();
    };

    inputApi["register_topdown_defaults"] = [&actions]() {
        actions.registerTopDownDefaults();
    };

    inputApi["register_flight_defaults"] = [&actions]() {
        actions.registerFlightDefaults();
    };

    // =========================================================================
    // grid_movement API — grid-based movement for top-down games
    // =========================================================================
    auto gridApi = lua.create_named_table("grid_movement");

    gridApi["add"] = [&engine](uint32_t entityId, sol::optional<sol::table> opts) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return;

        GridMovement gm;
        if (opts) {
            gm.gridSize = opts->get_or("grid_size", 16);
            gm.moveSpeed = opts->get_or("move_speed", 4.0f);
        }

        // Set up walkability check using the tile map
        TileMap& tileMap = engine.getTileMap();
        gm.isWalkable = [&tileMap](int tileX, int tileY) -> bool {
            Tile tile = tileMap.getTile(tileX, tileY);
            return !tile.isSolid();
        };

        registry.add<GridMovement>(entity, std::move(gm));
    };

    gridApi["move"] = [&engine](uint32_t entityId, const std::string& direction) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Transform>(entity) ||
            !registry.has<GridMovement>(entity)) return false;

        auto& transform = registry.get<Transform>(entity);
        auto& grid = registry.get<GridMovement>(entity);
        return GridMovementSystem::requestMove(transform, grid, parseFacing(direction));
    };

    gridApi["is_moving"] = [&engine](uint32_t entityId) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<GridMovement>(entity)) return false;
        return registry.get<GridMovement>(entity).isMoving;
    };

    gridApi["get_facing"] = [&engine](uint32_t entityId) -> std::string {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<GridMovement>(entity)) return "down";
        return facingToString(registry.get<GridMovement>(entity).facing);
    };

    gridApi["snap_to_grid"] = [&engine](uint32_t entityId) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Transform>(entity) ||
            !registry.has<GridMovement>(entity)) return;

        auto& transform = registry.get<Transform>(entity);
        auto& grid = registry.get<GridMovement>(entity);
        transform.position = grid.snapToGrid(transform.position);
    };

    // =========================================================================
    // camera API — camera controller configuration
    // =========================================================================
    auto cameraApi = lua.create_named_table("camera");

    cameraApi["set_mode"] = [&engine](const std::string& mode) {
        auto* ctrl = engine.getSystemScheduler().getSystem<CameraControllerSystem>();
        if (!ctrl) return;

        auto& config = ctrl->getConfig();
        if (mode == "free_follow")       config.mode = CameraMode::FreeFollow;
        else if (mode == "grid_snap")    config.mode = CameraMode::GridSnap;
        else if (mode == "auto_scroll")  config.mode = CameraMode::AutoScroll;
        else if (mode == "room_based")   config.mode = CameraMode::RoomBased;
        else if (mode == "locked")       config.mode = CameraMode::Locked;
    };

    cameraApi["set_smoothness"] = [&engine](float s) {
        auto* ctrl = engine.getSystemScheduler().getSystem<CameraControllerSystem>();
        if (ctrl) ctrl->getConfig().smoothness = s;
    };

    cameraApi["set_deadzone"] = [&engine](float x, float y) {
        auto* ctrl = engine.getSystemScheduler().getSystem<CameraControllerSystem>();
        if (ctrl) ctrl->getConfig().deadzone = {x, y};
    };

    cameraApi["set_scroll_speed"] = [&engine](float x, float y) {
        auto* ctrl = engine.getSystemScheduler().getSystem<CameraControllerSystem>();
        if (ctrl) ctrl->getConfig().scrollSpeed = {x, y};
    };

    cameraApi["set_room_size"] = [&engine](float w, float h) {
        auto* ctrl = engine.getSystemScheduler().getSystem<CameraControllerSystem>();
        if (ctrl) {
            ctrl->getConfig().roomWidth = w;
            ctrl->getConfig().roomHeight = h;
        }
    };

    cameraApi["set_zoom"] = [&engine](float zoom) {
        auto* ctrl = engine.getSystemScheduler().getSystem<CameraControllerSystem>();
        if (ctrl) ctrl->getConfig().targetZoom = zoom;
        else engine.getCamera().setZoom(zoom);
    };

    cameraApi["set_bounds"] = [&engine](float x, float y, float w, float h) {
        auto* ctrl = engine.getSystemScheduler().getSystem<CameraControllerSystem>();
        if (ctrl) {
            ctrl->getConfig().useBounds = true;
            ctrl->getConfig().bounds = {x, y, w, h};
        }
    };

    cameraApi["lock_axis"] = [&engine](const std::string& axis) {
        auto* ctrl = engine.getSystemScheduler().getSystem<CameraControllerSystem>();
        if (!ctrl) return;
        if (axis == "x")      ctrl->getConfig().axisLock = AxisLock::LockX;
        else if (axis == "y") ctrl->getConfig().axisLock = AxisLock::LockY;
        else                  ctrl->getConfig().axisLock = AxisLock::None;
    };

    cameraApi["set_target"] = [&engine](uint32_t entityId, sol::optional<sol::table> opts) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) return;

        CameraTarget target;
        if (opts) {
            target.offset.x = opts->get_or("offset_x", 0.0f);
            target.offset.y = opts->get_or("offset_y", 0.0f);
            target.priority = opts->get_or("priority", 0);
        }
        registry.addOrReplace<CameraTarget>(entity, target);
    };

    // =========================================================================
    // pathfinding API — A* on the tile grid
    // =========================================================================
    auto pathApi = lua.create_named_table("pathfinding");

    pathApi["find_path"] = [&pathfinder, &engine](
            int startX, int startY, int goalX, int goalY,
            sol::optional<sol::table> opts) -> sol::object {

        int maxNodes = 5000;
        bool diagonals = false;
        if (opts) {
            maxNodes = opts->get_or("max_nodes", 5000);
            diagonals = opts->get_or("diagonals", false);
        }
        pathfinder.setMaxNodes(maxNodes);
        pathfinder.setAllowDiagonals(diagonals);

        TileMap& tileMap = engine.getTileMap();
        auto result = pathfinder.findPath(
            {startX, startY}, {goalX, goalY},
            [&tileMap](int x, int y) -> bool {
                Tile tile = tileMap.getTile(x, y);
                return !tile.isSolid();
            }
        );

        if (!result.found) return sol::nil;

        // Convert path to Lua table
        sol::state_view luaView = engine.getModLoader().getLuaBindings().getState();
        sol::table pathTable = luaView.create_table();
        for (size_t i = 0; i < result.path.size(); ++i) {
            sol::table point = luaView.create_table();
            point["x"] = result.path[i].x;
            point["y"] = result.path[i].y;
            pathTable[i + 1] = point;
        }
        return pathTable;
    };

    pathApi["is_reachable"] = [&pathfinder, &engine](
            int startX, int startY, int goalX, int goalY,
            sol::optional<int> maxDist) -> bool {

        TileMap& tileMap = engine.getTileMap();
        return pathfinder.isReachable(
            {startX, startY}, {goalX, goalY},
            [&tileMap](int x, int y) -> bool {
                Tile tile = tileMap.getTile(x, y);
                return !tile.isSolid();
            },
            maxDist.value_or(100)
        );
    };

    // =========================================================================
    // fsm API — finite state machine for entities
    // =========================================================================
    auto fsmApi = lua.create_named_table("fsm");

    fsmApi["add"] = [&engine](uint32_t entityId) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (registry.valid(entity) && !registry.has<StateMachine>(entity)) {
            registry.add<StateMachine>(entity);
        }
    };

    fsmApi["add_state"] = [&engine](uint32_t entityId, const std::string& name,
                                     sol::optional<sol::table> callbacks) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<StateMachine>(entity)) return;

        StateCallbacks cbs;
        if (callbacks) {
            sol::optional<sol::function> onEnter = callbacks->get<sol::optional<sol::function>>("on_enter");
            sol::optional<sol::function> onUpdate = callbacks->get<sol::optional<sol::function>>("on_update");
            sol::optional<sol::function> onExit = callbacks->get<sol::optional<sol::function>>("on_exit");

            if (onEnter) {
                sol::function fn = *onEnter;
                cbs.onEnter = [fn](Entity e) {
                    try { fn(static_cast<uint32_t>(e)); }
                    catch (const std::exception& ex) { MOD_LOG_ERROR("FSM onEnter error: {}", ex.what()); }
                };
            }
            if (onUpdate) {
                sol::function fn = *onUpdate;
                cbs.onUpdate = [fn](Entity e, float dt) {
                    try { fn(static_cast<uint32_t>(e), dt); }
                    catch (const std::exception& ex) { MOD_LOG_ERROR("FSM onUpdate error: {}", ex.what()); }
                };
            }
            if (onExit) {
                sol::function fn = *onExit;
                cbs.onExit = [fn](Entity e) {
                    try { fn(static_cast<uint32_t>(e)); }
                    catch (const std::exception& ex) { MOD_LOG_ERROR("FSM onExit error: {}", ex.what()); }
                };
            }
        }

        registry.get<StateMachine>(entity).addState(name, std::move(cbs));
    };

    fsmApi["set_state"] = [&engine](uint32_t entityId, const std::string& state) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<StateMachine>(entity)) return;
        StateMachineSystem::setState(registry.get<StateMachine>(entity), entity, state);
    };

    fsmApi["get_state"] = [&engine](uint32_t entityId) -> std::string {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<StateMachine>(entity)) return "";
        return registry.get<StateMachine>(entity).getCurrentState();
    };

    fsmApi["get_state_time"] = [&engine](uint32_t entityId) -> float {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<StateMachine>(entity)) return 0.0f;
        return registry.get<StateMachine>(entity).getStateTime();
    };

    // =========================================================================
    // dialogue API — dialogue boxes for NPC conversations
    // =========================================================================
    auto dialogueApi = lua.create_named_table("dialogue");

    dialogueApi["start"] = [&dialogue, &engine](sol::table nodes) {
        std::vector<DialogueNode> nodeList;

        size_t len = nodes.size();
        for (size_t i = 1; i <= len; ++i) {
            sol::table n = nodes[i];
            DialogueNode node;
            node.id = n.get_or<std::string>("id", "node_" + std::to_string(i));
            node.speaker = n.get_or<std::string>("speaker", "");
            node.text = n.get_or<std::string>("text", "");
            node.portraitId = n.get_or<std::string>("portrait", "");
            node.nextNodeId = n.get_or<std::string>("next", "");

            // Parse choices
            sol::optional<sol::table> choices = n.get<sol::optional<sol::table>>("choices");
            if (choices) {
                size_t cLen = choices->size();
                for (size_t j = 1; j <= cLen; ++j) {
                    sol::table c = (*choices)[j];
                    DialogueChoice choice;
                    choice.text = c.get_or<std::string>("text", "");
                    choice.nextNodeId = c.get_or<std::string>("next", "");

                    sol::optional<sol::function> onSelect = c.get<sol::optional<sol::function>>("on_select");
                    if (onSelect) {
                        sol::function fn = *onSelect;
                        choice.onSelect = [fn]() {
                            try { fn(); }
                            catch (const std::exception& ex) {
                                MOD_LOG_ERROR("Dialogue choice callback error: {}", ex.what());
                            }
                        };
                    }
                    node.choices.push_back(std::move(choice));
                }
            }

            // Parse onShow callback
            sol::optional<sol::function> onShow = n.get<sol::optional<sol::function>>("on_show");
            if (onShow) {
                sol::function fn = *onShow;
                node.onShow = [fn]() {
                    try { fn(); }
                    catch (const std::exception& ex) {
                        MOD_LOG_ERROR("Dialogue onShow error: {}", ex.what());
                    }
                };
            }

            nodeList.push_back(std::move(node));
        }

        dialogue.startDialogue(std::move(nodeList));
    };

    dialogueApi["close"] = [&dialogue]() {
        dialogue.close();
    };

    dialogueApi["is_active"] = [&dialogue]() -> bool {
        return dialogue.isActive();
    };

    dialogueApi["jump_to"] = [&dialogue](const std::string& nodeId) {
        dialogue.jumpToNode(nodeId);
    };

    dialogueApi["set_speed"] = [&dialogue](float charsPerSec) {
        dialogue.getConfig().typewriterSpeed = charsPerSec;
    };

    dialogueApi["on_end"] = [&dialogue](sol::function callback) {
        dialogue.setOnDialogueEnd([callback]() {
            try { callback(); }
            catch (const std::exception& ex) {
                MOD_LOG_ERROR("Dialogue on_end error: {}", ex.what());
            }
        });
    };

    // =========================================================================
    // tile_layers API — multi-layer tile rendering
    // =========================================================================
    auto layerApi = lua.create_named_table("tile_layers");

    layerApi["set"] = [&tileLayers](int worldX, int worldY, int layer, uint16_t tileId,
                                     sol::optional<uint8_t> variant,
                                     sol::optional<uint8_t> flags) {
        Tile tile;
        tile.id = tileId;
        tile.variant = variant.value_or(0);
        tile.flags = flags.value_or(0);
        tileLayers.setTile(worldX, worldY, layer, tile);
    };

    layerApi["get"] = [&tileLayers, &engine](int worldX, int worldY, int layer) -> sol::object {
        Tile tile = tileLayers.getTile(worldX, worldY, layer);
        if (tile.isEmpty()) return sol::nil;

        sol::state_view luaView = engine.getModLoader().getLuaBindings().getState();
        sol::table t = luaView.create_table();
        t["id"] = tile.id;
        t["variant"] = tile.variant;
        t["flags"] = tile.flags;
        return t;
    };

    layerApi["clear_chunk"] = [&tileLayers](int chunkX, int chunkY) {
        tileLayers.clearChunk({chunkX, chunkY});
    };

    // Layer index constants
    layerApi["BACKGROUND"] = static_cast<int>(TileLayerIndex::Background);
    layerApi["GROUND"]     = static_cast<int>(TileLayerIndex::Ground);
    layerApi["DECORATION"] = static_cast<int>(TileLayerIndex::Decoration);
    layerApi["FOREGROUND"] = static_cast<int>(TileLayerIndex::Foreground);
}

} // namespace gloaming
