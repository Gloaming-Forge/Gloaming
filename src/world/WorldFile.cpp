#include "world/WorldFile.hpp"
#include <chrono>
#include <cstring>

namespace gloaming {

namespace fs = std::filesystem;

// ============================================================================
// CRC32 Implementation (IEEE 802.3 polynomial: 0xEDB88320)
// ============================================================================

// Pre-computed CRC32 lookup table
const uint32_t CRC32::s_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd706b3,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t CRC32::calculate(const void* data, size_t length) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; ++i) {
        crc = s_table[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFF;
}

uint32_t CRC32::calculateChunkChecksum(const Tile* tiles, size_t count) {
    return calculate(tiles, count * sizeof(Tile));
}

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

    // Read and verify checksum
    uint32_t storedChecksum;
    if (!readValue(file, storedChecksum)) {
        m_lastError = "Failed to read chunk checksum";
        return FileResult::ReadError;
    }

    uint32_t calculatedChecksum = CRC32::calculateChunkChecksum(tileData, CHUNK_TILE_COUNT);
    if (storedChecksum != calculatedChecksum) {
        m_lastError = "Chunk checksum mismatch - data may be corrupted";
        return FileResult::CorruptedData;
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

    // Write checksum for data integrity verification
    uint32_t checksum = CRC32::calculateChunkChecksum(tileData, CHUNK_TILE_COUNT);
    if (!writeValue(file, checksum)) return FileResult::WriteError;

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
