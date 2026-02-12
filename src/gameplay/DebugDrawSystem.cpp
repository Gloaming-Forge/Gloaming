#include "gameplay/DebugDrawSystem.hpp"

namespace gloaming {

// =========================================================================
// World-space draw commands
// =========================================================================

void DebugDrawSystem::drawRect(float x, float y, float width, float height,
                                const Color& color) {
    DebugDrawCommand cmd;
    cmd.type = DebugDrawType::Rect;
    cmd.space = DebugDrawSpace::World;
    cmd.x = x; cmd.y = y;
    cmd.width = width; cmd.height = height;
    cmd.color = color;
    m_commands.push_back(cmd);
}

void DebugDrawSystem::drawRectOutline(float x, float y, float width, float height,
                                       const Color& color, float thickness) {
    DebugDrawCommand cmd;
    cmd.type = DebugDrawType::RectOutline;
    cmd.space = DebugDrawSpace::World;
    cmd.x = x; cmd.y = y;
    cmd.width = width; cmd.height = height;
    cmd.color = color;
    cmd.thickness = thickness;
    m_commands.push_back(cmd);
}

void DebugDrawSystem::drawCircle(float x, float y, float radius, const Color& color) {
    DebugDrawCommand cmd;
    cmd.type = DebugDrawType::Circle;
    cmd.space = DebugDrawSpace::World;
    cmd.x = x; cmd.y = y;
    cmd.radius = radius;
    cmd.color = color;
    m_commands.push_back(cmd);
}

void DebugDrawSystem::drawCircleOutline(float x, float y, float radius,
                                         const Color& color, float thickness) {
    DebugDrawCommand cmd;
    cmd.type = DebugDrawType::CircleOutline;
    cmd.space = DebugDrawSpace::World;
    cmd.x = x; cmd.y = y;
    cmd.radius = radius;
    cmd.color = color;
    cmd.thickness = thickness;
    m_commands.push_back(cmd);
}

void DebugDrawSystem::drawLine(float x1, float y1, float x2, float y2,
                                const Color& color, float thickness) {
    DebugDrawCommand cmd;
    cmd.type = DebugDrawType::Line;
    cmd.space = DebugDrawSpace::World;
    cmd.x = x1; cmd.y = y1;
    cmd.x2 = x2; cmd.y2 = y2;
    cmd.color = color;
    cmd.thickness = thickness;
    m_commands.push_back(cmd);
}

void DebugDrawSystem::drawPoint(float x, float y, const Color& color, float size) {
    DebugDrawCommand cmd;
    cmd.type = DebugDrawType::Point;
    cmd.space = DebugDrawSpace::World;
    cmd.x = x; cmd.y = y;
    cmd.radius = size;
    cmd.color = color;
    m_commands.push_back(cmd);
}

void DebugDrawSystem::drawText(const std::string& text, float x, float y,
                                const Color& color, int fontSize) {
    DebugDrawCommand cmd;
    cmd.type = DebugDrawType::Text;
    cmd.space = DebugDrawSpace::World;
    cmd.x = x; cmd.y = y;
    cmd.color = color;
    cmd.text = text;
    cmd.fontSize = fontSize;
    m_commands.push_back(cmd);
}

void DebugDrawSystem::drawPath(const std::vector<Vec2>& points, const Color& color,
                                float thickness) {
    if (points.size() < 2) return;
    DebugDrawPath path;
    path.points = points;
    path.color = color;
    path.thickness = thickness;
    path.space = DebugDrawSpace::World;
    m_paths.push_back(std::move(path));
}

// =========================================================================
// Screen-space draw commands
// =========================================================================

void DebugDrawSystem::drawTextScreen(const std::string& text, float x, float y,
                                      const Color& color, int fontSize) {
    DebugDrawCommand cmd;
    cmd.type = DebugDrawType::Text;
    cmd.space = DebugDrawSpace::Screen;
    cmd.x = x; cmd.y = y;
    cmd.color = color;
    cmd.text = text;
    cmd.fontSize = fontSize;
    m_commands.push_back(cmd);
}

void DebugDrawSystem::drawRectScreen(float x, float y, float width, float height,
                                      const Color& color) {
    DebugDrawCommand cmd;
    cmd.type = DebugDrawType::Rect;
    cmd.space = DebugDrawSpace::Screen;
    cmd.x = x; cmd.y = y;
    cmd.width = width; cmd.height = height;
    cmd.color = color;
    m_commands.push_back(cmd);
}

void DebugDrawSystem::drawLineScreen(float x1, float y1, float x2, float y2,
                                      const Color& color, float thickness) {
    DebugDrawCommand cmd;
    cmd.type = DebugDrawType::Line;
    cmd.space = DebugDrawSpace::Screen;
    cmd.x = x1; cmd.y = y1;
    cmd.x2 = x2; cmd.y2 = y2;
    cmd.color = color;
    cmd.thickness = thickness;
    m_commands.push_back(cmd);
}

// =========================================================================
// Rendering
// =========================================================================

void DebugDrawSystem::render(IRenderer* renderer, const Camera& camera) {
    if (!m_enabled || !renderer) {
        // Even when disabled, clear the queue so commands don't accumulate
        m_commands.clear();
        m_paths.clear();
        return;
    }

    // Render all commands
    for (const auto& cmd : m_commands) {
        renderCommand(cmd, renderer, camera);
    }

    // Render paths
    for (const auto& path : m_paths) {
        renderPath(path, renderer, camera);
    }

    // Auto-clear each frame
    m_commands.clear();
    m_paths.clear();
}

void DebugDrawSystem::renderCommand(const DebugDrawCommand& cmd, IRenderer* renderer,
                                     const Camera& camera) {
    // Transform world-space positions to screen space
    auto toScreen = [&](float x, float y) -> Vec2 {
        if (cmd.space == DebugDrawSpace::Screen) {
            return {x, y};
        }
        return camera.worldToScreen({x, y});
    };

    float zoom = (cmd.space == DebugDrawSpace::World) ? camera.getZoom() : 1.0f;

    switch (cmd.type) {
        case DebugDrawType::Rect: {
            Vec2 screenPos = toScreen(cmd.x, cmd.y);
            Rect rect(screenPos.x, screenPos.y,
                      cmd.width * zoom, cmd.height * zoom);
            renderer->drawRectangle(rect, cmd.color);
            break;
        }
        case DebugDrawType::RectOutline: {
            Vec2 screenPos = toScreen(cmd.x, cmd.y);
            Rect rect(screenPos.x, screenPos.y,
                      cmd.width * zoom, cmd.height * zoom);
            renderer->drawRectangleOutline(rect, cmd.color, cmd.thickness);
            break;
        }
        case DebugDrawType::Circle: {
            Vec2 screenPos = toScreen(cmd.x, cmd.y);
            renderer->drawCircle(screenPos, cmd.radius * zoom, cmd.color);
            break;
        }
        case DebugDrawType::CircleOutline: {
            Vec2 screenPos = toScreen(cmd.x, cmd.y);
            renderer->drawCircleOutline(screenPos, cmd.radius * zoom,
                                         cmd.color, cmd.thickness);
            break;
        }
        case DebugDrawType::Line: {
            Vec2 start = toScreen(cmd.x, cmd.y);
            Vec2 end = toScreen(cmd.x2, cmd.y2);
            renderer->drawLine(start, end, cmd.color, cmd.thickness);
            break;
        }
        case DebugDrawType::Point: {
            Vec2 screenPos = toScreen(cmd.x, cmd.y);
            float halfSize = cmd.radius * 0.5f * zoom;
            Rect rect(screenPos.x - halfSize, screenPos.y - halfSize,
                      cmd.radius * zoom, cmd.radius * zoom);
            renderer->drawRectangle(rect, cmd.color);
            break;
        }
        case DebugDrawType::Text: {
            Vec2 screenPos = toScreen(cmd.x, cmd.y);
            renderer->drawText(cmd.text, screenPos, cmd.fontSize, cmd.color);
            break;
        }
    }
}

void DebugDrawSystem::renderPath(const DebugDrawPath& path, IRenderer* renderer,
                                  const Camera& camera) {
    for (size_t i = 0; i + 1 < path.points.size(); ++i) {
        Vec2 start, end;
        if (path.space == DebugDrawSpace::World) {
            start = camera.worldToScreen(path.points[i]);
            end = camera.worldToScreen(path.points[i + 1]);
        } else {
            start = path.points[i];
            end = path.points[i + 1];
        }
        renderer->drawLine(start, end, path.color, path.thickness);
    }

    // Draw points at each node
    for (const auto& point : path.points) {
        Vec2 screenPos;
        if (path.space == DebugDrawSpace::World) {
            screenPos = camera.worldToScreen(point);
        } else {
            screenPos = point;
        }
        float zoom = (path.space == DebugDrawSpace::World) ? camera.getZoom() : 1.0f;
        float dotSize = 3.0f * zoom;
        Rect dot(screenPos.x - dotSize * 0.5f, screenPos.y - dotSize * 0.5f,
                 dotSize, dotSize);
        renderer->drawRectangle(dot, path.color);
    }
}

} // namespace gloaming
