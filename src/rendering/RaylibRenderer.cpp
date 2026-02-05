#include "rendering/RaylibRenderer.hpp"
#include "engine/Log.hpp"

namespace gloaming {

// Helper to convert our Color to Raylib Color
static ::Color toRaylibColor(const Color& c) {
    return {c.r, c.g, c.b, c.a};
}

// Helper to convert our Rect to Raylib Rectangle
static Rectangle toRaylibRect(const Rect& r) {
    return {r.x, r.y, r.width, r.height};
}

// Helper to convert our Vec2 to Raylib Vector2
static Vector2 toRaylibVec2(const Vec2& v) {
    return {v.x, v.y};
}

RaylibRenderer::~RaylibRenderer() {
    if (m_initialized) {
        shutdown();
    }
}

bool RaylibRenderer::init(int screenWidth, int screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    m_initialized = true;

    LOG_INFO("RaylibRenderer: Initialized ({}x{})", screenWidth, screenHeight);
    return true;
}

void RaylibRenderer::shutdown() {
    // Unload all textures
    for (auto& [id, data] : m_textures) {
        if (data.raylibTexture) {
            UnloadTexture(*data.raylibTexture);
            delete data.raylibTexture;
        }
    }
    m_textures.clear();

    m_initialized = false;
    LOG_INFO("RaylibRenderer: Shut down");
}

void RaylibRenderer::beginFrame() {
    BeginDrawing();
}

void RaylibRenderer::endFrame() {
    EndDrawing();
}

void RaylibRenderer::clear(const Color& color) {
    ClearBackground(toRaylibColor(color));
}

void RaylibRenderer::setScreenSize(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;
}

Texture* RaylibRenderer::loadTexture(const std::string& path) {
    // Load via Raylib
    ::Texture2D rlTexture = LoadTexture(path.c_str());

    if (rlTexture.id == 0) {
        LOG_ERROR("RaylibRenderer: Failed to load texture '{}'", path);
        return nullptr;
    }

    // Create our texture wrapper
    unsigned int id = m_nextTextureId++;
    auto engineTexture = std::make_unique<Texture>(rlTexture.width, rlTexture.height, id);

    // Store Raylib texture
    RaylibTextureData data;
    data.raylibTexture = new ::Texture2D(rlTexture);
    data.engineTexture = std::move(engineTexture);

    Texture* result = data.engineTexture.get();
    m_textures[id] = std::move(data);

    LOG_DEBUG("RaylibRenderer: Loaded texture '{}' ({}x{}, id={})",
              path, rlTexture.width, rlTexture.height, id);

    return result;
}

void RaylibRenderer::unloadTexture(Texture* texture) {
    if (!texture) return;

    unsigned int id = texture->getId();
    auto it = m_textures.find(id);
    if (it != m_textures.end()) {
        if (it->second.raylibTexture) {
            UnloadTexture(*it->second.raylibTexture);
            delete it->second.raylibTexture;
        }
        m_textures.erase(it);
        LOG_DEBUG("RaylibRenderer: Unloaded texture id={}", id);
    }
}

const ::Texture2D* RaylibRenderer::getRaylibTexture(const Texture* texture) const {
    if (!texture) return nullptr;

    auto it = m_textures.find(texture->getId());
    if (it != m_textures.end()) {
        return it->second.raylibTexture;
    }
    return nullptr;
}

void RaylibRenderer::drawTexture(const Texture* texture, Vec2 position, Color tint) {
    const ::Texture2D* rlTex = getRaylibTexture(texture);
    if (!rlTex) return;

    DrawTexture(*rlTex, static_cast<int>(position.x), static_cast<int>(position.y),
                toRaylibColor(tint));
}

void RaylibRenderer::drawTextureRegion(const Texture* texture, const Rect& source,
                                       const Rect& dest, Color tint) {
    const ::Texture2D* rlTex = getRaylibTexture(texture);
    if (!rlTex) return;

    DrawTexturePro(*rlTex,
                   toRaylibRect(source),
                   toRaylibRect(dest),
                   {0, 0},  // origin
                   0.0f,    // rotation
                   toRaylibColor(tint));
}

void RaylibRenderer::drawTextureEx(const Texture* texture, Vec2 position,
                                   float rotation, float scale, Color tint) {
    const ::Texture2D* rlTex = getRaylibTexture(texture);
    if (!rlTex) return;

    DrawTextureEx(*rlTex, toRaylibVec2(position), rotation, scale, toRaylibColor(tint));
}

void RaylibRenderer::drawRectangle(const Rect& rect, const Color& color) {
    DrawRectangle(static_cast<int>(rect.x), static_cast<int>(rect.y),
                  static_cast<int>(rect.width), static_cast<int>(rect.height),
                  toRaylibColor(color));
}

void RaylibRenderer::drawRectangleOutline(const Rect& rect, const Color& color,
                                          float thickness) {
    DrawRectangleLinesEx(toRaylibRect(rect), thickness, toRaylibColor(color));
}

void RaylibRenderer::drawText(const std::string& text, Vec2 position,
                              int fontSize, const Color& color) {
    DrawText(text.c_str(), static_cast<int>(position.x), static_cast<int>(position.y),
             fontSize, toRaylibColor(color));
}

int RaylibRenderer::measureTextWidth(const std::string& text, int fontSize) {
    return MeasureText(text.c_str(), fontSize);
}

} // namespace gloaming
