#pragma once

#include "ui/UITypes.hpp"

#include <string>
#include <vector>
#include <memory>

namespace gloaming {

// Forward declarations
class IRenderer;
class UIInput;

/// Base class for all UI elements.
/// Elements form a tree structure. Container elements hold children.
class UIElement {
public:
    explicit UIElement(UIElementType type, const std::string& id = "");
    virtual ~UIElement() = default;

    // Non-copyable
    UIElement(const UIElement&) = delete;
    UIElement& operator=(const UIElement&) = delete;

    // Identity
    UIElementType getType() const { return m_type; }
    const std::string& getId() const { return m_id; }
    void setId(const std::string& id) { m_id = id; }

    // Style
    UIStyle& getStyle() { return m_style; }
    const UIStyle& getStyle() const { return m_style; }
    void setStyle(const UIStyle& style) { m_style = style; }

    // Layout
    const UIComputedLayout& getLayout() const { return m_layout; }
    UIComputedLayout& getLayoutMut() { return m_layout; }

    // Tree structure
    void addChild(std::shared_ptr<UIElement> child);
    void removeChild(const std::string& id);
    void clearChildren();
    const std::vector<std::shared_ptr<UIElement>>& getChildren() const { return m_children; }
    std::vector<std::shared_ptr<UIElement>>& getChildren() { return m_children; }
    size_t getChildCount() const { return m_children.size(); }
    UIElement* getParent() const { return m_parent; }

    // Find element by ID in this subtree
    UIElement* findById(const std::string& id);
    const UIElement* findById(const std::string& id) const;

    // Rendering
    virtual void render(IRenderer* renderer) const;

    // Input handling
    virtual bool handleMousePress(float mx, float my);
    virtual bool handleMouseRelease(float mx, float my);
    virtual bool handleMouseMove(float mx, float my);
    virtual bool handleKeyPress(int key);

    // Focus
    bool isFocusable() const { return m_focusable; }
    void setFocusable(bool focusable) { m_focusable = focusable; }
    bool isFocused() const { return m_focused; }
    void setFocused(bool focused) { m_focused = focused; }

    // Hover state
    bool isHovered() const { return m_hovered; }
    void setHovered(bool hovered) { m_hovered = hovered; }

    // Pressed state (for buttons)
    bool isPressed() const { return m_pressed; }
    void setPressed(bool pressed) { m_pressed = pressed; }

    // Content size (for Auto sizing)
    virtual float getContentWidth() const { return 0.0f; }
    virtual float getContentHeight() const { return 0.0f; }

protected:
    void renderBackground(IRenderer* renderer) const;
    void renderBorder(IRenderer* renderer) const;
    void renderChildren(IRenderer* renderer) const;

    UIElementType m_type;
    std::string m_id;
    UIStyle m_style;
    UIComputedLayout m_layout;

    std::vector<std::shared_ptr<UIElement>> m_children;
    UIElement* m_parent = nullptr;

    bool m_focusable = false;
    bool m_focused = false;
    bool m_hovered = false;
    bool m_pressed = false;
};

} // namespace gloaming
