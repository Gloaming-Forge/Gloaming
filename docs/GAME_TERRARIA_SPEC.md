# Gloaming: Sandbox Survival Game
### Design Document — Terraria-Inspired 2D Sandbox

> A side-scrolling sandbox survival game built as a mod on the Gloaming engine. Mine, craft, fight, explore.

This document describes the design for the **first flagship game** built on the Gloaming engine. It is a mod — it uses the same APIs available to all community mods.

For the engine specification, see [ENGINE_SPEC.md](ENGINE_SPEC.md).

---

## Table of Contents

1. [Game Overview](#1-game-overview)
2. [Engine Configuration](#2-engine-configuration)
3. [Content](#3-content)
4. [World Generation](#4-world-generation)
5. [Gameplay Systems](#5-gameplay-systems)
6. [UI Design](#6-ui-design)
7. [Audio Design](#7-audio-design)
8. [File Structure](#8-file-structure)
9. [Content Checklist](#9-content-checklist)
10. [Future Expansions](#10-future-expansions)

---

## 1. Game Overview

### 1.1 Concept

A 2D side-scrolling sandbox where the player mines resources, crafts tools and weapons, builds structures, and fights enemies in a procedurally generated world with a day/night cycle.

Inspired by Terraria with Ori-inspired visual polish.

### 1.2 Core Loop

```
Explore → Mine → Craft → Build → Fight → Explore deeper
```

### 1.3 Key Features

- Procedurally generated infinite world with caves and biomes
- Tile-based terrain that can be mined and placed
- Crafting system with workstations
- Day/night cycle affecting enemy spawns
- Underground exploration with lighting mechanics
- NPC that provides guidance

---

## 2. Engine Configuration

This game uses the engine's side-view mode with standard platformer physics.

```lua
-- init.lua for the sandbox game mod
function mod.init()
    -- Configure engine for side-scrolling platformer
    game_mode.set_physics("platformer")     -- gravity down at 980
    input_actions.register_platformer_defaults()

    -- Camera follows the player horizontally
    camera.set_mode("free_follow")
    camera.set_smoothness(5.0)
    camera.set_deadzone(64, 32)

    -- Enable full lighting (caves, torches, day/night)
    -- (lighting is on by default — no action needed)

    -- Load content
    content.loadTiles("content/tiles.json")
    content.loadItems("content/items.json")
    content.loadEnemies("content/enemies.json")
    content.loadRecipes("content/recipes.json")
end
```

---

## 3. Content

### 3.1 Tiles (~25 types)

| Tile | Properties | Notes |
|------|------------|-------|
| Dirt | Soft, common | Surface layer |
| Grass | Dirt variant | Top of dirt, grows |
| Stone | Hard | Deep underground |
| Sand | Soft, falls | Beach/desert areas |
| Copper Ore | Hard, drops ore | Underground |
| Wood | Medium | From trees |
| Leaves | Non-solid | Tree canopy |
| Platform (wood) | One-way | Player can jump through |
| Platform (stone) | One-way | Crafted |
| Torch | Non-solid, light source | 12-tile radius warm light |
| Workbench | Solid, crafting station | Basic recipes |
| Furnace | Solid, crafting station | Smelting |
| Anvil | Solid, crafting station | Metal items |
| Chest | Solid, storage | 40-slot container |
| Door | Toggleable | Opens/closes |
| Wood Wall | Background wall | Prevents enemy spawns |
| Stone Wall | Background wall | Stronger variant |
| Dirt Wall | Background wall | Natural underground |
| Water | Liquid | Flows, fills cavities |

### 3.2 Items (~30 items)

**Tools:**
| Item | Type | Power | Notes |
|------|------|-------|-------|
| Copper Pickaxe | Tool | 35% | Mines most tiles |
| Copper Axe | Tool | 35% | Chops trees faster |

**Weapons:**
| Item | Type | Damage | Notes |
|------|------|--------|-------|
| Copper Sword | Melee | 10 | Fast swing |
| Copper Broadsword | Melee | 18 | Wide arc, slower |
| Wood Bow | Ranged | 8 | Requires arrows |
| Copper Bow | Ranged | 14 | Better range |

**Ammo:**
| Item | Type | Damage | Notes |
|------|------|--------|-------|
| Wood Arrow | Ammo | 3 | Basic arrow |
| Flaming Arrow | Ammo | 5 | Lights up, fire damage |

**Materials:**
| Item | Notes |
|------|-------|
| Copper Ore (item) | Mined from ore tiles |
| Copper Bar | Smelted at furnace (3 ore = 1 bar) |
| Wood (item) | Chopped from trees |
| Stone (item) | Mined from stone |
| Gel | Dropped by slimes |
| Wool | Dropped by sheep |
| Cloth | Crafted from wool |

**Consumables:**
| Item | Effect | Notes |
|------|--------|-------|
| Minor Health Potion | +50 HP | Crafted or found |
| Lesser Health Potion | +100 HP | Crafted |

**Placeable:**
| Item | Notes |
|------|-------|
| Torches | Crafted from wood + gel |
| Platforms | Crafted from wood |

### 3.3 Enemies

| Enemy | Health | Damage | Behavior | Spawn |
|-------|--------|--------|----------|-------|
| Bat | 15 | 8 | Flies erratically toward player | Underground, dark areas |
| Boar | 40 | 15 | Charges at player on ground | Surface, day and night |
| Sheep | 20 | 0 | Wanders passively | Surface, daytime |

**Drop Tables:**
- Bat: 1-2 Gel (100%)
- Boar: 1-3 Leather (100%), Copper Coin 5-15 (100%)
- Sheep: 1-3 Wool (100%)

### 3.4 NPCs

| NPC | Role | Spawn Condition |
|-----|------|-----------------|
| Guide | Tips, recipe hints | Available from start |

The Guide provides contextual tips and shows what items can be crafted from materials the player has.

### 3.5 Recipes

**Workbench:**
| Result | Ingredients |
|--------|-------------|
| Wood Platform (5) | Wood (1) |
| Workbench | Wood (10) |
| Chest | Wood (8) |
| Wood Wall (4) | Wood (1) |
| Wood Bow | Wood (10) |
| Wood Arrow (5) | Wood (1), Stone (1) |

**Furnace (built at Workbench):**
| Result | Ingredients |
|--------|-------------|
| Furnace | Stone (20), Wood (4) |
| Copper Bar | Copper Ore (3) |

**Anvil (built at Furnace):**
| Result | Ingredients |
|--------|-------------|
| Anvil | Copper Bar (5) |
| Copper Pickaxe | Copper Bar (9), Wood (3) |
| Copper Axe | Copper Bar (6), Wood (3) |
| Copper Sword | Copper Bar (6) |
| Copper Broadsword | Copper Bar (10) |
| Copper Bow | Copper Bar (5), Wood (5) |

**Hand-crafted (no station):**
| Result | Ingredients |
|--------|-------------|
| Torch (3) | Wood (1), Gel (1) |
| Minor Health Potion | Gel (3), Mushroom (2) |

---

## 4. World Generation

### 4.1 Terrain Shape

> **Note:** The `worldgen` and `noise` APIs shown below are **planned** (Stage 10).
> The code examples illustrate the intended design; these functions are not yet implemented.

```lua
worldgen.registerTerrainShaper("default", function(x, seed)
    local base = 100
    local hills = noise.perlin(x * 0.005, seed) * 40
    local detail = noise.perlin(x * 0.05, seed + 1000) * 8
    return math.floor(base + hills + detail)
end)
```

### 4.2 Underground Layers

| Depth | Layer | Tiles | Notes |
|-------|-------|-------|-------|
| 0-surface | Sky | Air | Above ground |
| surface-20 | Topsoil | Dirt, grass surface | Trees, grass |
| 20-100 | Underground | Stone, dirt pockets | Copper ore |
| 100-200 | Deep | Stone | More ore, larger caves |
| 200+ | Abyss | Stone, obsidian | Rare materials |

### 4.3 Cave Generation

```lua
worldgen.registerCaveCarver("default_caves", function(x, y, seed)
    local cave_noise = noise.perlin3d(x * 0.03, y * 0.03, seed)
    local threshold = 0.4 + (y / 500) * 0.2  -- More caves deeper
    return cave_noise > threshold
end)
```

### 4.4 Ore Distribution

| Ore | Depth Range | Vein Size | Frequency |
|-----|-------------|-----------|-----------|
| Copper | 20-200 | 3-8 tiles | Common |

### 4.5 Structures

| Structure | Depth | Contents |
|-----------|-------|----------|
| Trees | Surface | Wood, leaves |
| Small caves | 10-50 | Open areas near surface |
| Underground cabin | 50-200 | Chest with loot, furniture |

---

## 5. Gameplay Systems

### 5.1 Player Movement

- Standard platformer controls (WASD/arrows + Space for jump)
- Gravity: 980 px/s^2
- Jump height: ~4 tiles
- Walk speed: 200 px/s
- Fall damage above threshold

### 5.2 Mining

- Hold attack button on tile to mine
- Each tile has hardness — time to break depends on tool power
- Pickaxe for stone/ore, axe for wood (faster)
- Mined tiles drop as items
- Adjacent tiles update visuals

### 5.3 Building

- Select tile/item from hotbar
- Click to place at cursor position
- Must be within placement range (~5 tiles)
- Cannot overlap with player or solid entities

### 5.4 Inventory

- 40-slot grid inventory
- 10-slot hotbar (first row of inventory)
- Stack sizes vary by item type
- Click to pick up, click to place
- Right-click to split stack

### 5.5 Combat

- Melee: swing arc in facing direction
- Ranged: fire projectile toward cursor
- Knockback on hit (both player and enemy)
- Invincibility frames after taking damage
- Enemies drop items on death

### 5.6 Day/Night Cycle

| Phase | Duration | Effect |
|-------|----------|--------|
| Dawn | 60s | Light increasing |
| Day | 300s | Full brightness, peaceful spawns |
| Dusk | 60s | Light decreasing |
| Night | 180s | Dark, hostile spawns increase |

### 5.7 Lighting

- Surface lit by skylight (follows day/night)
- Underground completely dark
- Torches provide warm light (12 tile radius)
- Player can hold torch for mobile light
- Enemies spawn more in dark areas

### 5.8 NPC Housing

The Guide requires a valid house to remain:
- Enclosed room (walls, floor, ceiling)
- Background walls
- A door
- A light source (torch)
- A flat surface (table/workbench)

---

## 6. UI Design

### 6.1 HUD

```
┌─────────────────────────────────────────────────┐
│  [♥♥♥♥♥♥♥♥♥♥]  Health: 100/100                  │
│                                                  │
│                                                  │
│                                   [Time: Day]    │
│                                                  │
│  ┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐               │
│  │1 │2 │3 │4 │5 │6 │7 │8 │9 │0 │  Hotbar       │
│  └──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘               │
└─────────────────────────────────────────────────┘
```

### 6.2 Inventory Screen

- Press Tab/Escape to open
- 4 rows x 10 columns grid
- Crafting panel on the right
- Equipment slots (future)

### 6.3 Crafting

- Available recipes shown based on inventory contents
- Filter by crafting station proximity
- Shows required materials with have/need counts
- Click to craft

### 6.4 Main Menu

- New World (seed input)
- Load World (world list)
- Settings
- Quit

---

## 7. Audio Design

### 7.1 Music

| Track | When |
|-------|------|
| Title theme | Main menu |
| Forest day | Daytime, surface |
| Forest night | Nighttime, surface |
| Underground | Below surface |
| Boss fight | Boss encounters (future) |

### 7.2 Sound Effects

| Sound | Trigger |
|-------|---------|
| Dirt break/place | Mining/placing dirt |
| Stone break/place | Mining/placing stone |
| Wood chop | Mining wood |
| Swing | Melee attack |
| Bow fire | Ranged attack |
| Arrow hit | Projectile impact |
| Player hurt | Taking damage |
| Enemy hurt/death | Damaging/killing enemies |
| Item pickup | Collecting items |
| Chest open/close | Interacting with chest |
| Door open/close | Toggling door |
| Menu click | UI interaction |

---

## 8. File Structure

```
mods/base-game/
├── mod.json
├── scripts/
│   ├── init.lua                # Entry point, engine config
│   ├── player.lua              # Player mechanics (movement, mining, combat)
│   ├── guide_npc.lua           # Guide AI and dialogue
│   ├── bat_ai.lua              # Bat behavior
│   ├── boar_ai.lua             # Boar behavior
│   └── sheep_ai.lua            # Sheep (passive) behavior
├── content/
│   ├── tiles.json              # ~25 tile definitions
│   ├── items.json              # ~30 item definitions
│   ├── recipes.json            # ~20 recipes
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
│   ├── hud.lua                 # Health, hotbar, time
│   ├── inventory.lua           # Full inventory grid
│   ├── crafting.lua            # Crafting interface
│   ├── chest.lua               # Chest UI
│   ├── main_menu.lua
│   ├── pause_menu.lua
│   ├── settings.lua
│   └── styles.lua              # Visual theme
├── assets/
│   ├── textures/
│   │   ├── tiles/
│   │   ├── items/
│   │   ├── player/
│   │   ├── enemies/
│   │   ├── npcs/
│   │   ├── ui/
│   │   ├── particles/
│   │   └── backgrounds/
│   ├── sounds/
│   └── music/
└── locale/
    └── en.json
```

---

## 9. Content Checklist

### Tiles (~25)
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
- [ ] Copper Pickaxe, Copper Axe
- [ ] Copper Sword, Copper Broadsword
- [ ] Wood Bow, Copper Bow
- [ ] Arrows (wood, flaming)
- [ ] Copper Ore (item), Copper Bar
- [ ] Wood (item), Stone (item)
- [ ] Wool, Cloth, Gel, Leather
- [ ] Torches, Platforms
- [ ] Health Potions (minor, lesser)

### Enemies (3)
- [ ] Bat (flies, erratic)
- [ ] Boar (charges, ground enemy)
- [ ] Sheep (passive, drops wool)

### NPCs (1)
- [ ] Guide (tips, recipe hints)

### UI Screens
- [ ] Main menu
- [ ] HUD (health, hotbar, time)
- [ ] Inventory
- [ ] Crafting
- [ ] Chest
- [ ] Pause menu
- [ ] Settings

### Audio
- [ ] 3 music tracks (title, day, underground)
- [ ] ~15 sound effects

---

## 10. Future Expansions

These would be separate mods that depend on `base-game`:

| Mod | Description |
|-----|-------------|
| **Desert Biome** | Sand, cacti, sandstorm, mummies, pyramid |
| **Snow Biome** | Snow, ice, blizzard, ice golems, igloo |
| **Jungle Biome** | Jungle trees, vines, piranhas, temple |
| **Ocean Biome** | Water expansion, coral, sharks, caves |
| **Magic System** | Mana, spells, staves, magic enemies |
| **Hardmode** | Post-boss difficulty spike, new ores, new enemies |
| **Building Pack** | More furniture, decorations, materials |
| **Boss Pack** | Boss encounters with phases and unique drops |
| **Quality of Life** | Minimap, recipe browser, auto-sort, teleporters |

---

*This is a game design document for a mod built on the Gloaming engine.*
*For engine internals, see [ENGINE_SPEC.md](ENGINE_SPEC.md).*
