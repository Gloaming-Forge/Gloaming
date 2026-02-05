#include "rendering/ParallaxBackground.hpp"
#include "engine/Log.hpp"
#include <cmath>

namespace gloaming {

size_t ParallaxBackground::addLayer(const ParallaxLayer& layer) {
    m_layers.push_back(layer);
    return m_layers.size() - 1;
}

size_t ParallaxBackground::addLayer(const Texture* texture, float parallaxX, float parallaxY) {
    ParallaxLayer layer;
    layer.texture = texture;
    layer.parallaxX = parallaxX;
    layer.parallaxY = parallaxY;
    return addLayer(layer);
}

ParallaxLayer* ParallaxBackground::getLayer(size_t index) {
    if (index < m_layers.size()) {
        return &m_layers[index];
    }
    return nullptr;
}

void ParallaxBackground::removeLayer(size_t index) {
    if (index < m_layers.size()) {
        m_layers.erase(m_layers.begin() + static_cast<ptrdiff_t>(index));
    }
}

void ParallaxBackground::update(float dt) {
    m_autoScrollTime += dt;

    // Update automatic scrolling for each layer
    for (auto& layer : m_layers) {
        if (layer.scrollSpeedX != 0.0f) {
            layer.offsetX += layer.scrollSpeedX * dt;
            // Wrap offset to prevent floating point issues over time
            if (layer.texture && layer.repeatX) {
                float texWidth = static_cast<float>(layer.texture->getWidth()) * layer.scale;
                while (layer.offsetX > texWidth) layer.offsetX -= texWidth;
                while (layer.offsetX < -texWidth) layer.offsetX += texWidth;
            }
        }
        if (layer.scrollSpeedY != 0.0f) {
            layer.offsetY += layer.scrollSpeedY * dt;
            if (layer.texture && layer.repeatY) {
                float texHeight = static_cast<float>(layer.texture->getHeight()) * layer.scale;
                while (layer.offsetY > texHeight) layer.offsetY -= texHeight;
                while (layer.offsetY < -texHeight) layer.offsetY += texHeight;
            }
        }
    }
}

void ParallaxBackground::render() {
    for (size_t i = 0; i < m_layers.size(); ++i) {
        renderLayerInternal(m_layers[i]);
    }
}

void ParallaxBackground::renderLayer(size_t index) {
    if (index < m_layers.size()) {
        renderLayerInternal(m_layers[index]);
    }
}

void ParallaxBackground::renderLayerInternal(const ParallaxLayer& layer) {
    if (!m_renderer || !layer.texture || !layer.texture->isValid()) {
        return;
    }

    int screenWidth = m_renderer->getScreenWidth();
    int screenHeight = m_renderer->getScreenHeight();

    float texWidth = static_cast<float>(layer.texture->getWidth()) * layer.scale;
    float texHeight = static_cast<float>(layer.texture->getHeight()) * layer.scale;

    // Calculate parallax offset based on camera position
    float parallaxOffsetX = 0.0f;
    float parallaxOffsetY = 0.0f;

    if (m_camera) {
        Vec2 camPos = m_camera->getPosition();
        // Parallax factor: 0 = no movement, 1 = moves with camera
        // Subtract camera position scaled by parallax factor
        parallaxOffsetX = -camPos.x * layer.parallaxX;
        parallaxOffsetY = -camPos.y * layer.parallaxY;
    }

    // Add base offset and scroll offset
    float totalOffsetX = layer.offsetX + parallaxOffsetX + m_scrollOffset.x;
    float totalOffsetY = layer.offsetY + parallaxOffsetY + m_scrollOffset.y;

    if (layer.repeatX || layer.repeatY) {
        // Tiling mode - wrap offsets
        if (layer.repeatX && texWidth > 0) {
            totalOffsetX = std::fmod(totalOffsetX, texWidth);
            if (totalOffsetX > 0) totalOffsetX -= texWidth;
        }
        if (layer.repeatY && texHeight > 0) {
            totalOffsetY = std::fmod(totalOffsetY, texHeight);
            if (totalOffsetY > 0) totalOffsetY -= texHeight;
        }

        // Calculate how many tiles we need to fill the screen
        int tilesX = layer.repeatX ? static_cast<int>(std::ceil(screenWidth / texWidth)) + 2 : 1;
        int tilesY = layer.repeatY ? static_cast<int>(std::ceil(screenHeight / texHeight)) + 2 : 1;

        // Draw tiled
        Rect source(0, 0, static_cast<float>(layer.texture->getWidth()),
                   static_cast<float>(layer.texture->getHeight()));

        for (int ty = 0; ty < tilesY; ++ty) {
            for (int tx = 0; tx < tilesX; ++tx) {
                float destX = totalOffsetX + tx * texWidth;
                float destY = totalOffsetY + ty * texHeight;

                // Skip if completely off screen
                if (destX + texWidth < 0 || destX > screenWidth) continue;
                if (destY + texHeight < 0 || destY > screenHeight) continue;

                Rect dest(destX, destY, texWidth, texHeight);
                m_renderer->drawTextureRegion(layer.texture, source, dest, layer.tint);
            }
        }
    } else {
        // Single image mode - center or position as specified
        Rect source(0, 0, static_cast<float>(layer.texture->getWidth()),
                   static_cast<float>(layer.texture->getHeight()));

        // Center the image and apply offset
        float destX = (screenWidth - texWidth) * 0.5f + totalOffsetX;
        float destY = (screenHeight - texHeight) * 0.5f + totalOffsetY;

        Rect dest(destX, destY, texWidth, texHeight);
        m_renderer->drawTextureRegion(layer.texture, source, dest, layer.tint);
    }
}

} // namespace gloaming
