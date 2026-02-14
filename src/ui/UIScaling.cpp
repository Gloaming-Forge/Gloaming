#include "ui/UIScaling.hpp"

#include <algorithm>
#include <cmath>

namespace gloaming {

void UIScaling::configure(const UIScalingConfig& config) {
    m_config = config;
    // Clamp minimum font size to a sane range
    m_config.minFontSize = std::max(m_config.minFontSize, 1);
}

int UIScaling::scaleFontSize(int designSize) const {
    float scaled = static_cast<float>(designSize) * getScale();
    int result = static_cast<int>(std::round(scaled));
    return std::max(result, m_config.minFontSize);
}

float UIScaling::scaleDimension(float designValue) const {
    return designValue * getScale();
}

Vec2 UIScaling::scalePosition(Vec2 designPos) const {
    float s = getScale();
    return {designPos.x * s, designPos.y * s};
}

float UIScaling::getScale() const {
    return m_config.baseScale * m_config.dpiScale;
}

void UIScaling::autoDetect(int screenWidth, int screenHeight) {
    // Use the smaller of the two axis ratios relative to 1280x720 so that
    // UI elements never overflow the screen.
    constexpr float refWidth  = 1280.0f;
    constexpr float refHeight = 720.0f;

    if (screenWidth <= 0 || screenHeight <= 0) return;

    float scaleX = static_cast<float>(screenWidth) / refWidth;
    float scaleY = static_cast<float>(screenHeight) / refHeight;

    m_config.dpiScale = std::min(scaleX, scaleY);
}

void UIScaling::setBaseScale(float scale) {
    m_config.baseScale = std::max(scale, 0.1f);
}

void UIScaling::setMinFontSize(int minSize) {
    m_config.minFontSize = std::max(minSize, 1);
}

} // namespace gloaming
