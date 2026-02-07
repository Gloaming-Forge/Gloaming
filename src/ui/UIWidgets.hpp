#pragma once

#include "ui/UIElement.hpp"
#include "rendering/IRenderer.hpp"

#include <string>
#include <functional>

namespace gloaming {

/// Box: a generic container element. The basic building block.
class UIBox : public UIElement {
public:
    explicit UIBox(const std::string& id = "")
        : UIElement(UIElementType::Box, id) {}

    void render(IRenderer* renderer) const override;
};

/// Text: renders a string of text.
class UIText : public UIElement {
public:
    explicit UIText(const std::string& id = "", const std::string& text = "")
        : UIElement(UIElementType::Text, id), m_text(text) {}

    void setText(const std::string& text) { m_text = text; }
    const std::string& getText() const { return m_text; }

    float getContentWidth() const override;
    float getContentHeight() const override;

    void render(IRenderer* renderer) const override;

    /// Set a renderer for text measurement (must be called before layout for Auto sizing)
    void setMeasureRenderer(IRenderer* renderer) { m_measureRenderer = renderer; }

private:
    std::string m_text;
    IRenderer* m_measureRenderer = nullptr;
};

/// Image: renders a texture.
class UIImage : public UIElement {
public:
    explicit UIImage(const std::string& id = "")
        : UIElement(UIElementType::Image, id) {}

    void setTexture(Texture* texture) { m_texture = texture; }
    Texture* getTexture() const { return m_texture; }

    void setSourceRect(const Rect& src) { m_sourceRect = src; m_hasSourceRect = true; }
    void clearSourceRect() { m_hasSourceRect = false; }

    void setTint(const Color& tint) { m_tint = tint; }

    float getContentWidth() const override;
    float getContentHeight() const override;

    void render(IRenderer* renderer) const override;

private:
    Texture* m_texture = nullptr;
    Rect m_sourceRect;
    bool m_hasSourceRect = false;
    Color m_tint = Color::White();
};

/// Button: a clickable element with hover/press states and a callback.
class UIButton : public UIElement {
public:
    explicit UIButton(const std::string& id = "", const std::string& label = "")
        : UIElement(UIElementType::Button, id), m_label(label)
    {
        m_focusable = true;
        // Default button styling
        m_style.backgroundColor = Color(60, 60, 80, 255);
        m_style.border = {1.0f, Color(100, 100, 130, 255)};
        m_style.padding = UIEdges(8, 16);
        m_style.textColor = Color::White();
    }

    void setLabel(const std::string& label) { m_label = label; }
    const std::string& getLabel() const { return m_label; }

    void setOnClick(UICallback callback) { m_onClick = std::move(callback); }

    void setHoverColor(const Color& color) { m_hoverColor = color; }
    void setPressColor(const Color& color) { m_pressColor = color; }

    float getContentWidth() const override;
    float getContentHeight() const override;

    void render(IRenderer* renderer) const override;

    bool handleMousePress(float mx, float my) override;
    bool handleMouseRelease(float mx, float my) override;

private:
    std::string m_label;
    UICallback m_onClick;
    Color m_hoverColor = Color(80, 80, 110, 255);
    Color m_pressColor = Color(40, 40, 60, 255);
    IRenderer* m_measureRenderer = nullptr;
};

/// Slider: a draggable value selector.
class UISlider : public UIElement {
public:
    explicit UISlider(const std::string& id = "")
        : UIElement(UIElementType::Slider, id)
    {
        m_focusable = true;
        m_style.width = UIDimension::Fixed(200.0f);
        m_style.height = UIDimension::Fixed(24.0f);
        m_style.backgroundColor = Color(40, 40, 60, 255);
        m_style.border = {1.0f, Color(80, 80, 110, 255)};
    }

    float getValue() const { return m_value; }
    void setValue(float value);

    float getMinValue() const { return m_minValue; }
    float getMaxValue() const { return m_maxValue; }
    void setRange(float minVal, float maxVal) { m_minValue = minVal; m_maxValue = maxVal; }

    void setOnChange(UIValueCallback callback) { m_onChange = std::move(callback); }

    void setTrackColor(const Color& color) { m_trackColor = color; }
    void setFillColor(const Color& color) { m_fillColor = color; }
    void setKnobColor(const Color& color) { m_knobColor = color; }

    void render(IRenderer* renderer) const override;

    bool handleMousePress(float mx, float my) override;
    bool handleMouseRelease(float mx, float my) override;
    bool handleMouseMove(float mx, float my) override;

private:
    void updateValueFromMouse(float mx);
    float getNormalizedValue() const;

    float m_value = 0.0f;
    float m_minValue = 0.0f;
    float m_maxValue = 1.0f;
    bool m_dragging = false;
    UIValueCallback m_onChange;
    Color m_trackColor = Color(40, 40, 60, 255);
    Color m_fillColor = Color(80, 140, 220, 255);
    Color m_knobColor = Color(200, 200, 220, 255);
};

/// Grid: lays out children in a grid with a fixed number of columns.
class UIGrid : public UIElement {
public:
    explicit UIGrid(const std::string& id = "", int columns = 1)
        : UIElement(UIElementType::Grid, id), m_columns(columns) {}

    int getColumns() const { return m_columns; }
    void setColumns(int columns) { m_columns = columns > 0 ? columns : 1; }

    float getCellWidth() const { return m_cellWidth; }
    float getCellHeight() const { return m_cellHeight; }
    void setCellSize(float width, float height) { m_cellWidth = width; m_cellHeight = height; }

    void render(IRenderer* renderer) const override;

private:
    int m_columns = 1;
    float m_cellWidth = 0.0f;  // 0 = auto from available width
    float m_cellHeight = 0.0f; // 0 = auto from cell width
};

/// ScrollPanel: a container that clips its children and supports scrolling.
class UIScrollPanel : public UIElement {
public:
    explicit UIScrollPanel(const std::string& id = "")
        : UIElement(UIElementType::ScrollPanel, id)
    {
        m_style.overflowHidden = true;
    }

    float getScrollX() const { return m_scrollX; }
    float getScrollY() const { return m_scrollY; }
    void setScroll(float x, float y) { m_scrollX = x; m_scrollY = y; }

    float getContentHeight() const override;
    float getContentWidth() const override;

    void render(IRenderer* renderer) const override;
    bool handleMousePress(float mx, float my) override;
    bool handleMouseRelease(float mx, float my) override;
    bool handleMouseMove(float mx, float my) override;

    /// Handle mouse wheel for scrolling
    void handleScroll(float delta);

private:
    void clampScroll();

    float m_scrollX = 0.0f;
    float m_scrollY = 0.0f;
    float m_scrollSpeed = 30.0f;
};

} // namespace gloaming
