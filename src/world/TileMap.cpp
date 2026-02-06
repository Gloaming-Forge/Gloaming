#include "world/TileMap.hpp"
#include "rendering/TileRenderer.hpp"
#include <cmath>

namespace gloaming {

TileMap::TileMap(const TileMapConfig& config)
    : m_config(config)
    , m_chunkManager(config.chunkManager) {
}

bool TileMap::createWorld(const std::string& worldPath, const std::string& worldName, uint64_t seed) {
    // Close any existing world
    closeWorld();

    // Set up world file
    m_worldFile.setWorldPath(worldPath);

    // Create metadata
    m_metadata = WorldMetadata{};
    m_metadata.seed = seed;
    m_metadata.name = worldName;

    // Create the world
    FileResult result = m_worldFile.createWorld(m_metadata);
    if (result != FileResult::Success) {
        return false;
    }

    // Initialize chunk manager
    m_chunkManager.setConfig(m_config.chunkManager);
    m_chunkManager.init(seed);
    m_chunkManager.setWorldPath(worldPath);

    // Set up save/load callbacks
    setupChunkCallbacks();

    m_worldLoaded = true;
    return true;
}

bool TileMap::loadWorld(const std::string& worldPath) {
    // Close any existing world
    closeWorld();

    // Set up world file
    m_worldFile.setWorldPath(worldPath);

    // Check if world exists
    if (!m_worldFile.worldExists()) {
        return false;
    }

    // Load metadata
    FileResult result = m_worldFile.loadMetadata(m_metadata);
    if (result != FileResult::Success) {
        return false;
    }

    // Initialize chunk manager
    m_chunkManager.setConfig(m_config.chunkManager);
    m_chunkManager.init(m_metadata.seed);
    m_chunkManager.setWorldPath(worldPath);

    // Set up save/load callbacks
    setupChunkCallbacks();

    m_worldLoaded = true;
    return true;
}

bool TileMap::saveWorld() {
    if (!m_worldLoaded) {
        return false;
    }

    // Update last played time
    auto now = std::chrono::system_clock::now();
    m_metadata.lastPlayedTime = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    // Save metadata
    FileResult result = m_worldFile.saveMetadata(m_metadata);
    if (result != FileResult::Success) {
        return false;
    }

    // Save all dirty chunks
    m_chunkManager.saveAllDirtyChunks();

    return true;
}

void TileMap::closeWorld() {
    if (!m_worldLoaded) {
        return;
    }

    // Save if auto-save is enabled (ignore result - best effort save on close)
    if (m_config.autoSave) {
        (void)saveWorld();
    }

    // Unload all chunks
    m_chunkManager.unloadAllChunks(m_config.autoSave);

    m_worldLoaded = false;
}

// ============================================================================
// World Update
// ============================================================================

void TileMap::update(const Camera& camera) {
    if (!m_worldLoaded) return;

    Vec2 pos = camera.getPosition();
    // Convert world pixel coordinates to tile coordinates for the chunk manager
    float tileX = pos.x / m_config.tileSize;
    float tileY = pos.y / m_config.tileSize;
    m_chunkManager.update(tileX, tileY);
}

void TileMap::update(float worldX, float worldY) {
    if (!m_worldLoaded) return;

    // Convert world pixel coordinates to tile coordinates for the chunk manager
    float tileX = worldX / m_config.tileSize;
    float tileY = worldY / m_config.tileSize;
    m_chunkManager.update(tileX, tileY);
}

// ============================================================================
// Tile Access
// ============================================================================

Tile TileMap::getTile(int worldX, int worldY) const {
    if (!m_worldLoaded) {
        return Tile{};
    }
    return m_chunkManager.getTile(worldX, worldY);
}

bool TileMap::setTile(int worldX, int worldY, const Tile& tile) {
    if (!m_worldLoaded) {
        return false;
    }
    return m_chunkManager.setTile(worldX, worldY, tile);
}

bool TileMap::setTileId(int worldX, int worldY, uint16_t id, uint8_t variant, uint8_t flags) {
    if (!m_worldLoaded) {
        return false;
    }
    return m_chunkManager.setTileId(worldX, worldY, id, variant, flags);
}

bool TileMap::isSolid(int worldX, int worldY) const {
    if (!m_worldLoaded) {
        return false;
    }
    return m_chunkManager.isSolid(worldX, worldY);
}

bool TileMap::isEmpty(int worldX, int worldY) const {
    return getTile(worldX, worldY).isEmpty();
}

void TileMap::worldToTile(float worldX, float worldY, int& tileX, int& tileY) const {
    tileX = static_cast<int>(std::floor(worldX / m_config.tileSize));
    tileY = static_cast<int>(std::floor(worldY / m_config.tileSize));
}

void TileMap::tileToWorld(int tileX, int tileY, float& worldX, float& worldY) const {
    worldX = static_cast<float>(tileX * m_config.tileSize);
    worldY = static_cast<float>(tileY * m_config.tileSize);
}

// ============================================================================
// Rendering Support
// ============================================================================

void TileMap::render(TileRenderer& renderer, const Camera& camera) const {
    if (!m_worldLoaded) return;

    int minX, maxX, minY, maxY;
    getVisibleTileRange(camera, minX, maxX, minY, maxY);

    // Use the template render method with our tile callback
    auto getTile = [this](int x, int y) -> Tile {
        return this->getTile(x, y);
    };

    renderer.render(getTile, minX, maxX, minY, maxY);
}

std::function<Tile(int, int)> TileMap::getTileCallback() const {
    return [this](int x, int y) -> Tile {
        return this->getTile(x, y);
    };
}

void TileMap::getVisibleTileRange(const Camera& camera, int& minX, int& maxX,
                                   int& minY, int& maxY) const {
    Rect visibleArea = camera.getVisibleArea();

    // Convert to tile coordinates with some padding
    const int padding = 2;
    minX = static_cast<int>(std::floor(visibleArea.x / m_config.tileSize)) - padding;
    minY = static_cast<int>(std::floor(visibleArea.y / m_config.tileSize)) - padding;
    maxX = static_cast<int>(std::ceil((visibleArea.x + visibleArea.width) / m_config.tileSize)) + padding;
    maxY = static_cast<int>(std::ceil((visibleArea.y + visibleArea.height) / m_config.tileSize)) + padding;
}

// ============================================================================
// World Properties
// ============================================================================

uint64_t TileMap::getSeed() const {
    return m_metadata.seed;
}

Vec2 TileMap::getSpawnPoint() const {
    return Vec2(m_metadata.spawnX, m_metadata.spawnY);
}

void TileMap::setSpawnPoint(float x, float y) {
    m_metadata.spawnX = x;
    m_metadata.spawnY = y;
}

void TileMap::addPlayTime(uint64_t seconds) {
    m_metadata.totalPlayTime += seconds;
}

// ============================================================================
// Chunk Access
// ============================================================================

void TileMap::setGeneratorCallback(ChunkGeneratorCallback callback) {
    m_chunkManager.getGenerator().setGeneratorCallback(std::move(callback));
}

// ============================================================================
// Configuration
// ============================================================================

void TileMap::setConfig(const TileMapConfig& config) {
    m_config = config;
    m_chunkManager.setConfig(config.chunkManager);
}

// ============================================================================
// Private Methods
// ============================================================================

void TileMap::setupChunkCallbacks() {
    // Set up save callback
    m_chunkManager.setSaveCallback([this](const Chunk& chunk, const std::string& worldPath) -> bool {
        (void)worldPath;  // We use m_worldFile which already has the path
        FileResult result = m_worldFile.saveChunk(chunk);
        return result == FileResult::Success;
    });

    // Set up load callback
    m_chunkManager.setLoadCallback([this](Chunk& chunk, const std::string& worldPath) -> bool {
        (void)worldPath;  // We use m_worldFile which already has the path
        if (!m_worldFile.chunkExists(chunk.getPosition().x, chunk.getPosition().y)) {
            return false;  // Chunk doesn't exist, needs generation
        }
        FileResult result = m_worldFile.loadChunk(chunk);
        return result == FileResult::Success;
    });
}

} // namespace gloaming
