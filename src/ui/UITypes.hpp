#pragma once

#include "rendering/IRenderer.hpp"

#include <string>
#include <cstdint>
#include <functional>

namespace gloaming {

/// How a UI dimension (width/height) is specified
enum class SizeMode {
    Auto,       // Determined by children/content
    Fixed,      // Fixed pixel value
    Percent,    // Percentage of parent
    Grow        // Fill remaining space (flex-grow)
};

/// A dimension value that can be absolute, percent, auto, or grow
struct UIDimension {
    SizeMode mode = SizeMode::Auto;
    float value = 0.0f;

    static UIDimension Auto() { return {SizeMode::Auto, 0.0f}; }
    static UIDimension Fixed(float px) { return {SizeMode::Fixed, px}; }
    static UIDimension Percent(float pct) { return {SizeMode::Percent, pct}; }
    static UIDimension Grow(float weight = 1.0f) { return {SizeMode::Grow, weight}; }
};

/// Direction for laying out children in a container
enum class FlexDirection {
    Row,        // Left to right
    Column      // Top to bottom
};

/// How children are aligned on the main axis
enum class JustifyContent {
    Start,      // Pack toward start
    Center,     // Pack toward center
    End,        // Pack toward end
    SpaceBetween, // Even spacing between children
    SpaceAround   // Even spacing around children
};

/// How children are aligned on the cross axis
enum class AlignItems {
    Start,
    Center,
    End,
    Stretch     // Stretch to fill cross axis
};

/// Text alignment within a text element
enum class TextAlign {
    Left,
    Center,
    Right
};

/// Edge values (padding, margin, border)
struct UIEdges {
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    float left = 0.0f;

    UIEdges() = default;
    explicit UIEdges(float all) : top(all), right(all), bottom(all), left(all) {}
    UIEdges(float vertical, float horizontal)
        : top(vertical), right(horizontal), bottom(vertical), left(horizontal) {}
    UIEdges(float t, float r, float b, float l)
        : top(t), right(r), bottom(b), left(l) {}

    float horizontal() const { return left + right; }
    float vertical() const { return top + bottom; }
};

/// Border specification
struct UIBorder {
    float width = 0.0f;
    Color color = Color::White();
};

/// Complete style for a UI element
struct UIStyle {
    // Sizing
    UIDimension width = UIDimension::Auto();
    UIDimension height = UIDimension::Auto();
    float minWidth = 0.0f;
    float minHeight = 0.0f;
    float maxWidth = 0.0f;  // 0 = no max
    float maxHeight = 0.0f; // 0 = no max

    // Layout (for containers)
    FlexDirection flexDirection = FlexDirection::Column;
    JustifyContent justifyContent = JustifyContent::Start;
    AlignItems alignItems = AlignItems::Start;
    float gap = 0.0f; // Space between children

    // Spacing
    UIEdges padding;
    UIEdges margin;

    // Appearance
    Color backgroundColor = Color(0, 0, 0, 0); // Transparent by default
    UIBorder border;
    float cornerRadius = 0.0f; // Not yet rendered, but stored for future use

    // Text
    int fontSize = 20;
    Color textColor = Color::White();
    TextAlign textAlign = TextAlign::Left;

    // Visibility
    bool visible = true;

    // Scrolling
    bool overflowHidden = false;
};

/// Computed layout result for a UI element
struct UIComputedLayout {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    Rect toRect() const { return Rect(x, y, width, height); }
    bool containsPoint(float px, float py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

/// Types of UI elements
enum class UIElementType {
    Box,
    Text,
    Image,
    Button,
    Slider,
    Grid,
    ScrollPanel
};

/// Callback types used by interactive elements
using UICallback = std::function<void()>;
using UIValueCallback = std::function<void(float)>;

} // namespace gloaming
