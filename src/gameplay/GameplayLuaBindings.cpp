#include "gameplay/GameplayLuaBindings.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"
#include "engine/Gamepad.hpp"
#include "engine/InputDeviceTracker.hpp"
#include "engine/InputGlyphs.hpp"
#include "engine/Haptics.hpp"
#include "gameplay/Gameplay.hpp"

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
    LOG_WARN("parseKey: unrecognized key name '{}', defaulting to Space", name);
    return Key::Space;
}

/// Helper: convert a Lua gamepad button name to GamepadButton enum
static GamepadButton parseGamepadButton(const std::string& name) {
    if (name == "a" || name == "face_down")    return GamepadButton::FaceDown;
    if (name == "b" || name == "face_right")   return GamepadButton::FaceRight;
    if (name == "x" || name == "face_left")    return GamepadButton::FaceLeft;
    if (name == "y" || name == "face_up")      return GamepadButton::FaceUp;
    if (name == "lb" || name == "left_bumper")  return GamepadButton::LeftBumper;
    if (name == "rb" || name == "right_bumper") return GamepadButton::RightBumper;
    if (name == "select" || name == "back")     return GamepadButton::Select;
    if (name == "start" || name == "menu")      return GamepadButton::Start;
    if (name == "guide")                        return GamepadButton::Guide;
    if (name == "ls" || name == "left_thumb")   return GamepadButton::LeftThumb;
    if (name == "rs" || name == "right_thumb")  return GamepadButton::RightThumb;
    if (name == "dpad_up")                      return GamepadButton::DpadUp;
    if (name == "dpad_down")                    return GamepadButton::DpadDown;
    if (name == "dpad_left")                    return GamepadButton::DpadLeft;
    if (name == "dpad_right")                   return GamepadButton::DpadRight;
    LOG_WARN("parseGamepadButton: unrecognized button '{}', defaulting to FaceDown", name);
    return GamepadButton::FaceDown;
}

/// Helper: convert a Lua gamepad axis name to GamepadAxis enum
static GamepadAxis parseGamepadAxis(const std::string& name) {
    if (name == "left_x")         return GamepadAxis::LeftX;
    if (name == "left_y")         return GamepadAxis::LeftY;
    if (name == "right_x")        return GamepadAxis::RightX;
    if (name == "right_y")        return GamepadAxis::RightY;
    if (name == "left_trigger")   return GamepadAxis::LeftTrigger;
    if (name == "right_trigger")  return GamepadAxis::RightTrigger;
    LOG_WARN("parseGamepadAxis: unrecognized axis '{}', defaulting to LeftX", name);
    return GamepadAxis::LeftX;
}

/// Helper: parse a PlaybackMode from Lua string (case-insensitive)
static PlaybackMode parsePlaybackMode(const std::string& mode) {
    // Lowercase the input for case-insensitive comparison
    std::string lower;
    lower.reserve(mode.size());
    for (char c : mode) lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (lower == "once")      return PlaybackMode::Once;
    if (lower == "ping_pong" || lower == "pingpong") return PlaybackMode::PingPong;
    return PlaybackMode::Loop; // default
}

void bindGameplayAPI(sol::state& lua, Engine& engine,
                     InputActionMap& actions,
                     Pathfinder& pathfinder,
                     DialogueSystem& dialogue,
                     TileLayerManager& tileLayers,
                     CollisionLayerRegistry& collisionLayers) {

    // =========================================================================
    // game_mode API — configure the game type
    // =========================================================================
    auto gameMode = lua.create_named_table("game_mode");

    gameMode["set_view"] = [&engine](const std::string& mode) {
        auto& gmc = engine.getGameModeConfig();
        if (mode == "side_view" || mode == "sideview")      gmc.viewMode = ViewMode::SideView;
        else if (mode == "top_down" || mode == "topdown")   gmc.viewMode = ViewMode::TopDown;
        else if (mode == "custom")                          gmc.viewMode = ViewMode::Custom;
        else LOG_WARN("game_mode.set_view: unknown mode '{}' (use side_view, top_down, custom)", mode);
        LOG_INFO("Game view mode set to: {}", mode);
    };

    gameMode["get_view"] = [&engine]() -> std::string {
        switch (engine.getGameModeConfig().viewMode) {
            case ViewMode::SideView: return "side_view";
            case ViewMode::TopDown:  return "top_down";
            case ViewMode::Custom:   return "custom";
        }
        return "side_view";
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
        return actions.isActionPressed(name, engine.getInput(), engine.getGamepad());
    };

    inputApi["is_down"] = [&actions, &engine](const std::string& name) -> bool {
        return actions.isActionDown(name, engine.getInput(), engine.getGamepad());
    };

    inputApi["is_released"] = [&actions, &engine](const std::string& name) -> bool {
        return actions.isActionReleased(name, engine.getInput(), engine.getGamepad());
    };

    inputApi["get_bindings"] = [&actions, &engine](const std::string& name) -> sol::object {
        const auto& bindings = actions.getBindings(name);
        if (bindings.empty()) return sol::nil;
        sol::state_view luaView = engine.getModLoader().getLuaBindings().getState();
        sol::table result = luaView.create_table();
        for (size_t i = 0; i < bindings.size(); ++i) {
            result[i + 1] = static_cast<int>(bindings[i].key);
        }
        return result;
    };

    inputApi["clear_all"] = [&actions]() {
        actions.clearAll();
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

        // Initialize tile coords from current transform position
        if (registry.has<Transform>(entity)) {
            auto& transform = registry.get<Transform>(entity);
            gm.snapToGrid(transform.position);
            transform.position = gm.tileToWorldPos();
        }

        registry.add<GridMovement>(entity, std::move(gm));
    };

    gridApi["move"] = [&engine](uint32_t entityId, const std::string& direction) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Transform>(entity) ||
            !registry.has<GridMovement>(entity)) return false;

        auto* gridSystem = engine.getSystemScheduler().getSystem<GridMovementSystem>();
        if (!gridSystem) return false;

        auto& transform = registry.get<Transform>(entity);
        auto& grid = registry.get<GridMovement>(entity);
        return gridSystem->requestMove(transform, grid, parseFacing(direction));
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

        TileMap& tileMap = engine.getTileMap();
        // Use the explicit-params overload to avoid mutating shared Pathfinder state
        auto result = pathfinder.findPath(
            {startX, startY}, {goalX, goalY},
            [&tileMap](int x, int y) -> bool {
                Tile tile = tileMap.getTile(x, y);
                return !tile.isSolid();
            },
            diagonals, maxNodes
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
            std::string defaultId = "node_" + std::to_string(i);
            node.id = n.get_or<std::string>("id", defaultId);
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

    // =========================================================================
    // animation API — sprite animation controller (Stage 10)
    // =========================================================================
    auto animApi = lua.create_named_table("animation");

    // animation.add(entityId, clipName, opts)
    //
    // Two modes:
    //   Grid mode (row-based): { row = 0, frames = 4, fps = 6, mode = "loop",
    //     frame_width = 16, frame_height = 16, start_col = 0, padding = 0 }
    //
    //   Rect mode (atlas): { fps = 6, mode = "loop",
    //     rects = { {x,y,w,h}, {x,y,w,h}, ... } }
    animApi["add"] = [&engine](uint32_t entityId, const std::string& clipName, sol::table opts) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity)) {
            MOD_LOG_WARN("animation.add: invalid entity {}", entityId);
            return;
        }

        // Ensure the entity has an AnimationController
        if (!registry.has<AnimationController>(entity)) {
            registry.add<AnimationController>(entity);
        }
        auto& ctrl = registry.get<AnimationController>(entity);

        float fps = opts.get_or("fps", 10.0f);
        std::string modeStr = opts.get_or<std::string>("mode", "loop");
        PlaybackMode mode = parsePlaybackMode(modeStr);

        // Check for explicit rects array (atlas mode)
        sol::optional<sol::table> rectsOpt = opts["rects"];
        if (rectsOpt) {
            AnimationClip clip;
            clip.fps = fps;
            clip.mode = mode;
            sol::table rects = *rectsOpt;
            for (size_t i = 1; i <= rects.size(); ++i) {
                sol::table r = rects[i];
                float x = r.get_or("x", 0.0f);
                float y = r.get_or("y", 0.0f);
                float w = r.get_or("w", 0.0f);
                float h = r.get_or("h", 0.0f);
                clip.frames.push_back(Rect(x, y, w, h));
            }
            if (clip.frames.empty()) {
                MOD_LOG_WARN("animation.add: empty rects array for clip '{}'", clipName);
                return;
            }
            ctrl.addClip(clipName, std::move(clip));
            return;
        }

        // Grid mode (row-based sprite sheet)
        int row = opts.get_or("row", 0);
        int frameCount = opts.get_or("frames", 1);
        int startCol = opts.get_or("start_col", 0);
        int padding = opts.get_or("padding", 0);

        // Frame dimensions: explicit or derived from texture
        int frameWidth = opts.get_or("frame_width", 0);
        int frameHeight = opts.get_or("frame_height", 0);

        // If frame dimensions are not specified, try to derive from the Sprite's texture.
        // Heuristic: width = textureWidth / frameCount, height = frameWidth (assumes
        // square frames).  This works well for common sprite sheets where each frame is
        // a square tile.  For non-square frames, callers should provide explicit
        // frame_width / frame_height values.
        if ((frameWidth <= 0 || frameHeight <= 0) && registry.has<Sprite>(entity)) {
            auto& sprite = registry.get<Sprite>(entity);
            if (sprite.texture && sprite.texture->isValid()) {
                if (frameWidth <= 0 && frameCount > 0) {
                    frameWidth = sprite.texture->getWidth() / frameCount;
                }
                if (frameHeight <= 0) {
                    frameHeight = frameWidth > 0 ? frameWidth : sprite.texture->getHeight();
                }
            }
        }

        if (frameWidth <= 0 || frameHeight <= 0) {
            MOD_LOG_WARN("animation.add: cannot determine frame dimensions for clip '{}'", clipName);
            return;
        }

        ctrl.addClipFromSheet(clipName, row, frameCount, frameWidth, frameHeight, fps, mode,
                              startCol, padding);
    };

    // animation.play(entityId, clipName)
    animApi["play"] = [&engine](uint32_t entityId, const std::string& clipName) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<AnimationController>(entity)) return false;
        return registry.get<AnimationController>(entity).play(clipName);
    };

    // animation.stop(entityId)
    animApi["stop"] = [&engine](uint32_t entityId) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<AnimationController>(entity)) return;
        registry.get<AnimationController>(entity).stop();
    };

    // animation.play_directional(entityId, baseName, facing)
    // e.g. animation.play_directional(player, "walk", "down") -> plays "walk_down"
    animApi["play_directional"] = [&engine](uint32_t entityId, const std::string& baseName,
                                             const std::string& direction) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<AnimationController>(entity)) return false;
        return registry.get<AnimationController>(entity).playDirectional(baseName, direction);
    };

    // animation.current(entityId) -> string (clip name) or nil
    animApi["current"] = [&engine, &lua](uint32_t entityId) -> sol::object {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<AnimationController>(entity)) return sol::nil;
        const auto& name = registry.get<AnimationController>(entity).getCurrentClipName();
        if (name.empty()) return sol::nil;
        return sol::make_object(lua, name);
    };

    // animation.is_finished(entityId) -> bool
    animApi["is_finished"] = [&engine](uint32_t entityId) -> bool {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<AnimationController>(entity)) return true;
        return registry.get<AnimationController>(entity).isFinished();
    };

    // animation.on_frame(entityId, clipName, frameIndex, callback)
    animApi["on_frame"] = [&engine](uint32_t entityId, const std::string& clipName,
                                     int frameIndex, sol::function callback) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<AnimationController>(entity)) {
            MOD_LOG_WARN("animation.on_frame: entity {} has no AnimationController", entityId);
            return;
        }

        sol::function fn = callback;
        registry.get<AnimationController>(entity).addFrameEvent(
            clipName, frameIndex,
            [fn](Entity e) {
                try { fn(static_cast<uint32_t>(e)); }
                catch (const std::exception& ex) {
                    MOD_LOG_ERROR("animation.on_frame callback error: {}", ex.what());
                }
            }
        );
    };

    // animation.get_frame(entityId) -> int (current frame index)
    animApi["get_frame"] = [&engine](uint32_t entityId) -> int {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<AnimationController>(entity)) return -1;
        return registry.get<AnimationController>(entity).currentFrame;
    };

    // =========================================================================
    // collision API — collision layer management (Stage 10)
    // =========================================================================
    auto collisionApi = lua.create_named_table("collision");

    // collision.set_layer(entityId, layerName)
    collisionApi["set_layer"] = [&engine, &collisionLayers](uint32_t entityId,
                                                              const std::string& layerName) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Collider>(entity)) {
            MOD_LOG_WARN("collision.set_layer: entity {} has no Collider", entityId);
            return;
        }
        collisionLayers.setLayer(registry.get<Collider>(entity), layerName);
    };

    // collision.set_mask(entityId, { "tile", "enemy", "npc" })
    collisionApi["set_mask"] = [&engine, &collisionLayers](uint32_t entityId, sol::table maskTable) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Collider>(entity)) {
            MOD_LOG_WARN("collision.set_mask: entity {} has no Collider", entityId);
            return;
        }

        std::vector<std::string> names;
        size_t len = maskTable.size();
        names.reserve(len);
        for (size_t i = 1; i <= len; ++i) {
            sol::optional<std::string> name = maskTable.get<sol::optional<std::string>>(i);
            if (name) names.push_back(*name);
        }
        collisionLayers.setMask(registry.get<Collider>(entity), names);
    };

    // collision.add_mask(entityId, layerName)
    collisionApi["add_mask"] = [&engine, &collisionLayers](uint32_t entityId,
                                                             const std::string& layerName) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Collider>(entity)) return;
        collisionLayers.addMask(registry.get<Collider>(entity), layerName);
    };

    // collision.remove_mask(entityId, layerName)
    collisionApi["remove_mask"] = [&engine, &collisionLayers](uint32_t entityId,
                                                                const std::string& layerName) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Collider>(entity)) return;
        collisionLayers.removeMask(registry.get<Collider>(entity), layerName);
    };

    // collision.get_layer(entityId) -> int (raw bitmask)
    collisionApi["get_layer"] = [&engine](uint32_t entityId) -> uint32_t {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Collider>(entity)) return 0;
        return registry.get<Collider>(entity).layer;
    };

    // collision.get_mask(entityId) -> int (raw bitmask)
    collisionApi["get_mask"] = [&engine](uint32_t entityId) -> uint32_t {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Collider>(entity)) return 0;
        return registry.get<Collider>(entity).mask;
    };

    // collision.register_layer(name, bit)
    collisionApi["register_layer"] = [&collisionLayers](const std::string& name, int bit) -> bool {
        return collisionLayers.registerLayer(name, bit);
    };

    // collision.can_collide(entityA, entityB) -> bool
    collisionApi["can_collide"] = [&engine](uint32_t idA, uint32_t idB) -> bool {
        auto& registry = engine.getRegistry();
        Entity a = static_cast<Entity>(idA);
        Entity b = static_cast<Entity>(idB);
        if (!registry.valid(a) || !registry.valid(b)) return false;
        if (!registry.has<Collider>(a) || !registry.has<Collider>(b)) return false;
        return registry.get<Collider>(a).canCollideWith(registry.get<Collider>(b));
    };

    // collision.set_enabled(entityId, enabled)
    collisionApi["set_enabled"] = [&engine](uint32_t entityId, bool enabled) {
        auto& registry = engine.getRegistry();
        Entity entity = static_cast<Entity>(entityId);
        if (!registry.valid(entity) || !registry.has<Collider>(entity)) return;
        registry.get<Collider>(entity).enabled = enabled;
    };

    // =========================================================================
    // Gamepad input API (Stage 19A) — gamepad state, device detection, glyphs
    // =========================================================================
    auto gpApi = lua.create_named_table("gamepad");

    gpApi["is_connected"] = [&engine](sol::optional<int> id) -> bool {
        return engine.getGamepad().isConnected(id.value_or(0));
    };

    gpApi["connected_count"] = [&engine]() -> int {
        return engine.getGamepad().getConnectedCount();
    };

    gpApi["button_pressed"] = [&engine](const std::string& button) -> bool {
        auto btn = parseGamepadButton(button);
        return engine.getGamepad().isButtonPressed(btn);
    };

    gpApi["button_down"] = [&engine](const std::string& button) -> bool {
        auto btn = parseGamepadButton(button);
        return engine.getGamepad().isButtonDown(btn);
    };

    gpApi["button_released"] = [&engine](const std::string& button) -> bool {
        auto btn = parseGamepadButton(button);
        return engine.getGamepad().isButtonReleased(btn);
    };

    gpApi["axis"] = [&engine](const std::string& axis) -> float {
        auto ax = parseGamepadAxis(axis);
        return engine.getGamepad().getAxis(ax);
    };

    gpApi["left_stick"] = [&engine]() -> sol::table {
        Vec2 stick = engine.getGamepad().getLeftStick();
        sol::state_view luaView = engine.getModLoader().getLuaBindings().getState();
        sol::table t = luaView.create_table();
        t["x"] = stick.x;
        t["y"] = stick.y;
        return t;
    };

    gpApi["right_stick"] = [&engine]() -> sol::table {
        Vec2 stick = engine.getGamepad().getRightStick();
        sol::state_view luaView = engine.getModLoader().getLuaBindings().getState();
        sol::table t = luaView.create_table();
        t["x"] = stick.x;
        t["y"] = stick.y;
        return t;
    };

    gpApi["left_trigger"] = [&engine]() -> float {
        return engine.getGamepad().getLeftTrigger();
    };

    gpApi["right_trigger"] = [&engine]() -> float {
        return engine.getGamepad().getRightTrigger();
    };

    gpApi["set_deadzone"] = [&engine](float dz) {
        engine.getGamepad().setDeadzone(dz);
    };

    gpApi["get_deadzone"] = [&engine]() -> float {
        return engine.getGamepad().getDeadzone();
    };

    // =========================================================================
    // Input device detection API (Stage 19A)
    // =========================================================================
    inputApi["active_device"] = [&engine]() -> std::string {
        auto dev = engine.getInputDeviceTracker().getActiveDevice();
        return (dev == InputDevice::Gamepad) ? "gamepad" : "keyboard";
    };

    inputApi["device_changed"] = [&engine]() -> bool {
        return engine.getInputDeviceTracker().didDeviceChange();
    };

    // Analog action values
    inputApi["action_value"] = [&actions, &engine](const std::string& name) -> float {
        return actions.getActionValue(name, engine.getInput(), engine.getGamepad());
    };

    inputApi["movement_vector"] = [&actions, &engine]() -> sol::table {
        Vec2 mv = actions.getMovementVector(
            "move_left", "move_right", "move_up", "move_down",
            engine.getInput(), engine.getGamepad());
        sol::state_view luaView = engine.getModLoader().getLuaBindings().getState();
        sol::table t = luaView.create_table();
        t["x"] = mv.x;
        t["y"] = mv.y;
        return t;
    };

    // Gamepad bindings from Lua
    inputApi["add_gamepad_binding"] = [&actions](const std::string& actionName,
                                                  const std::string& button) {
        actions.addGamepadBinding(actionName, parseGamepadButton(button));
    };

    inputApi["add_gamepad_axis_binding"] = [&actions](const std::string& actionName,
                                                       const std::string& axis,
                                                       float threshold) {
        actions.addGamepadBinding(actionName, parseGamepadAxis(axis), threshold);
    };

    // =========================================================================
    // Glyph API (Stage 19A) — input glyph text for current device
    // =========================================================================
    inputApi["get_glyph"] = [&actions, &engine](const std::string& actionName) -> std::string {
        auto& glyphs = engine.getInputGlyphProvider();
        auto& tracker = engine.getInputDeviceTracker();
        return glyphs.getActionGlyph(actionName, actions, tracker.getActiveDevice(),
                                     glyphs.getGlyphStyle());
    };

    inputApi["get_glyph_style"] = [&engine]() -> std::string {
        switch (engine.getInputGlyphProvider().getGlyphStyle()) {
            case GlyphStyle::Xbox:        return "xbox";
            case GlyphStyle::PlayStation:  return "playstation";
            case GlyphStyle::Nintendo:     return "nintendo";
            case GlyphStyle::Keyboard:     return "keyboard";
            case GlyphStyle::SteamDeck:    return "deck";
        }
        return "xbox";
    };

    inputApi["set_glyph_style"] = [&engine](const std::string& style) {
        auto& glyphs = engine.getInputGlyphProvider();
        if (style == "xbox")             glyphs.setGlyphStyle(GlyphStyle::Xbox);
        else if (style == "playstation") glyphs.setGlyphStyle(GlyphStyle::PlayStation);
        else if (style == "nintendo")    glyphs.setGlyphStyle(GlyphStyle::Nintendo);
        else if (style == "keyboard")    glyphs.setGlyphStyle(GlyphStyle::Keyboard);
        else if (style == "deck")        glyphs.setGlyphStyle(GlyphStyle::SteamDeck);
        else LOG_WARN("input.set_glyph_style: unknown style '{}'", style);
    };

    // =========================================================================
    // Haptics API (Stage 19A) — gamepad vibration/rumble
    // =========================================================================
    auto hapticsApi = lua.create_named_table("haptics");

    hapticsApi["vibrate"] = [&engine](float left, float right, float duration) {
        engine.getHaptics().vibrate(left, right, duration);
    };

    hapticsApi["impulse"] = [&engine](float intensity, sol::optional<float> durationMs) {
        engine.getHaptics().impulse(intensity, durationMs.value_or(100.0f));
    };

    hapticsApi["stop"] = [&engine]() {
        engine.getHaptics().stop();
    };

    hapticsApi["set_enabled"] = [&engine](bool enabled) {
        engine.getHaptics().setEnabled(enabled);
    };

    hapticsApi["is_enabled"] = [&engine]() -> bool {
        return engine.getHaptics().isEnabled();
    };

    hapticsApi["set_intensity"] = [&engine](float intensity) {
        engine.getHaptics().setIntensity(intensity);
    };
}

} // namespace gloaming
