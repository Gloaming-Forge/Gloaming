#include "rendering/Texture.hpp"
#include "engine/Log.hpp"

namespace gloaming {

void TextureAtlas::addRegion(const std::string& name, const Rect& bounds, Vec2 pivot) {
    m_regions[name] = AtlasRegion(name, bounds, pivot);
}

void TextureAtlas::addGrid(const std::string& prefix, int startX, int startY,
                           int cellWidth, int cellHeight, int columns, int rows,
                           int paddingX, int paddingY) {
    int index = 0;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            float x = static_cast<float>(startX + col * (cellWidth + paddingX));
            float y = static_cast<float>(startY + row * (cellHeight + paddingY));
            std::string name = prefix + "_" + std::to_string(index);
            addRegion(name, Rect(x, y, static_cast<float>(cellWidth),
                                 static_cast<float>(cellHeight)));
            ++index;
        }
    }
}

const AtlasRegion* TextureAtlas::getRegion(const std::string& name) const {
    auto it = m_regions.find(name);
    if (it != m_regions.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> TextureAtlas::getRegionNames() const {
    std::vector<std::string> names;
    names.reserve(m_regions.size());
    for (const auto& [name, region] : m_regions) {
        names.push_back(name);
    }
    return names;
}

bool TextureAtlas::hasRegion(const std::string& name) const {
    return m_regions.find(name) != m_regions.end();
}

// TextureManager implementation

Texture* TextureManager::loadTexture(const std::string& path) {
    // Check cache first
    auto it = m_textures.find(path);
    if (it != m_textures.end()) {
        return it->second.get();
    }

    if (!m_renderer) {
        LOG_ERROR("TextureManager: No renderer set, cannot load texture '{}'", path);
        return nullptr;
    }

    // Load via renderer
    Texture* texture = m_renderer->loadTexture(path);
    if (!texture) {
        LOG_ERROR("TextureManager: Failed to load texture '{}'", path);
        return nullptr;
    }

    // Take ownership and cache
    m_textures[path] = std::unique_ptr<Texture>(texture);
    LOG_DEBUG("TextureManager: Loaded texture '{}' ({}x{})", path,
              texture->getWidth(), texture->getHeight());
    return texture;
}

Texture* TextureManager::getTexture(const std::string& path) const {
    auto it = m_textures.find(path);
    if (it != m_textures.end()) {
        return it->second.get();
    }
    return nullptr;
}

void TextureManager::unloadTexture(const std::string& path) {
    auto it = m_textures.find(path);
    if (it != m_textures.end()) {
        if (m_renderer) {
            m_renderer->unloadTexture(it->second.get());
        }
        m_textures.erase(it);
        LOG_DEBUG("TextureManager: Unloaded texture '{}'", path);
    }
}

void TextureManager::unloadAll() {
    if (m_renderer) {
        for (auto& [path, texture] : m_textures) {
            m_renderer->unloadTexture(texture.get());
        }
    }
    m_textures.clear();
    m_atlases.clear();
    LOG_DEBUG("TextureManager: Unloaded all textures");
}

TextureAtlas* TextureManager::createAtlas(const std::string& name, const std::string& texturePath) {
    Texture* texture = loadTexture(texturePath);
    if (!texture) {
        return nullptr;
    }

    auto atlas = std::make_unique<TextureAtlas>(texture);
    TextureAtlas* atlasPtr = atlas.get();
    m_atlases[name] = std::move(atlas);
    LOG_DEBUG("TextureManager: Created atlas '{}' from '{}'", name, texturePath);
    return atlasPtr;
}

TextureAtlas* TextureManager::getAtlas(const std::string& name) const {
    auto it = m_atlases.find(name);
    if (it != m_atlases.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool TextureManager::hasTexture(const std::string& path) const {
    return m_textures.find(path) != m_textures.end();
}

} // namespace gloaming
