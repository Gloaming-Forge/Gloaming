#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace gloaming {

/// Definition of a biome with its terrain and content properties.
/// Biomes are registered by mods via Lua or JSON and selected based on
/// temperature/humidity noise at each world column.
struct BiomeDef {
    std::string id;                 // Unique identifier (e.g. "forest", "desert")
    std::string name;               // Display name

    // Climate selection ranges [0.0, 1.0]
    float temperatureMin = 0.0f;
    float temperatureMax = 1.0f;
    float humidityMin = 0.0f;
    float humidityMax = 1.0f;

    // Terrain generation parameters
    uint16_t surfaceTile = 1;       // Tile ID for surface (e.g. grass)
    uint16_t subsurfaceTile = 2;    // Tile ID for dirt layer
    uint16_t stoneTile = 3;         // Tile ID for deep stone
    uint16_t fillerTile = 0;        // Optional extra filler tile (e.g. sand underwater)

    // Terrain shape modifiers
    float heightOffset = 0.0f;      // Added to base surface height
    float heightScale = 1.0f;       // Multiplied with terrain amplitude
    int dirtDepth = 5;              // Depth of the subsurface layer

    // Visual/content modifiers
    float treeChance = 0.0f;        // Probability of tree placement per surface tile
    float grassChance = 0.0f;       // Probability of decorative grass per surface tile
    float caveFrequency = 1.0f;     // Multiplier for cave density (1.0 = normal)

    // Custom data table for mod-specific properties
    std::unordered_map<std::string, float> customFloats;
    std::unordered_map<std::string, std::string> customStrings;
};

/// Manages biome definitions and selection based on climate noise.
///
/// Biome selection uses two noise channels:
///   - Temperature noise (east-west variation)
///   - Humidity noise (with different frequency)
///
/// Each biome defines a (temperature, humidity) rectangle.
/// When multiple biomes match, the closest center wins.
class BiomeSystem {
public:
    BiomeSystem() = default;

    /// Register a biome definition.
    /// Returns false if a biome with this ID already exists.
    bool registerBiome(const BiomeDef& biome);

    /// Remove a biome by ID.
    bool removeBiome(const std::string& id);

    /// Get a biome definition by ID.
    const BiomeDef* getBiome(const std::string& id) const;

    /// Get all registered biome IDs.
    std::vector<std::string> getBiomeIds() const;

    /// Get the number of registered biomes.
    size_t biomeCount() const { return m_biomes.size(); }

    /// Clear all registered biomes.
    void clear();

    /// Determine the biome at a given world X coordinate.
    /// Uses temperature and humidity noise derived from the seed.
    /// Returns the default biome if no biomes match.
    const BiomeDef& getBiomeAt(int worldX, uint64_t seed) const;

    /// Get the raw temperature value at a world X coordinate.
    float getTemperature(int worldX, uint64_t seed) const;

    /// Get the raw humidity value at a world X coordinate.
    float getHumidity(int worldX, uint64_t seed) const;

    // ========================================================================
    // Configuration
    // ========================================================================

    /// Set the noise scale for temperature variation.
    /// Smaller values = larger biomes.
    void setTemperatureScale(float scale) { m_temperatureScale = scale; }
    float getTemperatureScale() const { return m_temperatureScale; }

    /// Set the noise scale for humidity variation.
    void setHumidityScale(float scale) { m_humidityScale = scale; }
    float getHumidityScale() const { return m_humidityScale; }

private:
    /// Find the best matching biome for a (temperature, humidity) pair.
    const BiomeDef* findBestBiome(float temperature, float humidity) const;

    std::unordered_map<std::string, BiomeDef> m_biomes;

    // Default biome returned when no biomes are registered or none match
    BiomeDef m_defaultBiome;

    // Noise parameters for climate
    float m_temperatureScale = 0.003f;  // Very slow variation (~333 tiles per cycle)
    float m_humidityScale = 0.005f;     // Slightly faster than temperature
};

} // namespace gloaming
