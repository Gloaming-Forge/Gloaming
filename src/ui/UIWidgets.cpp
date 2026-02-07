#include "ui/UIWidgets.hpp"
#include "engine/Input.hpp"
#include "rendering/Texture.hpp"

#include <algorithm>
#include <cmath>

namespace gloaming {

// ---------------------------------------------------------------------------
// UIBox
// ---------------------------------------------------------------------------

void UIBox::render(IRenderer* renderer) const {
    if (!m_style.visible) return;
    renderBackground(renderer);
    renderBorder(renderer);
    renderChildren(renderer);
}

// ---------------------------------------------------------------------------
// UIText
// ---------------------------------------------------------------------------

float UIText::getContentWidth() const {
    if (m_text.empty()) return 0.0f;
    if (m_measureRenderer) {
        return static_cast<float>(m_measureRenderer->measureTextWidth(m_text, m_style.fontSize));
    }
    // Rough estimate: ~0.6 * fontSize per character
    return static_cast<float>(m_text.size()) * m_style.fontSize * 0.6f;
}

float UIText::getContentHeight() const {
    if (m_text.empty()) return 0.0f;
    return static_cast<float>(m_style.fontSize);
}

void UIText::render(IRenderer* renderer) const {
    if (!m_style.visible || m_text.empty()) return;

    renderBackground(renderer);

    // Calculate text position based on alignment
    float textX = m_layout.x + m_style.padding.left;
    float textY = m_layout.y + m_style.padding.top;

    if (m_style.textAlign != TextAlign::Left) {
        int textWidth = renderer->measureTextWidth(m_text, m_style.fontSize);
        float availWidth = m_layout.width - m_style.padding.horizontal();
        if (m_style.textAlign == TextAlign::Center) {
            textX += (availWidth - textWidth) * 0.5f;
        } else if (m_style.textAlign == TextAlign::Right) {
            textX += availWidth - textWidth;
        }
    }

    renderer->drawText(m_text, {textX, textY}, m_style.fontSize, m_style.textColor);
    renderBorder(renderer);
}

// ---------------------------------------------------------------------------
// UIImage
// ---------------------------------------------------------------------------

float UIImage::getContentWidth() const {
    if (m_hasSourceRect) return m_sourceRect.width;
    // No good way to get texture dimensions without renderer integration
    return 0.0f;
}

float UIImage::getContentHeight() const {
    if (m_hasSourceRect) return m_sourceRect.height;
    return 0.0f;
}

void UIImage::render(IRenderer* renderer) const {
    if (!m_style.visible) return;

    renderBackground(renderer);

    if (m_texture) {
        Rect dest = m_layout.toRect();
        // Apply padding
        dest.x += m_style.padding.left;
        dest.y += m_style.padding.top;
        dest.width -= m_style.padding.horizontal();
        dest.height -= m_style.padding.vertical();

        if (m_hasSourceRect) {
            renderer->drawTextureRegion(m_texture, m_sourceRect, dest, m_tint);
        } else {
            // Draw full texture stretched to fill the element
            Rect fullSrc(0, 0, static_cast<float>(m_texture->getWidth()),
                              static_cast<float>(m_texture->getHeight()));
            renderer->drawTextureRegion(m_texture, fullSrc, dest, m_tint);
        }
    }

    renderBorder(renderer);
}

// ---------------------------------------------------------------------------
// UIButton
// ---------------------------------------------------------------------------

float UIButton::getContentWidth() const {
    if (m_label.empty()) return 0.0f;
    // Rough estimate
    return static_cast<float>(m_label.size()) * m_style.fontSize * 0.6f;
}

float UIButton::getContentHeight() const {
    return static_cast<float>(m_style.fontSize);
}

void UIButton::render(IRenderer* renderer) const {
    if (!m_style.visible) return;

    // Choose color based on state
    Color bg = m_style.backgroundColor;
    if (m_pressed) {
        bg = m_pressColor;
    } else if (m_hovered) {
        bg = m_hoverColor;
    }

    if (bg.a > 0) {
        renderer->drawRectangle(m_layout.toRect(), bg);
    }

    // Draw label centered
    if (!m_label.empty()) {
        int textWidth = renderer->measureTextWidth(m_label, m_style.fontSize);
        float textX = m_layout.x + (m_layout.width - textWidth) * 0.5f;
        float textY = m_layout.y + (m_layout.height - m_style.fontSize) * 0.5f;
        renderer->drawText(m_label, {textX, textY}, m_style.fontSize, m_style.textColor);
    }

    // Border - highlight if focused
    if (m_focused) {
        renderer->drawRectangleOutline(m_layout.toRect(), Color(180, 180, 255, 255), 2.0f);
    } else {
        renderBorder(renderer);
    }

    renderChildren(renderer);
}

bool UIButton::handleMousePress(float mx, float my) {
    if (!m_style.visible) return false;
    if (m_layout.containsPoint(mx, my)) {
        m_pressed = true;
        return true;
    }
    return false;
}

bool UIButton::handleMouseRelease(float mx, float my) {
    if (!m_style.visible) return false;
    if (m_pressed) {
        m_pressed = false;
        if (m_layout.containsPoint(mx, my) && m_onClick) {
            m_onClick();
        }
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// UISlider
// ---------------------------------------------------------------------------

void UISlider::setValue(float value) {
    m_value = std::clamp(value, m_minValue, m_maxValue);
}

float UISlider::getNormalizedValue() const {
    if (m_maxValue <= m_minValue) return 0.0f;
    return (m_value - m_minValue) / (m_maxValue - m_minValue);
}

void UISlider::updateValueFromMouse(float mx) {
    float trackX = m_layout.x + m_style.padding.left;
    float trackWidth = m_layout.width - m_style.padding.horizontal();
    if (trackWidth <= 0.0f) return;

    float normalized = std::clamp((mx - trackX) / trackWidth, 0.0f, 1.0f);
    float newValue = m_minValue + normalized * (m_maxValue - m_minValue);

    if (newValue != m_value) {
        m_value = newValue;
        if (m_onChange) {
            m_onChange(m_value);
        }
    }
}

void UISlider::render(IRenderer* renderer) const {
    if (!m_style.visible) return;

    renderBackground(renderer);

    float trackX = m_layout.x + m_style.padding.left + 2.0f;
    float trackY = m_layout.y + m_layout.height * 0.5f - 3.0f;
    float trackWidth = m_layout.width - m_style.padding.horizontal() - 4.0f;
    float trackHeight = 6.0f;

    // Track background
    renderer->drawRectangle({trackX, trackY, trackWidth, trackHeight}, m_trackColor);

    // Fill
    float fillWidth = trackWidth * getNormalizedValue();
    if (fillWidth > 0.0f) {
        renderer->drawRectangle({trackX, trackY, fillWidth, trackHeight}, m_fillColor);
    }

    // Knob
    float knobX = trackX + fillWidth - 6.0f;
    float knobY = m_layout.y + m_layout.height * 0.5f - 8.0f;
    renderer->drawRectangle({knobX, knobY, 12.0f, 16.0f}, m_knobColor);

    // Focus highlight
    if (m_focused) {
        renderer->drawRectangleOutline(m_layout.toRect(), Color(180, 180, 255, 255), 2.0f);
    } else {
        renderBorder(renderer);
    }
}

bool UISlider::handleMousePress(float mx, float my) {
    if (!m_style.visible) return false;
    if (m_layout.containsPoint(mx, my)) {
        m_dragging = true;
        m_pressed = true;
        updateValueFromMouse(mx);
        return true;
    }
    return false;
}

bool UISlider::handleMouseRelease(float mx, float my) {
    if (m_dragging) {
        m_dragging = false;
        m_pressed = false;
        return true;
    }
    return false;
}

bool UISlider::handleMouseMove(float mx, float my) {
    UIElement::handleMouseMove(mx, my);
    if (m_dragging) {
        updateValueFromMouse(mx);
        return true;
    }
    return false;
}

bool UISlider::handleKeyPress(int key) {
    float step = (m_maxValue - m_minValue) * 0.05f; // 5% per press
    if (key == static_cast<int>(Key::Left)) {
        float oldValue = m_value;
        setValue(m_value - step);
        if (m_value != oldValue && m_onChange) m_onChange(m_value);
        return true;
    }
    if (key == static_cast<int>(Key::Right)) {
        float oldValue = m_value;
        setValue(m_value + step);
        if (m_value != oldValue && m_onChange) m_onChange(m_value);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// UIGrid
// ---------------------------------------------------------------------------

void UIGrid::render(IRenderer* renderer) const {
    if (!m_style.visible) return;
    renderBackground(renderer);
    renderBorder(renderer);
    renderChildren(renderer);
}

// ---------------------------------------------------------------------------
// UIScrollPanel
// ---------------------------------------------------------------------------

float UIScrollPanel::getContentHeight() const {
    float maxBottom = 0.0f;
    for (const auto& child : m_children) {
        const auto& cl = child->getLayout();
        float bottom = cl.y + cl.height - m_layout.y + m_scrollY;
        maxBottom = std::max(maxBottom, bottom);
    }
    return maxBottom;
}

float UIScrollPanel::getContentWidth() const {
    float maxRight = 0.0f;
    for (const auto& child : m_children) {
        const auto& cl = child->getLayout();
        float right = cl.x + cl.width - m_layout.x + m_scrollX;
        maxRight = std::max(maxRight, right);
    }
    return maxRight;
}

void UIScrollPanel::clampScroll() {
    float contentH = getContentHeight();
    float viewH = m_layout.height - m_style.padding.vertical();
    m_scrollY = std::clamp(m_scrollY, 0.0f, std::max(0.0f, contentH - viewH));

    float contentW = getContentWidth();
    float viewW = m_layout.width - m_style.padding.horizontal();
    m_scrollX = std::clamp(m_scrollX, 0.0f, std::max(0.0f, contentW - viewW));
}

void UIScrollPanel::render(IRenderer* renderer) const {
    if (!m_style.visible) return;
    renderBackground(renderer);

    // Render children (clipping is not possible with basic renderer, but we still render)
    // In a full implementation, we'd use scissor rectangles
    renderChildren(renderer);

    // Scrollbar indicator
    float contentH = getContentHeight();
    float viewH = m_layout.height - m_style.padding.vertical();
    if (contentH > viewH && viewH > 0.0f) {
        float scrollBarHeight = std::max(20.0f, viewH * (viewH / contentH));
        float scrollBarY = m_layout.y + m_style.padding.top +
                          (m_scrollY / contentH) * viewH;
        float scrollBarX = m_layout.x + m_layout.width - 6.0f;
        renderer->drawRectangle({scrollBarX, scrollBarY, 4.0f, scrollBarHeight},
                               Color(150, 150, 150, 120));
    }

    renderBorder(renderer);
}

bool UIScrollPanel::handleMousePress(float mx, float my) {
    if (!m_style.visible) return false;
    if (!m_layout.containsPoint(mx, my)) return false;

    // Adjust coordinates for scroll offset before passing to children
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it)->handleMousePress(mx, my)) return true;
    }
    return true; // Consume the click even if no child handled it
}

bool UIScrollPanel::handleMouseRelease(float mx, float my) {
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it)->handleMouseRelease(mx, my)) return true;
    }
    return false;
}

bool UIScrollPanel::handleMouseMove(float mx, float my) {
    UIElement::handleMouseMove(mx, my);
    for (auto& child : m_children) {
        child->handleMouseMove(mx, my);
    }
    return m_hovered;
}

void UIScrollPanel::handleScroll(float delta) {
    m_scrollY -= delta * m_scrollSpeed;
    clampScroll();
}

} // namespace gloaming
