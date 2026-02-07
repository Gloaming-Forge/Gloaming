#include "ui/UILayout.hpp"
#include "ui/UIWidgets.hpp"

#include <algorithm>
#include <numeric>
#include <cmath>

namespace gloaming {

float UILayout::resolveDimension(const UIDimension& dim, float available, float content) const {
    switch (dim.mode) {
        case SizeMode::Fixed:   return dim.value;
        case SizeMode::Percent: return available * (dim.value / 100.0f);
        case SizeMode::Grow:    return available; // Will be refined during flex layout
        case SizeMode::Auto:    return content;
    }
    return content;
}

float UILayout::applyConstraints(float size, float minSize, float maxSize) const {
    if (minSize > 0.0f) size = std::max(size, minSize);
    if (maxSize > 0.0f) size = std::min(size, maxSize);
    return std::max(size, 0.0f);
}

void UILayout::computeLayout(UIElement* root, float availableWidth, float availableHeight) {
    if (!root) return;

    auto& style = root->getStyle();
    auto& layout = root->getLayoutMut();

    // Resolve root element width/height
    float contentW = root->getContentWidth();
    float contentH = root->getContentHeight();

    float w = resolveDimension(style.width, availableWidth, contentW + style.padding.horizontal());
    float h = resolveDimension(style.height, availableHeight, contentH + style.padding.vertical());
    w = applyConstraints(w, style.minWidth, style.maxWidth);
    h = applyConstraints(h, style.minHeight, style.maxHeight);

    layout.x = style.margin.left;
    layout.y = style.margin.top;
    layout.width = w;
    layout.height = h;

    // Layout children
    layoutContainer(root, w, h);
}

void UILayout::layoutContainer(UIElement* element, float availableWidth, float availableHeight) {
    if (!element || element->getChildren().empty()) return;

    auto& style = element->getStyle();
    float innerWidth = availableWidth - style.padding.horizontal();
    float innerHeight = availableHeight - style.padding.vertical();

    // Choose layout strategy based on element type
    switch (element->getType()) {
        case UIElementType::Grid:
            layoutGrid(element, innerWidth, innerHeight);
            return;
        case UIElementType::ScrollPanel:
            layoutScrollPanel(element, innerWidth, innerHeight);
            return;
        default:
            break;
    }

    // Flexbox layout based on direction
    if (style.flexDirection == FlexDirection::Row) {
        layoutRow(element, innerWidth, innerHeight);
    } else {
        layoutColumn(element, innerWidth, innerHeight);
    }
}

void UILayout::layoutRow(UIElement* element, float innerWidth, float innerHeight) {
    auto& style = element->getStyle();
    auto& layout = element->getLayout();
    auto& children = element->getChildren();

    float startX = layout.x + style.padding.left;
    float startY = layout.y + style.padding.top;

    // First pass: measure children and identify grow items
    float totalFixedWidth = 0.0f;
    float totalGrowWeight = 0.0f;
    int visibleCount = 0;

    struct ChildInfo {
        float width = 0.0f;
        float height = 0.0f;
        bool isGrow = false;
        float growWeight = 0.0f;
    };
    std::vector<ChildInfo> infos(children.size());

    for (size_t i = 0; i < children.size(); ++i) {
        auto& child = children[i];
        if (!child->getStyle().visible) continue;
        visibleCount++;

        auto& cs = child->getStyle();
        auto& info = infos[i];

        // Resolve width
        if (cs.width.mode == SizeMode::Grow) {
            info.isGrow = true;
            info.growWeight = cs.width.value > 0.0f ? cs.width.value : 1.0f;
            totalGrowWeight += info.growWeight;
        } else {
            float contentW = child->getContentWidth();
            info.width = resolveDimension(cs.width, innerWidth, contentW + cs.padding.horizontal());
            info.width += cs.margin.horizontal();
            info.width = applyConstraints(info.width, cs.minWidth, cs.maxWidth);
            totalFixedWidth += info.width;
        }

        // Resolve height
        float contentH = child->getContentHeight();
        info.height = resolveDimension(cs.height, innerHeight, contentH + cs.padding.vertical());
        info.height = applyConstraints(info.height, cs.minHeight, cs.maxHeight);
    }

    // Account for gaps
    float totalGap = (visibleCount > 1) ? style.gap * (visibleCount - 1) : 0.0f;
    float remainingWidth = innerWidth - totalFixedWidth - totalGap;

    // Distribute remaining width to grow items
    if (totalGrowWeight > 0.0f && remainingWidth > 0.0f) {
        for (size_t i = 0; i < children.size(); ++i) {
            if (infos[i].isGrow) {
                infos[i].width = remainingWidth * (infos[i].growWeight / totalGrowWeight);
                auto& cs = children[i]->getStyle();
                infos[i].width = applyConstraints(infos[i].width, cs.minWidth, cs.maxWidth);
            }
        }
    }

    // Calculate total content width for justify
    float totalContentWidth = totalGap;
    for (size_t i = 0; i < children.size(); ++i) {
        if (!children[i]->getStyle().visible) continue;
        totalContentWidth += infos[i].width;
    }

    // Apply justify-content
    float cursorX = startX;
    float extraSpace = innerWidth - totalContentWidth;
    float justifyGap = 0.0f;
    float justifyOffset = 0.0f;

    switch (style.justifyContent) {
        case JustifyContent::Start:
            break;
        case JustifyContent::Center:
            justifyOffset = extraSpace * 0.5f;
            break;
        case JustifyContent::End:
            justifyOffset = extraSpace;
            break;
        case JustifyContent::SpaceBetween:
            if (visibleCount > 1) justifyGap = extraSpace / (visibleCount - 1);
            break;
        case JustifyContent::SpaceAround:
            if (visibleCount > 0) {
                float spacing = extraSpace / visibleCount;
                justifyOffset = spacing * 0.5f;
                justifyGap = spacing;
            }
            break;
    }
    cursorX += justifyOffset;

    // Second pass: position children
    for (size_t i = 0; i < children.size(); ++i) {
        auto& child = children[i];
        if (!child->getStyle().visible) continue;

        auto& cs = child->getStyle();
        auto& cl = child->getLayoutMut();

        cl.width = infos[i].width - cs.margin.horizontal();
        cl.height = infos[i].height;

        // Cross-axis alignment
        float childY = startY + cs.margin.top;
        switch (style.alignItems) {
            case AlignItems::Start:
                break;
            case AlignItems::Center:
                childY = startY + (innerHeight - cl.height) * 0.5f;
                break;
            case AlignItems::End:
                childY = startY + innerHeight - cl.height - cs.margin.bottom;
                break;
            case AlignItems::Stretch:
                cl.height = innerHeight - cs.margin.vertical();
                break;
        }

        cl.x = cursorX + cs.margin.left;
        cl.y = childY;

        cursorX += infos[i].width + style.gap + justifyGap;

        // Recursively layout children
        layoutContainer(child.get(), cl.width, cl.height);
    }
}

void UILayout::layoutColumn(UIElement* element, float innerWidth, float innerHeight) {
    auto& style = element->getStyle();
    auto& layout = element->getLayout();
    auto& children = element->getChildren();

    float startX = layout.x + style.padding.left;
    float startY = layout.y + style.padding.top;

    // First pass: measure children and identify grow items
    float totalFixedHeight = 0.0f;
    float totalGrowWeight = 0.0f;
    int visibleCount = 0;

    struct ChildInfo {
        float width = 0.0f;
        float height = 0.0f;
        bool isGrow = false;
        float growWeight = 0.0f;
    };
    std::vector<ChildInfo> infos(children.size());

    for (size_t i = 0; i < children.size(); ++i) {
        auto& child = children[i];
        if (!child->getStyle().visible) continue;
        visibleCount++;

        auto& cs = child->getStyle();
        auto& info = infos[i];

        // Resolve width
        float contentW = child->getContentWidth();
        info.width = resolveDimension(cs.width, innerWidth, contentW + cs.padding.horizontal());
        info.width = applyConstraints(info.width, cs.minWidth, cs.maxWidth);

        // Resolve height
        if (cs.height.mode == SizeMode::Grow) {
            info.isGrow = true;
            info.growWeight = cs.height.value > 0.0f ? cs.height.value : 1.0f;
            totalGrowWeight += info.growWeight;
        } else {
            float contentH = child->getContentHeight();
            info.height = resolveDimension(cs.height, innerHeight, contentH + cs.padding.vertical());
            info.height += cs.margin.vertical();
            info.height = applyConstraints(info.height, cs.minHeight, cs.maxHeight);
            totalFixedHeight += info.height;
        }
    }

    // Account for gaps
    float totalGap = (visibleCount > 1) ? style.gap * (visibleCount - 1) : 0.0f;
    float remainingHeight = innerHeight - totalFixedHeight - totalGap;

    // Distribute remaining height to grow items
    if (totalGrowWeight > 0.0f && remainingHeight > 0.0f) {
        for (size_t i = 0; i < children.size(); ++i) {
            if (infos[i].isGrow) {
                infos[i].height = remainingHeight * (infos[i].growWeight / totalGrowWeight);
                auto& cs = children[i]->getStyle();
                infos[i].height = applyConstraints(infos[i].height, cs.minHeight, cs.maxHeight);
            }
        }
    }

    // Calculate total content height for justify
    float totalContentHeight = totalGap;
    for (size_t i = 0; i < children.size(); ++i) {
        if (!children[i]->getStyle().visible) continue;
        totalContentHeight += infos[i].height;
    }

    // Apply justify-content
    float cursorY = startY;
    float extraSpace = innerHeight - totalContentHeight;
    float justifyGap = 0.0f;
    float justifyOffset = 0.0f;

    switch (style.justifyContent) {
        case JustifyContent::Start:
            break;
        case JustifyContent::Center:
            justifyOffset = extraSpace * 0.5f;
            break;
        case JustifyContent::End:
            justifyOffset = extraSpace;
            break;
        case JustifyContent::SpaceBetween:
            if (visibleCount > 1) justifyGap = extraSpace / (visibleCount - 1);
            break;
        case JustifyContent::SpaceAround:
            if (visibleCount > 0) {
                float spacing = extraSpace / visibleCount;
                justifyOffset = spacing * 0.5f;
                justifyGap = spacing;
            }
            break;
    }
    cursorY += justifyOffset;

    // Second pass: position children
    for (size_t i = 0; i < children.size(); ++i) {
        auto& child = children[i];
        if (!child->getStyle().visible) continue;

        auto& cs = child->getStyle();
        auto& cl = child->getLayoutMut();

        cl.width = infos[i].width;
        cl.height = infos[i].height - cs.margin.vertical();

        // Cross-axis alignment
        float childX = startX + cs.margin.left;
        switch (style.alignItems) {
            case AlignItems::Start:
                break;
            case AlignItems::Center:
                childX = startX + (innerWidth - cl.width) * 0.5f;
                break;
            case AlignItems::End:
                childX = startX + innerWidth - cl.width - cs.margin.right;
                break;
            case AlignItems::Stretch:
                cl.width = innerWidth - cs.margin.horizontal();
                break;
        }

        cl.x = childX;
        cl.y = cursorY + cs.margin.top;

        cursorY += infos[i].height + style.gap + justifyGap;

        // Recursively layout children
        layoutContainer(child.get(), cl.width, cl.height);
    }
}

void UILayout::layoutGrid(UIElement* element, float innerWidth, float innerHeight) {
    auto* grid = static_cast<UIGrid*>(element);
    auto& style = element->getStyle();
    auto& layout = element->getLayout();
    auto& children = element->getChildren();

    int columns = grid->getColumns();
    float cellWidth = grid->getCellWidth();
    float cellHeight = grid->getCellHeight();

    // If cell size is auto, calculate from available space
    if (cellWidth <= 0.0f) {
        cellWidth = (innerWidth - style.gap * (columns - 1)) / columns;
    }
    if (cellHeight <= 0.0f) {
        cellHeight = cellWidth; // Square cells by default
    }

    float startX = layout.x + style.padding.left;
    float startY = layout.y + style.padding.top;

    int col = 0;
    int row = 0;

    for (auto& child : children) {
        if (!child->getStyle().visible) continue;

        auto& cl = child->getLayoutMut();
        auto& cs = child->getStyle();

        cl.x = startX + col * (cellWidth + style.gap) + cs.margin.left;
        cl.y = startY + row * (cellHeight + style.gap) + cs.margin.top;
        cl.width = cellWidth - cs.margin.horizontal();
        cl.height = cellHeight - cs.margin.vertical();

        layoutContainer(child.get(), cl.width, cl.height);

        col++;
        if (col >= columns) {
            col = 0;
            row++;
        }
    }
}

void UILayout::layoutScrollPanel(UIElement* element, float innerWidth, float innerHeight) {
    auto* scroll = static_cast<UIScrollPanel*>(element);
    auto& style = element->getStyle();
    auto& layout = element->getLayout();
    auto& children = element->getChildren();

    float startX = layout.x + style.padding.left - scroll->getScrollX();
    float startY = layout.y + style.padding.top - scroll->getScrollY();

    // Layout as column within the scroll area (use a large available height)
    float cursorY = startY;
    int visibleCount = 0;

    for (auto& child : children) {
        if (!child->getStyle().visible) continue;
        visibleCount++;

        auto& cs = child->getStyle();
        auto& cl = child->getLayoutMut();

        float contentW = child->getContentWidth();
        float contentH = child->getContentHeight();

        cl.width = resolveDimension(cs.width, innerWidth, contentW + cs.padding.horizontal());
        cl.width = applyConstraints(cl.width, cs.minWidth, cs.maxWidth);
        cl.height = resolveDimension(cs.height, innerHeight, contentH + cs.padding.vertical());
        cl.height = applyConstraints(cl.height, cs.minHeight, cs.maxHeight);

        // Cross-axis alignment
        float childX = startX + cs.margin.left;
        switch (style.alignItems) {
            case AlignItems::Start:
                break;
            case AlignItems::Center:
                childX = startX + (innerWidth - cl.width) * 0.5f;
                break;
            case AlignItems::End:
                childX = startX + innerWidth - cl.width - cs.margin.right;
                break;
            case AlignItems::Stretch:
                cl.width = innerWidth - cs.margin.horizontal();
                break;
        }

        cl.x = childX;
        cl.y = cursorY + cs.margin.top;

        cursorY += cl.height + cs.margin.vertical() + style.gap;

        layoutContainer(child.get(), cl.width, cl.height);
    }
}

} // namespace gloaming
