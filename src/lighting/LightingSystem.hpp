#pragma once

#include "ecs/Systems.hpp"
#include "ecs/Components.hpp"
#include "lighting/LightMap.hpp"
#include "lighting/DayNightCycle.hpp"
#include "world/TileMap.hpp"
#include "rendering/Camera.hpp"
#include "rendering/IRenderer.hpp"

#include <vector>

namespace gloaming {

/// Configuration for the integrated lighting system
struct LightingSystemConfig {
    LightingConfig lightMap;
    DayNightConfig dayNight;
    bool enabled = true;
    float recalcInterval = 0.1f;        // Seconds between full light recalculations
    int visiblePaddingTiles = 4;        // Extra tiles around camera for lighting calc
};

/// Statistics about the lighting system
struct LightingStats {
    size_t pointLightCount = 0;
    size_t tilesLit = 0;
    float lastRecalcTimeMs = 0.0f;
    float skyBrightness = 0.0f;
};

/// Main lighting system that coordinates:
/// - Collecting entity LightSource components
/// - Tile-based light emission
/// - Skylight and day/night cycle
/// - Light propagation via LightMap
/// - Rendering the light overlay
class LightingSystem : public System {
public:
    LightingSystem() : System("LightingSystem", 50) {}
    explicit LightingSystem(const LightingSystemConfig& config)
        : System("LightingSystem", 50), m_config(config),
          m_lightMap(config.lightMap), m_dayNightCycle(config.dayNight) {}

    void init(Registry& registry, Engine& engine) override;
    void update(float dt) override;
    void shutdown() override {}

    /// Render the lighting overlay on top of the scene.
    /// Call this AFTER tiles and sprites are rendered.
    void renderLightOverlay(IRenderer* renderer, const Camera& camera);

    // ========================================================================
    // Accessors
    // ========================================================================

    LightMap& getLightMap() { return m_lightMap; }
    const LightMap& getLightMap() const { return m_lightMap; }

    DayNightCycle& getDayNightCycle() { return m_dayNightCycle; }
    const DayNightCycle& getDayNightCycle() const { return m_dayNightCycle; }

    const LightingStats& getStats() const { return m_stats; }

    void setConfig(const LightingSystemConfig& config);
    const LightingSystemConfig& getConfig() const { return m_config; }

    /// Force a full light recalculation next frame
    void markDirty() { m_needsRecalc = true; }

    /// Get the current sky color
    TileLight getSkyColor() const { return m_dayNightCycle.getSkyColor(); }

    /// Get light at a world tile position
    TileLight getLightAt(int worldTileX, int worldTileY) const {
        return m_lightMap.getLight(worldTileX, worldTileY);
    }

    /// Get interpolated corner light (for smooth rendering)
    TileLight getCornerLight(int tileX, int tileY) const {
        return m_lightMap.getCornerLight(tileX, tileY);
    }

private:
    /// Collect all light sources (entities + tile emissions)
    void collectLightSources();

    /// Synchronize light map chunks with loaded world chunks
    void syncChunksWithWorld();

    /// Recalculate all lighting
    void recalculate();

    /// Render a single light-overlay tile with smooth interpolation
    void renderSmoothTile(IRenderer* renderer, const Camera& camera,
                          int tileX, int tileY, int tileSize);

    /// Render a single light-overlay tile (flat, no interpolation)
    void renderFlatTile(IRenderer* renderer, const Camera& camera,
                        int tileX, int tileY, int tileSize);

    LightingSystemConfig m_config;
    LightMap m_lightMap;
    DayNightCycle m_dayNightCycle;

    // Cached state
    TileMap* m_tileMap = nullptr;
    Camera* m_camera = nullptr;
    std::vector<TileLightSource> m_lightSources;
    LightingStats m_stats;

    float m_recalcTimer = 0.0f;
    bool m_needsRecalc = true;
};

} // namespace gloaming
