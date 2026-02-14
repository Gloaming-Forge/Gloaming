#include "rendering/ViewportScaler.hpp"

#include <algorithm>
#include <cmath>

namespace gloaming {

void ViewportScaler::configure(const ViewportConfig& config) {
    m_config = config;
}

void ViewportScaler::update(int windowWidth, int windowHeight) {
    if (windowWidth <= 0 || windowHeight <= 0) return;

    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;

    const float designW = static_cast<float>(m_config.designWidth);
    const float designH = static_cast<float>(m_config.designHeight);
    const float winW    = static_cast<float>(windowWidth);
    const float winH    = static_cast<float>(windowHeight);

    const float designAspect = designW / designH;
    const float windowAspect = winW / winH;

    switch (m_config.scaleMode) {
        case ScaleMode::FillCrop: {
            // Scale so the design area fills the window entirely; excess is cropped.
            m_scale = std::max(winW / designW, winH / designH);

            float scaledW = designW * m_scale;
            float scaledH = designH * m_scale;

            m_viewport.x     = (winW - scaledW) * 0.5f;
            m_viewport.y     = (winH - scaledH) * 0.5f;
            m_viewport.width  = scaledW;
            m_viewport.height = scaledH;

            m_effectiveWidth  = m_config.designWidth;
            m_effectiveHeight = m_config.designHeight;
            break;
        }

        case ScaleMode::FitLetterbox: {
            // Scale so the entire design area is visible; bars fill the gap.
            m_scale = std::min(winW / designW, winH / designH);

            float scaledW = designW * m_scale;
            float scaledH = designH * m_scale;

            m_viewport.x      = std::floor((winW - scaledW) * 0.5f);
            m_viewport.y      = std::floor((winH - scaledH) * 0.5f);
            m_viewport.width  = scaledW;
            m_viewport.height = scaledH;

            m_effectiveWidth  = m_config.designWidth;
            m_effectiveHeight = m_config.designHeight;
            break;
        }

        case ScaleMode::Stretch: {
            // Fill the entire window, potentially distorting the aspect ratio.
            m_viewport.x      = 0;
            m_viewport.y      = 0;
            m_viewport.width  = winW;
            m_viewport.height = winH;

            m_scale = winW / designW;  // Horizontal scale (vertical may differ)

            m_effectiveWidth  = m_config.designWidth;
            m_effectiveHeight = m_config.designHeight;
            break;
        }

        case ScaleMode::Expand: {
            // Keep the design width, expand height (or vice versa) so no
            // content is lost and no bars are needed.  The game world simply
            // shows more on the longer axis.
            if (windowAspect >= designAspect) {
                // Window is wider than design — keep design height, expand width
                m_scale = winH / designH;
                m_effectiveHeight = m_config.designHeight;
                m_effectiveWidth  = static_cast<int>(std::round(winW / m_scale));
            } else {
                // Window is taller than design — keep design width, expand height
                m_scale = winW / designW;
                m_effectiveWidth  = m_config.designWidth;
                m_effectiveHeight = static_cast<int>(std::round(winH / m_scale));
            }

            m_viewport.x      = 0;
            m_viewport.y      = 0;
            m_viewport.width  = winW;
            m_viewport.height = winH;
            break;
        }
    }
}

Vec2 ViewportScaler::screenToGame(Vec2 screenPos) const {
    if (m_config.scaleMode == ScaleMode::Stretch) {
        float scaleX = static_cast<float>(m_windowWidth)  / static_cast<float>(m_effectiveWidth);
        float scaleY = static_cast<float>(m_windowHeight) / static_cast<float>(m_effectiveHeight);
        return {screenPos.x / scaleX, screenPos.y / scaleY};
    }

    float gameX = (screenPos.x - m_viewport.x) / m_scale;
    float gameY = (screenPos.y - m_viewport.y) / m_scale;
    return {gameX, gameY};
}

Vec2 ViewportScaler::gameToScreen(Vec2 gamePos) const {
    if (m_config.scaleMode == ScaleMode::Stretch) {
        float scaleX = static_cast<float>(m_windowWidth)  / static_cast<float>(m_effectiveWidth);
        float scaleY = static_cast<float>(m_windowHeight) / static_cast<float>(m_effectiveHeight);
        return {gamePos.x * scaleX, gamePos.y * scaleY};
    }

    float screenX = gamePos.x * m_scale + m_viewport.x;
    float screenY = gamePos.y * m_scale + m_viewport.y;
    return {screenX, screenY};
}

void ViewportScaler::renderBars(IRenderer* renderer) const {
    if (!renderer) return;
    if (m_config.scaleMode != ScaleMode::FitLetterbox) return;

    const float winW = static_cast<float>(m_windowWidth);
    const float winH = static_cast<float>(m_windowHeight);
    const Color& c = m_config.letterboxColor;

    // Pillarbox bars (left and right)
    if (m_viewport.x > 0) {
        renderer->drawRectangle({0, 0, m_viewport.x, winH}, c);
        renderer->drawRectangle({m_viewport.x + m_viewport.width, 0,
                                 winW - (m_viewport.x + m_viewport.width), winH}, c);
    }

    // Letterbox bars (top and bottom)
    if (m_viewport.y > 0) {
        renderer->drawRectangle({0, 0, winW, m_viewport.y}, c);
        renderer->drawRectangle({0, m_viewport.y + m_viewport.height,
                                 winW, winH - (m_viewport.y + m_viewport.height)}, c);
    }
}

} // namespace gloaming
