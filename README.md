# Gloaming

An open-source, moddable 2D sandbox engine with Ori-inspired visuals. We build the platform — the community builds the worlds.

**Organization:** [Gloaming-Forge](https://github.com/Gloaming-Forge)

## About

Gloaming is not a game — it's a **platform for games**. The engine provides infrastructure (rendering, physics, ECS, audio, UI, mod loading), while all content — tiles, items, enemies, world generation, UI layouts, and more — is provided by mods. The "base game" is simply a reference mod that ships with the engine.

This repository contains the **core engine** and the **base game mod**.

## Technology Stack

| Component | Choice |
|-----------|--------|
| **Language** | C++20 |
| **Build System** | CMake |
| **Rendering** | Raylib (abstracted) |
| **ECS** | EnTT |
| **Scripting** | Lua (LuaJIT) |
| **Data Format** | JSON + binary |
| **Audio** | miniaudio |
| **Networking** | ENet (future) |

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  MODS LAYER          Base Game, Content Packs, etc.     │
├─────────────────────────────────────────────────────────┤
│  MOD API LAYER       Content Registry, WorldGen, UI,    │
│                      Audio, Gameplay Events              │
├─────────────────────────────────────────────────────────┤
│  ENGINE CORE         ECS, Renderer, Physics, Audio,     │
│                      Mod Loader, Chunk Manager, Input    │
└─────────────────────────────────────────────────────────┘
```

## Features

### Engine
- Sprite rendering with batching and texture atlases
- Per-tile lighting with smooth propagation and day/night cycle
- Chunk-based infinite world with save/load
- AABB and swept collision, raycasting, trigger volumes
- Flexbox-style UI layout engine
- Positional audio with music crossfade
- ECS powered by EnTT
- Full mod loading with dependency resolution and hot reload (debug builds)

### Modding
- **Mods are first-class** — the base game uses the same APIs as community mods
- **Data-driven** — JSON definitions for tiles, items, enemies, recipes, biomes
- **Lua scripting** — custom behaviors, AI, world generation, UI layouts
- **Events system** — loose coupling between mods via event bus
- **Sandboxed** — mods cannot access the filesystem or execute system commands
- **Hot reload** — change a file, see results immediately in debug builds

### Base Game Mod
- ~30 tile types, ~50 items, 3 enemy types, 1 NPC, 1 boss
- Forest biome with cave systems and underground structures
- Copper / Iron / Silver ore progression
- Complete UI: main menu, HUD, inventory, crafting, settings
- Giant Slime boss fight

## Project Structure

```
gloaming/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── GLOAMING_SPEC.md
├── src/
│   ├── main.cpp
│   ├── engine/          # Core engine
│   ├── ecs/             # EnTT wrapper, components, systems
│   ├── rendering/       # Renderer abstraction, camera, shaders
│   ├── world/           # Chunk manager, tile map, serialization
│   ├── physics/         # Collision, raycasting, triggers
│   ├── audio/           # Sound and music systems
│   ├── ui/              # Layout engine and primitives
│   ├── mod/             # Mod loader, Lua bindings, hot reload
│   └── api/             # Public mod API surface
├── mods/
│   └── base-game/       # Reference mod that ships with the engine
├── docs/                # Modding guide, API reference, tutorials
└── tools/               # Mod validator, asset packer
```

## Building from Source

### Prerequisites
- C++20 compatible compiler (GCC 11+, Clang 13+, MSVC 2022+)
- CMake 3.20+
- Git

### Build

```bash
git clone https://github.com/Gloaming-Forge/gloaming.git
cd gloaming
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Creating Mods

1. Fork the [mod template](https://github.com/Gloaming-Forge/mod-template)
2. Edit `mod.json` with your mod's metadata
3. Add content via JSON definitions and Lua scripts
4. Tag a release — GitHub Actions builds your mod automatically
5. Add the `gloaming-mod` topic to your repo for auto-discovery

See the [modding guide](docs/modding-guide/) and [API reference](docs/api-reference/) for details.

## Target Platforms

- **Primary:** Windows (64-bit)
- **Secondary:** Linux, macOS
- **Stretch:** Steam Deck verified

## Contributing

Contributions are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

The engine and base game mod are licensed under the [MIT License](LICENSE).

Official art assets are licensed under [CC-BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/).

## Links

- [Engine Specification](GLOAMING_SPEC.md)
- [Mod Template](https://github.com/Gloaming-Forge/mod-template)
- [Mod Registry](https://github.com/Gloaming-Forge/mod-registry)
- [Mod Browser](https://gloaming-forge.github.io/mods)
