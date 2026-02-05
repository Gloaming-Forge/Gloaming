#include "world/WorldFile.hpp"
#include <chrono>
#include <cstring>

namespace gloaming {

namespace fs = std::filesystem;

WorldFile::WorldFile(const std::string& worldPath)
    : m_worldPath(worldPath) {
}

void WorldFile::setWorldPath(const std::string& path) {
    m_worldPath = path;
}

bool WorldFile::worldExists() const {
    if (m_worldPath.empty()) {
        return false;
    }
    return fs::exists(m_worldPath) && fs::is_directory(m_worldPath);
}

FileResult WorldFile::createWorld(const WorldMetadata& metadata) {
    if (m_worldPath.empty()) {
        m_lastError = "World path not set";
        return FileResult::WriteError;
    }

    // Create directory structure
    if (!createWorldDirectory(m_worldPath)) {
        m_lastError = "Failed to create world directory";
        return FileResult::WriteError;
    }

    // Save metadata
    WorldMetadata meta = metadata;
    meta.version = WORLD_FILE_VERSION;

    // Set creation time if not set
    if (meta.createdTime == 0) {
        auto now = std::chrono::system_clock::now();
        meta.createdTime = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
    }
    meta.lastPlayedTime = meta.createdTime;

    return saveMetadata(meta);
}

FileResult WorldFile::deleteWorld() {
    if (m_worldPath.empty() || !worldExists()) {
        return FileResult::FileNotFound;
    }

    std::error_code ec;
    fs::remove_all(m_worldPath, ec);
    if (ec) {
        m_lastError = "Failed to delete world: " + ec.message();
        return FileResult::WriteError;
    }

    return FileResult::Success;
}

// ============================================================================
// Metadata Operations
// ============================================================================

FileResult WorldFile::loadMetadata(WorldMetadata& metadata) const {
    std::string metaPath = getMetadataFilePath();
    std::ifstream file(metaPath, std::ios::binary);
    if (!file.is_open()) {
        m_lastError = "Could not open metadata file: " + metaPath;
        return FileResult::FileNotFound;
    }

    // Read magic number
    uint32_t magic;
    if (!readValue(file, magic) || magic != WORLD_FILE_MAGIC) {
        m_lastError = "Invalid world file format";
        return FileResult::InvalidFormat;
    }

    // Read version
    if (!readValue(file, metadata.version)) {
        m_lastError = "Failed to read version";
        return FileResult::ReadError;
    }

    if (metadata.version > WORLD_FILE_VERSION) {
        m_lastError = "World file version too new";
        return FileResult::VersionMismatch;
    }

    // Read metadata fields
    if (!readValue(file, metadata.seed)) return FileResult::ReadError;
    if (!readString(file, metadata.name)) return FileResult::ReadError;
    if (!readValue(file, metadata.createdTime)) return FileResult::ReadError;
    if (!readValue(file, metadata.lastPlayedTime)) return FileResult::ReadError;
    if (!readValue(file, metadata.spawnX)) return FileResult::ReadError;
    if (!readValue(file, metadata.spawnY)) return FileResult::ReadError;
    if (!readValue(file, metadata.totalPlayTime)) return FileResult::ReadError;
    if (!readValue(file, metadata.tilesPlaced)) return FileResult::ReadError;
    if (!readValue(file, metadata.tilesMined)) return FileResult::ReadError;

    return FileResult::Success;
}

FileResult WorldFile::saveMetadata(const WorldMetadata& metadata) {
    std::string metaPath = getMetadataFilePath();

    // Ensure directory exists
    fs::path dir = fs::path(metaPath).parent_path();
    if (!fs::exists(dir)) {
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec) {
            m_lastError = "Failed to create metadata directory";
            return FileResult::WriteError;
        }
    }

    std::ofstream file(metaPath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        m_lastError = "Could not create metadata file: " + metaPath;
        return FileResult::WriteError;
    }

    // Write magic number
    if (!writeValue(file, WORLD_FILE_MAGIC)) return FileResult::WriteError;

    // Write version
    if (!writeValue(file, WORLD_FILE_VERSION)) return FileResult::WriteError;

    // Write metadata fields
    if (!writeValue(file, metadata.seed)) return FileResult::WriteError;
    if (!writeString(file, metadata.name)) return FileResult::WriteError;
    if (!writeValue(file, metadata.createdTime)) return FileResult::WriteError;
    if (!writeValue(file, metadata.lastPlayedTime)) return FileResult::WriteError;
    if (!writeValue(file, metadata.spawnX)) return FileResult::WriteError;
    if (!writeValue(file, metadata.spawnY)) return FileResult::WriteError;
    if (!writeValue(file, metadata.totalPlayTime)) return FileResult::WriteError;
    if (!writeValue(file, metadata.tilesPlaced)) return FileResult::WriteError;
    if (!writeValue(file, metadata.tilesMined)) return FileResult::WriteError;

    m_metadata = metadata;
    return FileResult::Success;
}

// ============================================================================
// Chunk Operations
// ============================================================================

bool WorldFile::chunkExists(ChunkCoord chunkX, ChunkCoord chunkY) const {
    std::string path = getChunkFilePath(chunkX, chunkY);
    return fs::exists(path);
}

FileResult WorldFile::loadChunk(Chunk& chunk) const {
    const ChunkPosition& pos = chunk.getPosition();
    std::string path = getChunkFilePath(pos.x, pos.y);

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        m_lastError = "Could not open chunk file: " + path;
        return FileResult::FileNotFound;
    }

    // Read magic number
    uint32_t magic;
    if (!readValue(file, magic) || magic != CHUNK_FILE_MAGIC) {
        m_lastError = "Invalid chunk file format";
        return FileResult::InvalidFormat;
    }

    // Read version
    uint32_t version;
    if (!readValue(file, version)) {
        m_lastError = "Failed to read chunk version";
        return FileResult::ReadError;
    }

    if (version > WORLD_FILE_VERSION) {
        m_lastError = "Chunk file version too new";
        return FileResult::VersionMismatch;
    }

    // Read chunk position (for verification)
    ChunkCoord fileX, fileY;
    if (!readValue(file, fileX) || !readValue(file, fileY)) {
        m_lastError = "Failed to read chunk position";
        return FileResult::ReadError;
    }

    if (fileX != pos.x || fileY != pos.y) {
        m_lastError = "Chunk position mismatch";
        return FileResult::CorruptedData;
    }

    // Read tile data
    Tile* tileData = chunk.getTileData();
    file.read(reinterpret_cast<char*>(tileData), CHUNK_TILE_COUNT * sizeof(Tile));
    if (!file.good()) {
        m_lastError = "Failed to read chunk tile data";
        return FileResult::ReadError;
    }

    // Chunk was loaded from file, so it's not dirty
    chunk.clearDirty();

    return FileResult::Success;
}

FileResult WorldFile::saveChunk(const Chunk& chunk) {
    const ChunkPosition& pos = chunk.getPosition();
    std::string path = getChunkFilePath(pos.x, pos.y);

    // Ensure chunks directory exists
    fs::path dir = fs::path(path).parent_path();
    if (!fs::exists(dir)) {
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec) {
            m_lastError = "Failed to create chunks directory";
            return FileResult::WriteError;
        }
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        m_lastError = "Could not create chunk file: " + path;
        return FileResult::WriteError;
    }

    // Write magic number
    if (!writeValue(file, CHUNK_FILE_MAGIC)) return FileResult::WriteError;

    // Write version
    if (!writeValue(file, WORLD_FILE_VERSION)) return FileResult::WriteError;

    // Write chunk position
    if (!writeValue(file, pos.x)) return FileResult::WriteError;
    if (!writeValue(file, pos.y)) return FileResult::WriteError;

    // Write tile data
    const Tile* tileData = chunk.getTileData();
    file.write(reinterpret_cast<const char*>(tileData), CHUNK_TILE_COUNT * sizeof(Tile));
    if (!file.good()) {
        m_lastError = "Failed to write chunk tile data";
        return FileResult::WriteError;
    }

    return FileResult::Success;
}

bool WorldFile::deleteChunk(ChunkCoord chunkX, ChunkCoord chunkY) {
    std::string path = getChunkFilePath(chunkX, chunkY);
    if (!fs::exists(path)) {
        return false;
    }

    std::error_code ec;
    fs::remove(path, ec);
    return !ec;
}

std::vector<ChunkPosition> WorldFile::getSavedChunkPositions() const {
    std::vector<ChunkPosition> positions;

    if (m_worldPath.empty()) {
        return positions;
    }

    std::string chunksDir = m_worldPath + "/chunks";
    if (!fs::exists(chunksDir)) {
        return positions;
    }

    // Iterate through chunk files
    for (const auto& entry : fs::directory_iterator(chunksDir)) {
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().stem().string();
        // Expected format: "chunk_X_Y"
        if (filename.find("chunk_") != 0) continue;

        ChunkCoord x, y;
        if (sscanf(filename.c_str(), "chunk_%d_%d", &x, &y) == 2) {
            positions.emplace_back(x, y);
        }
    }

    return positions;
}

// ============================================================================
// Utilities
// ============================================================================

std::string WorldFile::getChunkFilePath(ChunkCoord chunkX, ChunkCoord chunkY) const {
    return m_worldPath + "/chunks/chunk_" +
           std::to_string(chunkX) + "_" +
           std::to_string(chunkY) + ".bin";
}

std::string WorldFile::getMetadataFilePath() const {
    return m_worldPath + "/world.dat";
}

bool WorldFile::createWorldDirectory(const std::string& path) {
    std::error_code ec;

    // Create main directory
    if (!fs::exists(path)) {
        fs::create_directories(path, ec);
        if (ec) return false;
    }

    // Create chunks subdirectory
    std::string chunksDir = path + "/chunks";
    if (!fs::exists(chunksDir)) {
        fs::create_directories(chunksDir, ec);
        if (ec) return false;
    }

    return true;
}

const char* WorldFile::resultToString(FileResult result) {
    switch (result) {
        case FileResult::Success:         return "Success";
        case FileResult::FileNotFound:    return "File not found";
        case FileResult::InvalidFormat:   return "Invalid format";
        case FileResult::VersionMismatch: return "Version mismatch";
        case FileResult::ReadError:       return "Read error";
        case FileResult::WriteError:      return "Write error";
        case FileResult::CorruptedData:   return "Corrupted data";
        default:                          return "Unknown error";
    }
}

bool WorldFile::writeString(std::ofstream& file, const std::string& str) {
    uint32_t length = static_cast<uint32_t>(str.length());
    if (!writeValue(file, length)) return false;
    if (length > 0) {
        file.write(str.data(), length);
    }
    return file.good();
}

bool WorldFile::readString(std::ifstream& file, std::string& str) {
    uint32_t length;
    if (!readValue(file, length)) return false;

    if (length > 0) {
        str.resize(length);
        file.read(str.data(), length);
    } else {
        str.clear();
    }
    return file.good();
}

} // namespace gloaming
