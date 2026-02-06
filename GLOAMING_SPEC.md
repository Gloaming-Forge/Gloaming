# Gloaming
## Engine & Platform Specification Document
### Version 0.3 — Engine-First, Mod-Driven Architecture

> **Vision Statement:** An open-source, moddable 2D sandbox engine with Ori-inspired visuals. We build the platform — the community builds the worlds.

**Organization:** [Gloaming-Forge](https://github.com/Gloaming-Forge)

---

## Table of Contents

1. [Philosophy & Approach](#1-philosophy--approach)
2. [Project Overview](#2-project-overview)
3. [Technical Architecture](#3-technical-architecture)
4. [Core Engine Systems](#4-core-engine-systems)
5. [Modding Framework](#5-modding-framework)
6. [Mod Distribution Ecosystem](#6-mod-distribution-ecosystem)
7. [Base Game Mod](#7-base-game-mod)
8. [Development Stages](#8-development-stages)
9. [Documentation Requirements](#9-documentation-requirements)
10. [Community Strategy](#10-community-strategy)
11. [Distribution & Licensing](#11-distribution--licensing)
12. [Open Questions](#12-open-questions)

---

## 1. Philosophy & Approach

### 1.1 The Core Insight

We are not building a game. **We are building a platform for games.**

The engine provides infrastructure. Content, UI, audio, even world generation — all provided by mods. The "base game" is simply a reference mod that ships with the engine.

### 1.2 The Split

| Layer | What It Is | Who Builds It |
|-------|------------|---------------|
| **Engine** | Infrastructure, primitives, mod loading | Us (core team) |
| **Base Game Mod** | Reference implementation, minimal content | Us (as a mod) |
| **Content Mods** | Items, enemies, biomes, bosses, expansions | Community |
| **Total Conversions** | Different games entirely | Community |

### 1.3 Design Principles

| Principle | Meaning |
|-----------|---------|
| **Mods are first-class** | The base game uses the same APIs as community mods |
| **Data over code** | JSON/config files wherever possible; scripting for complex behaviors |
| **Composition over inheritance** | Mix-and-match components, not rigid class hierarchies |
| **Hot-reload everything** | Change a file, see results immediately (debug builds) |
| **Document as you build** | Undocumented features don't exist to modders |
| **Fail gracefully** | Bad mod data logs warnings, doesn't crash |

### 1.4 What This Enables

- **Total conversions** — Sci-fi, fantasy, horror, anything
- **Different world types** — Floating islands, infinite caves, flat creative
- **UI overhauls** — Completely different inventory systems, HUD styles
- **Content packs** — "Hardmode expansion," "Ocean biome pack," etc.
- **Quality of life mods** — Minimap, recipe browser, auto-sort
- **Multiplayer modes** — PvP arenas, co-op dungeons (future)

---

## 2. Project Overview

### 2.1 Elevator Pitch

"A moddable 2D sandbox engine inspired by Terraria and Ori. Open source. Infinitely extensible. Beautiful by default."

### 2.2 Target Audience

| Audience | What They Want |
|----------|----------------|
| **Players** | Fun sandbox gameplay, visual polish, mod variety |
| **Modders** | Easy content creation, good docs, powerful APIs |
| **Developers** | Clean architecture, contribution opportunities |
| **Artists** | Platform to showcase work, asset packs |

### 2.3 Success Metrics

| Metric | Target |
|--------|--------|
| Engine MVP | Playable base game with all systems functional |
| Documentation | 100% of mod APIs documented with examples |
| Community | First third-party mod within 1 month of release |
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
| **Scripting** | Lua (LuaJIT) | Industry standard for game modding |
| **Data Format** | JSON + binary | JSON for mods, binary for world saves |
| **Audio** | miniaudio | Lightweight, cross-platform |
| **Networking** | ENet (future) | Reliable UDP |

### 3.2 Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                          MODS LAYER                             │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌───────────┐  │
│  │  Base Game  │ │ UI Mod      │ │ Content Pack│ │ Total     │  │
│  │  (required) │ │ (optional)  │ │ (optional)  │ │ Conversion│  │
│  └─────────────┘ └─────────────┘ └─────────────┘ └───────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                        MOD API LAYER                            │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌───────────┐  │
│  │ Content │ │ World   │ │ UI      │ │ Audio   │ │ Gameplay  │  │
│  │ Registry│ │ Gen API │ │ API     │ │ API     │ │ Events    │  │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └───────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                       ENGINE CORE                               │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌───────────┐  │
│  │ ECS     │ │Renderer │ │ Physics │ │ Audio   │ │ Mod       │  │
│  │ (EnTT)  │ │(Raylib) │ │ System  │ │ System  │ │ Loader    │  │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └───────────┘  │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐               │
│  │ Window  │ │ Input   │ │ Chunk   │ │ Network │               │
│  │ Manager │ │ System  │ │ Manager │ │ (future)│               │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘               │
└─────────────────────────────────────────────────────────────────┘
```

### 3.3 Project Structure

```
gloaming/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── GLOAMING_SPEC.md             # This document
├── src/
│   ├── main.cpp
│   ├── engine/                 # Core engine (hardcoded)
│   │   ├── Engine.hpp          # Main engine class
│   │   ├── Window.hpp
│   │   ├── Input.hpp
│   │   └── Time.hpp
│   ├── ecs/
│   │   ├── Registry.hpp        # EnTT wrapper
│   │   ├── Components.hpp      # Core components
│   │   └── Systems.hpp         # Core systems
│   ├── rendering/
│   │   ├── IRenderer.hpp       # Abstract interface
│   │   ├── RaylibRenderer.hpp
│   │   ├── Camera.hpp
│   │   ├── SpriteBatch.hpp
│   │   └── ShaderManager.hpp
│   ├── world/
│   │   ├── ChunkManager.hpp    # Chunk loading/saving
│   │   ├── Chunk.hpp
│   │   ├── TileMap.hpp
│   │   └── WorldFile.hpp       # Serialization
│   ├── physics/
│   │   ├── Collision.hpp       # AABB, raycasting
│   │   ├── TileCollision.hpp
│   │   └── Trigger.hpp
│   ├── audio/
│   │   ├── AudioSystem.hpp
│   │   ├── SoundManager.hpp
│   │   └── MusicManager.hpp
│   ├── ui/
│   │   ├── UIRenderer.hpp      # Primitives only
│   │   ├── UIElement.hpp
│   │   ├── UILayout.hpp
│   │   └── UIInput.hpp
│   ├── mod/
│   │   ├── ModLoader.hpp
│   │   ├── ModManifest.hpp
│   │   ├── ContentRegistry.hpp
│   │   ├── LuaBindings.hpp
│   │   └── HotReload.hpp
│   └── api/                    # Public mod API
│       ├── ContentAPI.hpp
│       ├── WorldGenAPI.hpp
│       ├── UIAPI.hpp
│       ├── AudioAPI.hpp
│       ├── EntityAPI.hpp
│       └── EventAPI.hpp
├── mods/
│   └── base-game/              # Ships with engine
│       ├── mod.json
│       ├── scripts/            # Lua scripts
│       ├── content/            # JSON definitions
│       ├── worldgen/           # World generation
│       ├── ui/                 # UI definitions
│       └── assets/             # Textures, sounds
├── docs/
│   ├── modding-guide/
│   ├── api-reference/
│   └── tutorials/
└── tools/
    ├── mod-validator/
    └── asset-packer/
```

### 3.4 Engine vs Mod Responsibilities

#### Engine Provides (Hardcoded C++)

| System | Responsibilities |
|--------|------------------|
| **Window** | Create window, handle resize, fullscreen toggle |
| **Input** | Keyboard, mouse, gamepad input; key binding system |
| **Renderer** | Sprite batching, shader pipeline, render targets, lighting infrastructure |
| **ECS** | Entity management, component storage, system scheduling |
| **Chunk Manager** | Load/unload/save chunks, spatial queries |
| **Physics** | AABB collision, swept collision, raycasting, trigger volumes |
| **Audio** | Sound playback, music streaming, volume control, channels |
| **UI Core** | Layout engine, input routing, rendering primitives |
| **Mod Loader** | Discover mods, resolve dependencies, load assets, run scripts |
| **Networking** | Connection management, packet serialization, sync primitives |

#### Mods Provide (JSON + Lua)

| System | Responsibilities |
|--------|------------------|
| **Content** | Tiles, items, enemies, NPCs, projectiles, particles |
| **World Gen** | Terrain shape, biome rules, ore distribution, structures |
| **UI Layouts** | HUD, inventory, crafting, menus, settings |
| **Audio Mapping** | Sound files, event→sound bindings, music tracks |
| **Game Rules** | Day length, spawn rates, difficulty, progression |
| **Behaviors** | AI scripts, item effects, boss patterns |
| **Shaders** | Custom visual effects (optional) |
| **Localization** | Text strings for any language |

---

## 4. Core Engine Systems

### 4.1 Rendering System

**Engine provides:**
- Sprite rendering with batching
- Texture atlas management
- Shader loading and uniform binding
- Render target (framebuffer) management
- Tile rendering with culling
- Particle system infrastructure
- Per-tile lighting calculation and rendering
- Post-processing pipeline

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
```

### 4.2 ECS System

**Core Components (engine-defined):**
- `Transform` — position, rotation, scale
- `Velocity` — movement vector
- `Sprite` — texture, animation, layer
- `Collider` — bounds, layer, mask
- `Health` — current, max, invincibility
- `LightSource` — radius, color, flicker
- `ParticleEmitter` — emitter type reference
- `Trigger` — callback on entity enter/exit
- `NetworkSync` — replication settings (future)

**Mod-defined Components:**
Mods can register custom components for specialized data.

```lua
-- Register a custom component
ecs.registerComponent("Poisoned", {
    damage_per_second = "number",
    duration = "number",
    source = "entity"
})

-- Systems can query for it
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
- Coordinate system: chunk coords + local tile coords
- Binary serialization for fast save/load

**Engine provides:**
- Chunk lifecycle management (create, load, save, unload)
- Spatial queries (get tile at position, entities in region)
- Tile change notifications (for lighting updates, etc.)
- World metadata storage

**World Generation API:**
```lua
-- Register a terrain generator
worldgen.registerTerrainGenerator("default_surface", function(chunk_x, seed)
    local heights = {}
    for x = 0, 63 do
        local world_x = chunk_x * 64 + x
        heights[x] = 100 + noise.perlin(world_x * 0.02, seed) * 20
    end
    return heights
end)

-- Register a biome
worldgen.registerBiome("forest", {
    temperature = { min = 0.3, max = 0.7 },
    humidity = { min = 0.4, max = 1.0 },
    surface_tile = "grass",
    subsurface_tile = "dirt",
    stone_tile = "stone",
    trees = { density = 0.1, types = {"oak", "birch"} },
    ambient_music = "music/forest_day.ogg"
})

-- Register a structure
worldgen.registerStructure("underground_cabin", {
    rarity = 0.001,  -- per chunk
    depth = { min = 50, max = 200 },
    generator = "structures/cabin.lua"
})
```

### 4.4 Physics System

**Engine provides:**
- AABB vs AABB collision
- AABB vs tilemap collision
- Swept collision for fast objects
- Raycasting against tiles and entities
- Trigger volumes with callbacks
- Collision layers and masks

**Not provided (keep it simple):**
- Rigid body dynamics
- Joints/constraints
- Complex polygon collision

**Mod API:**
```lua
-- Raycasting
local hit = physics.raycast(start_pos, direction, max_distance, layer_mask)
if hit then
    print("Hit " .. hit.entity .. " at " .. hit.point)
end

-- Custom collision response
events.on("collision", function(entity_a, entity_b, normal)
    if entity_a.has("Bouncy") then
        entity_a.velocity = reflect(entity_a.velocity, normal)
    end
end)
```

### 4.5 Audio System

**Engine provides:**
- Sound effect playback (positional, volume, pitch)
- Music streaming with crossfade
- Multiple audio channels (master, music, sfx, ambient)
- Audio bus/mixer

**Mod API:**
```lua
-- Register sounds
audio.registerSound("player_hurt", "sounds/hurt.ogg", {
    volume = 0.8,
    pitch_variance = 0.1,
    cooldown = 0.1
})

-- Play sounds
audio.playSound("player_hurt", player.position)

-- Music
audio.playMusic("music/boss_fight.ogg", { fade_in = 2.0 })

-- Bind sounds to events
audio.bindEvent("tile_break", function(tile_type, position)
    local sound = tiles.get(tile_type).break_sound
    audio.playSound(sound, position)
end)
```

### 4.6 UI System

**Engine provides:**
- Layout primitives: Box, Text, Image, Button, Slider, Grid, Scroll
- Flexbox-style layout system
- Input focus and navigation
- Render to screen-space

**Mods define actual UI:**
```lua
-- Define HUD
ui.register("hud", function(player)
    return ui.Box({ id = "hud_root", style = styles.hud_container }, {
        -- Health bar
        ui.Box({ style = styles.health_bar_bg }, {
            ui.Box({ 
                style = styles.health_bar_fill,
                width = (player.health.current / player.health.max) * 100 .. "%"
            })
        }),
        -- Hotbar
        ui.Grid({ columns = 10, style = styles.hotbar }, function()
            local slots = {}
            for i = 1, 10 do
                local item = player.inventory[i]
                slots[i] = ui.ItemSlot({ item = item, index = i })
            end
            return slots
        end)
    })
end)

-- Define styles separately
styles.health_bar_bg = {
    width = 200,
    height = 20,
    background = "#333333",
    border = { width = 2, color = "#000000" }
}
```

### 4.7 Mod Loader

**Load Order:**
1. Discover all mods in `mods/` directory
2. Parse `mod.json` manifests
3. Build dependency graph
4. Topological sort (dependencies load first)
5. Load assets (textures, sounds)
6. Execute Lua scripts
7. Register content with appropriate APIs
8. Validate (warn on missing dependencies, invalid data)

**Hot Reload (debug builds):**
- File watcher on mod directories
- On change: reload affected mod
- Preserve game state where possible
- Log what was reloaded

---

## 5. Modding Framework

### 5.1 Mod Structure

```
my-awesome-mod/
├── mod.json                    # Required: manifest
├── scripts/
│   ├── init.lua                # Entry point
│   ├── items.lua
│   ├── enemies.lua
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
│       └── dungeon.lua
├── ui/
│   ├── layouts.lua
│   └── styles.lua
├── assets/
│   ├── textures/
│   │   ├── tiles/
│   │   ├── items/
│   │   ├── enemies/
│   │   └── ui/
│   ├── sounds/
│   ├── music/
│   └── shaders/
└── locale/
    ├── en.json
    └── es.json
```

### 5.2 Mod Manifest (mod.json)

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
    "entry_point": "scripts/init.lua",
    "provides": {
        "content": true,
        "worldgen": true,
        "ui": false,
        "audio": true
    }
}
```

### 5.3 Content Definition (JSON)

**Tiles (content/tiles.json):**
```json
{
    "tiles": [
        {
            "id": "crystal_ore",
            "name": "tile.crystal_ore.name",
            "texture": "textures/tiles/crystal_ore.png",
            "variants": 3,
            "solid": true,
            "transparent": false,
            "hardness": 3.0,
            "required_pickaxe_power": 55,
            "drop": { "item": "crystal_ore_item", "count": 1 },
            "light_emission": { "r": 100, "g": 150, "b": 255, "intensity": 0.3 },
            "break_sound": "sounds/crystal_break.ogg",
            "place_sound": "sounds/stone_place.ogg"
        }
    ]
}
```

**Items (content/items.json):**
```json
{
    "items": [
        {
            "id": "crystal_sword",
            "name": "item.crystal_sword.name",
            "description": "item.crystal_sword.desc",
            "texture": "textures/items/crystal_sword.png",
            "type": "weapon",
            "weapon_type": "melee",
            "damage": 45,
            "knockback": 5.0,
            "use_time": 20,
            "swing_arc": 120,
            "crit_chance": 0.08,
            "rarity": "rare",
            "sell_value": 5000,
            "light_emission": { "r": 100, "g": 150, "b": 255, "intensity": 0.2 },
            "on_hit": "scripts/crystal_sword_hit.lua"
        }
    ]
}
```

**Enemies (content/enemies.json):**
```json
{
    "enemies": [
        {
            "id": "crystal_golem",
            "name": "enemy.crystal_golem.name",
            "texture": "textures/enemies/crystal_golem.png",
            "animations": {
                "idle": { "frames": [0, 1, 2, 1], "fps": 4 },
                "walk": { "frames": [3, 4, 5, 6], "fps": 8 },
                "attack": { "frames": [7, 8, 9], "fps": 12 }
            },
            "health": 250,
            "damage": 40,
            "defense": 20,
            "knockback_resist": 0.7,
            "behavior": "scripts/golem_ai.lua",
            "spawn_conditions": {
                "biomes": ["crystal_caves"],
                "depth": { "min": 100, "max": 300 },
                "light_level": { "max": 0.3 }
            },
            "drops": [
                { "item": "crystal_shard", "count": [2, 5], "chance": 1.0 },
                { "item": "crystal_core", "count": 1, "chance": 0.1 }
            ],
            "sounds": {
                "hurt": "sounds/golem_hurt.ogg",
                "death": "sounds/golem_death.ogg"
            }
        }
    ]
}
```

**Recipes (content/recipes.json):**
```json
{
    "recipes": [
        {
            "id": "crystal_sword_recipe",
            "result": { "item": "crystal_sword", "count": 1 },
            "ingredients": [
                { "item": "crystal_bar", "count": 15 },
                { "item": "enchanted_hilt", "count": 1 }
            ],
            "station": "mythril_anvil",
            "category": "weapons"
        }
    ]
}
```

### 5.4 Scripting (Lua)

**Entry Point (scripts/init.lua):**
```lua
-- Mod initialization
local mod = {}

function mod.init()
    log.info("Awesome Expansion loading...")
    
    -- Load content from JSON
    content.loadTiles("content/tiles.json")
    content.loadItems("content/items.json")
    content.loadEnemies("content/enemies.json")
    content.loadRecipes("content/recipes.json")
    
    -- Register custom behaviors
    require("scripts/items")
    require("scripts/enemies")
    
    -- Register worldgen
    require("scripts/worldgen")
    
    log.info("Awesome Expansion loaded!")
end

function mod.postInit()
    -- Called after all mods loaded
    -- Good for cross-mod compatibility
    if mods.isLoaded("magic-overhaul") then
        require("scripts/magic_compat")
    end
end

return mod
```

**Custom Item Behavior (scripts/crystal_sword_hit.lua):**
```lua
-- Called when crystal sword hits an enemy
return function(attacker, target, damage)
    -- 20% chance to spawn crystal shards
    if math.random() < 0.2 then
        local pos = target.position
        for i = 1, 3 do
            local projectile = entities.spawn("crystal_shard_projectile", pos)
            local angle = math.random() * math.pi * 2
            projectile.velocity = {
                x = math.cos(angle) * 200,
                y = math.sin(angle) * 200
            }
        end
        audio.playSound("crystal_burst", pos)
    end
    
    -- Apply glow effect to target
    target.addComponent("GlowEffect", {
        color = {100, 150, 255},
        duration = 0.5
    })
end
```

**Enemy AI (scripts/golem_ai.lua):**
```lua
local GolemAI = {}

function GolemAI.new(entity)
    return {
        entity = entity,
        state = "idle",
        target = nil,
        attack_cooldown = 0
    }
end

function GolemAI.update(self, dt)
    local pos = self.entity.position
    
    -- Find nearest player
    self.target = entities.findNearest(pos, "Player", 300)
    
    if self.state == "idle" then
        if self.target then
            self.state = "chase"
        end
        
    elseif self.state == "chase" then
        if not self.target then
            self.state = "idle"
            return
        end
        
        local dir = vector.normalize(self.target.position - pos)
        self.entity.velocity.x = dir.x * 60
        
        -- Attack when close
        local dist = vector.distance(pos, self.target.position)
        if dist < 50 and self.attack_cooldown <= 0 then
            self.state = "attack"
            self.entity.animation = "attack"
        end
        
    elseif self.state == "attack" then
        -- Attack animation handles damage via animation events
        if self.entity.animation_finished then
            self.state = "chase"
            self.attack_cooldown = 1.5
        end
    end
    
    self.attack_cooldown = math.max(0, self.attack_cooldown - dt)
end

return GolemAI
```

### 5.5 World Generation API

**Terrain Generator (worldgen/terrain.lua):**
```lua
local terrain = {}

-- Register terrain shaper
worldgen.registerTerrainShaper("default", function(x, seed)
    local base = 100
    
    -- Large hills
    local hills = noise.perlin(x * 0.005, seed) * 40
    
    -- Small variation
    local detail = noise.perlin(x * 0.05, seed + 1000) * 8
    
    return math.floor(base + hills + detail)
end)

-- Register cave carver
worldgen.registerCaveCarver("default_caves", function(x, y, seed)
    local cave_noise = noise.perlin3d(x * 0.03, y * 0.03, seed)
    local threshold = 0.4 + (y / 500) * 0.2  -- More caves deeper
    return cave_noise > threshold
end)

-- Register ore placement
worldgen.registerOreVein("crystal_ore", {
    tile = "crystal_ore",
    depth = { min = 150, max = 400 },
    size = { min = 3, max = 8 },
    frequency = 0.0003,
    cluster_noise_scale = 0.1
})

return terrain
```

**Structure Generator (worldgen/structures/dungeon.lua):**
```lua
return function(world, x, y, seed)
    local rng = random.create(seed)
    
    -- Dungeon entrance
    local width = rng.int(20, 30)
    local height = rng.int(15, 25)
    
    -- Carve main room
    for dx = 0, width do
        for dy = 0, height do
            world.setTile(x + dx, y + dy, "air")
            world.setWall(x + dx, y + dy, "dungeon_brick_wall")
        end
    end
    
    -- Place floor
    for dx = 0, width do
        world.setTile(x + dx, y + height, "dungeon_brick")
    end
    
    -- Place chest with loot
    local chest_x = x + math.floor(width / 2)
    world.placeTile(chest_x, y + height - 1, "chest")
    world.setTileData(chest_x, y + height - 1, {
        loot_table = "dungeon_chest"
    })
    
    -- Spawn point for guardian
    world.addSpawnPoint(x + width/2, y + height - 2, "dungeon_guardian")
end
```

### 5.6 Events System

Mods communicate through events — loose coupling, no direct dependencies.

```lua
-- Subscribe to events
events.on("player_damaged", function(player, damage, source)
    if player.hasAccessory("crystal_shield") then
        if math.random() < 0.15 then
            -- Reflect damage
            if source and source.health then
                source.health.current = source.health.current - damage * 0.5
            end
            audio.playSound("crystal_reflect", player.position)
            return true  -- Cancel original damage
        end
    end
end)

-- Emit events
events.emit("boss_defeated", {
    boss = boss_entity,
    player = killer,
    time = game.time
})

-- Built-in events:
-- player_spawned, player_damaged, player_died, player_respawned
-- enemy_spawned, enemy_damaged, enemy_killed
-- tile_placed, tile_broken, tile_updated
-- item_picked_up, item_used, item_crafted
-- world_generated, chunk_loaded, chunk_unloaded
-- day_started, night_started, blood_moon_started
-- boss_spawned, boss_defeated
```

---

## 6. Mod Distribution Ecosystem

### 6.1 Overview

The mod ecosystem runs entirely on GitHub infrastructure at zero cost:

```
┌─────────────────────────────────────────────────────────────────┐
│  MOD TEMPLATE REPO          Fork to start a new mod             │
│  github.com/Gloaming-Forge/mod-template                         │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  MODDER'S REPO              Add content, tag release            │
│  github.com/modder/cool-weapons                                 │
│  └── Releases: cool-weapons-1.0.0.zip (auto-built)              │
└─────────────────────────────────────────────────────────────────┘
                               │
                               │ Add topic: "gloaming-mod"
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  MOD REGISTRY               Auto-discovers, tracks stars        │
│  github.com/Gloaming-Forge/mod-registry                         │
│  └── mod-registry.json                                          │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  MOD WEBSITE                Browse, download, instructions      │
│  gloaming-forge.github.io/mods                                  │
└─────────────────────────────────────────────────────────────────┘
                               │
                               │ User downloads ZIP
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  GAME                       Drop ZIP → auto-install             │
│  mods/install/cool-weapons-1.0.0.zip                            │
└─────────────────────────────────────────────────────────────────┘
```

### 6.2 Mod Template Repository

A ready-to-fork template that gives modders everything they need:

```
mod-template/
├── mod.json                        # Edit this - mod metadata
├── content/
│   └── .gitkeep
├── scripts/
│   └── init.lua                    # Minimal example
├── assets/
│   ├── textures/
│   └── sounds/
├── locale/
│   └── en.json
├── .github/
│   └── workflows/
│       ├── validate.yml            # Validates on every push
│       └── release.yml             # Builds ZIP on version tag
├── README.md                       # Instructions for modders
├── CHANGELOG.md
└── LICENSE                         # MIT default
```

**Template mod.json:**
```json
{
    "id": "my-mod-name",
    "name": "My Awesome Mod",
    "version": "1.0.0",
    "authors": ["YourName"],
    "description": "Describe what your mod does",
    "engine_version": ">=1.0.0",
    "dependencies": ["base-game"],
    "content": {
        "definitions": "content/",
        "scripts": "scripts/",
        "textures": "assets/textures/",
        "sounds": "assets/sounds/",
        "locale": "locale/"
    },
    "tags": []
}
```

### 6.3 GitHub Actions in Template

**validate.yml — Runs on every push:**
- Validates mod.json exists and is valid
- Validates all JSON against schemas
- Static analysis on Lua (forbidden patterns)
- Validates assets are real files

**release.yml — Runs when modder tags a version:**
- Verifies tag matches mod.json version
- Builds clean release ZIP (excludes .git, dev files)
- Creates GitHub Release with ZIP attached
- Generates release notes

**Modder workflow:**
```bash
# Make changes, then:
git add .
git commit -m "Add new weapons"

# Update version in mod.json to 1.1.0, then:
git tag v1.1.0
git push origin main --tags

# GitHub Actions automatically builds v1.1.0 release
```

### 6.4 Mod Registry

**Auto-discovery via GitHub Topics:**

Modders add the topic `gloaming-mod` to their repo. A scheduled GitHub Action:
1. Searches GitHub for repos with that topic
2. Checks each has a valid release with ZIP
3. Validates mod.json from the release
4. Updates mod-registry.json with current star counts

**mod-registry.json:**
```json
{
    "schema_version": "1.0",
    "updated_at": "2025-02-01T16:00:00Z",
    "template_repo": "Gloaming-Forge/mod-template",
    "topic": "gloaming-mod",
    
    "tiers": {
        "popular": { "min_stars": 50 },
        "community": { "min_stars": 10 },
        "new": { "min_stars": 1 },
        "beta": { "min_stars": 0 }
    },
    
    "official": ["base-game", "desert-expansion"],
    
    "mods": {
        "cool-weapons": {
            "repo": "ModderPerson/cool-weapons",
            "description": "50 awesome new weapons",
            "stars": 124,
            "tier": "popular",
            "latest_version": "2.1.0",
            "download_url": "https://github.com/.../cool-weapons-2.1.0.zip",
            "download_size": 3400000,
            "dependencies": ["base-game"],
            "tags": ["weapons", "items"],
            "updated_at": "2025-01-30T00:00:00Z"
        }
    },
    
    "flagged": {}
}
```

### 6.5 Trust Tiers (Star-Based)

Community vetting through GitHub stars:

| Tier | Stars | Badge | Warning |
|------|-------|-------|---------|
| **Official** | — | ★ OFFICIAL | None |
| **Popular** | 50+ | ⭐ POPULAR | None |
| **Community** | 10-49 | ✓ COMMUNITY | None |
| **New** | 1-9 | ○ NEW | "Limited feedback" |
| **Beta** | 0 | ⚠ BETA | "Unreviewed" |
| **Flagged** | — | ⛔ FLAGGED | Blocked |

### 6.6 Mod Website

Static site hosted on GitHub Pages (free), reads mod-registry.json:

```
┌─────────────────────────────────────────────────────────────────┐
│  🎮 GLOAMING MODS                         [How to Install]      │
├─────────────────────────────────────────────────────────────────┤
│  [All ▼]  [Search: _______________]  [Sort: Popular ▼]          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  ★ Desert Expansion                              v1.2.0 │   │
│  │  Full desert biome with Pharaoh boss                    │   │
│  │  ⭐ 847 stars  │  15.2 MB  │  OFFICIAL                  │   │
│  │                          [⬇ Download]  [GitHub]         │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  ⭐ Cool Weapons Pack                            v2.1.0 │   │
│  │  50 awesome new weapons with unique effects             │   │
│  │  ⭐ 124 stars  │  3.4 MB  │  POPULAR                    │   │
│  │                          [⬇ Download]  [GitHub]         │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  ○ Sparkly Particles                             v1.0.0 │   │
│  │  ⚠️ NEW - Limited community feedback                    │   │
│  │  ⭐ 3 stars  │  0.8 MB  │  NEW                          │   │
│  │                          [⬇ Download]  [GitHub]         │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

Website benefits:
- Browse mods without launching game
- Works on mobile
- Direct links to share mods
- SEO for discoverability
- Same site for Steam, itch.io, standalone

### 6.7 Game-Side Installation

**Folder structure:**
```
GameFolder/
├── game.exe
├── mods/
│   ├── install/                 ← USER DROPS ZIPS HERE
│   ├── processed/               ← Successfully installed ZIPs
│   ├── failed/                  ← Failed ZIPs with .error files
│   ├── base-game/               ← Ships with game
│   ├── cool-weapons/            ← Extracted mod
│   └── desert-expansion/        ← Extracted mod
├── config/
│   └── mods.json                ← Enabled/disabled state
└── saves/
```

**Startup flow:**
1. Check `mods/install/` for ZIP files
2. For each ZIP:
   - Extract to temp folder
   - Validate mod.json and content
   - Run security scan
   - Move to `mods/{mod-id}/`
   - Move ZIP to `processed/` (or `failed/` with error)
3. Show notification: "X mods installed"
4. Load all enabled mods

**In-game mod management (minimal):**
```
┌─────────────────────────────────────────────────────────────────┐
│  SETTINGS > MODS                                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  [✓] Base Game (required)                              v1.0.0   │
│  [✓] Desert Expansion                                  v1.2.0   │
│  [✓] Cool Weapons Pack                                 v2.1.0   │
│  [ ] Sparkly Particles                                 v1.0.0   │
│                                                                 │
│  [Open Mods Folder]        [Get More Mods ↗]                    │
│                                                                 │
│  ⚠ Changes require restart                                      │
└─────────────────────────────────────────────────────────────────┘
```

- Checkbox to enable/disable
- [Open Mods Folder] opens file explorer
- [Get More Mods] opens website in browser

### 6.8 Security Layers

Even with community vetting, local security always runs:

**1. Lua Sandbox (runtime):**
```lua
-- REMOVED from Lua environment:
os.execute, os.remove, os.rename, os.exit
io.open, io.popen
loadfile, dofile, load (unrestricted)
package.*, debug.*
rawget, rawset, getfenv, setfenv
```

**2. Static Analysis (on install):**
- Scan for forbidden patterns before enabling
- Detect obfuscated code
- Flag suspicious constructs

**3. Schema Validation:**
- All JSON validated against strict schemas
- Reject malformed content

**4. Mod Isolation:**
- Each mod's data directory isolated
- Cannot access other mods' storage

**5. Auto-Backup:**
- Backup saves before enabling new mods
- Recovery mode if game crashes on startup

### 6.9 Modder Experience Summary

```
1. Fork github.com/Gloaming-Forge/mod-template
2. Edit mod.json (name, description, etc.)
3. Add your content (JSON, Lua, assets)
4. Push to GitHub → auto-validates
5. Tag a version → auto-builds release ZIP
6. Add topic "gloaming-mod" to repo
7. Wait for auto-discovery (or post in Discord)
8. Watch stars roll in
```

### 6.10 Player Experience Summary

```
1. Browse gloaming-forge.github.io/mods
2. Click Download on a mod
3. Drop ZIP into GameFolder/mods/install/
4. Launch game → "Mod installed!" notification
5. Settings → Mods → Enable it
6. Play
```

---

## 7. Base Game Mod

The base game is a mod like any other — it just ships with the engine and demonstrates all systems.

### 7.1 Base Game Scope

| Category | Contents | Purpose |
|----------|----------|---------|
| **Tiles** | ~25 types | Dirt, stone, copper ore, wood, platforms, torches, doors, chests |
| **Items** | ~30 items | Basic tools, weapons, materials, consumables |
| **Enemies** | 2 hostile, 1 passive | Bat, Boar, Sheep |
| **NPCs** | 1 NPC | Guide (explains mechanics) |
| **Biomes** | 1 biome | Forest (surface and underground) |
| **Structures** | 3 types | Trees, small caves, underground cabin |
| **UI** | Complete | All menus, HUD, inventory, crafting |
| **Audio** | Basic set | Essential sound effects, 2-3 music tracks |

### 7.2 Base Game Purpose

1. **Prove the engine works** — Every system exercised
2. **Reference implementation** — "This is how you do it"
3. **Playable experience** — Fun even with minimal content
4. **Modding foundation** — Base content for mods to extend

### 7.3 Base Game File Structure

```
mods/base-game/
├── mod.json
├── scripts/
│   ├── init.lua
│   ├── player.lua              # Player mechanics
│   ├── guide_npc.lua           # Guide AI and dialogue
│   ├── bat_ai.lua              # Bat behavior
│   ├── boar_ai.lua             # Boar behavior
│   └── sheep_ai.lua            # Sheep (passive) behavior
├── content/
│   ├── tiles.json              # 30 tile definitions
│   ├── items.json              # 50 item definitions
│   ├── recipes.json            # 40 recipes
│   ├── enemies.json            # 3 enemies
│   ├── npcs.json               # Guide NPC
│   └── loot_tables.json        # Drop tables
├── worldgen/
│   ├── biomes.json             # Forest biome
│   ├── terrain.lua             # Surface generation
│   ├── caves.lua               # Cave carving
│   ├── ores.lua                # Ore distribution
│   └── structures/
│       ├── tree.lua
│       ├── cabin.lua
│       └── cave_entrance.lua
├── ui/
│   ├── init.lua
│   ├── hud.lua                 # Health, mana, hotbar
│   ├── inventory.lua           # Full inventory
│   ├── crafting.lua            # Crafting interface
│   ├── chest.lua               # Chest UI
│   ├── main_menu.lua
│   ├── pause_menu.lua
│   ├── settings.lua
│   └── styles.lua              # Visual theme
├── assets/
│   ├── textures/
│   │   ├── tiles/              # Tile sprites
│   │   ├── items/              # Item icons
│   │   ├── player/             # Player animations
│   │   ├── enemies/            # Enemy sprites
│   │   ├── npcs/               # NPC sprites
│   │   ├── ui/                 # UI elements
│   │   ├── particles/          # Particle textures
│   │   └── backgrounds/        # Parallax layers
│   ├── sounds/
│   │   ├── player/
│   │   ├── enemies/
│   │   ├── tiles/
│   │   ├── items/
│   │   └── ui/
│   ├── music/
│   │   ├── title.ogg
│   │   ├── forest_day.ogg
│   │   └── boss_fight.ogg
│   └── shaders/
│       ├── lighting.frag
│       └── fog.frag
└── locale/
    └── en.json                 # English strings
```

### 7.4 Progression (Base Game)

**Ore Tier:**
1. Copper → Copper Bar → Copper Tools

**Resources:**
- Sheep → Wool → Cloth (crafting material)

**Goal:** Explore, build, and craft. The base game provides a minimal sandbox experience; mods add bosses, additional ores, and deeper progression.

---

## 8. Development Stages

### Stage 0: Project Skeleton (Week 1-2) ✓
**Goal:** Compilable project with dependencies

- [x] CMake setup with Raylib, EnTT, Lua 5.4/sol2, nlohmann/json, spdlog
- [x] Basic window creation and game loop
- [x] Logging system
- [x] Configuration file loading
- [ ] Debug console (imgui?) — deferred to post-MVP

**Deliverable:** Window opens, logs "Hello World", reads config

**Implemented:** `src/engine/` contains Engine, Window, Input, Config, Log, Time classes. 21 unit tests (GoogleTest) for Config, Log, Time. Fixed-timestep game loop. JSON config with dot-notation access.

---

### Stage 1: Core Rendering (Week 3-5) ✓
**Goal:** Render sprites and tiles

- [x] Renderer abstraction (`IRenderer`)
- [x] Raylib implementation
- [x] Texture loading and atlas management
- [x] Sprite batching
- [x] Camera system (position, zoom, bounds)
- [x] Tile rendering with culling
- [x] Basic parallax background

**Deliverable:** Render a test tilemap with moving camera

**Implemented:** `src/rendering/` contains IRenderer interface, RaylibRenderer, Camera, TextureManager/TextureAtlas, SpriteBatch, TileRenderer, and ParallaxBackground. 47 unit tests for rendering math types, camera transformations, texture atlases, and tile structures. Camera supports position, zoom, rotation, world-to-screen conversion, and bounds clamping. Engine integrated with rendering systems.

---

### Stage 2: ECS Foundation (Week 6-7) ✓
**Goal:** Entity management working

- [x] EnTT integration
- [x] Core components defined
- [x] System scheduling
- [x] Entity factory (spawn from definition)
- [x] Entity queries and iteration

**Deliverable:** Spawn entities, move them, render them

**Implemented:** `src/ecs/` contains Registry (EnTT wrapper with convenience methods), Components (Transform, Velocity, Sprite with animations, Collider with layer/mask, Health with invincibility, LightSource with flicker, ParticleEmitter, Trigger, NetworkSync, tag components, Gravity, Name, Lifetime), Systems (base System class with phase enum, SystemScheduler for multi-phase execution), CoreSystems (MovementSystem, AnimationSystem, LifetimeSystem, HealthSystem, LightUpdateSystem, SpriteRenderSystem, ColliderDebugRenderSystem), EntityFactory (JSON-based entity definitions with spawn callbacks). Full support for entity queries via view(), each(), findFirst(), findAll(), collect().

---

### Stage 3: Chunk System (Week 8-10) ✓
**Goal:** Infinite world support

- [x] Chunk data structure
- [x] Chunk manager (load/unload)
- [x] Chunk serialization (save/load)
- [x] Chunk generation hooks (placeholder noise)
- [x] Tile modification (set/get)
- [x] Dirty chunk tracking for re-render

**Deliverable:** Walk infinitely, changes persist through save/load

**Implemented:** `src/world/` contains Chunk (64x64 tile container with dirty flags and coordinate conversion), ChunkGenerator (Noise utilities for 1D/2D smooth noise and fractal noise, callback system for mod-provided generation, default terrain generator), ChunkManager (load/unload radius management, configurable max loaded chunks, auto-save on unload, stats tracking), WorldFile (binary serialization with magic numbers GLWF/GLCF, CRC32 checksums for data integrity, world metadata with stats), TileMap (unified interface combining ChunkManager+WorldFile+rendering with [[nodiscard]] attributes). 72 unit tests covering coordinate conversion, chunk operations, world persistence, and tile access.

---

### Stage 4: Physics (Week 11-13) ✓
**Goal:** Solid collision

- [x] AABB collision detection
- [x] Tile collision response
- [x] Gravity and jumping
- [x] Slopes (45°)
- [x] One-way platforms
- [x] Swept collision for fast objects
- [x] Trigger volumes

**Deliverable:** Player walks, jumps, collides with world

**Implemented:** `src/physics/` contains AABB (collision primitives with center+halfExtents representation, intersection tests, swept collision), Collision (entity-to-entity collision detection with layer/mask filtering), TileCollision (tile-based collision response with slope and one-way platform support), Trigger (trigger volume tracking with enter/stay/exit callbacks), Raycast (DDA-based tile raycasting, entity raycasting, line-of-sight), PhysicsSystem (main system integrating gravity, velocity, tile collision, entity collision, triggers). 581-line test suite covering AABB operations, swept collision, raycasting, collision layers, and integration tests.

---

### Stage 5: Mod Loader (Week 14-17) ✓
**Goal:** Mods can add content

- [x] Mod discovery and manifest parsing
- [x] Dependency resolution and load order
- [x] Asset loading (textures, sounds)
- [x] LuaJIT integration
- [x] Content registry (tiles, items, enemies)
- [x] Lua bindings for core APIs
- [x] Input abstraction improvement (engine-defined key/button enums instead of raw raylib ints)
- [x] Hot reload (debug builds)

**Deliverable:** Load base-game mod, tiles defined in JSON appear

**Implemented:** `src/mod/` contains ModManifest (manifest parsing with semantic versioning, version requirements, dependency specs, validation), ModLoader (directory scanning for mod.json, topological sort with cycle detection for load order, mod state tracking, enable/disable support), ContentRegistry (tile/item/enemy/recipe definitions with JSON loading, ContentId with mod namespacing, runtime ID assignment), LuaBindings (sol2 integration with sandboxed Lua environment, API bindings for log/content/events/mods/util, per-mod environment isolation, path security validation, instruction count limiting), EventBus (priority-based event handlers, typed EventData, subscription/cancellation), HotReload (polling-based file watcher with timestamp tracking, configurable poll interval). `src/engine/Input.hpp` upgraded with engine-defined Key enum (A-Z, 0-9, F1-F12, arrows, modifiers, special keys) and MouseButton enum replacing raw Raylib ints. 1514 lines of unit tests covering version parsing, manifest validation, content registry, and Lua sandbox.

---

### Stage 6: Lighting System (Week 18-20) ✓
**Goal:** Beautiful lighting

- [x] Per-tile light values (RGB)
- [x] Light propagation algorithm
- [x] Skylight from surface
- [x] Dynamic light sources
- [x] Smooth rendering (corner interpolation)
- [x] Day/night cycle (ambient changes)
- [ ] Lighting shader — deferred; current implementation uses dark overlay rendering which achieves the same visual result without a custom shader

**Deliverable:** Dark caves, torches glow, sunset/sunrise

**Implemented:** `src/lighting/` contains LightMap (per-tile RGB light values with chunk-aligned storage, BFS flood-fill propagation, skylight penetration from surface with configurable falloff, corner interpolation for smooth rendering, cross-chunk boundary propagation), DayNightCycle (configurable cycle with Dawn/Day/Dusk/Night phases, smooth color interpolation between phases via float-safe lerp, sky brightness tracking), LightingSystem (main ECS system coordinating entity LightSource collection, chunk sync with world ChunkManager, periodic recalculation with timing stats, dark overlay rendering in flat and smooth 4-quadrant modes, setLightingEnabled toggle, configurable via config.json). Engine integration includes lighting overlay in the render pipeline (after tiles/sprites, before UI), L key toggle, HUD stats showing light source count, tile count, recalc time, time-of-day, and day count. 54 unit tests covering TileLight operations, ChunkLightData bounds, LightMap CRUD and multi-chunk ops, BFS propagation with falloff/solid blocking/colored light/multiple sources, skylight above/below surface, DayNightCycle phases/transitions/rollover with decreasing-channel regression test, cross-chunk boundary propagation, and edge cases.

---

### Stage 7: Audio System (Week 21-22) ✓
**Goal:** Sound and music

- [x] miniaudio integration
- [x] Sound effect playback
- [x] Positional audio
- [x] Music streaming with crossfade
- [x] Audio API for mods
- [x] Event→sound binding system

**Deliverable:** Footsteps, tile breaking, background music

**Implemented:** `src/audio/` contains AudioSystem (main ECS system coordinating sound+music, volume channels master/sfx/music/ambient, positional audio with distance attenuation and stereo pan, event→sound binding via EventBus, AudioConfig from config.json, AudioStats for HUD), SoundManager (sound definition registry with volume/pitch_variance/cooldown, lazy-loading of audio files, concurrent playback via Raylib Sound aliases, max concurrent sound limiting, quadratic distance falloff for positional audio), MusicManager (music streaming with crossfade via smoothstep interpolation, fade-in/fade-out support, pause/resume, track progress tracking). Full Lua mod API: audio.registerSound, audio.playSound (with optional position), audio.stopSound, audio.stopAllSounds, audio.playMusic (with fade_in/loop options), audio.stopMusic (with fade_out), audio.setVolume/getVolume per channel, audio.bindEvent/unbindEvent, audio.isMusicPlaying, audio.getCurrentMusic. Uses Raylib's audio API (backed by miniaudio) for actual playback. Engine integration includes audio device init/shutdown, per-frame update, camera-based listener position, HUD stats. 45 unit tests covering AudioConfig defaults, SoundDef, distance attenuation (origin/max/beyond/half/quarter/diagonal/zero/negative/symmetric/monotonic), stereo pan (center/left/right/max/clamp/zero/symmetric), crossfade smoothstep math (start/end/middle/quarter/zero/negative/beyond/monotonic/boundaries), AudioSystem construction/config/listener/stats/registration/playback-without-device.

---

### Stage 8: UI System (Week 23-26)
**Goal:** Mod-defined UI working

- [ ] UI layout engine (flexbox-style)
- [ ] Core widgets (box, text, image, button, slider)
- [ ] Input handling (focus, navigation)
- [ ] Lua UI API
- [ ] Base game: HUD
- [ ] Base game: Main menu
- [ ] Base game: Inventory (grid, drag-drop)
- [ ] Base game: Crafting interface
- [ ] Base game: Settings

**Deliverable:** Full UI flow from main menu to gameplay

---

### Stage 9: World Generation (Week 27-30)
**Goal:** Procedural worlds

- [ ] WorldGen API implementation
- [ ] Terrain generation hooks
- [ ] Biome system
- [ ] Cave generation
- [ ] Ore distribution
- [ ] Structure placement
- [ ] Base game: Forest biome complete
- [ ] Base game: Trees, caves, underground cabins

**Deliverable:** Generate new world, explore varied terrain

---

### Stage 10: Gameplay Systems (Week 31-35)
**Goal:** Core loop complete

- [ ] Inventory system
- [ ] Item pickup and drop
- [ ] Tool use (mining, placing)
- [ ] Weapon system (melee swing, projectiles)
- [ ] Damage and health
- [ ] Death and respawn
- [ ] Crafting system
- [ ] Recipe matching and station requirements
- [ ] Base game: All items, tools, weapons working

**Deliverable:** Mine, craft, fight, progress

---

### Stage 11: Enemies & AI (Week 36-38)
**Goal:** Things that fight back

- [ ] Enemy spawning system
- [ ] AI behavior API
- [ ] Pathfinding (simple: ground follow, flying)
- [ ] Base game: Bat, Boar, Sheep AI
- [ ] Enemy drops
- [ ] Despawning rules

**Deliverable:** Night brings danger

---

### Stage 12: NPCs (Week 39-40)
**Goal:** Living world

- [ ] NPC entity type
- [ ] Housing validation
- [ ] NPC spawning conditions
- [ ] Dialogue system
- [ ] Shop system
- [ ] Base game: Guide NPC

**Deliverable:** Guide moves in, offers tips

---

### Stage 13: Boss Framework & Events (Week 41-43)
**Goal:** Boss infrastructure for mods

- [ ] Boss entity framework
- [ ] Boss phases and patterns
- [ ] Boss health bar UI
- [ ] Summon item system
- [ ] Blood moon event (increased spawns)

**Deliverable:** Boss framework ready for mods to use; blood moon event functional

---

### Stage 14: Polish & Release Prep (Week 44-48)
**Goal:** MVP complete

- [ ] Performance optimization
- [ ] Bug fixing
- [ ] Balance tuning
- [ ] Particle effects pass
- [ ] Post-processing effects (bloom, fog)
- [ ] Settings completeness
- [ ] World selection UI
- [ ] Mod browser UI (local mods)
- [ ] Documentation complete
- [ ] Example mods
- [ ] README, CONTRIBUTING, build instructions

**Deliverable:** Releasable MVP

---

### Post-MVP Roadmap

| Phase | Focus |
|-------|-------|
| **Community Building** | Release, gather feedback, support modders |
| **Official Content Pack 1** | More biomes, enemies, bosses (as a mod) |
| **Multiplayer** | Server architecture, sync, 8-16 players |
| **Steam Release** | Store page, achievements, workshop |
| **Official Content Pack 2** | Hardmode equivalent |
| **Scripting Expansion** | More powerful Lua APIs |
| **Tools** | In-game mod browser, asset editors |

---

## 9. Documentation Requirements

### 9.1 Documentation as Deliverables

Each stage must include documentation before it's considered complete.

| Stage | Required Docs |
|-------|---------------|
| Mod Loader | Mod structure guide, mod.json reference |
| Content Registry | JSON schemas for all content types |
| Lua Bindings | API reference for each module |
| World Gen | World generation tutorial, biome guide |
| UI System | UI component reference, styling guide |
| Every system | Example code, common patterns |

### 9.2 Documentation Structure

```
docs/
├── getting-started/
│   ├── installation.md
│   ├── first-mod.md
│   └── building-from-source.md
├── modding-guide/
│   ├── mod-structure.md
│   ├── content-types.md
│   ├── scripting-basics.md
│   ├── worldgen.md
│   ├── ui-customization.md
│   └── best-practices.md
├── api-reference/
│   ├── content-api.md
│   ├── entity-api.md
│   ├── worldgen-api.md
│   ├── ui-api.md
│   ├── audio-api.md
│   ├── events-api.md
│   └── utility-api.md
├── json-schemas/
│   ├── mod.schema.json
│   ├── tiles.schema.json
│   ├── items.schema.json
│   ├── enemies.schema.json
│   └── ...
├── tutorials/
│   ├── adding-a-tile.md
│   ├── creating-an-enemy.md
│   ├── custom-worldgen.md
│   ├── ui-overhaul.md
│   └── boss-creation.md
└── examples/
    ├── minimal-content-mod/
    ├── custom-biome/
    ├── ui-reskin/
    └── total-conversion/
```

### 9.3 Documentation Standards

- Every public API function documented with signature, description, parameters, return value, example
- JSON schemas provided for validation and editor support
- Runnable example mods for every major feature
- Changelog maintained from day one

---

## 10. Community Strategy

### 10.1 Pre-Release

| Action | Purpose |
|--------|---------|
| GitHub repo public early | Build anticipation, gather feedback |
| Devlogs (weekly/biweekly) | Show progress, build audience |
| Discord server | Direct community interaction |
| Alpha testing | Early modders identify API pain points |

### 10.2 Release

| Action | Purpose |
|--------|---------|
| Clear README | First impression matters |
| Video trailer | Show visual potential |
| Example mods | Lower barrier to entry |
| "First mod" tutorial | Immediate engagement |

### 10.3 Post-Release

| Action | Purpose |
|--------|---------|
| Featured mods | Spotlight community work |
| Mod jams | Time-limited creation events |
| Official content packs | Prove continued development |
| API feedback loop | Improve based on modder needs |
| Contributor recognition | Thank contributors publicly |

### 10.4 Attracting Artists

| Strategy | Details |
|----------|---------|
| Style guide | Clear direction for interested artists |
| Placeholder art feedback | "This works, this needs polish" |
| Asset pack system | Artists can release standalone packs |
| Revenue share (future) | If Steam revenue, share with contributors |
| Credits system | Prominent attribution |

---

## 11. Distribution & Licensing

### 11.1 Recommended Licenses

| Component | License | Rationale |
|-----------|---------|-----------|
| Engine code | MIT | Maximum adoption |
| Base game mod | MIT | Reference implementation, forkable |
| Official art | CC-BY-SA 4.0 | Shareable with attribution |
| Documentation | CC-BY 4.0 | Freely usable |

### 11.2 Steam Strategy

**What we sell:**
- Convenience (no building from source)
- Automatic updates
- Steam Workshop integration
- Achievements
- Cloud saves
- Support the project

**Price point:** $4.99

**Workshop flow:**
- Mods published to Workshop
- One-click subscribe/install
- Automatic updates
- Dependency management

### 11.3 GitHub Organization Structure

**Organization:** [github.com/Gloaming-Forge](https://github.com/Gloaming-Forge)

| Repository | Purpose | URL |
|------------|---------|-----|
| `gloaming` | Main engine + base-game mod | `Gloaming-Forge/gloaming` |
| `mod-template` | Fork-to-start template for modders | `Gloaming-Forge/mod-template` |
| `mod-registry` | JSON registry + discovery bot | `Gloaming-Forge/mod-registry` |
| `gloaming-forge.github.io` | Mod browser website + docs | `Gloaming-Forge/gloaming-forge.github.io` |
| `desert-expansion` | Official content pack (post-MVP) | `Gloaming-Forge/desert-expansion` |

**GitHub Topic for Mods:** `gloaming-mod`

### 11.4 Repository Setup

```
README.md               # Project overview, screenshots
LICENSE                 # MIT license
CONTRIBUTING.md         # How to contribute
BUILDING.md             # Build from source
CODE_OF_CONDUCT.md      # Community standards
CHANGELOG.md            # Version history
ENGINE_SPEC.md          # This document

.github/
├── ISSUE_TEMPLATE/
│   ├── bug_report.md
│   ├── feature_request.md
│   └── mod_api_request.md
├── PULL_REQUEST_TEMPLATE.md
└── workflows/
    ├── build.yml       # CI build
    └── release.yml     # Release automation
```

---

## 12. Open Questions

### 12.1 Needs Decision

| Question | Options | Notes |
|----------|---------|-------|
| Mod signing | Required, optional, none | Security vs convenience |

### 12.2 Research Needed

- LuaJIT sandboxing (prevent filesystem access from mods)
- Raylib shader limitations for lighting
- Best practices for hot-reloading textures
- JSON schema tooling for editor support

---

## Appendix A: Content Checklist (Base Game)

### Tiles (~30)
- [ ] Dirt, Grass, Stone, Sand
- [ ] Copper Ore
- [ ] Wood, Leaves
- [ ] Platforms (wood, stone)
- [ ] Torches
- [ ] Workbench, Furnace, Anvil
- [ ] Chest, Door
- [ ] Wood Wall, Stone Wall, Dirt Wall
- [ ] Water (liquid tile)

### Items (~30)
- [ ] Copper Pickaxe
- [ ] Copper Axe
- [ ] Copper Sword
- [ ] Copper Broadsword
- [ ] Wood Bow, Copper Bow
- [ ] Arrows (wood, flaming)
- [ ] Copper Bar
- [ ] Wood (item), Stone (item)
- [ ] Wool, Cloth
- [ ] Torches, Platforms
- [ ] Health Potion (minor, lesser)

### Enemies (2 hostile, 1 passive)
- [ ] Bat (flies, erratic)
- [ ] Boar (charges, ground enemy)
- [ ] Sheep (passive, drops wool)

### NPCs (1)
- [ ] Guide (tips, recipe hints)

---

## Appendix B: Example Mod Ideas

Suggestions for early community mods (or official expansions):

| Mod | Description |
|-----|-------------|
| **Desert Biome** | Sand, cacti, sandstorm, mummies, pyramid structure |
| **Snow Biome** | Snow, ice, blizzard, ice golems, igloo structure |
| **Jungle Biome** | Jungle trees, vines, piranhas, temple structure |
| **Ocean Biome** | Water expansion, coral, sharks, underwater caves |
| **Magic System** | Mana, spells, staves, magic enemies |
| **Hardmode** | Post-boss difficulty spike, new ores, new enemies |
| **Building Pack** | More furniture, decorations, building materials |
| **Quality of Life** | Minimap, recipe browser, auto-sort, teleporters |
| **Dark Souls Mode** | Higher difficulty, fewer drops, permadeath option |

---

## Appendix C: Technology & Project Links

**Gloaming Project:**
- [Engine Repository](https://github.com/Gloaming-Forge/gloaming)
- [Mod Template](https://github.com/Gloaming-Forge/mod-template)
- [Mod Registry](https://github.com/Gloaming-Forge/mod-registry)
- [Mod Browser](https://gloaming-forge.github.io/mods)

**Dependencies:**
- [Raylib](https://www.raylib.com/)
- [EnTT](https://github.com/skypjack/entt)
- [LuaJIT](https://luajit.org/)
- [miniaudio](https://miniaud.io/)
- [nlohmann/json](https://github.com/nlohmann/json)
- [sol2](https://github.com/ThePhD/sol2) (C++ Lua bindings)
- [ENet](http://enet.bespin.org/)

---

*Document Version: 0.6 (Stages 0-7 Complete)*
*Last Updated: February 2026*
*Status: Draft — Stage 7 verified complete, Stage 8 next*
