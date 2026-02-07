#pragma once

#include "ui/UIElement.hpp"
#include "rendering/IRenderer.hpp"

namespace gloaming {

/// Flexbox-style layout engine for UI elements.
/// Computes position and size for a tree of UIElements based on their styles.
class UILayout {
public:
    UILayout() = default;

    /// Set the renderer (needed for text measurement)
    void setRenderer(IRenderer* renderer) { m_renderer = renderer; }

    /// Compute layout for an element tree, given the available space.
    /// This recursively computes the layout for all children.
    void computeLayout(UIElement* root, float availableWidth, float availableHeight);

    /// Set the layout renderer on all text elements in a tree.
    /// Call once after creating/rebuilding a UI tree, not every frame.
    void prepareMeasurement(UIElement* element);

private:
    /// Resolve a dimension value given the available space and content size
    float resolveDimension(const UIDimension& dim, float available, float content) const;

    /// Compute layout for a box/container element
    void layoutContainer(UIElement* element, float availableWidth, float availableHeight);

    /// Layout children in a row (FlexDirection::Row)
    void layoutRow(UIElement* element, float innerWidth, float innerHeight);

    /// Layout children in a column (FlexDirection::Column)
    void layoutColumn(UIElement* element, float innerWidth, float innerHeight);

    /// Layout children in a grid
    void layoutGrid(UIElement* element, float innerWidth, float innerHeight);

    /// Layout children in a scroll panel (same as column but with scroll offset)
    void layoutScrollPanel(UIElement* element, float innerWidth, float innerHeight);

    /// Apply min/max constraints to a computed size
    float applyConstraints(float size, float minSize, float maxSize) const;

    IRenderer* m_renderer = nullptr;
};

} // namespace gloaming
