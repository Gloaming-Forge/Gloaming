#pragma once

#include "world/Chunk.hpp"
#include <string>
#include <cstdint>
#include <fstream>
#include <unordered_set>
#include <filesystem>

namespace gloaming {

/// World file format version for compatibility checking
constexpr uint32_t WORLD_FILE_VERSION = 1;

/// Magic number for world files ("GLWF" - Gloaming World File)
constexpr uint32_t WORLD_FILE_MAGIC = 0x46574C47;

/// Magic number for chunk files ("GLCF" - Gloaming Chunk File)
constexpr uint32_t CHUNK_FILE_MAGIC = 0x46434C47;

/// World metadata stored in the main world file
struct WorldMetadata {
    uint32_t version = WORLD_FILE_VERSION;
    uint64_t seed = 12345;
    std::string name = "World";
    int64_t createdTime = 0;    // Unix timestamp
    int64_t lastPlayedTime = 0; // Unix timestamp
    float spawnX = 0.0f;        // Player spawn X
    float spawnY = 0.0f;        // Player spawn Y

    // Statistics
    uint64_t totalPlayTime = 0; // Seconds
    uint32_t tilesPlaced = 0;
    uint32_t tilesMined = 0;
};

/// Result of a file operation
enum class FileResult {
    Success,
    FileNotFound,
    InvalidFormat,
    VersionMismatch,
    ReadError,
    WriteError,
    CorruptedData
};

/// Handles reading and writing world data to disk
/// Uses a directory-based structure with individual chunk files
class WorldFile {
public:
    WorldFile() = default;
    explicit WorldFile(const std::string& worldPath);

    /// Set the world directory path
    void setWorldPath(const std::string& path);
    const std::string& getWorldPath() const { return m_worldPath; }

    /// Check if a world exists at the current path
    bool worldExists() const;

    /// Create a new world with the given metadata
    FileResult createWorld(const WorldMetadata& metadata);

    /// Delete the world directory and all its contents
    FileResult deleteWorld();

    // ========================================================================
    // Metadata Operations
    // ========================================================================

    /// Load world metadata
    FileResult loadMetadata(WorldMetadata& metadata) const;

    /// Save world metadata
    FileResult saveMetadata(const WorldMetadata& metadata);

    /// Get the last loaded/saved metadata
    const WorldMetadata& getMetadata() const { return m_metadata; }

    // ========================================================================
    // Chunk Operations
    // ========================================================================

    /// Check if a chunk file exists
    bool chunkExists(ChunkCoord chunkX, ChunkCoord chunkY) const;

    /// Load a chunk from file
    /// @param chunk The chunk to populate (position should be set)
    FileResult loadChunk(Chunk& chunk) const;

    /// Save a chunk to file
    FileResult saveChunk(const Chunk& chunk);

    /// Delete a chunk file
    bool deleteChunk(ChunkCoord chunkX, ChunkCoord chunkY);

    /// Get list of all saved chunk positions
    std::vector<ChunkPosition> getSavedChunkPositions() const;

    // ========================================================================
    // Utilities
    // ========================================================================

    /// Get the file path for a chunk
    std::string getChunkFilePath(ChunkCoord chunkX, ChunkCoord chunkY) const;

    /// Get the metadata file path
    std::string getMetadataFilePath() const;

    /// Get the last error message
    const std::string& getLastError() const { return m_lastError; }

    /// Static utility: Create world directory structure
    static bool createWorldDirectory(const std::string& path);

    /// Static utility: Convert FileResult to string
    static const char* resultToString(FileResult result);

private:
    // Binary read/write helpers
    template<typename T>
    static bool writeValue(std::ofstream& file, const T& value);

    template<typename T>
    static bool readValue(std::ifstream& file, T& value);

    static bool writeString(std::ofstream& file, const std::string& str);
    static bool readString(std::ifstream& file, std::string& str);

    std::string m_worldPath;
    WorldMetadata m_metadata;
    mutable std::string m_lastError;
};

// ============================================================================
// Template Implementations
// ============================================================================

template<typename T>
bool WorldFile::writeValue(std::ofstream& file, const T& value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(T));
    return file.good();
}

template<typename T>
bool WorldFile::readValue(std::ifstream& file, T& value) {
    file.read(reinterpret_cast<char*>(&value), sizeof(T));
    return file.good();
}

} // namespace gloaming
