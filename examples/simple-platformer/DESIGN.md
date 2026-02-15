# Simple Platformer — Design Document

**Engine:** Gloaming v0.5.0+
**Type:** Example mod / Proof of concept / Tutorial reference
**Assets:** Kenney "New Platformer Pack" (CC0)
**Target:** Desktop + Steam Deck

---

## 1. Purpose

This is the first game built on the Gloaming engine. It serves three goals:

1. **Proof of concept** — Demonstrate that the engine can produce a complete, playable game
2. **Tutorial foundation** — Every design decision here becomes a teaching moment in the accompanying guide
3. **Engine validation** — Exercise core subsystems (rendering, physics, input, audio, mod loading) so bugs surface early

The game is intentionally simple. Complexity would obscure the patterns we want to teach. A new developer should be able to read every file in this mod and understand what it does within an afternoon.

---

## 2. Game Overview

A single-player side-scrolling platformer. The player controls a character who runs, jumps, and collects coins across a series of hand-crafted levels. Enemies patrol platforms and can be defeated by jumping on them. The goal is to reach the flag at the end of each level.

### Core Loop

1. Player spawns at the start of a level
2. Navigate platforms — jump gaps, avoid hazards
3. Collect coins for score
4. Defeat or avoid enemies
5. Reach the end-of-level flag
6. Advance to the next level

### What This Game Is Not

- Not a sandbox or open world — levels are fixed, hand-designed
- Not a combat-heavy game — stomp enemies, no weapons or inventory
- Not a progression system — no upgrades, crafting, or shops
- Not a narrative game — no dialogue, NPCs, or story

These are all engine features that exist but are deliberately excluded to keep the example focused.

---

## 3. Asset Pack

### Kenney "Platformer Pack"

The assets ship as packed sprite sheets with XML atlas descriptors, not individual PNGs.

| Sheet | File | Frame size | Padding | Contents |
|-------|------|-----------|---------|----------|
| **Characters** | `spritesheet-characters-double.png/.xml` | 256x256 | 1px | 5 character colors (beige, green, pink, purple, yellow), 9 poses each: climb_a, climb_b, duck, front, hit, idle, jump, walk_a, walk_b |
| **Enemies** | `spritesheet-enemies-double.png/.xml` | 128x128 | 1px | 15+ enemy types: barnacle, bee, block, fish (3 colors), fly, frog, ladybug, mouse, saw, slime (4 variants), snail, worm (2 variants) |
| **Tiles** | `spritesheet-tiles-double.png/.xml` | 128x128 | 1px | 316 tiles: terrain (6 biomes x full 9-piece blocks, platforms, ramps, clouds), coins (gold/silver/bronze), flags, gems, spikes, springs, HUD elements (hearts, digits, coin icon, player portraits), doors, keys, locks, decorations |
| **Backgrounds** | `spritesheet-backgrounds-double.png/.xml` | 512x512 | 1px | 15 backgrounds: clouds, color/fade variants (desert, hills, mushrooms, trees), solid fills (cloud, dirt, grass, sand, sky) |

**Sounds** (10 OGG files):

| File | Use |
|------|-----|
| `sfx_jump.ogg` | Player jump |
| `sfx_jump-high.ogg` | High jump / stomp bounce |
| `sfx_coin.ogg` | Coin collected |
| `sfx_gem.ogg` | Gem collected (unused, reserve) |
| `sfx_hurt.ogg` | Player takes damage |
| `sfx_bump.ogg` | Hit a wall / blocked |
| `sfx_disappear.ogg` | Enemy defeated |
| `sfx_select.ogg` | Menu selection |
| `sfx_magic.ogg` | Flag / level complete |
| `sfx_throw.ogg` | Unused, reserve |

No music track is included. We will either source a CC0 loop separately or ship without
background music in v1.

### Atlas XML Format

Each `.xml` file maps named sub-textures to pixel rectangles in the sheet:

```xml
<TextureAtlas imagePath="spritesheet-enemies-double.png">
    <SubTexture name="slime_normal_walk_a" x="516" y="645" width="128" height="128"/>
    <SubTexture name="slime_normal_walk_b" x="645" y="645" width="128" height="128"/>
    ...
</TextureAtlas>
```

The engine's `TextureAtlas` class supports `addRegion()` and `addGrid()` with padding
parameters in C++, but does not currently parse XML atlas files. See Section 4.4 for
how the mod bridges this gap.

---

## 4. Technical Design

### 4.1 Mod Manifest

```json
{
    "id": "simple-platformer",
    "name": "Simple Platformer",
    "version": "1.0.0",
    "authors": ["Gloaming-Forge"],
    "description": "A simple platformer example — proof of concept and tutorial reference for the Gloaming engine.",
    "entryPoint": "scripts/init.lua",
    "provides": {
        "content": true,
        "worldgen": false,
        "ui": true,
        "audio": true
    }
}
```

Key decisions:
- **No worldgen** — levels are hand-crafted, not procedural
- **UI provided** — HUD for score, lives, level indicator
- **Audio provided** — sound effects and background music

### 4.2 Game Configuration

```lua
-- Physics: standard platformer gravity
game_mode.set_view("side_view")
game_mode.set_physics("platformer")

-- Camera: smooth follow, slight look-ahead
camera.set_mode("free_follow")
camera.set_smoothness(8.0)
camera.set_deadzone(48, 32)

-- Input: engine-provided platformer defaults
-- Binds: move_left, move_right, jump, interact (for pause/restart)
input_actions.register_platformer_defaults()
```

### 4.3 Tile Size and Scale

The Kenney pack uses 128x128 pixel tiles and 256x256 character sprites.

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| **Tile size** | 128x128 px | Matches Kenney tile grid |
| **Player size** | 256x256 px (2x2 tiles) | Matches Kenney character sprites — square astronaut characters |
| **Enemy size** | 128x128 px (1x1 tile) | Matches Kenney enemy grid |
| **Background size** | 512x512 px | Tiled to fill the viewport |
| **Design resolution** | 1280x720 | Engine default, 10 tiles wide x ~5.6 tiles tall visible |
| **Steam Deck** | 1280x800 | Handled by ViewportScaler, slightly more vertical view |

At 128px tiles with a 1280x720 viewport, the player sees 10 tiles horizontally and ~5.6
vertically. The 256x256 player character occupies 2x2 tiles — prominent but not
overwhelming. Good visibility for a platformer.

### 4.4 Atlas Loading Strategy

The engine does not parse XML atlas files natively. The mod bridges this gap with a
`scripts/atlas.lua` helper that hardcodes the sub-texture coordinates from each XML file
as Lua tables:

```lua
-- scripts/atlas.lua
-- Auto-generated from the Kenney XML atlas files.
-- Each entry maps a sprite name to { x, y, w, h } in the source sheet.

local atlas = {}

atlas.characters = {
    sheet = "reference/Spritesheets/spritesheet-characters-double.png",
    frame_size = 256,  -- all frames are 256x256
    padding = 1,
    regions = {
        character_beige_climb_a = { x = 0,    y = 0,   w = 256, h = 256 },
        character_beige_climb_b = { x = 257,  y = 0,   w = 256, h = 256 },
        character_beige_duck    = { x = 514,  y = 0,   w = 256, h = 256 },
        character_beige_front   = { x = 771,  y = 0,   w = 256, h = 256 },
        character_beige_hit     = { x = 1028, y = 0,   w = 256, h = 256 },
        character_beige_idle    = { x = 1285, y = 0,   w = 256, h = 256 },
        character_beige_jump    = { x = 1542, y = 0,   w = 256, h = 256 },
        character_beige_walk_a  = { x = 0,    y = 257, w = 256, h = 256 },
        character_beige_walk_b  = { x = 257,  y = 257, w = 256, h = 256 },
        -- ... (all 45 character sprites)
    }
}

atlas.enemies = {
    sheet = "reference/Spritesheets/spritesheet-enemies-double.png",
    frame_size = 128,
    padding = 1,
    regions = {
        slime_normal_walk_a = { x = 516, y = 645, w = 128, h = 128 },
        slime_normal_walk_b = { x = 645, y = 645, w = 128, h = 128 },
        slime_normal_flat   = { x = 258, y = 645, w = 128, h = 128 },
        slime_normal_rest   = { x = 387, y = 645, w = 128, h = 128 },
        fly_a               = { x = 258, y = 258, w = 128, h = 128 },
        fly_b               = { x = 387, y = 258, w = 128, h = 128 },
        fly_rest            = { x = 516, y = 258, w = 128, h = 128 },
        -- ... (all 62 enemy sprites)
    }
}

atlas.tiles = {
    sheet = "reference/Spritesheets/spritesheet-tiles-double.png",
    frame_size = 128,
    padding = 1,
    regions = {
        -- ... (all 316 tile sprites)
    }
}

atlas.backgrounds = {
    sheet = "reference/Spritesheets/spritesheet-backgrounds-double.png",
    frame_size = 512,
    padding = 1,
    regions = {
        -- ... (all 15 background sprites)
    }
}

-- Helper: get the source rect for a named sprite
function atlas.rect(sheet_table, name)
    local r = sheet_table.regions[name]
    if not r then
        log.error("Atlas: unknown region '" .. name .. "'")
        return nil
    end
    return r
end

return atlas
```

This is generated once from the XML files and checked into the mod. It avoids needing
an XML parser in Lua and makes sprite lookups fast (table indexing). A future engine
enhancement could add `atlas.load_xml()` as a native Lua binding, which would replace
this file.

---

## 5. Game Elements

### 5.1 Player

We use the **beige** character. Available poses from the atlas:

| Atlas name | Use |
|------------|-----|
| `character_beige_idle` | Standing still |
| `character_beige_walk_a`, `_walk_b` | Walk cycle (2 frames) |
| `character_beige_jump` | Ascending / in-air |
| `character_beige_duck` | Ducking (used for falling — visually distinct from jump) |
| `character_beige_hit` | Taking damage |
| `character_beige_front` | Title screen / HUD portrait |
| `character_beige_climb_a`, `_climb_b` | Ladder climbing (future use) |

**Behavior:**
- Horizontal movement at 350 px/s
- Jump velocity of -550 px/s (engine gravity at 980 px/s² handles the arc)
- Coyote time: 80ms (can jump briefly after leaving a platform edge)
- Jump buffering: 100ms (pressing jump slightly before landing still registers)
- 3 hit points (lives), displayed as hearts on HUD
- Taking damage triggers brief invincibility (1.5s) with sprite flashing
- Falling off the bottom of the level = lose a life, respawn at level start
- 0 lives = game over screen with option to restart

**Components:**
```lua
entity.set_sprite(player, atlas.characters.sheet)

entity.set_component(player, "collider", {
    width = 160,
    height = 200,
    offset_x = 48,
    offset_y = 48,
    layer = "player",
    mask = {"tile", "enemy", "pickup", "hazard", "trigger"}
})

entity.set_component(player, "health", {
    current = 3,
    max = 3
})

entity.set_component(player, "gravity", { scale = 1.0 })
```

The collider is smaller than the 256x256 sprite (160x200) for forgiving gameplay — a
common platformer technique worth teaching. The astronaut character has transparent
padding around the body that makes this natural.

**Animation clips** use atlas coordinates from `scripts/atlas.lua`:
```lua
local A = atlas.characters.regions

-- Single-frame poses: use the atlas rect directly as the source rect
animation.add(player, "idle", { row = 0, frames = 1, fps = 1,  mode = "loop",
    frame_width = 256, frame_height = 256, offset_x = A.character_beige_idle.x, offset_y = A.character_beige_idle.y })

-- Walk cycle: 2 frames from atlas
animation.add(player, "walk", { row = 0, frames = 2, fps = 6,  mode = "loop",
    frame_width = 256, frame_height = 256, offset_x = A.character_beige_walk_a.x, offset_y = A.character_beige_walk_a.y })

animation.add(player, "jump", { row = 0, frames = 1, fps = 1,  mode = "once",
    frame_width = 256, frame_height = 256, offset_x = A.character_beige_jump.x, offset_y = A.character_beige_jump.y })

animation.add(player, "fall", { row = 0, frames = 1, fps = 1,  mode = "once",
    frame_width = 256, frame_height = 256, offset_x = A.character_beige_duck.x, offset_y = A.character_beige_duck.y })

animation.add(player, "hurt", { row = 0, frames = 1, fps = 1,  mode = "once",
    frame_width = 256, frame_height = 256, offset_x = A.character_beige_hit.x, offset_y = A.character_beige_hit.y })
```

> **Note:** The exact animation API for atlas-based frames depends on engine support. If
> `animation.add()` doesn't support `offset_x/offset_y`, we'll set `Sprite.sourceRect`
> directly from the atlas coordinates in the update loop. This is a known integration
> point that will be resolved during implementation (Step 3).

### 5.2 Enemies

Two enemy types, both defeatable by jumping on them from above. The atlas includes
many more enemies (barnacles, bees, frogs, ladybugs, mice, snails, worms, saws) — these
are available for Levels 2 and 3 or future expansion, but we start with two.

**Slime (ground patrol)** — `slime_normal_*`
- Walks left/right on a platform, reverses at edges
- 128x128 sprite, walk animation (2 frames: `slime_normal_walk_a`, `_walk_b`)
- Squished sprite on defeat: `slime_normal_flat`
- Contact with player sides/bottom = player takes 1 damage
- Player landing on top = enemy defeated, player bounces upward
- Uses built-in `patrol_walk` AI behavior

```lua
local E = atlas.enemies.regions
local slime = entity.spawn("enemy", x, y)
entity.set_sprite(slime, atlas.enemies.sheet)
entity.set_component(slime, "collider", {
    width = 100, height = 80,
    offset_x = 14, offset_y = 48,
    layer = "enemy",
    mask = {"tile", "player"}
})
-- Walk animation: 2 frames from atlas
animation.add(slime, "walk", { row = 0, frames = 2, fps = 4, mode = "loop",
    frame_width = 128, frame_height = 128,
    offset_x = E.slime_normal_walk_a.x, offset_y = E.slime_normal_walk_a.y })
animation.play(slime, "walk")
```

**Fly (aerial patrol)** — `fly_*`
- Hovers in a sine-wave pattern
- 128x128 sprite, fly animation (2 frames: `fly_a`, `fly_b`)
- Resting sprite on defeat: `fly_rest`
- Same stomp/damage rules as slime
- Uses built-in `patrol_fly` AI behavior

**Available enemies for later levels:**

| Atlas prefix | Type | Movement | Notes |
|-------------|------|----------|-------|
| `slime_fire_*` | Ground | `patrol_walk` | Red slime, harder variant |
| `slime_spike_*` | Ground | `patrol_walk` | Cannot be stomped (spikes on top) |
| `bee_*` | Aerial | `patrol_fly` | Faster than fly |
| `ladybug_*` | Ground | `patrol_walk` | Can also fly (`ladybug_fly`) |
| `snail_*` | Ground | `patrol_walk` | Retreats to shell when hit |
| `frog_*` | Ground | Jumping | Hops toward player |
| `mouse_*` | Ground | `patrol_walk` | Fast, low profile |
| `saw_*` | Hazard | Fixed path | Rotating saw blade, indestructible |

### 5.3 Collectibles

**Coins** — `coin_gold` / `coin_gold_side` from the tiles atlas
- 128x128 sprite, 2-frame animation (face / side for a simple spin effect)
- Trigger collider (non-solid) — player walks through to collect
- Increments score counter on HUD
- Plays `sfx_coin.ogg` on pickup
- Despawns after collection

```lua
local T = atlas.tiles.regions
local coin = entity.create()
entity.set_position(coin, x, y)
entity.set_sprite(coin, atlas.tiles.sheet)
entity.set_component(coin, "collider", {
    width = 80, height = 80,
    offset_x = 24, offset_y = 24,
    trigger = true,
    layer = "pickup",
    mask = {"player"}
})
-- Source rect set to coin_gold from atlas
-- Animate between coin_gold and coin_gold_side for spin effect
```

**Gems** (bonus collectible) — `gem_blue`, `gem_green`, `gem_red`, `gem_yellow`
- Worth more than coins, placed in hard-to-reach locations
- Plays `sfx_gem.ogg` on pickup
- Optional: introduce in Level 2+

### 5.4 End-of-Level Flag

Atlas provides flags in 4 colors (`flag_green_a`/`_b`, `flag_red_a`/`_b`, etc.) plus
`flag_off` (lowered). We use `flag_green_a`/`_b` for the goal and `flag_off` at start.

- Placed at the end of each level
- 2-frame flag animation (waving)
- Trigger collider — player reaching it completes the level
- Plays `sfx_magic.ogg`, brief celebration pause, then loads next level
- On the final level, triggers a "You Win" screen

### 5.5 Hazards

**Spikes** — `spikes` from the tiles atlas
- 128x128 tile, placed on ground or ceiling
- Contact = instant 1 damage to player
- Plays `sfx_hurt.ogg`
- Static, indestructible

**Lava** — `lava`, `lava_top`, `lava_top_low` from the tiles atlas
- Instant kill on contact
- Optional: introduce in Level 3

**Pits**
- Falling below the level boundary = lose 1 life, respawn at level start

---

## 6. Levels

Three hand-crafted levels of increasing difficulty. Levels are defined as tile data in Lua tables — no procedural generation, no external level editor format. This keeps the tutorial self-contained.

### Level Structure

Each level is a Lua file that returns a table. Tile names reference atlas regions from
the tiles sprite sheet:

```lua
-- scripts/levels/level_1.lua
return {
    name = "Green Hills",
    width = 60,          -- tiles wide
    height = 12,         -- tiles tall
    player_spawn = { x = 2, y = 8 },
    background = "background_color_hills",  -- atlas region name
    biome = "grass",                        -- terrain prefix for auto-tiling

    -- Tile grid: 2D array indexed [row][col], top-to-bottom
    -- 0 = air, 1 = solid terrain (auto-tiled from biome), string = specific tile
    tiles = {
        -- row 1 (top)
        { 0, 0, 0, 0, ... },
        -- ...
        -- row 12 (bottom)
        { 1, 1, 1, 1, ... },
    },

    -- Entity placements (tile coordinates)
    entities = {
        { type = "coin",  x = 5,  y = 7 },
        { type = "coin",  x = 6,  y = 7 },
        { type = "coin",  x = 7,  y = 5 },
        { type = "slime", x = 15, y = 9 },
        { type = "fly",   x = 25, y = 5 },
        { type = "flag",  x = 58, y = 8 },
    }
}
```

The `biome` field selects which terrain set to use for auto-tiling. The atlas provides
6 complete terrain sets, each with 9-piece blocks, horizontal platforms, vertical
columns, ramps, and cloud/floating platforms:

| Biome | Atlas prefix | Visual | Level use |
|-------|-------------|--------|-----------|
| `grass` | `terrain_grass_*` | Green grass on brown earth | Level 1 |
| `stone` | `terrain_stone_*` | Grey stone bricks | Level 2 |
| `sand` | `terrain_sand_*` | Sandy desert | (available) |
| `snow` | `terrain_snow_*` | White snow on grey rock | (available) |
| `dirt` | `terrain_dirt_*` | Brown dirt/mud | (available) |
| `purple` | `terrain_purple_*` | Purple alien terrain | Level 3 |

Each terrain set includes these tile shapes for auto-tiling:

```
block_top_left    block_top     block_top_right
block_left        block_center  block_right
block_bottom_left block_bottom  block_bottom_right

horizontal_left   horizontal_middle   horizontal_right
                  (+ overhang_left, overhang_right)

vertical_top      vertical_middle     vertical_bottom

cloud_left        cloud_middle        cloud_right
                  (+ cloud, cloud_background)

ramp_short_a      ramp_short_b
ramp_long_a       ramp_long_b         ramp_long_c

block             (standalone single tile)
```

### Level 1: "Green Hills"

- **Biome:** `grass` terrain, `background_color_hills` background
- Teaches: moving, jumping, collecting coins
- Flat ground with simple gaps and low platforms
- Cloud platforms (`terrain_grass_cloud_*`) for floating ledges
- A few coins placed to guide the player forward
- Decorations: `bush`, `grass`, `fence`, `sign_right`
- One slime enemy near the middle
- 60 tiles wide (~8 screens of scrolling)
- Forgiving layout — hard to die

### Level 2: "Stone Cavern"

- **Biome:** `stone` terrain, `background_solid_dirt` background
- Teaches: timing, enemy avoidance, vertical platforming
- Mix of ground and elevated platforms
- Spikes introduced as a hazard
- Torches (`torch_on_a`/`_b`) for decoration (animated 2-frame)
- Multiple slimes and one fly enemy
- Some coins on hard-to-reach platforms (risk/reward)
- 80 tiles wide

### Level 3: "Purple Skylands"

- **Biome:** `purple` terrain, `background_fade_mushrooms` background
- Teaches: precision jumping, combining skills
- Floating cloud platforms over pits
- Flies and bees patrolling between platforms
- Spikes on ceilings above narrow passages
- Coins and gems rewarding exploration of optional paths
- 100 tiles wide
- Flag placement feels earned

---

## 7. HUD

Minimal overlay using sprites from the tiles atlas. The atlas provides dedicated HUD
elements:

| Atlas name | Use |
|------------|-----|
| `hud_heart` | Full heart (life) |
| `hud_heart_half` | Half heart |
| `hud_heart_empty` | Lost heart |
| `hud_coin` | Coin icon for score display |
| `hud_player_beige` | Player portrait |
| `hud_character_0` .. `hud_character_9` | Digit sprites for score |
| `hud_character_multiply` | "x" symbol |

Layout:

```
┌──────────────────────────────────────────────┐
│ [portrait] ♥ ♥ ♥      Level 1      [coin] x 12 │
│                                              │
│                 (game world)                 │
│                                              │
└──────────────────────────────────────────────┘
```

| Element | Position | Sprites |
|---------|----------|---------|
| **Player portrait** | Top-left | `hud_player_beige` |
| **Lives** | Top-left (after portrait) | `hud_heart` / `hud_heart_empty` |
| **Level name** | Top-center | Engine text rendering |
| **Coin count** | Top-right | `hud_coin` + `hud_character_*` digit sprites |

Implementation using the engine's UI system:

```lua
ui.register("hud", function()
    return ui.Box({ id = "hud_root", style = {
        width = "100%", height = "auto",
        flex_direction = "row",
        justify_content = "space_between",
        align_items = "center",
        padding = { top = 16, left = 16, right = 16 }
    }}, {
        -- Lives (left): portrait + hearts
        ui.Box({ id = "lives_row", style = { flex_direction = "row", gap = 8, align_items = "center" }}, {
            -- hud_player_beige image + hud_heart images generated dynamically
        }),
        -- Level name (center)
        ui.Text({ id = "level_name", text = "Level 1", style = {
            font_size = 24, text_color = "#FFFFFFFF"
        }}),
        -- Coins (right): coin icon + digit sprites or text
        ui.Box({ id = "coin_display", style = { flex_direction = "row", gap = 8, align_items = "center" }}, {
            ui.Image({ id = "coin_icon", style = { width = 32, height = 32 }}),
            ui.Text({ id = "coin_text", text = "x 0", style = {
                font_size = 24, text_color = "#FFFFFFFF"
            }})
        })
    })
end)
ui.show("hud")
```

---

## 8. Screens

Beyond the HUD, three full-screen UI overlays:

### Title Screen
- Game title centered
- "Press Start" / "Press Any Key" prompt
- Shown on launch, before gameplay begins
- Input glyph adapts to active device (keyboard vs gamepad)

### Pause Screen
- Triggered by pressing Start/Escape
- "Paused" text, "Resume" and "Quit" options
- Game world frozen behind the overlay (semi-transparent background)

### Game Over Screen
- Shown when lives reach 0
- Final score (coin count)
- "Try Again" and "Quit" options

### Victory Screen
- Shown when the final level is completed
- Final score
- "Play Again" and "Quit" options

---

## 9. Audio

Sound effects from the Kenney pack. No background music is included — we will either
source a CC0 music loop separately or ship without music in v1.

| Sound ID | File | Trigger | Volume |
|----------|------|---------|--------|
| `jump` | `reference/Sounds/sfx_jump.ogg` | Player jumps | 0.7 |
| `jump_high` | `reference/Sounds/sfx_jump-high.ogg` | Stomp bounce (enemy defeated) | 0.7 |
| `coin` | `reference/Sounds/sfx_coin.ogg` | Coin collected | 0.8 |
| `gem` | `reference/Sounds/sfx_gem.ogg` | Gem collected | 0.8 |
| `hurt` | `reference/Sounds/sfx_hurt.ogg` | Player takes damage | 0.6 |
| `stomp` | `reference/Sounds/sfx_disappear.ogg` | Enemy defeated (poof) | 0.8 |
| `bump` | `reference/Sounds/sfx_bump.ogg` | Hit a wall or ? block | 0.6 |
| `flag` | `reference/Sounds/sfx_magic.ogg` | Level completed | 0.9 |
| `select` | `reference/Sounds/sfx_select.ogg` | Menu selection | 0.7 |

```lua
-- Registration
audio.registerSound("jump",      "reference/Sounds/sfx_jump.ogg",       { volume = 0.7 })
audio.registerSound("jump_high", "reference/Sounds/sfx_jump-high.ogg",  { volume = 0.7 })
audio.registerSound("coin",      "reference/Sounds/sfx_coin.ogg",       { volume = 0.8 })
audio.registerSound("gem",       "reference/Sounds/sfx_gem.ogg",        { volume = 0.8 })
audio.registerSound("hurt",      "reference/Sounds/sfx_hurt.ogg",       { volume = 0.6 })
audio.registerSound("stomp",     "reference/Sounds/sfx_disappear.ogg",  { volume = 0.8 })
audio.registerSound("bump",      "reference/Sounds/sfx_bump.ogg",       { volume = 0.6 })
audio.registerSound("flag",      "reference/Sounds/sfx_magic.ogg",      { volume = 0.9 })
audio.registerSound("select",    "reference/Sounds/sfx_select.ogg",     { volume = 0.7 })

-- Background music (TODO: source a CC0 loop)
-- audio.playMusic("reference/Sounds/music.ogg", { fade_in = 1.0, loop = true })
```

---

## 10. Steam Deck Considerations

The engine handles most Steam Deck concerns automatically. The mod needs to:

1. **Use `input_actions` exclusively** — never raw key checks, so gamepad works automatically
2. **Use the UI system for text** — minimum font sizes enforced by engine
3. **Keep HUD elements away from screen edges** — 16px padding accounts for safe area
4. **Test at 1280x800** — slightly more vertical space than 1280x720, shouldn't affect layout

The engine's `ViewportScaler` handles resolution differences. The `InputGlyphs` system shows correct button prompts. Suspend/resume is automatic. No additional work needed in the mod.

---

## 11. Development Workflow

### 11.1 Building and Running

The engine must be built before running the example. From the repository root:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Once the engine is built, use the provided run scripts to symlink the example into the
engine's `mods/` directory and launch:

```bash
# Linux / macOS
./examples/simple-platformer/run.sh

# Windows (Command Prompt or PowerShell)
examples\simple-platformer\run.bat
```

The scripts are idempotent — safe to run repeatedly. They create a symbolic link from
`mods/simple-platformer` to `examples/simple-platformer` so the engine discovers the mod
without copying files. Edits to scripts and assets take effect on the next engine launch
(or on hot-reload once that feature is integrated).

### 11.2 Debugging

**Log output** is the primary debugging tool. The engine uses dual loggers:

| Logger tag | Source | Example |
|------------|--------|---------|
| `[ENGINE]` | C++ engine code | `[ENGINE] [error] Failed to load texture: path/missing.png` |
| `[MOD]` | Lua mod scripts | `[MOD] [error] [simple-platformer] init() error: scripts/player.lua:42: attempt to index nil value` |

Logs go to both **stdout** (colored) and **gloaming.log** (timestamped). Use `log.*`
calls liberally during development:

```lua
log.info("Player spawned at " .. x .. ", " .. y)
log.debug("Coin count: " .. coins)
log.warn("Enemy fell off level at y=" .. ey)
```

**F2 diagnostic overlay** cycles through three modes:

| Mode | Shows |
|------|-------|
| Off | Nothing |
| Minimal | FPS, frame time, budget bar (green/yellow/red) |
| Full | Profiler zones, frame graph, entity counts, system stats |

Use Minimal to spot frame drops. Use Full to see which system is expensive (physics,
rendering, AI, etc.) and to verify entity counts match expectations.

### 11.3 Is It a Mod Bug or an Engine Bug?

| Symptom | Likely source | What to check |
|---------|---------------|---------------|
| Log says `[MOD] [error]` with a `.lua` file and line number | **Mod** | Read the Lua error message — it includes the script path and line |
| Log says `[ENGINE] [error]` | **Engine** | File an engine issue with the log output |
| Engine crashes (segfault, abort) | **Engine** | Mod errors are caught by sol2 and never crash the engine |
| Infinite hang, then `instruction limit exceeded` | **Mod** | You have a `while true` loop or unbounded recursion in Lua |
| Wrong physics / collision behavior | **Either** | Add `log.debug` around the suspicious logic. If the Lua values are correct but behavior is wrong, it's engine. If the values are wrong, it's mod. |
| Texture missing or white square | **Mod (usually)** | Check that the asset path in your Lua/JSON matches the actual file path — paths are relative to the mod root |

The engine wraps every Lua call (init, postInit, event handlers) in sol2 protected
functions. A Lua error will never crash the engine — it logs the error, marks the mod as
`Failed`, and continues. If the engine itself crashes, that's always an engine bug.

### 11.4 Testing

**Automated Lua tests are not needed for this example.** The engine has C++ unit tests
(GoogleTest) that validate the mod system, content registry, and Lua bindings:

```bash
cmake --build build --target gloaming_tests && ctest --test-dir build
```

If those pass, the engine contract is solid. Gameplay correctness (does the player bounce
when stomping an enemy?) is validated through manual playtesting, which the run scripts
make fast to iterate on.

For future mods that need automated Lua tests, the engine would need a headless mode
(no window). That's an engine feature request, not something to hack into a mod.

### 11.5 Iteration Cycle

The fastest development loop:

1. Edit a `.lua` script or `.json` content file
2. Re-run `./examples/simple-platformer/run.sh` (or `run.bat`)
3. Check the console output for `[MOD] [error]` lines
4. Press **F2** in-game to verify entity counts and frame budget
5. Repeat

No rebuild is needed for Lua/JSON/asset changes — only the engine binary requires
`cmake --build`. The symlink means your edits are live in the `mods/` directory
immediately.

---

## 12. File Structure

> The run scripts (`run.sh`, `run.bat`) live at the mod root alongside `mod.json`.

```
examples/simple-platformer/
├── DESIGN.md                    # This document
├── mod.json                     # Mod manifest
├── run.sh                       # Linux/macOS: symlink into mods/ and launch engine
├── run.bat                      # Windows: same as run.sh
├── reference/                   # Kenney asset pack (CC0) — used as-is
│   ├── Spritesheets/
│   │   ├── spritesheet-characters-double.png   # 5 characters x 9 poses (256x256)
│   │   ├── spritesheet-characters-double.xml   # Atlas: 45 named regions
│   │   ├── spritesheet-enemies-double.png      # 15+ enemy types (128x128)
│   │   ├── spritesheet-enemies-double.xml      # Atlas: 62 named regions
│   │   ├── spritesheet-tiles-double.png        # Terrain, items, HUD (128x128)
│   │   ├── spritesheet-tiles-double.xml        # Atlas: 316 named regions
│   │   ├── spritesheet-backgrounds-double.png  # Background tiles (512x512)
│   │   └── spritesheet-backgrounds-double.xml  # Atlas: 15 named regions
│   └── Sounds/
│       ├── sfx_bump.ogg
│       ├── sfx_coin.ogg
│       ├── sfx_disappear.ogg
│       ├── sfx_gem.ogg
│       ├── sfx_hurt.ogg
│       ├── sfx_jump-high.ogg
│       ├── sfx_jump.ogg
│       ├── sfx_magic.ogg
│       ├── sfx_select.ogg
│       └── sfx_throw.ogg
├── scripts/
│   ├── init.lua                 # Entry point — config, content loading, event wiring
│   ├── atlas.lua                # Sprite sheet atlas data (generated from XMLs)
│   ├── player.lua               # Player spawning, movement, animation state machine
│   ├── enemies.lua              # Enemy spawning, stomp detection
│   ├── collectibles.lua         # Coin and flag behavior
│   ├── hud.lua                  # HUD layout and updates
│   ├── screens.lua              # Title, pause, game over, victory screens
│   └── levels/
│       ├── level_1.lua          # "Green Hills" tile data and entity placements
│       ├── level_2.lua          # "Stone Cavern" tile data and entity placements
│       └── level_3.lua          # "Purple Skylands" tile data and entity placements
└── content/
    └── tiles.json               # Tile definitions (terrain, hazards)
```

---

## 13. Scope Boundaries

Things this example deliberately does **not** include, and why:

| Feature | Why excluded |
|---------|-------------|
| Procedural world generation | Levels are hand-crafted for tutorial clarity |
| Inventory / items / tools | Unnecessary for a simple platformer |
| Crafting | Not relevant to the game loop |
| NPCs / dialogue | Adds narrative complexity we don't need |
| Day/night cycle | Visual feature that doesn't serve the tutorial |
| Multiplayer | Future engine feature, not ready |
| Save/load | Three short levels — no save needed |
| Shop system | No economy in this game |

Each excluded feature is an engine capability that a more complex mod could use. The tutorial guide will reference these as "next steps" for readers who want to extend the example.

---

## 14. Implementation Order

Suggested build order, where each step produces a testable result:

| Step | Deliverable | What you can test |
|------|-------------|-------------------|
| 1 | `mod.json` + `init.lua` with physics/camera/input setup | Engine loads the mod, empty world with correct physics |
| 2 | `scripts/atlas.lua` — sprite sheet atlas data from XMLs | Atlas lookups work, log a few region names to verify |
| 3 | `content/tiles.json` + terrain tiles using atlas regions | Place tiles, see them render from sprite sheets |
| 4 | `scripts/player.lua` — spawn, movement, animation from atlas | Run and jump on tiles with character sprite |
| 5 | `scripts/levels/level_1.lua` — first level layout | Play through a real level with auto-tiled terrain |
| 6 | `scripts/collectibles.lua` — coins and flag | Collect coins, complete level |
| 7 | `scripts/enemies.lua` — slime and fly from atlas | Enemies patrol and can be stomped |
| 8 | `scripts/hud.lua` — lives, coins, level name using HUD sprites | See score and health with atlas heart/coin icons |
| 9 | `scripts/screens.lua` — title, pause, game over, victory | Full game flow |
| 10 | Levels 2 and 3 (stone, purple biomes) | Complete game with progression |
| 11 | Audio — sounds | Polish pass with all SFX |
| 12 | Steam Deck testing | Verify gamepad, scaling, glyphs |

---

## 15. Tutorial Guide Outline

The accompanying tutorial (to be written after implementation) will follow the
implementation order above. Each chapter corresponds to one step:

1. **Setting Up a Mod** — mod.json, init.lua, running the engine
2. **Working with Sprite Sheets** — atlas.lua, XML format, sub-texture lookups
3. **Building a World** — tile definitions, terrain biomes, auto-tiling
4. **The Player** — entities, components, movement, animation from atlas
5. **Designing Levels** — level data format, loading, transitions
6. **Collectibles and Goals** — trigger colliders, events, scoring
7. **Enemies** — AI behaviors, collision response, stomp mechanics
8. **Heads-Up Display** — UI system, HUD sprites, dynamic updates
9. **Game Flow** — screens, state management, pause/resume
10. **More Levels** — new biomes, difficulty curve, introducing new elements
11. **Sound Effects** — audio registration, playback, event binding
12. **Steam Deck** — testing, input glyphs, what the engine handles for you

Each chapter will include the complete source code for that step, explanation of every
API call used, and a "try it yourself" exercise.
