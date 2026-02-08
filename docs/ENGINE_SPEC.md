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
- Collision layers and masks with Lua API (see §5.10 for details)
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

### 5.9 Sprite Animation

Frame-based sprite animation system with named animation states. Integrates with the FSM so that state transitions automatically trigger animation changes.

**Engine provides:**
- AnimationController component with named animation clips
- Frame-based playback with configurable FPS per clip
- Loop, once, ping-pong playback modes
- Animation events (callbacks at specific frames — e.g., "spawn projectile on frame 3")
- Direction-aware animations (walk_down, walk_up, walk_left, walk_right)
- Blend between animations (optional crossfade for smoother transitions)
- Sprite sheet atlas integration (row/column frame indexing)

| Playback Mode | Behavior | Use Case |
|---------------|----------|----------|
| **Loop** | Repeats indefinitely | Walk cycles, idle |
| **Once** | Plays once, holds last frame | Attack swing, death |
| **PingPong** | Forward then reverse | Breathing, pulsing |

```lua
-- Define animations from a sprite sheet
animation.add(player, "idle_down",  { sheet = "player.png", row = 0, frames = 4, fps = 6 })
animation.add(player, "walk_down",  { sheet = "player.png", row = 1, frames = 6, fps = 10 })
animation.add(player, "walk_up",    { sheet = "player.png", row = 2, frames = 6, fps = 10 })
animation.add(player, "walk_left",  { sheet = "player.png", row = 3, frames = 6, fps = 10 })
animation.add(player, "walk_right", { sheet = "player.png", row = 4, frames = 6, fps = 10 })
animation.add(player, "attack",     { sheet = "player.png", row = 5, frames = 4, fps = 12, mode = "once" })

-- Play an animation
animation.play(player, "walk_down")

-- Set animation event (e.g., spawn hit effect on attack frame 3)
animation.on_frame(player, "attack", 3, function(e)
    spawn_hit_effect(e)
end)

-- Query state
local name = animation.current(player)       -- "walk_down"
local done = animation.is_finished(player)   -- true if "once" mode completed

-- Direction-aware helper: auto-selects walk_up/down/left/right based on facing
animation.play_directional(player, "walk", facing)
```

### 5.10 Collision Layers & Masks

Fine-grained collision filtering so entities can selectively collide with or ignore each other.

**Engine provides:**
- 16-bit collision layer bitmask per entity (which layers this entity occupies)
- 16-bit collision mask per entity (which layers this entity collides with)
- Named layer constants for readability
- Runtime toggling (e.g., disable player-enemy collision during invincibility frames)
- Trigger volumes respect layer filtering

| Layer | Bit | Default Collides With |
|-------|-----|----------------------|
| Default | 0 | Everything |
| Player | 1 | Tile, Enemy, NPC, Item, Trigger |
| Enemy | 2 | Tile, Player, Projectile |
| NPC | 3 | Tile, Player |
| Projectile | 4 | Tile, Enemy (player projectiles) or Player (enemy projectiles) |
| Item | 5 | Tile, Player |
| Trigger | 6 | Player |
| Tile | 7 | Player, Enemy, NPC, Projectile, Item |

```lua
-- Set collision layers
collision.set_layer(player, "player")
collision.set_mask(player, { "tile", "enemy", "npc", "item", "trigger" })

-- During invincibility frames, stop colliding with enemies
collision.remove_mask(player, "enemy")
-- Re-enable after invincibility ends (tracked by game logic)
-- collision.add_mask(player, "enemy")

-- Player projectile only hits enemies and tiles
collision.set_layer(arrow, "projectile")
collision.set_mask(arrow, { "tile", "enemy" })

-- NPCs block the player but not each other
collision.set_layer(npc, "npc")
collision.set_mask(npc, { "tile", "player" })
```

### 5.11 Entity Spawning from Lua

Dynamic entity creation at runtime — enemies, projectiles, items, effects.

**Engine provides:**
- `entity.create()` for blank entities
- `entity.spawn(type, x, y)` for content-registry entities (enemies, NPCs, items)
- `entity.destroy(id)` for cleanup
- Component add/get/set from Lua for custom entity composition
- Entity query by type, position, or component

```lua
-- Create a blank entity and compose it manually
local e = entity.create()
entity.set_position(e, 100, 200)
entity.set_sprite(e, "textures/spark.png")
entity.set_velocity(e, 0, -50)

-- Spawn a registered enemy type at a position
local bat = entity.spawn("bat", player_x + 200, player_y - 100)

-- Spawn an item drop
local drop = entity.spawn_item("copper_ore", x, y)

-- Destroy an entity
entity.destroy(bat)

-- Query entities
local nearby = entity.find_in_radius(x, y, 100, { type = "enemy" })
for _, e in ipairs(nearby) do
    -- process each nearby enemy
end

-- Component access
entity.set_component(e, "health", { current = 50, max = 100 })
local hp = entity.get_component(e, "health")
```

### 5.12 Projectile System

Spawns entities with velocity, checks collision against targets, deals damage, and auto-despawns. Covers arrows, bullets, thrown items, magic bolts.

**Engine provides:**
- Projectile component with speed, damage, lifetime, pierce count
- Collision filtering via collision layers (player vs enemy projectiles)
- Auto-rotation to face movement direction
- Gravity-affected projectiles (arcing arrows) or straight-line
- On-hit callbacks for custom effects (explosion, poison, etc.)
- Auto-despawn on collision or after max lifetime/distance

```lua
-- Fire an arrow from the player toward the cursor
local arrow = projectile.spawn({
    owner = player,
    x = player_x, y = player_y,
    speed = 400,
    angle = angle_to_cursor,
    damage = 10,
    sprite = "textures/arrow.png",
    gravity = true,           -- arc like a real arrow
    lifetime = 3.0,           -- despawn after 3 seconds
    pierce = 0,               -- 0 = destroy on first hit
    layer = "projectile",
    hits = { "enemy", "tile" }
})

-- Fire a straight laser bolt (flight game)
projectile.spawn({
    owner = player,
    x = gun_x, y = gun_y,
    speed = 800,
    angle = 0,               -- straight right
    damage = 5,
    sprite = "textures/bullet.png",
    gravity = false,
    lifetime = 2.0,
    hits = { "enemy" },
    on_hit = function(proj, target)
        spawn_sparks(proj.x, proj.y)
    end
})

-- Bomb drop (Sopwith-style)
projectile.spawn({
    owner = player,
    x = plane_x, y = plane_y,
    speed = 50,
    angle = 90,              -- straight down
    damage = 40,
    gravity = true,
    hits = { "enemy", "tile" },
    on_hit = function(proj, target)
        explosion(proj.x, proj.y, 48)  -- area damage
    end
})
```

### 5.13 Scene / Level Management

For room-based or level-based games where discrete areas are loaded independently.

**Engine provides:**
- Named scenes with their own tile data, entity spawns, and configuration
- Scene transitions (instant, fade, slide, custom)
- Scene stack for overlays (pause menu scene on top of gameplay scene)
- Persistent entities that survive scene transitions (player)
- Scene-local entities that are destroyed on exit
- Entry/exit callbacks for setup and teardown

**Tile data model:**

Scenes and the infinite world are mutually exclusive approaches:

| Approach | Tile Data | Use Case |
|----------|-----------|----------|
| **Infinite world** (default) | Single TileMap with chunk streaming | Terraria, open sandbox |
| **Scene-based** | Each scene has its own fixed-size tile grid loaded from a binary file | Pokemon, Metroidvania |

When `scene.go_to()` is called:
1. The current scene's tile data is unloaded (chunk manager is cleared)
2. Scene-local entities are destroyed
3. The transition effect plays (fade, slide, etc.)
4. The new scene's tile data is loaded into the TileMap as a single pre-built chunk
5. `on_enter` is called — the mod spawns entities for the new scene
6. Camera is repositioned per the scene's camera config

Persistent entities (marked with `scene.set_persistent()`) are not destroyed during step 2 and carry over into the new scene. The player entity is typically persistent.

Scene tile files are pre-built binary chunks (same format as world save chunks) created by a level editor or export tool. Scenes do not use procedural world generation.

```lua
-- Define scenes
scene.register("overworld", {
    tiles = "data/overworld_tiles.bin",
    size = { width = 40, height = 30 },     -- tile dimensions
    on_enter = function()
        music.play("overworld_theme")
        spawn_overworld_npcs()
    end,
    on_exit = function()
        save_overworld_state()
    end
})

scene.register("house_1", {
    tiles = "data/house1_tiles.bin",
    size = { width = 20, height = 15 },
    camera = { mode = "locked", x = 160, y = 120 },
    on_enter = function()
        music.play("indoor_theme")
        spawn_house_npcs()
    end
})

-- Transition between scenes
scene.go_to("house_1", { transition = "fade", duration = 0.5 })

-- Mark the player entity as persistent (survives scene transitions)
scene.set_persistent(player)

-- Scene stack for overlays (does NOT unload tile data)
scene.push("pause_menu")   -- pauses game, shows menu on top
scene.pop()                 -- resumes game, removes overlay

-- Query current scene
local name = scene.current()  -- "overworld"
```

### 5.14 Particle System

Data-driven particle emitters for visual effects — dust, sparks, fire, rain, leaves.

**Engine provides:**
- Particle emitter component attachable to entities or world positions
- Configurable: count, lifetime, speed, spread angle, gravity, color, fade, size curve
- Burst mode (emit N particles at once) and continuous mode (emit N per second)
- Texture or colored-rectangle particles
- Particle pool for zero-allocation emission

```lua
-- Dust puff when landing
particles.burst({
    x = player_x, y = player_y + 8,
    count = 8,
    speed = { min = 20, max = 60 },
    angle = { min = -160, max = -20 },  -- upward fan
    lifetime = { min = 0.2, max = 0.5 },
    size = { start = 3, finish = 1 },
    color = { r = 180, g = 160, b = 120, a = 200 },
    fade = true,
    gravity = 100
})

-- Attach a torch flame emitter to an entity
local emitter = particles.attach(torch_entity, {
    rate = 15,                          -- particles per second
    speed = { min = 10, max = 30 },
    angle = { min = -100, max = -80 },  -- mostly upward
    lifetime = { min = 0.3, max = 0.6 },
    color_start = { r = 255, g = 200, b = 50, a = 255 },
    color_end   = { r = 255, g = 80,  b = 0,  a = 0 },
    size = { start = 4, finish = 1 },
    offset = { x = 0, y = -4 }
})

-- Rain across the screen
particles.spawn_emitter({
    x = camera.x, y = camera.y - 200,
    width = camera.width + 100,         -- emitter spans full screen width
    rate = 200,
    speed = { min = 300, max = 400 },
    angle = { min = 80, max = 100 },    -- mostly downward
    lifetime = { min = 0.5, max = 1.0 },
    size = { start = 2, finish = 2 },
    color = { r = 150, g = 180, b = 220, a = 150 },
    follow_camera = true
})
```

### 5.15 Timer / Scheduler

Simple time-based callbacks for delayed and repeating actions. Saves modders from manually tracking elapsed time in every `onUpdate`.

**Engine provides:**
- `timer.after(seconds, callback)` — one-shot delayed call
- `timer.every(seconds, callback)` — repeating call
- `timer.cancel(id)` — cancel a pending timer
- Timers are paused when the game is paused
- Entity-scoped timers that auto-cancel when the entity is destroyed

```lua
-- Flash invincibility for 1.5 seconds
collision.remove_mask(player, "enemy")
animation.set_flash(player, true)
timer.after(1.5, function()
    collision.add_mask(player, "enemy")
    animation.set_flash(player, false)
end)

-- Spawn an enemy every 10 seconds
local spawn_timer = timer.every(10.0, function()
    local x = math.random(0, world_width)
    entity.spawn("bat", x, 0)
end)

-- Cancel spawning when the boss appears
timer.cancel(spawn_timer)

-- Entity-scoped timer (auto-cancels if entity is destroyed)
timer.after_for(npc, 3.0, function()
    fsm.set_state(npc, "patrol")
end)
```

### 5.16 Save / Load for Gameplay State

Key-value persistence for mod-specific data — quest progress, player stats, NPC state, custom flags. Stored alongside the world save.

**Engine provides:**
- `save.set(key, value)` — store a value (string, number, boolean, or table)
- `save.get(key, default)` — retrieve a value with fallback
- `save.delete(key)` — remove a key
- Auto-save with the world file
- Per-mod namespacing (mods can't overwrite each other's data)
- Bulk save/load for efficiency

**Storage details:**

| Aspect | Design |
|--------|--------|
| **File format** | One JSON file per mod: `worlds/<world>/moddata/<mod-id>.json` |
| **Namespacing** | Each mod reads/writes only its own file. `save.set("key", val)` in mod `base-game` writes to `moddata/base-game.json` |
| **Value types** | string, number, boolean, table (nested tables up to 8 levels deep) |
| **Size limit** | 1 MB per mod save file. `save.set()` returns false if the limit would be exceeded |
| **Save timing** | Flushed to disk on world save (manual or auto-save). In-memory until then |
| **Corruption** | Engine keeps a `.bak` copy of the previous save. On load, if the primary file fails JSON parsing, the backup is used and a warning is logged |
| **Missing data** | `save.get("key", default)` returns the default if the key doesn't exist or the file is missing. Mods should always provide sensible defaults |

```lua
-- Track quest progress
save.set("quest_1_complete", true)
save.set("player_level", 5)
save.set("npc_guide_met", true)

-- Retrieve with defaults
local level = save.get("player_level", 1)
local quest_done = save.get("quest_1_complete", false)

-- Save complex data (serialized as JSON internally)
save.set("inventory", {
    { id = "copper_sword", count = 1, slot = 1 },
    { id = "torch", count = 15, slot = 2 }
})
local inv = save.get("inventory", {})

-- Delete a key
save.delete("temporary_flag")
```

### 5.17 Tweening / Easing

Interpolate entity properties over time for UI animations, camera shakes, scaling effects, and general "juice".

**Engine provides:**
- Tween any numeric entity property (position, scale, rotation, alpha)
- Standard easing functions (linear, ease_in/out/in_out for quad, cubic, elastic, bounce, back)
- Chainable tweens (sequence: move, then fade, then destroy)
- Tween cancellation and completion callbacks
- Camera shake helper

| Easing | Feel | Use Case |
|--------|------|----------|
| **linear** | Constant speed | Progress bars |
| **ease_out_quad** | Fast start, soft stop | UI slides |
| **ease_in_out_cubic** | Smooth both ends | Camera pans |
| **ease_out_elastic** | Overshoot + settle | Pop-in effects |
| **ease_out_bounce** | Bouncing stop | Item drops |
| **ease_in_back** | Pull back then go | Button press |

```lua
-- Slide a UI element into view
tween.to(panel, { x = 100 }, 0.3, "ease_out_quad")

-- Scale up an item pickup notification, then fade out
tween.to(popup, { scale = 1.5 }, 0.2, "ease_out_elastic", function()
    tween.to(popup, { alpha = 0 }, 0.5, "linear", function()
        entity.destroy(popup)
    end)
end)

-- Camera shake on damage
tween.shake(camera, { intensity = 8, duration = 0.3, decay = "ease_out_quad" })

-- Tween with cancellation
local t = tween.to(door, { y = door_y - 32 }, 1.0, "ease_in_out_cubic")
tween.cancel(t)  -- stop mid-tween
```

### 5.18 Debug Drawing

Overlay drawing from Lua for visualizing collision boxes, pathfinding, trigger volumes, and custom debug info during development.

**Engine provides:**
- Draw rectangles, circles, lines, and points in world or screen space
- Draw text labels at positions
- Path visualization (draw a sequence of connected points)
- Auto-clear each frame (no manual cleanup)
- Globally togglable (one key to hide all debug draws)
- Color and opacity control

```lua
-- Visualize an entity's collision box
debug.draw_rect(entity_x, entity_y, width, height, { r = 0, g = 255, b = 0, a = 100 })

-- Draw a pathfinding result
debug.draw_path(path, { r = 255, g = 255, b = 0 })

-- Show trigger volume
debug.draw_circle(trigger_x, trigger_y, trigger_radius, { r = 255, g = 0, b = 0, a = 80 })

-- Label an entity for debugging
debug.draw_text("State: " .. fsm.get_state(npc), npc_x, npc_y - 16)

-- Screen-space debug info (not affected by camera)
debug.draw_text_screen("Entities: " .. entity.count(), 20, 300)

-- Toggle all debug drawing (also bound to F3 by default)
debug.set_enabled(true)
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

### Stage 10: Sprite Animation & Collision Layers
The two most critical gaps before any game feels playable.

- **Sprite Animation System** (§5.9) — AnimationController component, frame-based playback, named clips with loop/once/ping-pong modes, animation events (callbacks on specific frames), direction-aware animation selection, sprite sheet atlas integration.
- **Collision Layers & Masks** (§5.10) — 16-bit layer/mask bitmasks on every collidable entity, named layer constants, runtime toggling for invincibility frames, Lua API for `collision.set_layer()`, `collision.set_mask()`, `collision.remove_mask()`, `collision.add_mask()`.

### Stage 11: Entity Spawning & Projectiles
Enable dynamic entity creation — the prerequisite for enemies, items, and projectiles.

- **Entity Spawning from Lua** (§5.11) — `entity.create()`, `entity.spawn(type, x, y)`, `entity.destroy(id)`, component access from Lua, spatial queries (`entity.find_in_radius()`).
- **Projectile System** (§5.12) — Projectile component with speed, damage, lifetime, pierce, gravity toggle. Collision filtering via layers. Auto-rotation, on-hit callbacks, auto-despawn. Covers arrows, bullets, bombs, and magic bolts.

### Stage 12: World Generation
WorldGen API, terrain generators, biome system, cave generation, ore distribution, structure placement. Lua-driven so mods define their own world generators.

### Stage 13: Gameplay Loop
Inventory system, item pickup/drop, tool use (mining, chopping), weapon system (melee swing, ranged aim), crafting with station proximity, health/damage/death/respawn.

### Stage 14: Enemies & AI
Enemy spawning rules (biome, depth, day/night), AI behavior API integrated with pathfinding and FSM, loot drops on death, despawn rules.

### Stage 15: NPCs
NPC entities, housing validation, dialogue integration with FSM, shops and trade.

### Stage 16: Scenes, Timers & Save State
Systems that enable complete game loops.

- **Scene / Level Management** (§5.13) — Named scenes with tile data, entity spawns, transitions (fade, slide), scene stack for overlays, persistent vs scene-local entities.
- **Timer / Scheduler** (§5.15) — `timer.after()`, `timer.every()`, `timer.cancel()`, entity-scoped timers, pause-aware.
- **Save / Load for Gameplay State** (§5.16) — Key-value persistence per mod, auto-save with world file, string/number/boolean/table support.

### Stage 17: Particles & Polish
Visual effects and developer quality-of-life.

- **Particle System** (§5.14) — Data-driven emitters, burst and continuous modes, configurable lifetime/speed/angle/color/size curves, entity-attached or world-position emitters, particle pool.
- **Tweening / Easing** (§5.17) — Tween any numeric property, standard easing functions, chainable sequences, camera shake helper.
- **Debug Drawing** (§5.18) — Overlay drawing from Lua (rects, circles, lines, paths, text), world-space and screen-space, globally togglable with F3.

### Stage 18: Polish & Release
Performance profiling, bug fixes, full API documentation, example mods for each game type (platformer, top-down RPG, flight), Steam Deck testing.

### Post-MVP Roadmap

| Phase | Focus |
|-------|-------|
| **Community Building** | Release, gather feedback, support modders |
| **Official Game Mods** | Terraria-clone, Pokemon-clone, Sopwith-clone |
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
| Gameplay Systems (Stage 9) | GameMode, GridMovement, Camera, InputActions, Pathfinding, FSM, Dialogue, TileLayers |
| Animation & Collision (Stage 10) | Animation clip format, collision layer guide with diagrams |
| Entity & Projectiles (Stage 11) | Entity creation guide, projectile configuration reference |
| World Gen (Stage 12) | World generation tutorial, biome guide, noise API reference |
| Gameplay Loop (Stage 13) | Inventory API, crafting system guide, combat reference |
| Scenes & Timers (Stage 16) | Scene management guide, timer API reference, save system guide |
| Particles & Polish (Stage 17) | Particle emitter reference, easing function chart, debug draw guide |
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
