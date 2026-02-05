#pragma once

#include "rendering/IRenderer.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace gloaming {

/// Texture handle - wrapper around the backend's texture representation
/// The actual texture data is managed by the renderer implementation
class Texture {
public:
    Texture() = default;
    Texture(int width, int height, unsigned int id)
        : m_width(width), m_height(height), m_id(id) {}

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    unsigned int getId() const { return m_id; }

    bool isValid() const { return m_id != 0; }

private:
    int m_width = 0;
    int m_height = 0;
    unsigned int m_id = 0;  // Backend-specific texture ID
};

/// A region within a texture atlas
struct AtlasRegion {
    std::string name;
    Rect bounds;      // Position and size in the atlas texture
    Vec2 pivot;       // Pivot point for rotation (normalized 0-1)

    AtlasRegion() = default;
    AtlasRegion(const std::string& name, const Rect& bounds, Vec2 pivot = {0.5f, 0.5f})
        : name(name), bounds(bounds), pivot(pivot) {}
};

/// Texture atlas for efficient sprite batching
/// Manages multiple sprites packed into a single texture
class TextureAtlas {
public:
    TextureAtlas() = default;
    explicit TextureAtlas(Texture* texture) : m_texture(texture) {}

    /// Set the backing texture
    void setTexture(Texture* texture) { m_texture = texture; }

    /// Get the backing texture
    Texture* getTexture() const { return m_texture; }

    /// Add a region to the atlas
    void addRegion(const std::string& name, const Rect& bounds, Vec2 pivot = {0.5f, 0.5f});

    /// Add a grid of uniform regions (for spritesheets)
    void addGrid(const std::string& prefix, int startX, int startY,
                 int cellWidth, int cellHeight, int columns, int rows,
                 int paddingX = 0, int paddingY = 0);

    /// Get a region by name
    const AtlasRegion* getRegion(const std::string& name) const;

    /// Get all region names
    std::vector<std::string> getRegionNames() const;

    /// Check if a region exists
    bool hasRegion(const std::string& name) const;

private:
    Texture* m_texture = nullptr;
    std::unordered_map<std::string, AtlasRegion> m_regions;
};

/// Manages loaded textures and atlases with caching
/// Note: Textures are owned by the renderer, TextureManager just caches pointers
class TextureManager {
public:
    TextureManager() = default;
    ~TextureManager() = default;

    // Prevent copying
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    /// Set the renderer (required before loading textures)
    void setRenderer(IRenderer* renderer) { m_renderer = renderer; }

    /// Load a texture from file (cached)
    Texture* loadTexture(const std::string& path);

    /// Get a cached texture (returns nullptr if not loaded)
    Texture* getTexture(const std::string& path) const;

    /// Unload a specific texture
    void unloadTexture(const std::string& path);

    /// Unload all textures
    void unloadAll();

    /// Create a texture atlas from a texture
    TextureAtlas* createAtlas(const std::string& name, const std::string& texturePath);

    /// Get a texture atlas by name
    TextureAtlas* getAtlas(const std::string& name) const;

    /// Check if a texture is loaded
    [[nodiscard]] bool hasTexture(const std::string& path) const;

private:
    IRenderer* m_renderer = nullptr;
    // Non-owning cache: renderer owns the textures, we just map paths to pointers
    std::unordered_map<std::string, Texture*> m_textures;
    std::unordered_map<std::string, std::unique_ptr<TextureAtlas>> m_atlases;
};

} // namespace gloaming
