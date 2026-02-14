#pragma once

#include "engine/Vec2.hpp"

namespace gloaming {

struct UIScalingConfig {
    float baseScale   = 1.0f;    // Global UI scale multiplier
    int   minFontSize = 12;      // Floor for any rendered font size (pixels)
    float dpiScale    = 1.0f;    // Auto-detected from display DPI (future)
};

/// Scales all UI dimensions and font sizes to ensure readability at the
/// target resolution.  Enforces a minimum font size (Steam Deck Verified
/// requires at least 9px at 1280x800; Valve recommends 12px).
class UIScaling {
public:
    void configure(const UIScalingConfig& config);

    /// Apply scale to a font size (enforces minimum)
    int scaleFontSize(int designSize) const;

    /// Apply scale to a dimension (padding, margin, widget size)
    float scaleDimension(float designValue) const;

    /// Apply scale to a position (for layout offsets)
    Vec2 scalePosition(Vec2 designPos) const;

    /// Get the effective scale factor (baseScale * dpiScale)
    float getScale() const;

    /// Auto-detect appropriate scale from screen size.
    /// Uses the reference resolution of 1280x720 as the baseline (scale 1.0).
    void autoDetect(int screenWidth, int screenHeight);

    /// Get current config (read-only)
    const UIScalingConfig& getConfig() const { return m_config; }

    /// Set the base scale multiplier at runtime
    void setBaseScale(float scale);

    /// Set the minimum font size at runtime
    void setMinFontSize(int minSize);

private:
    UIScalingConfig m_config;
};

} // namespace gloaming
