#pragma once

#include "rendering/IRenderer.hpp"
#include "rendering/Texture.hpp"
#include "rendering/Camera.hpp"
#include <vector>
#include <string>

namespace gloaming {

/// A single layer in a parallax background
struct ParallaxLayer {
    const Texture* texture = nullptr;
    float parallaxX = 1.0f;      // Horizontal parallax factor (0 = static, 1 = moves with camera)
    float parallaxY = 1.0f;      // Vertical parallax factor
    float offsetX = 0.0f;        // Base X offset
    float offsetY = 0.0f;        // Base Y offset
    float scrollSpeedX = 0.0f;   // Automatic horizontal scroll speed (pixels/sec)
    float scrollSpeedY = 0.0f;   // Automatic vertical scroll speed
    bool repeatX = true;         // Tile horizontally
    bool repeatY = false;        // Tile vertically
    Color tint = Color::White();
    float scale = 1.0f;          // Scale factor for the layer

    ParallaxLayer() = default;
    ParallaxLayer(const Texture* tex, float px = 0.5f, float py = 0.5f)
        : texture(tex), parallaxX(px), parallaxY(py) {}
};

/// Manages and renders parallax background layers
/// Layers are rendered back to front (index 0 = furthest back)
class ParallaxBackground {
public:
    ParallaxBackground() = default;
    explicit ParallaxBackground(IRenderer* renderer) : m_renderer(renderer) {}

    /// Set the renderer
    void setRenderer(IRenderer* renderer) { m_renderer = renderer; }

    /// Set the camera for parallax calculation
    void setCamera(const Camera* camera) { m_camera = camera; }

    /// Add a layer to the background (returns layer index)
    size_t addLayer(const ParallaxLayer& layer);

    /// Add a simple layer with texture and parallax factor
    size_t addLayer(const Texture* texture, float parallaxX, float parallaxY = 0.0f);

    /// Get a layer by index (for modification)
    ParallaxLayer* getLayer(size_t index);

    /// Get the number of layers
    size_t getLayerCount() const { return m_layers.size(); }

    /// Remove a layer by index
    void removeLayer(size_t index);

    /// Clear all layers
    void clear() { m_layers.clear(); }

    /// Update automatic scrolling
    void update(float dt);

    /// Render all layers
    void render();

    /// Render a specific layer
    void renderLayer(size_t index);

    /// Set the base scroll position (in addition to camera)
    void setScrollOffset(Vec2 offset) { m_scrollOffset = offset; }
    Vec2 getScrollOffset() const { return m_scrollOffset; }

private:
    void renderLayerInternal(const ParallaxLayer& layer);

    IRenderer* m_renderer = nullptr;
    const Camera* m_camera = nullptr;
    std::vector<ParallaxLayer> m_layers;
    Vec2 m_scrollOffset = {0.0f, 0.0f};
    float m_autoScrollTime = 0.0f;
};

} // namespace gloaming
