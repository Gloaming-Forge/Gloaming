#include "world/Chunk.hpp"
#include <algorithm>

namespace gloaming {

Chunk::Chunk(const ChunkPosition& position)
    : m_position(position) {
    clear();
}

Tile Chunk::getTile(int localX, int localY) const {
    if (!isValidLocalCoord(localX, localY)) {
        return Tile{};  // Return empty tile for out-of-bounds
    }
    return m_tiles[localToIndex(localX, localY)];
}

bool Chunk::setTile(int localX, int localY, const Tile& tile) {
    if (!isValidLocalCoord(localX, localY)) {
        return false;
    }
    m_tiles[localToIndex(localX, localY)] = tile;
    setDirty(ChunkDirtyFlags::TileData | ChunkDirtyFlags::NeedsSave);
    return true;
}

bool Chunk::setTileId(int localX, int localY, uint16_t id, uint8_t variant, uint8_t flags) {
    Tile tile;
    tile.id = id;
    tile.variant = variant;
    tile.flags = flags;
    return setTile(localX, localY, tile);
}

void Chunk::fill(const Tile& tile) {
    std::fill(m_tiles.begin(), m_tiles.end(), tile);
    setDirty(ChunkDirtyFlags::TileData | ChunkDirtyFlags::NeedsSave);
}

void Chunk::clear() {
    fill(Tile{});  // Fill with empty tiles
}

bool Chunk::isEmpty() const {
    return std::all_of(m_tiles.begin(), m_tiles.end(), [](const Tile& tile) {
        return tile.isEmpty();
    });
}

int Chunk::countNonEmptyTiles() const {
    return static_cast<int>(std::count_if(m_tiles.begin(), m_tiles.end(), [](const Tile& tile) {
        return !tile.isEmpty();
    }));
}

} // namespace gloaming
