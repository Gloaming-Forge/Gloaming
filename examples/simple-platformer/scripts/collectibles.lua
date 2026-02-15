-- scripts/collectibles.lua
-- Coins, gems, and end-of-level flag.

local atlas = _G.atlas
local T = atlas.tiles.regions
local TILE_SHEET = atlas.tiles.sheet

local collectibles_mod = {}

-- Track active collectibles for level cleanup
local active_collectibles = {}

-- ── Coins ────────────────────────────────────────────────────────────────────

function collectibles_mod.spawn_coin(x, y)
    local id = entity.create()
    entity.set_position(id, x, y)
    entity.set_sprite(id, TILE_SHEET)
    entity.set_source_rect(id, T.coin_gold.x, T.coin_gold.y, T.coin_gold.w, T.coin_gold.h)

    entity.set_component(id, "collider", {
        width = 80, height = 80,
        offset_x = 24, offset_y = 24,
        trigger = true,
        layer = "pickup",
        mask = {"player"}
    })

    entity.set_component(id, "name", { name = "coin", type = "pickup" })

    -- Simple spin animation
    animation.add(id, "spin", {
        fps = 3, mode = "loop",
        rects = { T.coin_gold, T.coin_gold_side }
    })
    animation.play(id, "spin")

    table.insert(active_collectibles, id)
    return id
end

-- ── Gems ─────────────────────────────────────────────────────────────────────

local gem_regions = {
    blue   = T.gem_blue,
    green  = T.gem_green,
    red    = T.gem_red,
    yellow = T.gem_yellow,
}

function collectibles_mod.spawn_gem(x, y, color)
    color = color or "blue"
    local region = gem_regions[color] or gem_regions.blue

    local id = entity.create()
    entity.set_position(id, x, y)
    entity.set_sprite(id, TILE_SHEET)
    entity.set_source_rect(id, region.x, region.y, region.w, region.h)

    entity.set_component(id, "collider", {
        width = 80, height = 80,
        offset_x = 24, offset_y = 24,
        trigger = true,
        layer = "pickup",
        mask = {"player"}
    })

    entity.set_component(id, "name", { name = "gem", type = "pickup" })

    table.insert(active_collectibles, id)
    return id
end

-- ── Flag ─────────────────────────────────────────────────────────────────────

function collectibles_mod.spawn_flag(x, y)
    local id = entity.create()
    entity.set_position(id, x, y)
    entity.set_sprite(id, TILE_SHEET)
    entity.set_source_rect(id, T.flag_green_a.x, T.flag_green_a.y,
                            T.flag_green_a.w, T.flag_green_a.h)

    entity.set_component(id, "collider", {
        width = 80, height = 128,
        offset_x = 24, offset_y = 0,
        trigger = true,
        layer = "trigger",
        mask = {"player"}
    })

    entity.set_component(id, "name", { name = "flag", type = "trigger" })

    -- Waving animation
    animation.add(id, "wave", {
        fps = 4, mode = "loop",
        rects = { T.flag_green_a, T.flag_green_b }
    })
    animation.play(id, "wave")

    table.insert(active_collectibles, id)
    return id
end

-- ── Access ──────────────────────────────────────────────────────────────────

function collectibles_mod.get_active()
    return active_collectibles
end

-- ── Cleanup ──────────────────────────────────────────────────────────────────

function collectibles_mod.clear_all()
    for _, id in ipairs(active_collectibles) do
        if entity.is_valid(id) then
            entity.destroy(id)
        end
    end
    active_collectibles = {}
end

return collectibles_mod
