#include "world/ChunkManager.hpp"
#include <algorithm>
#include <cmath>

namespace gloaming {

ChunkManager::ChunkManager(const ChunkManagerConfig& config)
    : m_config(config) {
}

void ChunkManager::init(uint64_t worldSeed) {
    m_generator.setSeed(worldSeed);
    m_chunks.clear();
    m_stats = ChunkManagerStats{};
}

void ChunkManager::update(float centerWorldX, float centerWorldY) {
    // Convert world position to chunk position
    ChunkCoord newCenterX = worldToChunkCoord(static_cast<int>(centerWorldX));
    ChunkCoord newCenterY = worldToChunkCoord(static_cast<int>(centerWorldY));

    updateAroundChunk(newCenterX, newCenterY);
}

void ChunkManager::updateAroundChunk(ChunkCoord chunkX, ChunkCoord chunkY) {
    m_centerChunkX = chunkX;
    m_centerChunkY = chunkY;

    // Load chunks within load radius
    for (ChunkCoord dy = -m_config.loadRadiusChunks; dy <= m_config.loadRadiusChunks; ++dy) {
        for (ChunkCoord dx = -m_config.loadRadiusChunks; dx <= m_config.loadRadiusChunks; ++dx) {
            ChunkCoord cx = chunkX + dx;
            ChunkCoord cy = chunkY + dy;
            if (!isChunkLoadedAt(cx, cy)) {
                loadChunk(cx, cy);
            }
        }
    }

    // Unload chunks outside unload radius
    std::vector<ChunkPosition> toUnload;
    for (auto& [pos, chunk] : m_chunks) {
        ChunkCoord dx = pos.x - chunkX;
        ChunkCoord dy = pos.y - chunkY;
        int distance = std::max(std::abs(dx), std::abs(dy));
        if (distance > m_config.unloadRadiusChunks) {
            toUnload.push_back(pos);
        }
    }

    for (const auto& pos : toUnload) {
        unloadChunk(pos.x, pos.y, m_config.autoSaveOnUnload);
    }

    // Enforce max loaded chunks if needed
    while (m_chunks.size() > static_cast<size_t>(m_config.maxLoadedChunks)) {
        // Find the chunk furthest from center
        ChunkPosition furthest;
        int maxDist = -1;
        for (auto& [pos, chunk] : m_chunks) {
            ChunkCoord dx = pos.x - chunkX;
            ChunkCoord dy = pos.y - chunkY;
            int dist = std::abs(dx) + std::abs(dy);
            if (dist > maxDist) {
                maxDist = dist;
                furthest = pos;
            }
        }
        if (maxDist >= 0) {
            unloadChunk(furthest.x, furthest.y, m_config.autoSaveOnUnload);
        } else {
            break;
        }
    }

    // Update stats
    m_stats.loadedChunks = m_chunks.size();
    m_stats.dirtyChunks = 0;
    for (const auto& [pos, chunk] : m_chunks) {
        if (chunk->isDirty(ChunkDirtyFlags::NeedsSave)) {
            ++m_stats.dirtyChunks;
        }
    }
}

// ============================================================================
// Tile Access
// ============================================================================

Tile ChunkManager::getTile(int worldX, int worldY) const {
    const Chunk* chunk = getChunkAt(worldX, worldY, false);
    if (!chunk) {
        return Tile{};  // Return empty tile for unloaded chunks
    }
    int localX = worldToLocalCoord(worldX);
    int localY = worldToLocalCoord(worldY);
    return chunk->getTile(localX, localY);
}

bool ChunkManager::setTile(int worldX, int worldY, const Tile& tile) {
    Chunk* chunk = getChunkAt(worldX, worldY, true);
    if (!chunk) {
        return false;
    }
    int localX = worldToLocalCoord(worldX);
    int localY = worldToLocalCoord(worldY);
    return chunk->setTile(localX, localY, tile);
}

bool ChunkManager::setTileId(int worldX, int worldY, uint16_t id, uint8_t variant, uint8_t flags) {
    Tile tile;
    tile.id = id;
    tile.variant = variant;
    tile.flags = flags;
    return setTile(worldX, worldY, tile);
}

bool ChunkManager::isSolid(int worldX, int worldY) const {
    Tile tile = getTile(worldX, worldY);
    return tile.isSolid();
}

bool ChunkManager::isChunkLoaded(int worldX, int worldY) const {
    ChunkPosition pos = worldToChunkPosition(worldX, worldY);
    return m_chunks.find(pos) != m_chunks.end();
}

bool ChunkManager::isChunkLoadedAt(ChunkCoord chunkX, ChunkCoord chunkY) const {
    return m_chunks.find(ChunkPosition(chunkX, chunkY)) != m_chunks.end();
}

// ============================================================================
// Chunk Access
// ============================================================================

Chunk* ChunkManager::getChunkAt(int worldX, int worldY, bool load) {
    ChunkPosition pos = worldToChunkPosition(worldX, worldY);
    return getChunk(pos, load);
}

const Chunk* ChunkManager::getChunkAt(int worldX, int worldY, bool load) const {
    ChunkPosition pos = worldToChunkPosition(worldX, worldY);
    return getChunk(pos, load);
}

Chunk* ChunkManager::getChunk(ChunkCoord chunkX, ChunkCoord chunkY, bool load) {
    return getChunk(ChunkPosition(chunkX, chunkY), load);
}

const Chunk* ChunkManager::getChunk(ChunkCoord chunkX, ChunkCoord chunkY, bool load) const {
    return getChunk(ChunkPosition(chunkX, chunkY), load);
}

Chunk* ChunkManager::getChunk(const ChunkPosition& pos, bool load) {
    auto it = m_chunks.find(pos);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    if (load) {
        return &loadChunk(pos.x, pos.y);
    }
    return nullptr;
}

const Chunk* ChunkManager::getChunk(const ChunkPosition& pos, bool load) const {
    auto it = m_chunks.find(pos);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    if (load) {
        // const_cast is safe here as we're creating new chunk
        return &const_cast<ChunkManager*>(this)->loadChunk(pos.x, pos.y);
    }
    return nullptr;
}

Chunk& ChunkManager::loadChunk(ChunkCoord chunkX, ChunkCoord chunkY) {
    ChunkPosition pos(chunkX, chunkY);

    // Check if already loaded
    auto it = m_chunks.find(pos);
    if (it != m_chunks.end()) {
        return *it->second;
    }

    // Create new chunk
    auto chunk = std::make_unique<Chunk>(pos);

    // Try to load from storage first
    if (!tryLoadFromStorage(*chunk)) {
        // Generate new chunk
        generateChunk(*chunk);
    }

    // Store the chunk
    Chunk* chunkPtr = chunk.get();
    m_chunks[pos] = std::move(chunk);

    // Call loaded callback
    if (m_onChunkLoaded) {
        m_onChunkLoaded(*chunkPtr);
    }

    return *chunkPtr;
}

bool ChunkManager::unloadChunk(ChunkCoord chunkX, ChunkCoord chunkY, bool save) {
    ChunkPosition pos(chunkX, chunkY);
    auto it = m_chunks.find(pos);
    if (it == m_chunks.end()) {
        return false;
    }

    Chunk* chunk = it->second.get();

    // Call unloading callback
    if (m_onChunkUnloading) {
        m_onChunkUnloading(*chunk);
    }

    // Save if requested and dirty
    if (save && chunk->isDirty(ChunkDirtyFlags::NeedsSave)) {
        saveChunk(chunkX, chunkY);
    }

    m_chunks.erase(it);
    ++m_stats.chunksUnloaded;
    return true;
}

void ChunkManager::unloadAllChunks(bool save) {
    if (save) {
        saveAllDirtyChunks();
    }

    // Call unloading callback for each chunk
    if (m_onChunkUnloading) {
        for (auto& [pos, chunk] : m_chunks) {
            m_onChunkUnloading(*chunk);
        }
    }

    m_stats.chunksUnloaded += m_chunks.size();
    m_chunks.clear();
}

// ============================================================================
// Saving/Loading
// ============================================================================

bool ChunkManager::saveChunk(ChunkCoord chunkX, ChunkCoord chunkY) {
    ChunkPosition pos(chunkX, chunkY);
    auto it = m_chunks.find(pos);
    if (it == m_chunks.end()) {
        return false;
    }

    Chunk* chunk = it->second.get();

    // Use custom save callback if provided
    if (m_saveCallback) {
        if (m_saveCallback(*chunk, m_worldPath)) {
            chunk->clearDirty(ChunkDirtyFlags::NeedsSave);
            ++m_stats.chunksSaved;
            return true;
        }
        return false;
    }

    // Default: no-op (WorldFile integration will provide the callback)
    return false;
}

void ChunkManager::saveAllDirtyChunks() {
    for (auto& [pos, chunk] : m_chunks) {
        if (chunk->isDirty(ChunkDirtyFlags::NeedsSave)) {
            saveChunk(pos.x, pos.y);
        }
    }
}

// ============================================================================
// Queries and Utilities
// ============================================================================

std::vector<Chunk*> ChunkManager::getLoadedChunks() {
    std::vector<Chunk*> result;
    result.reserve(m_chunks.size());
    for (auto& [pos, chunk] : m_chunks) {
        result.push_back(chunk.get());
    }
    return result;
}

std::vector<const Chunk*> ChunkManager::getLoadedChunks() const {
    std::vector<const Chunk*> result;
    result.reserve(m_chunks.size());
    for (const auto& [pos, chunk] : m_chunks) {
        result.push_back(chunk.get());
    }
    return result;
}

std::vector<Chunk*> ChunkManager::getDirtyChunks() {
    std::vector<Chunk*> result;
    for (auto& [pos, chunk] : m_chunks) {
        if (chunk->isDirty(ChunkDirtyFlags::NeedsSave)) {
            result.push_back(chunk.get());
        }
    }
    return result;
}

std::vector<Chunk*> ChunkManager::getChunksInRange(int minWorldX, int maxWorldX,
                                                    int minWorldY, int maxWorldY) {
    std::vector<Chunk*> result;

    ChunkCoord minChunkX = worldToChunkCoord(minWorldX);
    ChunkCoord maxChunkX = worldToChunkCoord(maxWorldX);
    ChunkCoord minChunkY = worldToChunkCoord(minWorldY);
    ChunkCoord maxChunkY = worldToChunkCoord(maxWorldY);

    for (ChunkCoord cy = minChunkY; cy <= maxChunkY; ++cy) {
        for (ChunkCoord cx = minChunkX; cx <= maxChunkX; ++cx) {
            Chunk* chunk = getChunk(cx, cy, false);
            if (chunk) {
                result.push_back(chunk);
            }
        }
    }

    return result;
}

void ChunkManager::markChunkDirty(ChunkCoord chunkX, ChunkCoord chunkY, ChunkDirtyFlags flags) {
    Chunk* chunk = getChunk(chunkX, chunkY, false);
    if (chunk) {
        chunk->setDirty(flags);
    }
}

// ============================================================================
// Private Methods
// ============================================================================

Chunk& ChunkManager::getOrCreateChunk(const ChunkPosition& pos) {
    auto it = m_chunks.find(pos);
    if (it != m_chunks.end()) {
        return *it->second;
    }
    return loadChunk(pos.x, pos.y);
}

bool ChunkManager::tryLoadFromStorage(Chunk& chunk) {
    if (m_loadCallback && !m_worldPath.empty()) {
        if (m_loadCallback(chunk, m_worldPath)) {
            ++m_stats.chunksLoaded;
            return true;
        }
    }
    return false;
}

void ChunkManager::generateChunk(Chunk& chunk) {
    m_generator.generate(chunk);
    ++m_stats.chunksGenerated;
}

} // namespace gloaming
