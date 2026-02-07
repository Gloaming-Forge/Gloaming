#include "ui/UIElement.hpp"
#include "rendering/IRenderer.hpp"

namespace gloaming {

UIElement::UIElement(UIElementType type, const std::string& id)
    : m_type(type), m_id(id) {}

void UIElement::addChild(std::shared_ptr<UIElement> child) {
    child->m_parent = this;
    m_children.push_back(std::move(child));
}

void UIElement::removeChild(const std::string& id) {
    m_children.erase(
        std::remove_if(m_children.begin(), m_children.end(),
            [&id](const auto& child) { return child->getId() == id; }),
        m_children.end());
}

void UIElement::clearChildren() {
    for (auto& child : m_children) {
        child->m_parent = nullptr;
    }
    m_children.clear();
}

UIElement* UIElement::findById(const std::string& id) {
    if (m_id == id) return this;
    for (auto& child : m_children) {
        UIElement* found = child->findById(id);
        if (found) return found;
    }
    return nullptr;
}

const UIElement* UIElement::findById(const std::string& id) const {
    if (m_id == id) return this;
    for (const auto& child : m_children) {
        const UIElement* found = child->findById(id);
        if (found) return found;
    }
    return nullptr;
}

void UIElement::render(IRenderer* renderer) const {
    if (!m_style.visible) return;

    renderBackground(renderer);
    renderBorder(renderer);
    renderChildren(renderer);
}

bool UIElement::handleMousePress(float mx, float my) {
    if (!m_style.visible) return false;
    if (!m_layout.containsPoint(mx, my)) return false;

    // Propagate to children in reverse order (top-most first)
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it)->handleMousePress(mx, my)) return true;
    }
    return false;
}

bool UIElement::handleMouseRelease(float mx, float my) {
    if (!m_style.visible) return false;

    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it)->handleMouseRelease(mx, my)) return true;
    }
    return false;
}

bool UIElement::handleMouseMove(float mx, float my) {
    if (!m_style.visible) return false;

    bool wasHovered = m_hovered;
    m_hovered = m_layout.containsPoint(mx, my);

    for (auto& child : m_children) {
        child->handleMouseMove(mx, my);
    }

    return m_hovered && !wasHovered;
}

bool UIElement::handleKeyPress(int key) {
    if (!m_style.visible) return false;

    for (auto& child : m_children) {
        if (child->handleKeyPress(key)) return true;
    }
    return false;
}

void UIElement::renderBackground(IRenderer* renderer) const {
    if (m_style.backgroundColor.a == 0) return;
    renderer->drawRectangle(m_layout.toRect(), m_style.backgroundColor);
}

void UIElement::renderBorder(IRenderer* renderer) const {
    if (m_style.border.width <= 0.0f) return;
    renderer->drawRectangleOutline(m_layout.toRect(), m_style.border.color,
                                    m_style.border.width);
}

void UIElement::renderChildren(IRenderer* renderer) const {
    for (const auto& child : m_children) {
        child->render(renderer);
    }
}

} // namespace gloaming
