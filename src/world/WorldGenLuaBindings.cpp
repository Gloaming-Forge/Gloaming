#include "world/WorldGenLuaBindings.hpp"
#include "world/WorldGenerator.hpp"
#include "world/BiomeSystem.hpp"
#include "world/OreDistribution.hpp"
#include "world/StructurePlacer.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"

namespace gloaming {

void bindWorldGenAPI(sol::state& lua, Engine& engine, WorldGenerator& worldGen) {
    auto wg = lua.create_named_table("worldgen");

    // =========================================================================
    // worldgen.registerTerrainGenerator(name, callback)
    // callback(chunk_x, seed) -> table of 64 heights
    // =========================================================================
    wg["registerTerrainGenerator"] = [&worldGen, &lua](
            const std::string& name, sol::function callback) {

        sol::function fn = callback;
        worldGen.registerTerrainGenerator(name,
            [fn, &lua](int chunkX, uint64_t seed) -> std::vector<int> {
                std::vector<int> heights;
                heights.reserve(CHUNK_SIZE);
                try {
                    // Pass seed as two 32-bit halves to avoid precision loss
                    uint32_t seedHi = static_cast<uint32_t>(seed >> 32);
                    uint32_t seedLo = static_cast<uint32_t>(seed & 0xFFFFFFFF);
                    sol::protected_function_result result = fn(chunkX, seedLo, seedHi);
                    if (!result.valid()) {
                        sol::error err = result;
                        MOD_LOG_ERROR("worldgen terrain generator error: {}", err.what());
                        heights.assign(CHUNK_SIZE, 100);
                        return heights;
                    }
                    sol::table heightTable = result;
                    // Detect indexing: try [1] first (Lua convention), fall back to [0]
                    sol::optional<int> firstAt1 = heightTable[1];
                    bool oneBased = firstAt1.has_value();
                    for (int x = 0; x < CHUNK_SIZE; ++x) {
                        int key = oneBased ? (x + 1) : x;
                        sol::optional<int> h = heightTable[key];
                        heights.push_back(h.value_or(100));
                    }
                } catch (const std::exception& e) {
                    MOD_LOG_ERROR("worldgen terrain generator exception: {}", e.what());
                    heights.assign(CHUNK_SIZE, 100);
                }
                return heights;
            });
        MOD_LOG_INFO("Registered terrain generator '{}'", name);
    };

    // =========================================================================
    // worldgen.setActiveTerrainGenerator(name)
    // =========================================================================
    wg["setActiveTerrainGenerator"] = [&worldGen](const std::string& name) {
        worldGen.setActiveTerrainGenerator(name);
    };

    // =========================================================================
    // worldgen.registerBiome(id, definition)
    // definition = { name, temperature_min, temperature_max, humidity_min,
    //                humidity_max, surface_tile, subsurface_tile, stone_tile,
    //                height_offset, height_scale, dirt_depth, tree_chance,
    //                grass_chance, cave_frequency }
    // =========================================================================
    wg["registerBiome"] = [&worldGen](const std::string& id, sol::table def) {
        BiomeDef biome;
        biome.id = id;
        biome.name = def.get_or<std::string>("name", id);

        biome.temperatureMin = def.get_or("temperature_min", 0.0f);
        biome.temperatureMax = def.get_or("temperature_max", 1.0f);
        biome.humidityMin = def.get_or("humidity_min", 0.0f);
        biome.humidityMax = def.get_or("humidity_max", 1.0f);

        biome.surfaceTile = def.get_or<uint16_t>("surface_tile", 1);
        biome.subsurfaceTile = def.get_or<uint16_t>("subsurface_tile", 2);
        biome.stoneTile = def.get_or<uint16_t>("stone_tile", 3);
        biome.fillerTile = def.get_or<uint16_t>("filler_tile", 0);

        biome.heightOffset = def.get_or("height_offset", 0.0f);
        biome.heightScale = def.get_or("height_scale", 1.0f);
        biome.dirtDepth = def.get_or("dirt_depth", 5);

        biome.treeChance = def.get_or("tree_chance", 0.0f);
        biome.grassChance = def.get_or("grass_chance", 0.0f);
        biome.caveFrequency = def.get_or("cave_frequency", 1.0f);

        // Parse custom data
        sol::optional<sol::table> customTable = def.get<sol::optional<sol::table>>("custom");
        if (customTable) {
            customTable->for_each([&biome](const sol::object& key, const sol::object& value) {
                if (!key.is<std::string>()) return;
                std::string k = key.as<std::string>();
                if (value.is<float>() || value.is<double>() || value.is<int>()) {
                    biome.customFloats[k] = value.as<float>();
                } else if (value.is<std::string>()) {
                    biome.customStrings[k] = value.as<std::string>();
                }
            });
        }

        if (worldGen.getBiomeSystem().registerBiome(biome)) {
            MOD_LOG_INFO("Registered biome '{}'", id);
        }
    };

    // =========================================================================
    // worldgen.getBiome(id) -> biome table or nil
    // =========================================================================
    wg["getBiome"] = [&worldGen, &lua](const std::string& id) -> sol::object {
        const BiomeDef* biome = worldGen.getBiomeSystem().getBiome(id);
        if (!biome) return sol::nil;

        sol::table t = lua.create_table();
        t["id"] = biome->id;
        t["name"] = biome->name;
        t["temperature_min"] = biome->temperatureMin;
        t["temperature_max"] = biome->temperatureMax;
        t["humidity_min"] = biome->humidityMin;
        t["humidity_max"] = biome->humidityMax;
        t["surface_tile"] = biome->surfaceTile;
        t["subsurface_tile"] = biome->subsurfaceTile;
        t["stone_tile"] = biome->stoneTile;
        t["height_offset"] = biome->heightOffset;
        t["height_scale"] = biome->heightScale;
        t["dirt_depth"] = biome->dirtDepth;
        t["tree_chance"] = biome->treeChance;
        t["grass_chance"] = biome->grassChance;
        t["cave_frequency"] = biome->caveFrequency;
        return t;
    };

    // =========================================================================
    // worldgen.getBiomeAt(worldX) -> biome id string
    // =========================================================================
    wg["getBiomeAt"] = [&worldGen](int worldX) -> std::string {
        return worldGen.getBiomeSystem().getBiomeAt(worldX, worldGen.getSeed()).id;
    };

    // =========================================================================
    // worldgen.registerOre(id, definition)
    // definition = { tile_id, min_depth, max_depth, vein_size_min, vein_size_max,
    //                frequency, noise_scale, noise_threshold, replace_tiles, biomes }
    // =========================================================================
    wg["registerOre"] = [&worldGen](const std::string& id, sol::table def) {
        OreRule rule;
        rule.id = id;
        rule.tileId = def.get_or<uint16_t>("tile_id", 0);
        rule.minDepth = def.get_or("min_depth", 0);
        rule.maxDepth = def.get_or("max_depth", 1000);
        rule.veinSizeMin = def.get_or("vein_size_min", 3);
        rule.veinSizeMax = def.get_or("vein_size_max", 8);
        rule.frequency = def.get_or("frequency", 0.1f);
        rule.noiseScale = def.get_or("noise_scale", 0.1f);
        rule.noiseThreshold = def.get_or("noise_threshold", 0.7f);

        // Parse replace_tiles
        sol::optional<sol::table> replaceTable = def.get<sol::optional<sol::table>>("replace_tiles");
        if (replaceTable) {
            rule.replaceTiles.clear();
            size_t len = replaceTable->size();
            for (size_t i = 1; i <= len; ++i) {
                sol::optional<uint16_t> tileId = replaceTable->get<sol::optional<uint16_t>>(i);
                if (tileId) rule.replaceTiles.push_back(*tileId);
            }
        }

        // Parse biome restrictions
        sol::optional<sol::table> biomesTable = def.get<sol::optional<sol::table>>("biomes");
        if (biomesTable) {
            size_t len = biomesTable->size();
            for (size_t i = 1; i <= len; ++i) {
                sol::optional<std::string> b = biomesTable->get<sol::optional<std::string>>(i);
                if (b) rule.biomes.push_back(*b);
            }
        }

        if (worldGen.getOreDistribution().registerOre(rule)) {
            MOD_LOG_INFO("Registered ore '{}'", id);
        }
    };

    // =========================================================================
    // worldgen.registerStructure(id, definition)
    // definition = { name, tiles = { {x, y, tile_id, variant, flags} ... },
    //                width, height, placement, chance, spacing,
    //                min_depth, max_depth, biomes, needs_ground, needs_air }
    // =========================================================================
    wg["registerStructure"] = [&worldGen](const std::string& id, sol::table def) {
        StructureTemplate structure;
        structure.id = id;
        structure.name = def.get_or<std::string>("name", id);
        structure.width = def.get_or("width", 0);
        structure.height = def.get_or("height", 0);
        structure.chance = def.get_or("chance", 0.01f);
        structure.spacing = def.get_or("spacing", 10);
        structure.minDepth = def.get_or("min_depth", 0);
        structure.maxDepth = def.get_or("max_depth", 1000);
        structure.needsGround = def.get_or("needs_ground", true);
        structure.needsAir = def.get_or("needs_air", true);

        // Parse placement type
        std::string placementStr = def.get_or<std::string>("placement", "surface");
        if (placementStr == "surface")          structure.placement = StructurePlacement::Surface;
        else if (placementStr == "underground") structure.placement = StructurePlacement::Underground;
        else if (placementStr == "ceiling")     structure.placement = StructurePlacement::Ceiling;
        else if (placementStr == "anywhere")    structure.placement = StructurePlacement::Anywhere;

        // Parse tiles
        sol::optional<sol::table> tilesTable = def.get<sol::optional<sol::table>>("tiles");
        if (tilesTable) {
            size_t len = tilesTable->size();
            for (size_t i = 1; i <= len; ++i) {
                sol::table tileDef = (*tilesTable)[i];
                StructureTile tile;
                tile.offsetX = tileDef.get_or("x", 0);
                tile.offsetY = tileDef.get_or("y", 0);
                tile.tileId = tileDef.get_or<uint16_t>("tile_id", 0);
                tile.variant = tileDef.get_or<uint8_t>("variant", 0);
                tile.flags = tileDef.get_or<uint8_t>("flags", Tile::FLAG_SOLID);
                tile.overwriteAir = tileDef.get_or("overwrite_air", true);
                structure.tiles.push_back(tile);
            }
        }

        // Parse biome restrictions
        sol::optional<sol::table> biomesTable = def.get<sol::optional<sol::table>>("biomes");
        if (biomesTable) {
            size_t len = biomesTable->size();
            for (size_t i = 1; i <= len; ++i) {
                sol::optional<std::string> b = biomesTable->get<sol::optional<std::string>>(i);
                if (b) structure.biomes.push_back(*b);
            }
        }

        if (worldGen.getStructurePlacer().registerStructure(structure)) {
            MOD_LOG_INFO("Registered structure '{}'", id);
        }
    };

    // =========================================================================
    // worldgen.registerPass(name, priority, callback)
    // callback(chunk_handle, seed_lo, seed_hi) where chunk_handle provides:
    //   chunk_handle.world_x, chunk_handle.world_y (world origin)
    //   chunk_handle:get_tile(local_x, local_y) -> {id, variant, flags}
    //   chunk_handle:set_tile(local_x, local_y, tile_id, variant, flags)
    // =========================================================================
    wg["registerPass"] = [&worldGen, &lua](const std::string& name, int priority,
                                      sol::function callback) {
        sol::function fn = callback;
        worldGen.registerPass(name, priority,
            [fn, &lua](Chunk& chunk, uint64_t seed, const WorldGenConfig&) {
                try {
                    sol::table chunkHandle = lua.create_table();
                    chunkHandle["world_x"] = chunk.getWorldMinX();
                    chunkHandle["world_y"] = chunk.getWorldMinY();
                    chunkHandle.set_function("get_tile",
                        [&chunk, &lua](int lx, int ly) -> sol::table {
                            Tile t = chunk.getTile(lx, ly);
                            sol::table result = lua.create_table();
                            result["id"] = t.id;
                            result["variant"] = t.variant;
                            result["flags"] = t.flags;
                            return result;
                        });
                    chunkHandle.set_function("set_tile",
                        [&chunk](int lx, int ly, uint16_t tileId,
                                 sol::optional<uint8_t> variant,
                                 sol::optional<uint8_t> flags) {
                            chunk.setTileId(lx, ly, tileId,
                                            variant.value_or(0),
                                            flags.value_or(Tile::FLAG_SOLID));
                        });
                    uint32_t seedHi = static_cast<uint32_t>(seed >> 32);
                    uint32_t seedLo = static_cast<uint32_t>(seed & 0xFFFFFFFF);
                    fn(chunkHandle, seedLo, seedHi);
                } catch (const std::exception& e) {
                    MOD_LOG_ERROR("worldgen pass error: {}", e.what());
                }
            });
        MOD_LOG_INFO("Registered worldgen pass '{}' (priority {})", name, priority);
    };

    // =========================================================================
    // worldgen.registerDecorator(name, callback)
    // callback(chunk_handle, seed_lo, seed_hi) -- same chunk_handle as passes
    // =========================================================================
    wg["registerDecorator"] = [&worldGen, &lua](const std::string& name, sol::function callback) {
        sol::function fn = callback;
        worldGen.registerDecorator(name,
            [fn, &lua](Chunk& chunk, uint64_t seed) {
                try {
                    sol::table chunkHandle = lua.create_table();
                    chunkHandle["world_x"] = chunk.getWorldMinX();
                    chunkHandle["world_y"] = chunk.getWorldMinY();
                    chunkHandle.set_function("get_tile",
                        [&chunk, &lua](int lx, int ly) -> sol::table {
                            Tile t = chunk.getTile(lx, ly);
                            sol::table result = lua.create_table();
                            result["id"] = t.id;
                            result["variant"] = t.variant;
                            result["flags"] = t.flags;
                            return result;
                        });
                    chunkHandle.set_function("set_tile",
                        [&chunk](int lx, int ly, uint16_t tileId,
                                 sol::optional<uint8_t> variant,
                                 sol::optional<uint8_t> flags) {
                            chunk.setTileId(lx, ly, tileId,
                                            variant.value_or(0),
                                            flags.value_or(Tile::FLAG_SOLID));
                        });
                    uint32_t seedHi = static_cast<uint32_t>(seed >> 32);
                    uint32_t seedLo = static_cast<uint32_t>(seed & 0xFFFFFFFF);
                    fn(chunkHandle, seedLo, seedHi);
                } catch (const std::exception& e) {
                    MOD_LOG_ERROR("worldgen decorator error: {}", e.what());
                }
            });
        MOD_LOG_INFO("Registered worldgen decorator '{}'", name);
    };

    // =========================================================================
    // worldgen.setSurfaceLevel(y)
    // =========================================================================
    wg["setSurfaceLevel"] = [&worldGen](int y) {
        worldGen.getConfig().surfaceLevel = y;
    };

    // =========================================================================
    // worldgen.getSurfaceLevel() -> int
    // =========================================================================
    wg["getSurfaceLevel"] = [&worldGen]() -> int {
        return worldGen.getConfig().surfaceLevel;
    };

    // =========================================================================
    // worldgen.setSeaLevel(y)
    // =========================================================================
    wg["setSeaLevel"] = [&worldGen](int y) {
        worldGen.getConfig().seaLevel = y;
    };

    // =========================================================================
    // worldgen.getSeaLevel() -> int
    // =========================================================================
    wg["getSeaLevel"] = [&worldGen]() -> int {
        return worldGen.getConfig().seaLevel;
    };

    // =========================================================================
    // worldgen.getSurfaceHeight(worldX) -> int
    // =========================================================================
    wg["getSurfaceHeight"] = [&worldGen](int worldX) -> int {
        return worldGen.getSurfaceHeight(worldX);
    };

    // =========================================================================
    // worldgen.setCaves(enabled)
    // =========================================================================
    wg["setCaves"] = [&worldGen](bool enabled) {
        worldGen.getConfig().generateCaves = enabled;
    };

    // =========================================================================
    // worldgen.setOres(enabled)
    // =========================================================================
    wg["setOres"] = [&worldGen](bool enabled) {
        worldGen.getConfig().generateOres = enabled;
    };

    // =========================================================================
    // worldgen.setStructures(enabled)
    // =========================================================================
    wg["setStructures"] = [&worldGen](bool enabled) {
        worldGen.getConfig().generateStructures = enabled;
    };

    // =========================================================================
    // worldgen.setCaveParams(scale, threshold, min_depth)
    // =========================================================================
    wg["setCaveParams"] = [&worldGen](float scale, float threshold, int minDepth) {
        auto& config = worldGen.getConfig();
        config.caveScale = scale;
        config.caveThreshold = threshold;
        config.caveMinDepth = minDepth;
    };

    // =========================================================================
    // worldgen.setTerrainParams(scale, amplitude)
    // =========================================================================
    wg["setTerrainParams"] = [&worldGen](float scale, float amplitude) {
        auto& config = worldGen.getConfig();
        config.terrainScale = scale;
        config.terrainAmplitude = amplitude;
    };

    // =========================================================================
    // worldgen.setBiomeScale(temperature_scale, humidity_scale)
    // =========================================================================
    wg["setBiomeScale"] = [&worldGen](float tempScale, float humidScale) {
        worldGen.getBiomeSystem().setTemperatureScale(tempScale);
        worldGen.getBiomeSystem().setHumidityScale(humidScale);
    };
}

} // namespace gloaming
