#pragma once

#include "rendering/IRenderer.hpp"

namespace gloaming {

enum class ScaleMode {
    /// Scales to fill the window; content may be cropped on one axis.
    FillCrop,

    /// Scales to fit within the window; may have letterbox/pillarbox bars.
    FitLetterbox,

    /// Stretches to fill exactly (distorts aspect ratio — not recommended).
    Stretch,

    /// Expand the game world to fill the extra space (show more of the world).
    Expand,
};

struct ViewportConfig {
    int designWidth = 1280;       // The resolution the game is designed for
    int designHeight = 720;
    ScaleMode scaleMode = ScaleMode::Expand;
    Color letterboxColor = Color::Black();
};

/// Handles aspect ratio adaptation between the design resolution and the
/// actual window resolution.  Computes viewport rectangles, scale factors,
/// and coordinate transforms for each of the four supported scale modes.
class ViewportScaler {
public:
    void configure(const ViewportConfig& config);

    /// Call each frame with the actual window size.
    /// Computes the viewport rectangle and scale factor.
    void update(int windowWidth, int windowHeight);

    /// Get the computed viewport (where to render within the window)
    Rect getViewport() const { return m_viewport; }

    /// Get the effective game resolution (may differ from design if Expand mode)
    int getEffectiveWidth() const { return m_effectiveWidth; }
    int getEffectiveHeight() const { return m_effectiveHeight; }

    /// Get the scale factor from design resolution to screen pixels
    float getScale() const { return m_scale; }

    /// Convert screen coordinates to game coordinates (for input)
    Vec2 screenToGame(Vec2 screenPos) const;

    /// Convert game coordinates to screen coordinates
    Vec2 gameToScreen(Vec2 gamePos) const;

    /// Render letterbox/pillarbox bars (call after game rendering)
    void renderBars(IRenderer* renderer) const;

    /// Get the current config
    const ViewportConfig& getConfig() const { return m_config; }

private:
    ViewportConfig m_config;

    // Computed each frame
    Rect  m_viewport = {0, 0, 1280, 720};
    int   m_effectiveWidth = 1280;
    int   m_effectiveHeight = 720;
    float m_scale = 1.0f;

    int m_windowWidth = 1280;
    int m_windowHeight = 720;
};

} // namespace gloaming
