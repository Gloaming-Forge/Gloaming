-- scripts/init.lua
-- Entry point for the Simple Platformer example mod.
-- Sets up physics, camera, input, audio, and loads levels using entities.

local atlas = require("scripts.atlas")
local T = atlas.tiles.regions
local TILE_SHEET = atlas.tiles.sheet

-- ─── Game state ──────────────────────────────────────────────────────────────
local state = {
    player = nil,
    lives = 3,
    coins = 0,
    current_level = 1,
    max_levels = 3,
    paused = false,
    level_height = 0,
    tile_entities = {},
}

-- Make state globally accessible for other scripts
_G.game_state = state
_G.atlas = atlas

-- ─── Physics / view ──────────────────────────────────────────────────────────
game_mode.set_view("side_view")
game_mode.set_physics("platformer")

-- ─── Camera ──────────────────────────────────────────────────────────────────
camera.set_mode("free_follow")
camera.set_smoothness(8.0)
camera.set_deadzone(48, 32)

-- ─── Input ───────────────────────────────────────────────────────────────────
input_actions.register_platformer_defaults()

-- ─── Custom collision layers ─────────────────────────────────────────────────
collision.register_layer("pickup", 8)
collision.register_layer("hazard", 9)

-- ─── Audio ───────────────────────────────────────────────────────────────────
audio.registerSound("jump",      "reference/Sounds/sfx_jump.ogg",       { volume = 0.7 })
audio.registerSound("jump_high", "reference/Sounds/sfx_jump-high.ogg",  { volume = 0.7 })
audio.registerSound("coin",      "reference/Sounds/sfx_coin.ogg",       { volume = 0.8 })
audio.registerSound("gem",       "reference/Sounds/sfx_gem.ogg",        { volume = 0.8 })
audio.registerSound("hurt",      "reference/Sounds/sfx_hurt.ogg",       { volume = 0.6 })
audio.registerSound("stomp",     "reference/Sounds/sfx_disappear.ogg",  { volume = 0.8 })
audio.registerSound("bump",      "reference/Sounds/sfx_bump.ogg",       { volume = 0.6 })
audio.registerSound("flag",      "reference/Sounds/sfx_magic.ogg",      { volume = 0.9 })
audio.registerSound("select",    "reference/Sounds/sfx_select.ogg",     { volume = 0.7 })

-- ─── Load sub-scripts ────────────────────────────────────────────────────────
local player_mod       = require("scripts.player")
local enemies_mod      = require("scripts.enemies")
local collectibles_mod = require("scripts.collectibles")
local hud_mod          = require("scripts.hud")
local screens_mod      = require("scripts.screens")

-- ─── Level data ──────────────────────────────────────────────────────────────
-- Load all level definitions upfront (they are static data tables).
local levels = {
    require("scripts.levels.level_1"),
    require("scripts.levels.level_2"),
    require("scripts.levels.level_3"),
}

-- ─── Hitbox constants (match collider definitions in sub-scripts) ────────────
local PLAYER_HB = { ox = 48, oy = 48, w = 160, h = 200 }
local ENEMY_HB  = { ox = 14, oy = 24, w = 100, h = 80 }
local PICKUP_HB = { ox = 24, oy = 24, w = 80,  h = 80 }
local FLAG_HB   = { ox = 24, oy = 0,  w = 80,  h = 128 }

-- ─── AABB overlap helper ─────────────────────────────────────────────────────
local function aabb_overlap(ax, ay, aw, ah, bx, by, bw, bh)
    return ax < bx + bw and ax + aw > bx and ay < by + bh and ay + ah > by
end

-- ─── Auto-tiling helper ─────────────────────────────────────────────────────
-- Returns the atlas region table for a solid terrain tile based on neighbors.
local function auto_tile(tiles, row, col, biome)
    local height = #tiles
    local width  = #tiles[1]

    local function solid(r, c)
        if r < 1 or r > height or c < 1 or c > width then return true end
        return tiles[r][c] == 1
    end

    local up    = solid(row - 1, col)
    local down  = solid(row + 1, col)
    local left  = solid(row, col - 1)
    local right = solid(row, col + 1)

    local prefix = "terrain_" .. biome .. "_"

    -- Standalone block
    if not up and not down and not left and not right then
        return T[prefix .. "block"]
    end

    -- Horizontal platform (no solid above or below)
    if not up and not down then
        if not left and right then return T[prefix .. "horizontal_left"] end
        if left and not right then return T[prefix .. "horizontal_right"] end
        return T[prefix .. "horizontal_middle"]
    end

    -- Vertical column
    if not left and not right then
        if not up and down then return T[prefix .. "vertical_top"] end
        if up and not down then return T[prefix .. "vertical_bottom"] end
        return T[prefix .. "vertical_middle"]
    end

    -- 9-slice block edges and corners
    if not up and not left  then return T[prefix .. "block_top_left"] end
    if not up and not right then return T[prefix .. "block_top_right"] end
    if not up               then return T[prefix .. "block_top"] end
    if not down and not left  then return T[prefix .. "block_bottom_left"] end
    if not down and not right then return T[prefix .. "block_bottom_right"] end
    if not down              then return T[prefix .. "block_bottom"] end
    if not left              then return T[prefix .. "block_left"] end
    if not right             then return T[prefix .. "block_right"] end

    return T[prefix .. "block_center"]
end

-- Auto-tile for cloud (one-way) platforms — horizontal only.
local function auto_tile_cloud(tiles, row, col, biome)
    local width = #tiles[1]
    local left_cloud  = (col > 1     and tiles[row][col - 1] == 2)
    local right_cloud = (col < width and tiles[row][col + 1] == 2)

    local prefix = "terrain_" .. biome .. "_"

    if not left_cloud and not right_cloud then return T[prefix .. "cloud"] end
    if not left_cloud  then return T[prefix .. "cloud_left"] end
    if not right_cloud then return T[prefix .. "cloud_right"] end
    return T[prefix .. "cloud_middle"]
end

-- ─── Clear current level ────────────────────────────────────────────────────
local function clear_level()
    -- Destroy tile entities
    for _, id in ipairs(state.tile_entities) do
        if entity.is_valid(id) then
            entity.destroy(id)
        end
    end
    state.tile_entities = {}

    -- Destroy enemies and collectibles
    enemies_mod.clear_all()
    collectibles_mod.clear_all()

    -- Destroy player
    if state.player and entity.is_valid(state.player) then
        entity.destroy(state.player)
        state.player = nil
    end
end

-- ─── Level loading ──────────────────────────────────────────────────────────
local function load_level(level_num)
    clear_level()

    state.current_level = level_num
    local level_data = levels[level_num]

    state.level_height = level_data.height * 128

    local biome = level_data.biome or "grass"

    -- ── Place terrain tiles as entities ─────────────────────────────────
    for row = 1, level_data.height do
        for col = 1, level_data.width do
            local tile_val = level_data.tiles[row][col]
            if tile_val ~= 0 then
                local wx = (col - 1) * 128
                local wy = (row - 1) * 128
                local region = nil

                if tile_val == 1 then
                    region = auto_tile(level_data.tiles, row, col, biome)
                elseif tile_val == 2 then
                    region = auto_tile_cloud(level_data.tiles, row, col, biome)
                end

                if region then
                    local id = entity.create()
                    entity.set_position(id, wx, wy)
                    entity.set_sprite(id, TILE_SHEET)
                    entity.set_source_rect(id, region.x, region.y, region.w, region.h)
                    entity.set_component(id, "sprite", { layer = 0 })
                    entity.set_component(id, "collider", {
                        width = 128,
                        height = 128,
                        layer = "tile",
                        mask = {"player", "enemy"},
                    })
                    table.insert(state.tile_entities, id)
                end
            end
        end
    end

    -- ── Spawn level entities ────────────────────────────────────────────
    for _, ent in ipairs(level_data.entities) do
        local wx = (ent.x - 1) * 128
        local wy = (ent.y - 1) * 128
        if ent.type == "coin" then
            collectibles_mod.spawn_coin(wx, wy)
        elseif ent.type == "gem" then
            collectibles_mod.spawn_gem(wx, wy, ent.color or "blue")
        elseif ent.type == "slime" then
            enemies_mod.spawn_slime(wx, wy)
        elseif ent.type == "fly" then
            enemies_mod.spawn_fly(wx, wy)
        elseif ent.type == "bee" then
            enemies_mod.spawn_bee(wx, wy)
        elseif ent.type == "flag" then
            collectibles_mod.spawn_flag(wx, wy)
        end
    end

    -- ── Spawn player ────────────────────────────────────────────────────
    local sx = (level_data.player_spawn.x - 1) * 128
    local sy = (level_data.player_spawn.y - 1) * 128
    state.player = player_mod.spawn(sx, sy)
    camera.set_target(state.player)

    -- Camera bounds
    camera.set_bounds(0, 0, level_data.width * 128, level_data.height * 128)

    -- Update HUD
    hud_mod.update()

    log.info("Loaded level " .. level_num .. ": " .. level_data.name)
end

-- ─── Event wiring ────────────────────────────────────────────────────────────

-- Coin collected
events.on("coin_collected", function()
    state.coins = state.coins + 1
    audio.playSound("coin")
    hud_mod.update()
end)

-- Gem collected
events.on("gem_collected", function()
    state.coins = state.coins + 5
    audio.playSound("gem")
    hud_mod.update()
end)

-- Player hurt
events.on("player_hurt", function()
    state.lives = state.lives - 1
    audio.playSound("hurt")
    hud_mod.update()
    if state.lives <= 0 then
        screens_mod.show_game_over(state.coins)
    end
end)

-- Player fell off level
events.on("player_fell", function()
    state.lives = state.lives - 1
    audio.playSound("hurt")
    hud_mod.update()
    if state.lives <= 0 then
        screens_mod.show_game_over(state.coins)
    else
        -- Respawn at level start
        local level_data = levels[state.current_level]
        local sx = (level_data.player_spawn.x - 1) * 128
        local sy = (level_data.player_spawn.y - 1) * 128
        entity.set_position(state.player, sx, sy)
        entity.set_velocity(state.player, 0, 0)
    end
end)

-- Enemy stomped
events.on("enemy_stomped", function()
    audio.playSound("stomp")
end)

-- Level completed (flag reached)
events.on("level_complete", function()
    audio.playSound("flag")
    if state.current_level >= state.max_levels then
        screens_mod.show_victory(state.coins)
    else
        timer.after(1.0, function()
            load_level(state.current_level + 1)
        end)
    end
end)

-- Pause toggle
events.on("pause_toggle", function()
    if state.paused then
        screens_mod.hide_pause()
        state.paused = false
    else
        screens_mod.show_pause()
        state.paused = true
    end
end)

-- Restart game
events.on("restart_game", function()
    state.lives = 3
    state.coins = 0
    load_level(1)
    hud_mod.update()
end)

-- ─── Per-frame update ────────────────────────────────────────────────────────
events.on("update", function(dt)
    if state.paused then return end
    if not state.player or not entity.is_valid(state.player) then return end

    -- Update player movement and animation
    player_mod.update(state.player, dt)

    -- Pause key
    if input_actions.is_pressed("pause") then
        events.emit("pause_toggle")
        return
    end

    -- ── Collision detection ─────────────────────────────────────────────
    local px, py = entity.get_position(state.player)
    local _, pvy = entity.get_velocity(state.player)

    -- Player hitbox in world space
    local phx = px + PLAYER_HB.ox
    local phy = py + PLAYER_HB.oy
    local phw = PLAYER_HB.w
    local phh = PLAYER_HB.h

    -- Check enemy collisions
    for _, eid in ipairs(enemies_mod.get_active()) do
        if entity.is_valid(eid) then
            local ex, ey = entity.get_position(eid)
            local ehx = ex + ENEMY_HB.ox
            local ehy = ey + ENEMY_HB.oy

            if aabb_overlap(phx, phy, phw, phh, ehx, ehy, ENEMY_HB.w, ENEMY_HB.h) then
                -- Stomp: player moving down and player's feet near enemy's head
                if pvy > 0 and (phy + phh) < (ehy + ENEMY_HB.h * 0.5) then
                    enemies_mod.defeat(eid)
                    player_mod.stomp_bounce(state.player)
                    events.emit("enemy_stomped")
                else
                    player_mod.hurt(state.player)
                end
            end
        end
    end

    -- Check collectible / trigger collisions
    for _, cid in ipairs(collectibles_mod.get_active()) do
        if entity.is_valid(cid) then
            local cx, cy = entity.get_position(cid)
            local name_comp = entity.get_component(cid, "name")
            if not name_comp then goto continue end

            local hb = PICKUP_HB
            if name_comp.name == "flag" then hb = FLAG_HB end

            local chx = cx + hb.ox
            local chy = cy + hb.oy

            if aabb_overlap(phx, phy, phw, phh, chx, chy, hb.w, hb.h) then
                if name_comp.name == "coin" then
                    events.emit("coin_collected")
                    entity.destroy(cid)
                elseif name_comp.name == "gem" then
                    events.emit("gem_collected")
                    entity.destroy(cid)
                elseif name_comp.name == "flag" then
                    events.emit("level_complete")
                end
            end

            ::continue::
        end
    end
end)

-- ─── Start ───────────────────────────────────────────────────────────────────
screens_mod.show_title()
