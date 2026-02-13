#pragma once

#include "engine/Vec2.hpp"

#include <string>
#include <cstdint>
#include <cmath>

namespace gloaming {

// Math constants
constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// Forward declarations
class Camera;
class SpriteBatch;
class Texture;
class TextureAtlas;

/// Color representation with RGBA components
struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    constexpr Color() = default;
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    static constexpr Color White()   { return {255, 255, 255, 255}; }
    static constexpr Color Black()   { return {0, 0, 0, 255}; }
    static constexpr Color Red()     { return {255, 0, 0, 255}; }
    static constexpr Color Green()   { return {0, 255, 0, 255}; }
    static constexpr Color Blue()    { return {0, 0, 255, 255}; }
    static constexpr Color Gray()    { return {128, 128, 128, 255}; }
    static constexpr Color DarkGray(){ return {40, 40, 40, 255}; }
};

/// Rectangle for sprite regions, collision bounds, etc.
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    constexpr Rect() = default;
    constexpr Rect(float x, float y, float w, float h)
        : x(x), y(y), width(w), height(h) {}

    bool contains(Vec2 point) const {
        return point.x >= x && point.x < x + width &&
               point.y >= y && point.y < y + height;
    }

    bool intersects(const Rect& other) const {
        return x < other.x + other.width && x + width > other.x &&
               y < other.y + other.height && y + height > other.y;
    }
};

/// Abstract renderer interface for backend-agnostic rendering
/// This allows swapping Raylib for Vulkan/SDL/etc. in the future
class IRenderer {
public:
    virtual ~IRenderer() = default;

    /// Initialize the renderer (called after window creation)
    virtual bool init(int screenWidth, int screenHeight) = 0;

    /// Shutdown and release resources
    virtual void shutdown() = 0;

    /// Begin a new frame
    virtual void beginFrame() = 0;

    /// End the current frame and present
    virtual void endFrame() = 0;

    /// Clear the screen with a color
    virtual void clear(const Color& color) = 0;

    /// Get the current screen dimensions
    virtual int getScreenWidth() const = 0;
    virtual int getScreenHeight() const = 0;

    /// Update screen dimensions (e.g., after window resize)
    virtual void setScreenSize(int width, int height) = 0;

    /// Load a texture from file
    virtual Texture* loadTexture(const std::string& path) = 0;

    /// Unload a texture
    virtual void unloadTexture(Texture* texture) = 0;

    /// Draw a texture at position
    virtual void drawTexture(const Texture* texture, Vec2 position,
                            Color tint = Color::White()) = 0;

    /// Draw a portion of a texture (for atlases/spritesheets)
    virtual void drawTextureRegion(const Texture* texture, const Rect& source,
                                   const Rect& dest, Color tint = Color::White()) = 0;

    /// Draw a portion of a texture with rotation and origin
    virtual void drawTextureRegionEx(const Texture* texture, const Rect& source,
                                     const Rect& dest, Vec2 origin, float rotation,
                                     Color tint = Color::White()) = 0;

    /// Draw a texture with full transform options
    virtual void drawTextureEx(const Texture* texture, Vec2 position,
                               float rotation, float scale,
                               Color tint = Color::White()) = 0;

    /// Draw a filled rectangle
    virtual void drawRectangle(const Rect& rect, const Color& color) = 0;

    /// Draw a rectangle outline
    virtual void drawRectangleOutline(const Rect& rect, const Color& color, float thickness = 1.0f) = 0;

    /// Draw a line between two points
    virtual void drawLine(Vec2 start, Vec2 end, const Color& color, float thickness = 1.0f) = 0;

    /// Draw a filled circle
    virtual void drawCircle(Vec2 center, float radius, const Color& color) = 0;

    /// Draw a circle outline
    virtual void drawCircleOutline(Vec2 center, float radius, const Color& color, float thickness = 1.0f) = 0;

    /// Draw text (basic, for debugging)
    virtual void drawText(const std::string& text, Vec2 position, int fontSize,
                         const Color& color) = 0;

    /// Measure text width (for layout)
    virtual int measureTextWidth(const std::string& text, int fontSize) = 0;
};

} // namespace gloaming
