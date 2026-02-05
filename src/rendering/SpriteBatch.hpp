#pragma once

#include "rendering/IRenderer.hpp"
#include "rendering/Texture.hpp"
#include "rendering/Camera.hpp"
#include <vector>

namespace gloaming {

/// Individual sprite in a batch
struct Sprite {
    const Texture* texture = nullptr;
    Rect sourceRect;       // Region of texture to draw (for atlases)
    Vec2 position;         // World position
    Vec2 origin;           // Origin point for rotation/scaling (0-1 normalized)
    Vec2 scale = {1.0f, 1.0f};
    float rotation = 0.0f; // Rotation in degrees
    Color tint = Color::White();
    int layer = 0;         // Draw order (lower = drawn first)

    Sprite() = default;
    Sprite(const Texture* tex, Vec2 pos)
        : texture(tex), position(pos) {
        if (tex) {
            sourceRect = Rect(0, 0, static_cast<float>(tex->getWidth()),
                             static_cast<float>(tex->getHeight()));
        }
    }
};

/// Batches sprite rendering for efficiency
/// Groups sprites by texture and sorts by layer for correct draw order
class SpriteBatch {
public:
    SpriteBatch() = default;
    explicit SpriteBatch(IRenderer* renderer) : m_renderer(renderer) {}

    /// Set the renderer to use
    void setRenderer(IRenderer* renderer) { m_renderer = renderer; }

    /// Set the camera for world-to-screen transformation
    void setCamera(const Camera* camera) { m_camera = camera; }

    /// Begin a new batch
    void begin();

    /// Add a sprite to the batch
    void draw(const Sprite& sprite);

    /// Convenience: draw a texture at position
    void draw(const Texture* texture, Vec2 position, Color tint = Color::White());

    /// Convenience: draw a texture region at position
    void draw(const Texture* texture, const Rect& sourceRect, Vec2 position,
              Color tint = Color::White());

    /// Convenience: draw with full options
    void draw(const Texture* texture, const Rect& sourceRect, Vec2 position,
              Vec2 origin, Vec2 scale, float rotation, Color tint, int layer = 0);

    /// Convenience: draw from an atlas region
    void draw(const TextureAtlas* atlas, const std::string& regionName,
              Vec2 position, Color tint = Color::White());

    /// Convenience: draw from atlas with full options
    void draw(const TextureAtlas* atlas, const std::string& regionName,
              Vec2 position, Vec2 origin, Vec2 scale, float rotation,
              Color tint, int layer = 0);

    /// End the batch and render all sprites
    void end();

    /// Flush the current batch (render without ending)
    void flush();

    /// Get the number of sprites in the current batch
    size_t getSpriteCount() const { return m_sprites.size(); }

    /// Get the number of draw calls in the last flush
    size_t getDrawCallCount() const { return m_lastDrawCalls; }

    /// Enable/disable sorting by layer (enabled by default)
    void setSortEnabled(bool enabled) { m_sortEnabled = enabled; }

    /// Enable/disable camera culling (enabled by default)
    void setCullingEnabled(bool enabled) { m_cullingEnabled = enabled; }

private:
    IRenderer* m_renderer = nullptr;
    const Camera* m_camera = nullptr;

    std::vector<Sprite> m_sprites;
    bool m_batching = false;
    bool m_sortEnabled = true;
    bool m_cullingEnabled = true;
    size_t m_lastDrawCalls = 0;

    void renderSprite(const Sprite& sprite);
};

} // namespace gloaming
