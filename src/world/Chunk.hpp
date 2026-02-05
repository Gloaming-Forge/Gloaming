#pragma once

#include "rendering/TileRenderer.hpp"
#include <array>
#include <cstdint>

namespace gloaming {

/// Chunk constants
constexpr int CHUNK_SIZE = 64;        // 64x64 tiles per chunk
constexpr int CHUNK_TILE_COUNT = CHUNK_SIZE * CHUNK_SIZE;

/// Chunk coordinate type (signed for infinite worlds in both directions)
using ChunkCoord = int32_t;

/// World coordinate to chunk coordinate conversion
inline ChunkCoord worldToChunkCoord(int worldCoord) {
    // Handle negative coordinates correctly with floor division
    if (worldCoord >= 0) {
        return static_cast<ChunkCoord>(worldCoord / CHUNK_SIZE);
    }
    return static_cast<ChunkCoord>((worldCoord - CHUNK_SIZE + 1) / CHUNK_SIZE);
}

/// World coordinate to local tile coordinate within a chunk
inline int worldToLocalCoord(int worldCoord) {
    int local = worldCoord % CHUNK_SIZE;
    return (local >= 0) ? local : local + CHUNK_SIZE;
}

/// Chunk coordinate to world coordinate (returns the minimum world coord of the chunk)
inline int chunkToWorldCoord(ChunkCoord chunkCoord) {
    return static_cast<int>(chunkCoord) * CHUNK_SIZE;
}

/// Position uniquely identifying a chunk
struct ChunkPosition {
    ChunkCoord x = 0;
    ChunkCoord y = 0;

    ChunkPosition() = default;
    ChunkPosition(ChunkCoord x, ChunkCoord y) : x(x), y(y) {}

    bool operator==(const ChunkPosition& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const ChunkPosition& other) const {
        return !(*this == other);
    }

    // For std::map/set ordering
    bool operator<(const ChunkPosition& other) const {
        if (y != other.y) return y < other.y;
        return x < other.x;
    }
};

/// Hash function for ChunkPosition (for std::unordered_map)
struct ChunkPositionHash {
    std::size_t operator()(const ChunkPosition& pos) const {
        // Use boost::hash_combine pattern for better distribution
        std::size_t h1 = std::hash<ChunkCoord>{}(pos.x);
        std::size_t h2 = std::hash<ChunkCoord>{}(pos.y);
        // Golden ratio hash combine: h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2))
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

/// Chunk dirty flags for tracking what needs updating
enum class ChunkDirtyFlags : uint8_t {
    None        = 0,
    TileData    = 1 << 0,   // Tile data has changed (needs re-render)
    Lighting    = 1 << 1,   // Lighting needs recalculation
    NeedsSave   = 1 << 2,   // Chunk has unsaved changes
    All         = TileData | Lighting | NeedsSave
};

inline ChunkDirtyFlags operator|(ChunkDirtyFlags a, ChunkDirtyFlags b) {
    return static_cast<ChunkDirtyFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline ChunkDirtyFlags operator&(ChunkDirtyFlags a, ChunkDirtyFlags b) {
    return static_cast<ChunkDirtyFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline ChunkDirtyFlags& operator|=(ChunkDirtyFlags& a, ChunkDirtyFlags b) {
    a = a | b;
    return a;
}

inline ChunkDirtyFlags& operator&=(ChunkDirtyFlags& a, ChunkDirtyFlags b) {
    a = a & b;
    return a;
}

inline ChunkDirtyFlags operator~(ChunkDirtyFlags a) {
    return static_cast<ChunkDirtyFlags>(~static_cast<uint8_t>(a));
}

/// A 64x64 tile chunk of the world
class Chunk {
public:
    Chunk() = default;
    explicit Chunk(const ChunkPosition& position);

    /// Get the chunk's position in chunk coordinates
    const ChunkPosition& getPosition() const { return m_position; }

    /// Get tile at local coordinates (0-63, 0-63)
    /// @return Tile at the position, or empty tile if out of bounds
    Tile getTile(int localX, int localY) const;

    /// Set tile at local coordinates (0-63, 0-63)
    /// @return true if successful, false if out of bounds
    bool setTile(int localX, int localY, const Tile& tile);

    /// Set tile at local coordinates with just an ID
    bool setTileId(int localX, int localY, uint16_t id, uint8_t variant = 0, uint8_t flags = 0);

    /// Get raw access to tile data (for serialization/rendering)
    const Tile* getTileData() const { return m_tiles.data(); }
    Tile* getTileData() { return m_tiles.data(); }

    /// Fill entire chunk with a single tile
    void fill(const Tile& tile);

    /// Fill entire chunk with air (clear)
    void clear();

    /// Check if coordinates are within chunk bounds
    static bool isValidLocalCoord(int localX, int localY) {
        return localX >= 0 && localX < CHUNK_SIZE && localY >= 0 && localY < CHUNK_SIZE;
    }

    /// Convert local coordinates to index in tile array
    static int localToIndex(int localX, int localY) {
        return localY * CHUNK_SIZE + localX;
    }

    /// Convert index to local X coordinate
    static int indexToLocalX(int index) {
        return index % CHUNK_SIZE;
    }

    /// Convert index to local Y coordinate
    static int indexToLocalY(int index) {
        return index / CHUNK_SIZE;
    }

    // Dirty flag management
    bool isDirty() const { return m_dirtyFlags != ChunkDirtyFlags::None; }
    bool isDirty(ChunkDirtyFlags flag) const {
        return (m_dirtyFlags & flag) != ChunkDirtyFlags::None;
    }
    void setDirty(ChunkDirtyFlags flags = ChunkDirtyFlags::All) { m_dirtyFlags |= flags; }
    void clearDirty(ChunkDirtyFlags flags = ChunkDirtyFlags::All) { m_dirtyFlags &= ~flags; }
    ChunkDirtyFlags getDirtyFlags() const { return m_dirtyFlags; }

    /// Check if chunk has any non-empty tiles
    bool isEmpty() const;

    /// Count non-empty tiles in the chunk
    int countNonEmptyTiles() const;

    /// Get the world coordinates of the chunk's minimum corner
    int getWorldMinX() const { return chunkToWorldCoord(m_position.x); }
    int getWorldMinY() const { return chunkToWorldCoord(m_position.y); }

    /// Get the world coordinates of the chunk's maximum corner (exclusive)
    int getWorldMaxX() const { return chunkToWorldCoord(m_position.x) + CHUNK_SIZE; }
    int getWorldMaxY() const { return chunkToWorldCoord(m_position.y) + CHUNK_SIZE; }

private:
    ChunkPosition m_position;
    std::array<Tile, CHUNK_TILE_COUNT> m_tiles{};
    ChunkDirtyFlags m_dirtyFlags = ChunkDirtyFlags::None;
};

} // namespace gloaming
