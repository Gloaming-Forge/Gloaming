#include "rendering/SpriteBatch.hpp"
#include "engine/Log.hpp"
#include <algorithm>

namespace gloaming {

void SpriteBatch::begin() {
    if (m_batching) {
        LOG_WARN("SpriteBatch::begin() called while already batching");
        return;
    }
    m_sprites.clear();
    m_batching = true;
}

void SpriteBatch::draw(const Sprite& sprite) {
    if (!m_batching) {
        LOG_WARN("SpriteBatch::draw() called without begin()");
        return;
    }
    if (!sprite.texture || !sprite.texture->isValid()) {
        return;
    }
    m_sprites.push_back(sprite);
}

void SpriteBatch::draw(const Texture* texture, Vec2 position, Color tint) {
    if (!texture) return;

    Sprite sprite;
    sprite.texture = texture;
    sprite.position = position;
    sprite.sourceRect = Rect(0, 0, static_cast<float>(texture->getWidth()),
                            static_cast<float>(texture->getHeight()));
    sprite.tint = tint;
    draw(sprite);
}

void SpriteBatch::draw(const Texture* texture, const Rect& sourceRect,
                       Vec2 position, Color tint) {
    if (!texture) return;

    Sprite sprite;
    sprite.texture = texture;
    sprite.position = position;
    sprite.sourceRect = sourceRect;
    sprite.tint = tint;
    draw(sprite);
}

void SpriteBatch::draw(const Texture* texture, const Rect& sourceRect,
                       Vec2 position, Vec2 origin, Vec2 scale, float rotation,
                       Color tint, int layer) {
    if (!texture) return;

    Sprite sprite;
    sprite.texture = texture;
    sprite.sourceRect = sourceRect;
    sprite.position = position;
    sprite.origin = origin;
    sprite.scale = scale;
    sprite.rotation = rotation;
    sprite.tint = tint;
    sprite.layer = layer;
    draw(sprite);
}

void SpriteBatch::draw(const TextureAtlas* atlas, const std::string& regionName,
                       Vec2 position, Color tint) {
    if (!atlas || !atlas->getTexture()) return;

    const AtlasRegion* region = atlas->getRegion(regionName);
    if (!region) {
        LOG_WARN("SpriteBatch: Atlas region '{}' not found", regionName);
        return;
    }

    Sprite sprite;
    sprite.texture = atlas->getTexture();
    sprite.sourceRect = region->bounds;
    sprite.position = position;
    sprite.origin = region->pivot;
    sprite.tint = tint;
    draw(sprite);
}

void SpriteBatch::draw(const TextureAtlas* atlas, const std::string& regionName,
                       Vec2 position, Vec2 origin, Vec2 scale, float rotation,
                       Color tint, int layer) {
    if (!atlas || !atlas->getTexture()) return;

    const AtlasRegion* region = atlas->getRegion(regionName);
    if (!region) {
        LOG_WARN("SpriteBatch: Atlas region '{}' not found", regionName);
        return;
    }

    Sprite sprite;
    sprite.texture = atlas->getTexture();
    sprite.sourceRect = region->bounds;
    sprite.position = position;
    sprite.origin = origin;
    sprite.scale = scale;
    sprite.rotation = rotation;
    sprite.tint = tint;
    sprite.layer = layer;
    draw(sprite);
}

void SpriteBatch::end() {
    if (!m_batching) {
        LOG_WARN("SpriteBatch::end() called without begin()");
        return;
    }
    flush();
    m_batching = false;
}

void SpriteBatch::flush() {
    if (!m_renderer) {
        LOG_WARN("SpriteBatch: No renderer set");
        return;
    }

    if (m_sprites.empty()) {
        m_lastDrawCalls = 0;
        return;
    }

    // Sort by layer if enabled
    if (m_sortEnabled) {
        std::stable_sort(m_sprites.begin(), m_sprites.end(),
            [](const Sprite& a, const Sprite& b) {
                return a.layer < b.layer;
            });
    }

    // Render each sprite
    m_lastDrawCalls = 0;
    for (const Sprite& sprite : m_sprites) {
        // Culling check
        if (m_cullingEnabled && m_camera) {
            float width = sprite.sourceRect.width * sprite.scale.x;
            float height = sprite.sourceRect.height * sprite.scale.y;
            Rect worldBounds(
                sprite.position.x - width * sprite.origin.x,
                sprite.position.y - height * sprite.origin.y,
                width, height
            );
            if (!m_camera->isVisible(worldBounds)) {
                continue;
            }
        }

        renderSprite(sprite);
        ++m_lastDrawCalls;
    }

    m_sprites.clear();
}

void SpriteBatch::renderSprite(const Sprite& sprite) {
    // Calculate destination rectangle
    float width = sprite.sourceRect.width * sprite.scale.x;
    float height = sprite.sourceRect.height * sprite.scale.y;

    // Apply origin offset
    float destX = sprite.position.x - width * sprite.origin.x;
    float destY = sprite.position.y - height * sprite.origin.y;

    // Transform to screen space if we have a camera
    if (m_camera) {
        Vec2 screenPos = m_camera->worldToScreen({destX + width * 0.5f,
                                                   destY + height * 0.5f});
        float zoom = m_camera->getZoom();
        width *= zoom;
        height *= zoom;
        destX = screenPos.x - width * 0.5f;
        destY = screenPos.y - height * 0.5f;
    }

    Rect destRect(destX, destY, width, height);

    // Use the renderer to draw
    if (sprite.rotation != 0.0f) {
        // For rotated sprites, we need a more complex draw call
        // The renderer's drawTextureRegion with rotation support would be ideal
        // For now, use the basic version (rotation handled differently by backend)
        m_renderer->drawTextureRegion(sprite.texture, sprite.sourceRect,
                                      destRect, sprite.tint);
    } else {
        m_renderer->drawTextureRegion(sprite.texture, sprite.sourceRect,
                                      destRect, sprite.tint);
    }
}

} // namespace gloaming
