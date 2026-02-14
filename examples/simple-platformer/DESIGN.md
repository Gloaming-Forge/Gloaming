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

### Kenney "New Platformer Pack"

| Detail | Value |
|--------|-------|
| **Asset count** | ~440 separate PNGs + spritesheets |
| **Tile size** | 128x128 pixels |
| **Player sprite** | 128x256 pixels (multiple poses) |
| **License** | CC0 1.0 (public domain) |
| **Contents** | Terrain tiles, player characters, enemies, items, HUD elements, backgrounds, sounds |

### Asset Organization

Assets from the pack should be placed in the following structure:

```
examples/simple-platformer/
└── assets/
    ├── sprites/
    │   ├── player/          # Player character poses (idle, walk, jump, hurt)
    │   └── enemies/         # Enemy sprites (walk, squished)
    ├── tiles/
    │   ├── terrain/         # Ground, grass, dirt, stone, sand
    │   ├── decoration/      # Bushes, signs, fences
    │   └── interactive/     # Coins, flag, spring, spikes
    ├── backgrounds/         # Parallax background layers
    ├── hud/                 # Hearts, coin icon, number sprites
    └── sounds/
        ├── jump.ogg
        ├── coin.ogg
        ├── stomp.ogg
        ├── hurt.ogg
        ├── flag.ogg
        └── music.ogg
```

> **Note to asset organizer:** Copy the relevant PNGs from the Kenney pack into this
> structure. Rename files to be descriptive (e.g., `player_idle.png` not `tile_0012.png`).
> The exact filenames will be referenced in the content JSON and Lua scripts.

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

The Kenney pack uses 128x128 pixel tiles. For the game world:

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| **Tile size** | 128x128 px | Matches Kenney asset grid |
| **Player size** | 128x256 px (1x2 tiles) | Matches Kenney player sprites |
| **Design resolution** | 1280x720 | Engine default, 10 tiles wide x ~5.6 tiles tall visible |
| **Steam Deck** | 1280x800 | Handled by ViewportScaler, slightly more vertical view |

At 128px tiles with a 1280x720 viewport, the player sees 10 tiles horizontally and ~5.6 vertically. This gives good visibility for a platformer without making the player sprite too small.

---

## 5. Game Elements

### 5.1 Player

**Sprites:** idle, walk (2-4 frames), jump (ascending), fall (descending), hurt

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
entity.set_component(player, "collider", {
    width = 80,
    height = 200,
    offset_x = 24,
    offset_y = 56,
    layer = "player",
    mask = {"tile", "enemy", "pickup", "hazard", "trigger"}
})

entity.set_component(player, "health", {
    current = 3,
    max = 3
})

entity.set_component(player, "gravity", { scale = 1.0 })
```

The collider is slightly smaller than the sprite (80x200 vs 128x256) for forgiving gameplay — a common platformer technique worth teaching.

**Animation clips:**
```lua
animation.add(player, "idle",  { sheet = "sprites/player/player_idle.png",  frames = 1, fps = 1,  mode = "loop", frame_width = 128, frame_height = 256 })
animation.add(player, "walk",  { sheet = "sprites/player/player_walk.png",  frames = 4, fps = 8,  mode = "loop", frame_width = 128, frame_height = 256 })
animation.add(player, "jump",  { sheet = "sprites/player/player_jump.png",  frames = 1, fps = 1,  mode = "once", frame_width = 128, frame_height = 256 })
animation.add(player, "fall",  { sheet = "sprites/player/player_fall.png",  frames = 1, fps = 1,  mode = "once", frame_width = 128, frame_height = 256 })
animation.add(player, "hurt",  { sheet = "sprites/player/player_hurt.png",  frames = 1, fps = 1,  mode = "once", frame_width = 128, frame_height = 256 })
```

### 5.2 Enemies

Two enemy types, both defeatable by jumping on them from above.

**Slime (ground patrol)**
- Walks left/right on a platform, reverses at edges
- 128x128 sprite, walk animation (2 frames)
- Contact with player sides/bottom = player takes 1 damage
- Player landing on top = enemy defeated, player bounces upward
- Uses built-in `patrol_walk` AI behavior

```lua
local slime = entity.spawn("enemy", x, y)
entity.set_component(slime, "collider", {
    width = 100, height = 100,
    offset_x = 14, offset_y = 28,
    layer = "enemy",
    mask = {"tile", "player"}
})
animation.add(slime, "walk", { sheet = "sprites/enemies/slime_walk.png", frames = 2, fps = 4, mode = "loop", frame_width = 128, frame_height = 128 })
animation.play(slime, "walk")
```

**Fly (aerial patrol)**
- Hovers in a sine-wave pattern
- 128x128 sprite, fly animation (2 frames)
- Same stomp/damage rules as slime
- Uses built-in `patrol_fly` AI behavior

### 5.3 Collectibles

**Coins**
- 128x128 sprite, static or simple rotation (2-4 frames)
- Trigger collider (non-solid) — player walks through to collect
- Increments score counter on HUD
- Plays coin sound on pickup
- Despawns after collection

```lua
local coin = entity.create()
entity.set_position(coin, x, y)
entity.set_sprite(coin, "tiles/interactive/coin.png")
entity.set_component(coin, "collider", {
    width = 80, height = 80,
    offset_x = 24, offset_y = 24,
    trigger = true,
    layer = "pickup",
    mask = {"player"}
})
```

### 5.4 End-of-Level Flag

- Placed at the end of each level
- Trigger collider — player reaching it completes the level
- Plays flag sound, brief celebration pause, then loads next level
- On the final level, triggers a "You Win" screen

### 5.5 Hazards

**Spikes**
- 128x128 tile, placed on ground or ceiling
- Contact = instant 1 damage to player
- Static, indestructible

**Pits**
- Falling below the level boundary = lose 1 life, respawn at level start

---

## 6. Levels

Three hand-crafted levels of increasing difficulty. Levels are defined as tile data in Lua tables — no procedural generation, no external level editor format. This keeps the tutorial self-contained.

### Level Structure

Each level is a Lua file that returns a table:

```lua
-- scripts/levels/level_1.lua
return {
    name = "Green Hills",
    width = 60,          -- tiles wide
    height = 12,         -- tiles tall
    player_spawn = { x = 2, y = 8 },
    background = "backgrounds/hills.png",
    music = "sounds/music.ogg",

    -- Tile grid: 2D array indexed [row][col], top-to-bottom
    -- 0 = air, tile IDs reference content/tiles.json
    tiles = {
        -- row 1 (top)
        { 0, 0, 0, 0, ... },
        -- ...
        -- row 12 (bottom)
        { 1, 1, 1, 1, ... },
    },

    -- Entity placements
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

### Level 1: "Green Hills"

- Teaches: moving, jumping, collecting coins
- Flat ground with simple gaps and low platforms
- A few coins placed to guide the player forward
- One slime enemy near the middle
- 60 tiles wide (~8 screens of scrolling)
- Forgiving layout — hard to die

### Level 2: "Underground"

- Teaches: timing, enemy avoidance, vertical platforming
- Mix of ground and elevated platforms
- Spikes introduced as a hazard
- Multiple slimes and one fly enemy
- Some coins on hard-to-reach platforms (risk/reward)
- 80 tiles wide

### Level 3: "Sky Fortress"

- Teaches: precision jumping, combining skills
- Floating platforms over pits
- Flies patrolling between platforms
- Spikes on ceilings above narrow passages
- Coins rewarding exploration of optional paths
- 100 tiles wide
- Flag placement feels earned

---

## 7. HUD

Minimal overlay showing essential information:

```
┌──────────────────────────────────────┐
│ ♥ ♥ ♥          Level 1       🪙 x 12 │
│                                      │
│                                      │
│            (game world)              │
│                                      │
│                                      │
└──────────────────────────────────────┘
```

| Element | Position | Content |
|---------|----------|---------|
| **Lives** | Top-left | Heart icons, filled = alive, empty = lost |
| **Level name** | Top-center | Current level name |
| **Coin count** | Top-right | Coin icon + count |

Implementation using the engine's UI system:

```lua
ui.register("hud", function()
    return ui.Box({ id = "hud_root", style = {
        width = "100%", height = "auto",
        flex_direction = "row",
        justify_content = "space_between",
        padding = { top = 16, left = 16, right = 16 }
    }}, {
        -- Lives (left)
        ui.Box({ id = "lives_row", style = { flex_direction = "row", gap = 8 }}, {
            -- heart images generated dynamically
        }),
        -- Level name (center)
        ui.Text({ id = "level_name", text = "Level 1", style = {
            font_size = 24, text_color = "#FFFFFFFF"
        }}),
        -- Coins (right)
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

Minimal sound design using the Kenney pack sounds plus any additional CC0 sounds needed.

| Sound | Trigger | Notes |
|-------|---------|-------|
| **Jump** | Player jumps | Short, snappy |
| **Coin** | Coin collected | Bright chime |
| **Stomp** | Enemy defeated | Satisfying pop |
| **Hurt** | Player takes damage | Brief negative tone |
| **Flag** | Level completed | Celebratory jingle |
| **Music** | Background loop | Light, upbeat, per-level or shared |

```lua
-- Registration
audio.registerSound("jump",  "sounds/jump.ogg",  { volume = 0.7 })
audio.registerSound("coin",  "sounds/coin.ogg",  { volume = 0.8 })
audio.registerSound("stomp", "sounds/stomp.ogg", { volume = 0.8 })
audio.registerSound("hurt",  "sounds/hurt.ogg",  { volume = 0.6 })
audio.registerSound("flag",  "sounds/flag.ogg",  { volume = 0.9 })

-- Background music
audio.playMusic("sounds/music.ogg", { fade_in = 1.0, loop = true })
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

## 11. File Structure

```
examples/simple-platformer/
├── DESIGN.md                    # This document
├── mod.json                     # Mod manifest
├── scripts/
│   ├── init.lua                 # Entry point — config, content loading, event wiring
│   ├── player.lua               # Player spawning, movement, animation state machine
│   ├── enemies.lua              # Enemy spawning, stomp detection
│   ├── collectibles.lua         # Coin and flag behavior
│   ├── hud.lua                  # HUD layout and updates
│   ├── screens.lua              # Title, pause, game over, victory screens
│   └── levels/
│       ├── level_1.lua          # "Green Hills" tile data and entity placements
│       ├── level_2.lua          # "Underground" tile data and entity placements
│       └── level_3.lua          # "Sky Fortress" tile data and entity placements
├── content/
│   └── tiles.json               # Tile definitions (terrain, hazards)
└── assets/
    ├── sprites/
    │   ├── player/              # Player character sprite sheets
    │   └── enemies/             # Enemy sprite sheets
    ├── tiles/
    │   ├── terrain/             # Ground, platform, wall tiles
    │   ├── decoration/          # Visual-only tiles (bushes, signs)
    │   └── interactive/         # Coins, flag, spikes, spring
    ├── backgrounds/             # Parallax background layers
    ├── hud/                     # Heart icons, coin icon
    └── sounds/
        ├── jump.ogg
        ├── coin.ogg
        ├── stomp.ogg
        ├── hurt.ogg
        ├── flag.ogg
        └── music.ogg
```

---

## 12. Scope Boundaries

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

## 13. Implementation Order

Suggested build order, where each step produces a testable result:

| Step | Deliverable | What you can test |
|------|-------------|-------------------|
| 1 | `mod.json` + `init.lua` with physics/camera/input setup | Engine loads the mod, empty world with correct physics |
| 2 | `content/tiles.json` + terrain tiles | Place tiles, see them render |
| 3 | `scripts/player.lua` — spawn, movement, animation | Run and jump on tiles |
| 4 | `scripts/levels/level_1.lua` — first level layout | Play through a real level |
| 5 | `scripts/collectibles.lua` — coins and flag | Collect coins, complete level |
| 6 | `scripts/enemies.lua` — slime and fly | Enemies patrol and can be stomped |
| 7 | `scripts/hud.lua` — lives, coins, level name | See score and health |
| 8 | `scripts/screens.lua` — title, pause, game over, victory | Full game flow |
| 9 | Levels 2 and 3 | Complete game with progression |
| 10 | Audio — sounds and music | Polish pass |
| 11 | Steam Deck testing | Verify gamepad, scaling, glyphs |

---

## 14. Tutorial Guide Outline

The accompanying tutorial (to be written after implementation) will follow the implementation order above. Each chapter corresponds to one step:

1. **Setting Up a Mod** — mod.json, init.lua, running the engine
2. **Building a World** — tile definitions, placing terrain
3. **The Player** — entities, components, movement, animation
4. **Designing Levels** — level data format, loading, transitions
5. **Collectibles and Goals** — trigger colliders, events, scoring
6. **Enemies** — AI behaviors, collision response, stomp mechanics
7. **Heads-Up Display** — UI system, dynamic updates
8. **Game Flow** — screens, state management, pause/resume
9. **More Levels** — difficulty curve, introducing new elements
10. **Sound and Music** — audio registration, playback, events
11. **Steam Deck** — testing, input glyphs, what the engine handles for you

Each chapter will include the complete source code for that step, explanation of every API call used, and a "try it yourself" exercise.
