#pragma once

#include "rendering/IRenderer.hpp"
#include "rendering/Texture.hpp"
#include <raylib.h>
#include <unordered_map>
#include <memory>

namespace gloaming {

/// Raylib implementation of the IRenderer interface
class RaylibRenderer : public IRenderer {
public:
    RaylibRenderer() = default;
    ~RaylibRenderer() override;

    // IRenderer interface implementation
    bool init(int screenWidth, int screenHeight) override;
    void shutdown() override;

    void beginFrame() override;
    void endFrame() override;

    void clear(const Color& color) override;

    int getScreenWidth() const override { return m_screenWidth; }
    int getScreenHeight() const override { return m_screenHeight; }
    void setScreenSize(int width, int height) override;

    Texture* loadTexture(const std::string& path) override;
    void unloadTexture(Texture* texture) override;

    void drawTexture(const Texture* texture, Vec2 position,
                    Color tint = Color::White()) override;

    void drawTextureRegion(const Texture* texture, const Rect& source,
                          const Rect& dest, Color tint = Color::White()) override;

    void drawTextureEx(const Texture* texture, Vec2 position,
                       float rotation, float scale,
                       Color tint = Color::White()) override;

    void drawRectangle(const Rect& rect, const Color& color) override;
    void drawRectangleOutline(const Rect& rect, const Color& color,
                              float thickness = 1.0f) override;

    void drawText(const std::string& text, Vec2 position, int fontSize,
                 const Color& color) override;

    int measureTextWidth(const std::string& text, int fontSize) override;

    /// Get the Raylib Texture2D for a texture (for advanced usage)
    const ::Texture2D* getRaylibTexture(const Texture* texture) const;

private:
    struct RaylibTextureData {
        ::Texture2D* raylibTexture = nullptr;
        std::unique_ptr<Texture> engineTexture;
    };

    int m_screenWidth = 0;
    int m_screenHeight = 0;
    bool m_initialized = false;

    // Map from engine texture ID to Raylib texture data
    std::unordered_map<unsigned int, RaylibTextureData> m_textures;
    unsigned int m_nextTextureId = 1;
};

} // namespace gloaming
