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

void SpriteBatch::draw(const SpriteData& sprite) {
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

    SpriteData sprite;
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

    SpriteData sprite;
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

    SpriteData sprite;
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

    SpriteData sprite;
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

    SpriteData sprite;
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
            [](const SpriteData& a, const SpriteData& b) {
                return a.layer < b.layer;
            });
    }

    // Render each sprite
    m_lastDrawCalls = 0;
    for (const SpriteData& sprite : m_sprites) {
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

void SpriteBatch::renderSprite(const SpriteData& sprite) {
    // Calculate destination rectangle size
    float width = sprite.sourceRect.width * sprite.scale.x;
    float height = sprite.sourceRect.height * sprite.scale.y;

    // Transform to screen space if we have a camera
    Vec2 screenPos = sprite.position;
    if (m_camera) {
        screenPos = m_camera->worldToScreen(sprite.position);
        float zoom = m_camera->getZoom();
        width *= zoom;
        height *= zoom;
    }

    // Use the renderer to draw
    if (sprite.rotation != 0.0f) {
        // For rotated sprites, position the dest rect at the sprite position
        // and use origin (in pixels) to define the rotation pivot
        Vec2 originPixels(width * sprite.origin.x, height * sprite.origin.y);
        Rect destRect(screenPos.x, screenPos.y, width, height);
        m_renderer->drawTextureRegionEx(sprite.texture, sprite.sourceRect,
                                        destRect, originPixels, sprite.rotation,
                                        sprite.tint);
    } else {
        // For non-rotated sprites, apply origin offset to position
        float destX = screenPos.x - width * sprite.origin.x;
        float destY = screenPos.y - height * sprite.origin.y;
        Rect destRect(destX, destY, width, height);
        m_renderer->drawTextureRegion(sprite.texture, sprite.sourceRect,
                                      destRect, sprite.tint);
    }
}

} // namespace gloaming
