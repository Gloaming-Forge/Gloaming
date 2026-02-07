# Gloaming Engine Specification
### Version 0.4 — Multi-Genre 2D Game Engine

> **Vision Statement:** An open-source, moddable 2D game engine. We build the platform — the community builds the games.

**Organization:** [Gloaming-Forge](https://github.com/Gloaming-Forge)

---

## Table of Contents

1. [Philosophy & Approach](#1-philosophy--approach)
2. [Project Overview](#2-project-overview)
3. [Technical Architecture](#3-technical-architecture)
4. [Core Engine Systems](#4-core-engine-systems)
5. [Gameplay Systems](#5-gameplay-systems)
6. [Modding Framework](#6-modding-framework)
7. [Mod Distribution Ecosystem](#7-mod-distribution-ecosystem)
8. [Development Stages](#8-development-stages)
9. [Documentation Requirements](#9-documentation-requirements)
10. [Community Strategy](#10-community-strategy)
11. [Distribution & Licensing](#11-distribution--licensing)
12. [Open Questions](#12-open-questions)

---

## 1. Philosophy & Approach

### 1.1 The Core Insight

We are not building a game. **We are building a platform for games.**

The engine provides infrastructure for any style of 2D game. Content, UI, audio, world generation, and game rules are all provided by mods. Each game built on Gloaming is a mod (or collection of mods) that ships on top of the engine.

### 1.2 The Split

| Layer | What It Is | Who Builds It |
|-------|------------|---------------|
| **Engine** | Infrastructure, primitives, mod loading | Us (core team) |
| **Game Mods** | Complete games built on the engine | Us + community |
| **Content Mods** | Extensions to existing games | Community |
| **Total Conversions** | Entirely different games | Community |

### 1.3 Design Principles

| Principle | Meaning |
|-----------|---------|
| **Game-agnostic engine** | The engine makes no assumptions about game genre — top-down, side-view, or hybrid |
| **Mods are first-class** | All games use the same APIs as community mods |
| **Data over code** | JSON/config files wherever possible; scripting for complex behaviors |
| **Composition over inheritance** | Mix-and-match components, not rigid class hierarchies |
| **Hot-reload everything** | Change a file, see results immediately (debug builds) |
| **Document as you build** | Undocumented features don't exist to modders |
| **Fail gracefully** | Bad mod data logs warnings, doesn't crash |

### 1.4 Target Game Styles

The engine is designed to support any 2D game style. These are the initial proof-of-concept targets:

| Game Style | Example | Key Engine Features Used |
|------------|---------|------------------------|
| **Side-scrolling sandbox** | Terraria | Gravity, slopes, one-way platforms, mining, day/night lighting |
| **Top-down RPG** | Pokemon Leaf Green | Grid movement, dialogue system, tile layers, room-based camera |
| **Side-scrolling flight** | Sopwith | Auto-scroll camera, flight physics, projectiles, terrain collision |

Each game is a mod — the engine provides the systems, the mod provides the rules and content.

### 1.5 What This Enables

- **Side-scrolling platformers** — Terraria, Metroidvania, run-and-gun
- **Top-down RPGs** — Pokemon, Zelda, action RPGs
- **Flight/shooters** — Sopwith, shoot-em-ups, space games
- **Total conversions** — Sci-fi, fantasy, horror, anything
- **Different world types** — Infinite scrolling, discrete rooms, open world
- **UI overhauls** — Completely different inventory systems, HUD styles
- **Content packs** — Expansion content for any game built on the engine
- **Quality of life mods** — Minimap, recipe browser, auto-sort

---

## 2. Project Overview

### 2.1 Elevator Pitch

"A moddable 2D game engine that supports any style of game — platformers, RPGs, shooters, sandboxes. Open source. Infinitely extensible."

### 2.2 Target Audience

| Audience | What They Want |
|----------|----------------|
| **Players** | Fun games, mod variety, visual polish |
| **Game Creators** | Easy content creation, good docs, powerful APIs |
| **Engine Contributors** | Clean architecture, contribution opportunities |
| **Artists** | Platform to showcase work, asset packs |

### 2.3 Success Metrics

| Metric | Target |
|--------|--------|
| Engine MVP | Multiple game styles demonstrably functional |
| Documentation | 100% of mod APIs documented with examples |
| Community | First third-party game mod within 1 month of release |
| Performance | 60 FPS with 10k entities on mid-range hardware |

### 2.4 Target Platforms

- **Primary:** Windows (64-bit)
- **Secondary:** Linux, macOS
- **Stretch:** Steam Deck verified

---

## 3. Technical Architecture

### 3.1 Technology Stack

| Component | Choice | Rationale |
|-----------|--------|-----------|
| **Language** | C++20 | Performance, control |
| **Build System** | CMake | Cross-platform standard |
| **Rendering** | Raylib (abstracted) | Fast iteration; abstract for future Vulkan |
| **ECS** | EnTT | Header-only, excellent performance |
| **Scripting** | Lua 5.4 (sol2) | Industry standard for game modding |
| **Data Format** | JSON + binary | JSON for mods, binary for world saves |
| **Audio** | miniaudio (via Raylib) | Lightweight, cross-platform |
| **Networking** | ENet (future) | Reliable UDP |

### 3.2 Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                          GAMES LAYER                              │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌───────────┐  │
│  │  Sandbox    │ │ Top-Down    │ │ Flight      │ │ Community │  │
│  │  (Terraria) │ │ RPG (Pkmn)  │ │ (Sopwith)   │ │ Games     │  │
│  └─────────────┘ └─────────────┘ └─────────────┘ └───────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                        MOD API LAYER                              │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌───────────┐  │
│  │ Content │ │ World   │ │ UI      │ │ Audio   │ │ Gameplay  │  │
│  │ Registry│ │ Gen API │ │ API     │ │ API     │ │ APIs      │  │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └───────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                     GAMEPLAY SYSTEMS                              │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌───────────┐  │
│  │ Grid    │ │ Camera  │ │ Path-   │ │ State   │ │ Dialogue  │  │
│  │Movement │ │ Control │ │ finding │ │ Machine │ │ System    │  │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └───────────┘  │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐                             │
│  │ Input   │ │ Tile    │ │ Game    │                             │
│  │ Actions │ │ Layers  │ │ Mode    │                             │
│  └─────────┘ └─────────┘ └─────────┘                             │
├─────────────────────────────────────────────────────────────────┤
│                       ENGINE CORE                                 │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌───────────┐  │
│  │ ECS     │ │Renderer │ │ Physics │ │ Audio   │ │ Mod       │  │
│  │ (EnTT)  │ │(Raylib) │ │ System  │ │ System  │ │ Loader    │  │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └───────────┘  │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌───────────┐  │
│  │ Window  │ │ Input   │ │ Chunk   │ │Lighting │ │ UI Layout │  │
│  │ Manager │ │ System  │ │ Manager │ │ System  │ │ Engine    │  │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └───────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

### 3.3 Project Structure

```
gloaming/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── docs/
│   ├── ENGINE_SPEC.md              # This document
│   └── GAME_TERRARIA_SPEC.md       # Terraria-clone game design
├── src/
│   ├── main.cpp
│   ├── engine/                 # Core engine (hardcoded)
│   │   ├── Engine.hpp/.cpp
│   │   ├── Window.hpp/.cpp
│   │   ├── Input.hpp/.cpp
│   │   ├── Config.hpp/.cpp
│   │   ├── Log.hpp/.cpp
│   │   └── Time.hpp/.cpp
│   ├── ecs/
│   │   ├── Registry.hpp        # EnTT wrapper
│   │   ├── Components.hpp      # Core components
│   │   ├── Systems.hpp         # System scheduling
│   │   ├── CoreSystems.hpp     # Built-in systems
│   │   └── EntityFactory.hpp   # JSON-based entity spawning
│   ├── rendering/
│   │   ├── IRenderer.hpp       # Abstract interface
│   │   ├── RaylibRenderer.hpp
│   │   ├── Camera.hpp
│   │   ├── SpriteBatch.hpp
│   │   ├── TileRenderer.hpp
│   │   ├── Texture.hpp
│   │   └── ParallaxBackground.hpp
│   ├── world/
│   │   ├── Chunk.hpp           # 64x64 tile container
│   │   ├── ChunkManager.hpp
│   │   ├── ChunkGenerator.hpp
│   │   ├── TileMap.hpp
│   │   └── WorldFile.hpp       # Binary serialization
│   ├── physics/
│   │   ├── AABB.hpp
│   │   ├── Collision.hpp
│   │   ├── TileCollision.hpp
│   │   ├── Trigger.hpp
│   │   ├── Raycast.hpp
│   │   └── PhysicsSystem.hpp
│   ├── lighting/
│   │   ├── LightMap.hpp
│   │   └── LightingSystem.hpp
│   ├── audio/
│   │   ├── AudioSystem.hpp
│   │   ├── SoundManager.hpp
│   │   └── MusicManager.hpp
│   ├── ui/
│   │   ├── UITypes.hpp
│   │   ├── UIElement.hpp
│   │   ├── UIWidgets.hpp
│   │   ├── UILayout.hpp
│   │   ├── UIInput.hpp
│   │   └── UISystem.hpp
│   ├── mod/
│   │   ├── ModLoader.hpp
│   │   ├── ModManifest.hpp
│   │   ├── ContentRegistry.hpp
│   │   ├── LuaBindings.hpp
│   │   ├── EventBus.hpp
│   │   └── HotReload.hpp
│   └── gameplay/               # Genre-support systems
│       ├── GameMode.hpp        # ViewMode, PhysicsPresets
│       ├── GridMovement.hpp    # Pokemon-style grid movement
│       ├── CameraController.hpp # Multi-mode camera system
│       ├── InputActions.hpp    # Abstract input mapping
│       ├── Pathfinding.hpp     # A* on tile grid
│       ├── StateMachine.hpp    # Entity FSM
│       ├── DialogueSystem.hpp  # Typewriter text boxes
│       ├── TileLayers.hpp      # Multi-layer tile rendering
│       └── GameplayLuaBindings.hpp/.cpp
├── mods/                       # Game mods ship here
├── tests/                      # GoogleTest unit tests
└── tools/
    ├── mod-validator/
    └── asset-packer/
```

### 3.4 Engine vs Mod Responsibilities

#### Engine Provides (Hardcoded C++)

| System | Responsibilities |
|--------|------------------|
| **Window** | Create window, handle resize, fullscreen toggle |
| **Input** | Keyboard, mouse, gamepad input; action mapping system |
| **Renderer** | Sprite batching, texture atlases, tile rendering, multi-layer tiles |
| **ECS** | Entity management, component storage, system scheduling |
| **Chunk Manager** | Load/unload/save chunks, spatial queries |
| **Physics** | AABB collision, swept collision, raycasting, trigger volumes, configurable gravity |
| **Audio** | Sound playback, music streaming, volume control, channels |
| **Lighting** | Per-tile light propagation, day/night cycle, smooth rendering |
| **UI Core** | Flexbox layout engine, input routing, rendering primitives |
| **Mod Loader** | Discover mods, resolve dependencies, load assets, run scripts |
| **Gameplay** | Grid movement, camera modes, pathfinding, state machines, dialogue, input actions |
| **Networking** | Connection management, packet serialization, sync primitives (future) |

#### Mods Provide (JSON + Lua)

| System | Responsibilities |
|--------|------------------|
| **Game Mode** | Physics preset, camera mode, input bindings for the game type |
| **Content** | Tiles, items, enemies, NPCs, projectiles, particles |
| **World Gen** | Terrain shape, biome rules, ore distribution, structures |
| **UI Layouts** | HUD, inventory, menus, settings — all screens |
| **Audio Mapping** | Sound files, event-to-sound bindings, music tracks |
| **Game Rules** | Day length, spawn rates, difficulty, progression |
| **Behaviors** | AI scripts via state machine, item effects, boss patterns |
| **Localization** | Text strings for any language |

---

## 4. Core Engine Systems

### 4.1 Rendering System

**Engine provides:**
- Sprite rendering with batching
- Texture atlas management
- Tile rendering with culling
- Multi-layer tile rendering (Background, Ground, Decoration, Foreground)
- Particle system infrastructure
- Per-tile lighting calculation and rendering
- Parallax background layers
- Camera with position, zoom, rotation, bounds

**Mod API:**
```lua
-- Register a custom shader
renderer.registerShader("magic_glow", "shaders/magic_glow.frag")

-- Define a particle emitter type
particles.register("torch_fire", {
    texture = "particles/fire.png",
    emission_rate = 30,
    lifetime = { min = 0.3, max = 0.8 },
    velocity = { x = 0, y = -20 },
    colors = { {255, 200, 50}, {255, 100, 0, 0} }
})

-- Set tiles on different layers
tile_layers.set(x, y, tile_layers.BACKGROUND, tile_id)
tile_layers.set(x, y, tile_layers.FOREGROUND, tree_canopy_id)
```

### 4.2 ECS System

**Core Components (engine-defined):**
- `Transform` — position, rotation, scale
- `Velocity` — linear + angular velocity
- `Sprite` — texture, animation, layer, flip
- `Collider` — AABB bounds, layer, mask
- `Health` — current, max, invincibility frames
- `LightSource` — radius, color, intensity, flicker
- `ParticleEmitter` — emitter type reference
- `Trigger` — callback on entity enter/exit
- `Gravity` — gravity scale, grounded state
- `GridMovement` — grid size, speed, facing direction
- `StateMachine` — named states with callbacks
- `CameraTarget` — marks entity as camera follow target
- `NetworkSync` — replication settings (future)

**Mod-defined Components:**
```lua
ecs.registerComponent("Poisoned", {
    damage_per_second = "number",
    duration = "number",
    source = "entity"
})

ecs.addSystem("poison_damage", {"Health", "Poisoned"}, function(entity, health, poison)
    health.current = health.current - poison.damage_per_second * dt
    poison.duration = poison.duration - dt
    if poison.duration <= 0 then
        ecs.removeComponent(entity, "Poisoned")
    end
end)
```

### 4.3 Chunk & World System

**Chunk Properties:**
- Size: 64x64 tiles
- Tile size: configurable (default 16px)
- Coordinate system: signed 32-bit chunk coords + local tile coords
- Binary serialization with CRC32 checksums

**Engine provides:**
- Chunk lifecycle management (create, load, save, unload)
- Spatial queries (get tile at position, entities in region)
- Tile change notifications (for lighting updates, etc.)
- World metadata storage
- Infinite world in all directions

**World Generation API:**
```lua
worldgen.registerTerrainGenerator("default_surface", function(chunk_x, seed)
    local heights = {}
    for x = 0, 63 do
        local world_x = chunk_x * 64 + x
        heights[x] = 100 + noise.perlin(world_x * 0.02, seed) * 20
    end
    return heights
end)
```

### 4.4 Physics System

**Engine provides:**
- AABB vs AABB collision
- AABB vs tilemap collision
- Swept collision for fast objects
- Raycasting against tiles and entities
- Trigger volumes with callbacks
- Collision layers and masks (Default, Player, Enemy, Projectile, Tile, Trigger, Item, NPC)
- Configurable gravity vector (any direction, or zero for top-down games)
- 45-degree slope support
- One-way platform support

**Physics Presets:**

| Preset | Gravity | Use Case |
|--------|---------|----------|
| **Platformer** | (0, 980) | Terraria, Metroidvania |
| **TopDown** | (0, 0) | Pokemon, Zelda |
| **Flight** | (0, 200) | Sopwith, side-scrolling shooters |
| **ZeroG** | (0, 0) | Space games |

**Mod API:**
```lua
-- Set physics mode
game_mode.set_physics("topdown")  -- or "platformer", "flight", "zero_g"
game_mode.set_gravity(0, 200)     -- custom gravity vector

-- Raycasting
local hit = physics.raycast(start_pos, direction, max_distance, layer_mask)
if hit then
    print("Hit " .. hit.entity .. " at " .. hit.point)
end
```

### 4.5 Audio System

**Engine provides:**
- Sound effect playback (positional, volume, pitch variance)
- Music streaming with crossfade
- Multiple audio channels (master, music, sfx, ambient)

**Mod API:**
```lua
audio.registerSound("player_hurt", "sounds/hurt.ogg", {
    volume = 0.8,
    pitch_variance = 0.1,
    cooldown = 0.1
})
audio.playSound("player_hurt", player.position)
audio.playMusic("music/boss_fight.ogg", { fade_in = 2.0 })
```

### 4.6 UI System

**Engine provides:**
- Layout primitives: Box, Text, Image, Button, Slider, Grid, Scroll
- Flexbox-style layout system (row, column, grid)
- Input focus and Tab navigation
- Screen management (show, hide, z-order)

**Mods define actual UI:**
```lua
ui.register("hud", function(player)
    return ui.Box({ id = "hud_root", style = styles.hud_container }, {
        ui.Box({ style = styles.health_bar_bg }, {
            ui.Box({
                style = styles.health_bar_fill,
                width = (player.health.current / player.health.max) * 100 .. "%"
            })
        }),
        ui.Grid({ columns = 10, style = styles.hotbar }, function()
            -- hotbar slots
        end)
    })
end)
```

### 4.7 Lighting System

**Engine provides:**
- Per-tile RGB light values
- BFS flood-fill light propagation
- Skylight from surface (configurable falloff)
- Dynamic light sources from entities
- Smooth 4-quadrant corner interpolation
- Day/night cycle (Dawn, Day, Dusk, Night)
- Toggleable via config (disable for games that don't need it)

### 4.8 Mod Loader

**Load Order:**
1. Discover all mods in `mods/` directory
2. Parse `mod.json` manifests
3. Build dependency graph
4. Topological sort (dependencies load first)
5. Register gameplay Lua APIs
6. Execute Lua scripts per mod
7. Register content with appropriate APIs
8. Validate (warn on missing dependencies, invalid data)

**Hot Reload (debug builds):**
- File watcher on mod directories
- On change: reload affected mod
- Preserve game state where possible

---

## 5. Gameplay Systems

These systems sit between the engine core and the mod layer. They provide common infrastructure that many 2D game types need, so individual mods don't have to reinvent them.

### 5.1 Game Mode & Physics Presets

Mods declare their game type at startup. The engine uses this to configure physics, camera, and input defaults.

```lua
-- Terraria-style sandbox
game_mode.set_physics("platformer")
input_actions.register_platformer_defaults()
camera.set_mode("free_follow")

-- Pokemon-style RPG
game_mode.set_physics("topdown")
input_actions.register_topdown_defaults()
camera.set_mode("room_based")
camera.set_room_size(320, 240)

-- Sopwith-style flight
game_mode.set_physics("flight")
input_actions.register_flight_defaults()
camera.set_mode("auto_scroll")
camera.set_scroll_speed(100, 0)
```

### 5.2 Grid Movement

For top-down RPGs and puzzle games where entities move tile-by-tile.

- Snap-to-grid movement with smooth visual interpolation (smoothstep)
- 4-directional facing (Up, Down, Left, Right)
- Configurable grid size and move speed
- Input buffering (queue next move while current move is in progress)
- Walkability callback (checks tile solidity by default)

```lua
grid_movement.add(player, { grid_size = 16, move_speed = 4 })
grid_movement.move(player, "up")
local facing = grid_movement.get_facing(player)
```

### 5.3 Camera Controller

Multiple camera modes for different game types:

| Mode | Behavior | Use Case |
|------|----------|----------|
| **FreeFollow** | Smooth follow with deadzone | General purpose |
| **GridSnap** | Snap to room boundaries | Pokemon-style rooms |
| **AutoScroll** | Constant directional scrolling | Sopwith, shmups |
| **RoomBased** | Discrete room transitions | Zelda-style dungeons |
| **Locked** | Camera does not move | Fixed screens |

Additional features: axis locking, zoom interpolation, look-ahead, world bounds.

```lua
camera.set_target(player, { priority = 1 })
camera.set_mode("free_follow")
camera.set_deadzone(64, 32)
camera.lock_axis("y")  -- horizontal side-scroller
```

### 5.4 Input Action Mapping

Abstract input system that decouples game logic from specific keys.

```lua
-- Register custom actions
input_actions.register("interact", "e")
input_actions.add_binding("interact", "enter")

-- Query by action name, not key
if input_actions.is_pressed("interact") then
    talk_to_npc()
end

-- Or use built-in presets
input_actions.register_platformer_defaults()  -- move, jump, attack, interact
input_actions.register_topdown_defaults()     -- move, interact, cancel, menu
input_actions.register_flight_defaults()      -- pitch, thrust, fire, bomb
```

### 5.5 Pathfinding

A* pathfinding on the tile grid for NPC movement and enemy AI.

- 4-directional and 8-directional movement
- Corner-cutting prevention (diagonals blocked by adjacent walls)
- Node budget to prevent expensive searches
- Reachability check (quick BFS)

```lua
local path = pathfinding.find_path(start_x, start_y, goal_x, goal_y, {
    diagonals = false,
    max_nodes = 5000
})
if path then
    for _, point in ipairs(path) do
        -- walk to point.x, point.y
    end
end

local reachable = pathfinding.is_reachable(ax, ay, bx, by, 100)
```

### 5.6 State Machine

Finite state machine component for entity behaviors — NPC routines, enemy AI, boss phases.

```lua
fsm.add(npc)
fsm.add_state(npc, "idle", {
    on_enter = function(e) set_animation(e, "idle") end,
    on_update = function(e, dt)
        if player_nearby(e) then fsm.set_state(e, "greet") end
    end
})
fsm.add_state(npc, "greet", {
    on_enter = function(e) dialogue.start(greet_dialogue) end,
    on_update = function(e, dt)
        if not dialogue.is_active() then fsm.set_state(e, "idle") end
    end
})
fsm.set_state(npc, "idle")
```

### 5.7 Dialogue System

Text box system for NPC conversations, tutorials, and story exposition.

- Typewriter text rendering (configurable speed)
- Speaker name and portrait
- Branching choices with callbacks
- Word wrapping
- Auto-advance or wait for input

```lua
dialogue.start({
    { id = "greet", speaker = "Oak", text = "Hello! Welcome to the world of...",
      next = "choose" },
    { id = "choose", speaker = "Oak", text = "Are you a boy or a girl?",
      choices = {
          { text = "Boy", next = "boy_chosen" },
          { text = "Girl", next = "girl_chosen" }
      }
    },
    { id = "boy_chosen", speaker = "Oak", text = "Right! So you're a boy." },
    { id = "girl_chosen", speaker = "Oak", text = "Right! So you're a girl." }
})
```

### 5.8 Multi-Layer Tiles

Tile rendering in 4 layers for visual depth:

| Layer | Index | Render Order | Use Case |
|-------|-------|-------------|----------|
| Background | 0 | First (behind everything) | Cave walls, sky tiles |
| Ground | 1 | Before entities | Main walkable terrain |
| Decoration | 2 | Before entities | Flowers, grass overlays |
| Foreground | 3 | After entities | Tree canopy, overhangs |

The existing TileMap system serves as the main ground layer. The TileLayerManager adds optional Background, Decoration, and Foreground layers on top.

```lua
tile_layers.set(x, y, tile_layers.BACKGROUND, cave_wall_id)
tile_layers.set(x, y, tile_layers.FOREGROUND, tree_canopy_id)
local tile = tile_layers.get(x, y, tile_layers.DECORATION)
```

---

## 6. Modding Framework

### 6.1 Mod Structure

```
my-game-mod/
├── mod.json                    # Required: manifest
├── scripts/
│   ├── init.lua                # Entry point
│   └── ...
├── content/
│   ├── tiles.json
│   ├── items.json
│   ├── recipes.json
│   ├── enemies.json
│   └── ...
├── worldgen/
│   ├── biomes.json
│   ├── terrain.lua
│   └── structures/
├── ui/
│   ├── layouts.lua
│   └── styles.lua
├── assets/
│   ├── textures/
│   ├── sounds/
│   └── music/
└── locale/
    ├── en.json
    └── es.json
```

### 6.2 Mod Manifest (mod.json)

```json
{
    "id": "awesome-expansion",
    "name": "Awesome Expansion Pack",
    "version": "1.2.0",
    "engine_version": ">=0.5.0",
    "authors": ["AuthorName"],
    "description": "Adds cool stuff to the game",
    "dependencies": [
        { "id": "base-game", "version": ">=1.0.0" }
    ],
    "optional_dependencies": [
        { "id": "magic-overhaul", "version": ">=2.0.0" }
    ],
    "incompatible": ["old-magic-mod"],
    "load_priority": 100,
    "entry_point": "scripts/init.lua"
}
```

### 6.3 Content Definition (JSON)

**Tiles:**
```json
{
    "tiles": [{
        "id": "crystal_ore",
        "name": "tile.crystal_ore.name",
        "texture": "textures/tiles/crystal_ore.png",
        "variants": 3,
        "solid": true,
        "hardness": 3.0,
        "drop": { "item": "crystal_ore_item", "count": 1 },
        "light_emission": { "r": 100, "g": 150, "b": 255, "intensity": 0.3 }
    }]
}
```

**Items:**
```json
{
    "items": [{
        "id": "crystal_sword",
        "name": "item.crystal_sword.name",
        "texture": "textures/items/crystal_sword.png",
        "type": "weapon",
        "damage": 45,
        "knockback": 5.0,
        "rarity": "rare",
        "on_hit": "scripts/crystal_sword_hit.lua"
    }]
}
```

**Enemies:**
```json
{
    "enemies": [{
        "id": "crystal_golem",
        "name": "enemy.crystal_golem.name",
        "texture": "textures/enemies/crystal_golem.png",
        "animations": {
            "idle": { "frames": [0, 1, 2, 1], "fps": 4 },
            "walk": { "frames": [3, 4, 5, 6], "fps": 8 }
        },
        "health": 250,
        "damage": 40,
        "behavior": "scripts/golem_ai.lua",
        "drops": [
            { "item": "crystal_shard", "count": [2, 5], "chance": 1.0 }
        ]
    }]
}
```

### 6.4 Events System

```lua
events.on("player_damaged", function(player, damage, source)
    if player.hasAccessory("crystal_shield") then
        if math.random() < 0.15 then
            source.health.current = source.health.current - damage * 0.5
            return true  -- Cancel original damage
        end
    end
end)

events.emit("boss_defeated", { boss = boss_entity, player = killer })
```

### 6.5 Lua Sandbox

Mods run in a sandboxed Lua environment. Removed from the environment:
- `os.execute`, `os.remove`, `os.rename`, `os.exit`
- `io.open`, `io.popen`
- `loadfile`, `dofile`, `load` (unrestricted)
- `package.*`, `debug.*`

Each mod gets its own isolated environment. Instruction count limiting prevents infinite loops.

---

## 7. Mod Distribution Ecosystem

### 7.1 Overview

The mod ecosystem runs on GitHub infrastructure:

```
Mod Template Repo → Modder Forks → Tags Release → Auto-Discovery → Mod Browser
```

1. Modders fork `Gloaming-Forge/mod-template`
2. Add content, push to GitHub, tag a release
3. GitHub Actions auto-builds release ZIP
4. Add `gloaming-mod` topic for auto-discovery
5. Mod registry indexes mods by stars (trust tiers)
6. Players browse/download from `gloaming-forge.github.io/mods`
7. Drop ZIP into `mods/install/` folder — game auto-installs on launch

### 7.2 Trust Tiers

| Tier | Stars | Warning |
|------|-------|---------|
| **Official** | — | None |
| **Popular** | 50+ | None |
| **Community** | 10-49 | None |
| **New** | 1-9 | "Limited feedback" |
| **Beta** | 0 | "Unreviewed" |

### 7.3 Security

- Lua sandbox (no filesystem/OS access)
- Static analysis on install
- JSON schema validation
- Mod isolation (can't access other mods' storage)
- Auto-backup saves before enabling new mods

---

## 8. Development Stages

### Stage 0: Project Skeleton ✓
CMake setup, window creation, game loop, logging, configuration.

### Stage 1: Core Rendering ✓
Renderer abstraction, sprite batching, camera, tile rendering, parallax backgrounds.

### Stage 2: ECS Foundation ✓
EnTT integration, core components, system scheduling, entity factory.

### Stage 3: Chunk System ✓
64x64 chunk management, binary serialization, infinite world, dirty tracking.

### Stage 4: Physics ✓
AABB collision, tile collision with slopes/platforms, swept collision, raycasting, triggers.

### Stage 5: Mod Loader ✓
Mod discovery, dependency resolution, Lua sandboxing, content registry, hot reload.

### Stage 6: Lighting System ✓
Per-tile light propagation, skylight, day/night cycle, smooth rendering.

### Stage 7: Audio System ✓
Sound effects, positional audio, music streaming with crossfade, Lua API.

### Stage 8: UI System ✓
Flexbox layout, widgets (box/text/image/button/slider/grid/scroll), screen management.

### Stage 9: Gameplay Systems ✓
GameMode/PhysicsPresets, GridMovement, CameraController, InputActions, Pathfinding, StateMachine, DialogueSystem, TileLayers. Full Lua bindings for all gameplay APIs.

### Stage 10: World Generation
WorldGen API, terrain generators, biome system, cave generation, ore distribution, structure placement.

### Stage 11: Gameplay Loop
Inventory, item pickup/drop, tool use, weapon system, crafting, damage/health/death/respawn.

### Stage 12: Enemies & AI
Enemy spawning, AI behavior API, pathfinding integration, drops, despawning.

### Stage 13: NPCs
NPC entities, housing, dialogue integration, shops.

### Stage 14: Polish & Release
Performance, bug fixes, documentation, example mods.

### Post-MVP Roadmap

| Phase | Focus |
|-------|-------|
| **Community Building** | Release, gather feedback, support modders |
| **Official Game Mods** | Terraria-clone, Pokemon-clone, more |
| **Multiplayer** | Server architecture, sync, 8-16 players |
| **Steam Release** | Store page, achievements, workshop |

---

## 9. Documentation Requirements

Each stage must include documentation before it's considered complete.

| Stage | Required Docs |
|-------|---------------|
| Mod Loader | Mod structure guide, mod.json reference |
| Content Registry | JSON schemas for all content types |
| Lua Bindings | API reference for each module |
| Gameplay Systems | GameMode, GridMovement, Camera, InputActions, Pathfinding, FSM, Dialogue |
| World Gen | World generation tutorial, biome guide |
| UI System | UI component reference, styling guide |
| Every system | Example code, common patterns |

---

## 10. Community Strategy

### Pre-Release
- GitHub repo public early for feedback
- Devlogs showing progress
- Discord server for community interaction
- Alpha testing to identify API pain points

### Release
- Clear README showing multiple game styles
- Video trailer demonstrating engine versatility
- Example game mods (sandbox, RPG, shooter)
- "First mod" tutorial for immediate engagement

### Post-Release
- Featured mods spotlight
- Mod jams (time-limited creation events)
- Official game mods proving continued development
- API feedback loop

---

## 11. Distribution & Licensing

### Licenses

| Component | License | Rationale |
|-----------|---------|-----------|
| Engine code | MIT | Maximum adoption |
| Game mods | MIT | Reference implementation, forkable |
| Official art | CC-BY-SA 4.0 | Shareable with attribution |
| Documentation | CC-BY 4.0 | Freely usable |

### GitHub Organization

| Repository | Purpose |
|------------|---------|
| `Gloaming` | Main engine repository |
| `mod-template` | Fork-to-start template for modders |
| `mod-registry` | JSON registry + discovery bot |
| `gloaming-forge.github.io` | Mod browser website + docs |

---

## 12. Open Questions

| Question | Options | Notes |
|----------|---------|-------|
| Mod signing | Required, optional, none | Security vs convenience |
| Circular tile layers | Separate save format? | Chunk serialization for extra layers |
| Scene/map transitions | Engine-level or mod-level? | Discrete maps vs continuous world |

---

*Document Version: 0.4*
*Last Updated: February 2026*
*Status: Stages 0-9 Complete*
