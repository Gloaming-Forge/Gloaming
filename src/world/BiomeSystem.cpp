#include "world/BiomeSystem.hpp"
#include "world/ChunkGenerator.hpp"
#include "engine/Log.hpp"
#include <cmath>
#include <limits>

namespace gloaming {

bool BiomeSystem::registerBiome(const BiomeDef& biome) {
    if (biome.id.empty()) {
        LOG_WARN("BiomeSystem: cannot register biome with empty ID");
        return false;
    }
    if (m_biomes.count(biome.id)) {
        LOG_WARN("BiomeSystem: biome '{}' already registered", biome.id);
        return false;
    }
    m_biomes[biome.id] = biome;
    LOG_DEBUG("BiomeSystem: registered biome '{}'", biome.id);
    return true;
}

bool BiomeSystem::removeBiome(const std::string& id) {
    auto it = m_biomes.find(id);
    if (it == m_biomes.end()) return false;
    m_biomes.erase(it);
    return true;
}

const BiomeDef* BiomeSystem::getBiome(const std::string& id) const {
    auto it = m_biomes.find(id);
    return (it != m_biomes.end()) ? &it->second : nullptr;
}

std::vector<std::string> BiomeSystem::getBiomeIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_biomes.size());
    for (const auto& [id, _] : m_biomes) {
        ids.push_back(id);
    }
    return ids;
}

void BiomeSystem::clear() {
    m_biomes.clear();
}

float BiomeSystem::getTemperature(int worldX, uint64_t seed) const {
    return Noise::fractalNoise1D(
        static_cast<float>(worldX) * m_temperatureScale,
        seed + 50000, 3, 0.5f
    );
}

float BiomeSystem::getHumidity(int worldX, uint64_t seed) const {
    return Noise::fractalNoise1D(
        static_cast<float>(worldX) * m_humidityScale,
        seed + 60000, 3, 0.5f
    );
}

const BiomeDef& BiomeSystem::getBiomeAt(int worldX, uint64_t seed) const {
    if (m_biomes.empty()) {
        return m_defaultBiome;
    }

    float temp = getTemperature(worldX, seed);
    float humid = getHumidity(worldX, seed);

    const BiomeDef* best = findBestBiome(temp, humid);
    return best ? *best : m_defaultBiome;
}

const BiomeDef* BiomeSystem::findBestBiome(float temperature, float humidity) const {
    const BiomeDef* best = nullptr;
    float bestDistance = std::numeric_limits<float>::max();

    for (const auto& [_, biome] : m_biomes) {
        // Check if the point falls within this biome's range
        if (temperature < biome.temperatureMin || temperature > biome.temperatureMax) continue;
        if (humidity < biome.humidityMin || humidity > biome.humidityMax) continue;

        // Compute distance to biome's center for tie-breaking
        float centerT = (biome.temperatureMin + biome.temperatureMax) * 0.5f;
        float centerH = (biome.humidityMin + biome.humidityMax) * 0.5f;
        float dt = temperature - centerT;
        float dh = humidity - centerH;
        float dist = dt * dt + dh * dh;

        if (dist < bestDistance) {
            bestDistance = dist;
            best = &biome;
        }
    }

    return best;
}

} // namespace gloaming
