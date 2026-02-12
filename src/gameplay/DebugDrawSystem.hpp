#pragma once

#include "rendering/IRenderer.hpp"
#include "rendering/Camera.hpp"

#include <string>
#include <vector>

namespace gloaming {

/// Debug draw command types
enum class DebugDrawType {
    Rect,
    RectOutline,
    Circle,
    CircleOutline,
    Line,
    Point,
    Text,
};

/// Whether the draw command is in world space or screen space
enum class DebugDrawSpace {
    World,
    Screen,
};

/// A single debug draw command (queued each frame, auto-cleared)
struct DebugDrawCommand {
    DebugDrawType type = DebugDrawType::Rect;
    DebugDrawSpace space = DebugDrawSpace::World;

    // Position / geometry
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;    // For rects
    float height = 0.0f;   // For rects
    float radius = 0.0f;   // For circles
    float x2 = 0.0f;       // End point for lines
    float y2 = 0.0f;       // End point for lines
    float thickness = 1.0f;

    // Appearance
    Color color = Color::Green();

    // Text
    std::string text;
    int fontSize = 14;
};

/// Debug draw path (sequence of connected points)
struct DebugDrawPath {
    std::vector<Vec2> points;
    Color color = Color(255, 255, 0, 255);
    float thickness = 1.0f;
    DebugDrawSpace space = DebugDrawSpace::World;
};

/// Debug Drawing System — overlay drawing for development and debugging.
///
/// Provides:
///   - Draw rectangles, circles, lines, points in world or screen space
///   - Draw text labels at positions
///   - Path visualization (connected points)
///   - Auto-clear each frame (no manual cleanup)
///   - Globally togglable with F3
///   - Color and opacity control
class DebugDrawSystem {
public:
    DebugDrawSystem() = default;

    /// Toggle debug drawing on/off
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /// Check if debug drawing is enabled
    bool isEnabled() const { return m_enabled; }

    /// Toggle debug drawing
    void toggle() { m_enabled = !m_enabled; }

    // =========================================================================
    // World-space draw commands
    // =========================================================================

    /// Draw a filled rectangle in world space
    void drawRect(float x, float y, float width, float height, const Color& color);

    /// Draw a rectangle outline in world space
    void drawRectOutline(float x, float y, float width, float height,
                         const Color& color, float thickness = 1.0f);

    /// Draw a filled circle in world space
    void drawCircle(float x, float y, float radius, const Color& color);

    /// Draw a circle outline in world space
    void drawCircleOutline(float x, float y, float radius, const Color& color,
                           float thickness = 1.0f);

    /// Draw a line in world space
    void drawLine(float x1, float y1, float x2, float y2,
                  const Color& color, float thickness = 1.0f);

    /// Draw a point in world space
    void drawPoint(float x, float y, const Color& color, float size = 4.0f);

    /// Draw text at a world position
    void drawText(const std::string& text, float x, float y,
                  const Color& color = Color::White(), int fontSize = 14);

    /// Draw a path (sequence of connected points) in world space
    void drawPath(const std::vector<Vec2>& points, const Color& color,
                  float thickness = 1.0f);

    // =========================================================================
    // Screen-space draw commands
    // =========================================================================

    /// Draw text in screen space (not affected by camera)
    void drawTextScreen(const std::string& text, float x, float y,
                        const Color& color = Color::White(), int fontSize = 14);

    /// Draw a filled rectangle in screen space
    void drawRectScreen(float x, float y, float width, float height,
                        const Color& color);

    /// Draw a line in screen space
    void drawLineScreen(float x1, float y1, float x2, float y2,
                        const Color& color, float thickness = 1.0f);

    // =========================================================================
    // Rendering
    // =========================================================================

    /// Render all queued debug draw commands, then clear the queue.
    /// Called at the end of the frame (PostRender phase).
    void render(IRenderer* renderer, const Camera& camera);

    /// Get number of commands queued this frame
    size_t commandCount() const { return m_commands.size() + m_paths.size(); }

private:
    void renderCommand(const DebugDrawCommand& cmd, IRenderer* renderer,
                       const Camera& camera);
    void renderPath(const DebugDrawPath& path, IRenderer* renderer,
                    const Camera& camera);

    std::vector<DebugDrawCommand> m_commands;
    std::vector<DebugDrawPath> m_paths;
    bool m_enabled = false; // Off by default, toggled with F3
};

} // namespace gloaming
