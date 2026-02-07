# Gloaming

A moddable 2D game engine that supports any style of game — platformers, RPGs, shooters, sandboxes. Open source. Infinitely extensible.

**Organization:** [Gloaming-Forge](https://github.com/Gloaming-Forge)

## About

Gloaming is not a game — it's a **platform for building 2D games**. The engine provides infrastructure (rendering, physics, ECS, audio, UI, mod loading, gameplay systems), while all game content and rules are provided by mods.

Any style of 2D game can be built on Gloaming:

| Game Style | Example | Engine Features Used |
|------------|---------|---------------------|
| **Side-scrolling sandbox** | Terraria | Platformer physics, mining, day/night lighting, infinite world |
| **Top-down RPG** | Pokemon Leaf Green | Grid movement, dialogue, tile layers, room-based camera |
| **Side-scrolling flight** | Sopwith | Flight physics, auto-scroll camera, projectiles |
| **Metroidvania** | Hollow Knight | Platformer physics, room transitions, state machines |
| **Top-down action** | Zelda | Zero gravity, pathfinding, state machines |

Each game is a mod. The engine provides the systems, the mod provides the rules and content.

## Technology Stack

| Component | Choice |
|-----------|--------|
| **Language** | C++20 |
| **Build System** | CMake |
| **Rendering** | Raylib (abstracted via IRenderer) |
| **ECS** | EnTT |
| **Scripting** | Lua 5.4 (sol2 bindings) |
| **Data Format** | JSON + binary |
| **Audio** | miniaudio (via Raylib) |
| **Networking** | ENet (future) |

## Architecture

```
┌──────────────────────────────────────────────────────────┐
│  GAMES        Sandbox, RPG, Flight, Community Games      │
├──────────────────────────────────────────────────────────┤
│  MOD API      Content Registry, WorldGen, UI, Audio      │
├──────────────────────────────────────────────────────────┤
│  GAMEPLAY     GridMovement, Camera, Pathfinding, FSM,    │
│  SYSTEMS      Dialogue, InputActions, TileLayers         │
├──────────────────────────────────────────────────────────┤
│  ENGINE       ECS, Renderer, Physics, Audio, Lighting,   │
│  CORE         Chunks, UI Layout, Mod Loader, Input       │
└──────────────────────────────────────────────────────────┘
```

## Engine Features

### Core Systems
- Sprite rendering with batching and texture atlases
- Chunk-based infinite world with binary serialization
- AABB collision, swept collision, raycasting, trigger volumes
- Configurable physics (gravity in any direction, or zero for top-down)
- Per-tile lighting with smooth propagation and day/night cycle
- Flexbox-style UI layout engine with screen management
- Positional audio with music crossfade
- ECS powered by EnTT

### Gameplay Systems
- **GameMode** — Physics presets for platformer, top-down, flight, zero-G
- **GridMovement** — Snap-to-grid movement with smooth interpolation (RPG-style)
- **CameraController** — FreeFollow, GridSnap, AutoScroll, RoomBased, Locked modes
- **InputActions** — Abstract action-to-key mapping with game-type presets
- **Pathfinding** — A* on tile grid with diagonal support
- **StateMachine** — FSM component for entity behaviors
- **DialogueSystem** — Typewriter text boxes with portraits and branching choices
- **TileLayers** — Multi-layer tile rendering (background, ground, decoration, foreground)

### Modding
- **Mods are first-class** — every game is a mod using the same APIs
- **Data-driven** — JSON definitions for tiles, items, enemies, recipes
- **Lua scripting** — custom behaviors, AI, world generation, UI layouts
- **Events system** — loose coupling between mods via event bus
- **Sandboxed** — mods cannot access the filesystem or execute system commands
- **Hot reload** — change a file, see results immediately in debug builds

## Quick Start: Building a Game

A mod declares its game type in `init.lua`:

```lua
-- Platformer (Terraria-style)
game_mode.set_physics("platformer")
camera.set_mode("free_follow")
input_actions.register_platformer_defaults()

-- Top-down RPG (Pokemon-style)
game_mode.set_physics("topdown")
camera.set_mode("room_based")
camera.set_room_size(320, 240)
input_actions.register_topdown_defaults()
grid_movement.add(player, { grid_size = 16, move_speed = 4 })

-- Flight game (Sopwith-style)
game_mode.set_physics("flight")
camera.set_mode("auto_scroll")
camera.set_scroll_speed(100, 0)
input_actions.register_flight_defaults()
```

## Project Structure

```
gloaming/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── config.json
├── docs/
│   ├── ENGINE_SPEC.md          # Engine specification
│   └── GAME_TERRARIA_SPEC.md   # Terraria-clone game design
├── src/
│   ├── engine/          # Core engine (window, input, config, time)
│   ├── ecs/             # EnTT wrapper, components, systems
│   ├── rendering/       # Renderer, camera, sprites, tiles
│   ├── world/           # Chunk manager, tile map, serialization
│   ├── physics/         # Collision, raycasting, triggers
│   ├── lighting/        # Light propagation, day/night
│   ├── audio/           # Sound and music systems
│   ├── ui/              # Layout engine and widgets
│   ├── mod/             # Mod loader, Lua bindings, hot reload
│   └── gameplay/        # Grid movement, camera, pathfinding, etc.
├── mods/                # Game mods
└── tests/               # Unit tests
```

## Building from Source

### Prerequisites
- C++20 compatible compiler (GCC 11+, Clang 13+, MSVC 2022+)
- CMake 3.20+
- Git

### Build

```bash
git clone https://github.com/Gloaming-Forge/Gloaming.git
cd Gloaming
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Creating Mods

1. Fork the [mod template](https://github.com/Gloaming-Forge/mod-template)
2. Edit `mod.json` with your mod's metadata
3. Configure the game type in `init.lua` (physics, camera, input)
4. Add content via JSON definitions and Lua scripts
5. Tag a release — GitHub Actions builds your mod automatically
6. Add the `gloaming-mod` topic to your repo for auto-discovery

See the [Engine Spec](docs/ENGINE_SPEC.md) for full API documentation.

## Target Platforms

- **Primary:** Windows (64-bit)
- **Secondary:** Linux, macOS
- **Stretch:** Steam Deck verified

## Contributing

Contributions are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

The engine is licensed under the [MIT License](LICENSE).

## Links

- [Engine Specification](docs/ENGINE_SPEC.md)
- [Terraria-Clone Game Design](docs/GAME_TERRARIA_SPEC.md)
- [Mod Template](https://github.com/Gloaming-Forge/mod-template)
- [Mod Registry](https://github.com/Gloaming-Forge/mod-registry)
- [Mod Browser](https://gloaming-forge.github.io/mods)
