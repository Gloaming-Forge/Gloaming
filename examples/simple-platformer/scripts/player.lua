-- scripts/player.lua
-- Player spawning, movement, and animation state machine.

local atlas = _G.atlas

local MOVE_SPEED   = 350
local JUMP_VELOCITY = -550
local COYOTE_TIME  = 0.08   -- seconds
local JUMP_BUFFER  = 0.10   -- seconds

local player_mod = {}

-- Atlas regions for beige character
local C = atlas.characters.regions
local CHAR_SHEET = atlas.characters.sheet

local POSES = {
    idle    = C.character_beige_idle,
    walk_a  = C.character_beige_walk_a,
    walk_b  = C.character_beige_walk_b,
    jump    = C.character_beige_jump,
    duck    = C.character_beige_duck,
    hit     = C.character_beige_hit,
    front   = C.character_beige_front,
}

-- Internal state per player
local player_state = {
    on_ground = false,
    coyote_timer = 0,
    jump_buffer_timer = 0,
    invincible_timer = 0,
    facing_right = true,
    current_anim = "",
}

function player_mod.spawn(x, y)
    local id = entity.create()
    entity.set_position(id, x, y)
    entity.set_sprite(id, CHAR_SHEET)

    -- Set initial source rect to idle pose
    entity.set_source_rect(id, POSES.idle.x, POSES.idle.y, POSES.idle.w, POSES.idle.h)

    entity.set_component(id, "collider", {
        width = 160,
        height = 200,
        offset_x = 48,
        offset_y = 48,
        layer = "player",
        mask = {"tile", "enemy", "pickup", "hazard", "trigger"}
    })

    entity.set_component(id, "health", {
        current = 3,
        max = 3
    })

    entity.set_component(id, "gravity", { scale = 1.0 })
    entity.set_component(id, "velocity", { x = 0, y = 0 })

    -- Register animation clips using atlas rects
    animation.add(id, "idle", {
        fps = 1, mode = "loop",
        rects = { POSES.idle }
    })
    animation.add(id, "walk", {
        fps = 6, mode = "loop",
        rects = { POSES.walk_a, POSES.walk_b }
    })
    animation.add(id, "jump", {
        fps = 1, mode = "once",
        rects = { POSES.jump }
    })
    animation.add(id, "fall", {
        fps = 1, mode = "once",
        rects = { POSES.duck }
    })
    animation.add(id, "hurt", {
        fps = 1, mode = "once",
        rects = { POSES.hit }
    })

    animation.play(id, "idle")
    player_state.current_anim = "idle"

    return id
end

function player_mod.update(id, dt)
    if not entity.is_valid(id) then return end

    local vx, vy = entity.get_velocity(id)
    local px, py = entity.get_position(id)

    -- ── Timers ───────────────────────────────────────────────────────────
    player_state.coyote_timer = math.max(0, player_state.coyote_timer - dt)
    player_state.jump_buffer_timer = math.max(0, player_state.jump_buffer_timer - dt)
    player_state.invincible_timer = math.max(0, player_state.invincible_timer - dt)

    -- ── Ground detection ─────────────────────────────────────────────────
    -- The physics system provides vy = 0 when grounded.
    -- Heuristic: if vertical velocity is near zero and we were moving down, we're on ground.
    local was_on_ground = player_state.on_ground
    player_state.on_ground = (math.abs(vy) < 1.0)

    if player_state.on_ground then
        player_state.coyote_timer = COYOTE_TIME
    end

    -- ── Horizontal movement ──────────────────────────────────────────────
    local move_x = 0
    if input_actions.is_down("move_right") then
        move_x = MOVE_SPEED
        player_state.facing_right = true
    elseif input_actions.is_down("move_left") then
        move_x = -MOVE_SPEED
        player_state.facing_right = false
    end

    entity.set_component(id, "sprite", { flip_x = not player_state.facing_right })

    -- ── Jump ─────────────────────────────────────────────────────────────
    if input_actions.is_pressed("jump") then
        player_state.jump_buffer_timer = JUMP_BUFFER
    end

    if player_state.jump_buffer_timer > 0 and player_state.coyote_timer > 0 then
        vy = JUMP_VELOCITY
        player_state.jump_buffer_timer = 0
        player_state.coyote_timer = 0
        player_state.on_ground = false
        audio.playSound("jump")
    end

    entity.set_velocity(id, move_x, vy)

    -- ── Fall detection ───────────────────────────────────────────────────
    -- If player falls below the level, emit event
    if py > _G.game_state.level_height then
        events.emit("player_fell")
    end

    -- ── Animation state machine ──────────────────────────────────────────
    local new_anim = "idle"
    if not player_state.on_ground then
        if vy < 0 then
            new_anim = "jump"
        else
            new_anim = "fall"
        end
    elseif math.abs(move_x) > 0 then
        new_anim = "walk"
    end

    if new_anim ~= player_state.current_anim then
        animation.play(id, new_anim)
        player_state.current_anim = new_anim
    end
end

-- Called when player takes damage
function player_mod.hurt(id)
    if player_state.invincible_timer > 0 then return false end
    player_state.invincible_timer = 1.5
    animation.play(id, "hurt")
    player_state.current_anim = "hurt"
    combat.set_invincible(id, 1.5)
    events.emit("player_hurt")
    return true
end

-- Called when player stomps an enemy — bounce upward
function player_mod.stomp_bounce(id)
    local vx, _ = entity.get_velocity(id)
    entity.set_velocity(id, vx, JUMP_VELOCITY * 0.7)
    audio.playSound("jump_high")
end

return player_mod
