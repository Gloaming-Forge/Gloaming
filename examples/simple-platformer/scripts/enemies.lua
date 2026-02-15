-- scripts/enemies.lua
-- Enemy spawning and behavior.

local atlas = _G.atlas
local E = atlas.enemies.regions
local ENEMY_SHEET = atlas.enemies.sheet

local enemies_mod = {}

-- Track all active enemies for cleanup
local active_enemies = {}

-- ── Slime ────────────────────────────────────────────────────────────────────

function enemies_mod.spawn_slime(x, y)
    local id = entity.create()
    entity.set_position(id, x, y)
    entity.set_sprite(id, ENEMY_SHEET)
    entity.set_source_rect(id,
        E.slime_normal_walk_a.x, E.slime_normal_walk_a.y,
        E.slime_normal_walk_a.w, E.slime_normal_walk_a.h)

    entity.set_component(id, "collider", {
        width = 100, height = 80,
        offset_x = 14, offset_y = 48,
        layer = "enemy",
        mask = {"tile", "player"}
    })

    entity.set_component(id, "velocity", { x = -60, y = 0 })
    entity.set_component(id, "gravity", { scale = 1.0 })
    entity.set_component(id, "name", { name = "slime", type = "enemy" })
    entity.set_component(id, "health", { current = 1, max = 1 })

    -- Walk animation
    animation.add(id, "walk", {
        fps = 4, mode = "loop",
        rects = { E.slime_normal_walk_a, E.slime_normal_walk_b }
    })
    animation.add(id, "squish", {
        fps = 1, mode = "once",
        rects = { E.slime_normal_flat }
    })
    animation.play(id, "walk")

    -- AI: patrol walk
    enemy_ai.add(id, {
        behavior = "patrol_walk",
        default_behavior = "patrol_walk",
        move_speed = 60,
        patrol_radius = 200,
        contact_damage = 1,
    })

    table.insert(active_enemies, id)
    return id
end

-- ── Fly ──────────────────────────────────────────────────────────────────────

function enemies_mod.spawn_fly(x, y)
    local id = entity.create()
    entity.set_position(id, x, y)
    entity.set_sprite(id, ENEMY_SHEET)
    entity.set_source_rect(id, E.fly_a.x, E.fly_a.y, E.fly_a.w, E.fly_a.h)

    entity.set_component(id, "collider", {
        width = 100, height = 80,
        offset_x = 14, offset_y = 24,
        layer = "enemy",
        mask = {"tile", "player"}
    })

    entity.set_component(id, "name", { name = "fly", type = "enemy" })
    entity.set_component(id, "health", { current = 1, max = 1 })

    -- Fly animation
    animation.add(id, "fly", {
        fps = 6, mode = "loop",
        rects = { E.fly_a, E.fly_b }
    })
    animation.add(id, "rest", {
        fps = 1, mode = "once",
        rects = { E.fly_rest }
    })
    animation.play(id, "fly")

    -- AI: patrol fly (sine wave)
    enemy_ai.add(id, {
        behavior = "patrol_fly",
        default_behavior = "patrol_fly",
        move_speed = 40,
        patrol_radius = 150,
        contact_damage = 1,
    })

    table.insert(active_enemies, id)
    return id
end

-- ── Bee ──────────────────────────────────────────────────────────────────────

function enemies_mod.spawn_bee(x, y)
    local id = entity.create()
    entity.set_position(id, x, y)
    entity.set_sprite(id, ENEMY_SHEET)
    entity.set_source_rect(id, E.bee_a.x, E.bee_a.y, E.bee_a.w, E.bee_a.h)

    entity.set_component(id, "collider", {
        width = 100, height = 80,
        offset_x = 14, offset_y = 24,
        layer = "enemy",
        mask = {"tile", "player"}
    })

    entity.set_component(id, "name", { name = "bee", type = "enemy" })
    entity.set_component(id, "health", { current = 1, max = 1 })

    animation.add(id, "fly", {
        fps = 8, mode = "loop",
        rects = { E.bee_a, E.bee_b }
    })
    animation.play(id, "fly")

    enemy_ai.add(id, {
        behavior = "patrol_fly",
        default_behavior = "patrol_fly",
        move_speed = 60,
        patrol_radius = 200,
        contact_damage = 1,
    })

    table.insert(active_enemies, id)
    return id
end

-- ── Defeat an enemy (called on stomp) ────────────────────────────────────────

function enemies_mod.defeat(enemy_id)
    -- Play squish/rest animation
    local name_comp = entity.get_component(enemy_id, "name")
    if name_comp then
        if name_comp.name == "slime" then
            animation.play(enemy_id, "squish")
        else
            animation.play(enemy_id, "rest")
        end
    end

    -- Disable collider and AI, then remove after delay
    collision.set_enabled(enemy_id, false)
    enemy_ai.remove(enemy_id)
    entity.set_velocity(enemy_id, 0, 0)

    timer.after(0.5, function()
        if entity.is_valid(enemy_id) then
            entity.destroy(enemy_id)
        end
    end)
end

-- ── Access ──────────────────────────────────────────────────────────────────

function enemies_mod.get_active()
    return active_enemies
end

-- ── Cleanup ──────────────────────────────────────────────────────────────────

function enemies_mod.clear_all()
    for _, id in ipairs(active_enemies) do
        if entity.is_valid(id) then
            entity.destroy(id)
        end
    end
    active_enemies = {}
end

return enemies_mod
