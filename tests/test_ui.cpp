#include <gtest/gtest.h>

#include "ui/UITypes.hpp"
#include "ui/UIElement.hpp"
#include "ui/UIWidgets.hpp"
#include "ui/UILayout.hpp"
#include "ui/UIInput.hpp"
#include "ui/UISystem.hpp"
#include "engine/Input.hpp"

using namespace gloaming;

// ============================================================================
// UITypes Tests
// ============================================================================

TEST(UITypesTest, UIDimensionFactories) {
    auto autoD = UIDimension::Auto();
    EXPECT_EQ(autoD.mode, SizeMode::Auto);

    auto fixed = UIDimension::Fixed(100.0f);
    EXPECT_EQ(fixed.mode, SizeMode::Fixed);
    EXPECT_FLOAT_EQ(fixed.value, 100.0f);

    auto pct = UIDimension::Percent(50.0f);
    EXPECT_EQ(pct.mode, SizeMode::Percent);
    EXPECT_FLOAT_EQ(pct.value, 50.0f);

    auto grow = UIDimension::Grow(2.0f);
    EXPECT_EQ(grow.mode, SizeMode::Grow);
    EXPECT_FLOAT_EQ(grow.value, 2.0f);
}

TEST(UITypesTest, UIEdgesConstructors) {
    UIEdges all(10.0f);
    EXPECT_FLOAT_EQ(all.top, 10.0f);
    EXPECT_FLOAT_EQ(all.right, 10.0f);
    EXPECT_FLOAT_EQ(all.bottom, 10.0f);
    EXPECT_FLOAT_EQ(all.left, 10.0f);
    EXPECT_FLOAT_EQ(all.horizontal(), 20.0f);
    EXPECT_FLOAT_EQ(all.vertical(), 20.0f);

    UIEdges vh(5.0f, 10.0f);
    EXPECT_FLOAT_EQ(vh.top, 5.0f);
    EXPECT_FLOAT_EQ(vh.right, 10.0f);
    EXPECT_FLOAT_EQ(vh.bottom, 5.0f);
    EXPECT_FLOAT_EQ(vh.left, 10.0f);

    UIEdges trbl(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(trbl.top, 1.0f);
    EXPECT_FLOAT_EQ(trbl.right, 2.0f);
    EXPECT_FLOAT_EQ(trbl.bottom, 3.0f);
    EXPECT_FLOAT_EQ(trbl.left, 4.0f);
}

TEST(UITypesTest, UIComputedLayoutContains) {
    UIComputedLayout layout;
    layout.x = 10.0f;
    layout.y = 20.0f;
    layout.width = 100.0f;
    layout.height = 50.0f;

    EXPECT_TRUE(layout.containsPoint(10.0f, 20.0f));    // top-left corner
    EXPECT_TRUE(layout.containsPoint(50.0f, 40.0f));    // center
    EXPECT_TRUE(layout.containsPoint(109.0f, 69.0f));   // near bottom-right
    EXPECT_FALSE(layout.containsPoint(110.0f, 70.0f));  // bottom-right edge (exclusive)
    EXPECT_FALSE(layout.containsPoint(9.0f, 20.0f));    // outside left
    EXPECT_FALSE(layout.containsPoint(50.0f, 71.0f));   // outside bottom
}

TEST(UITypesTest, UIComputedLayoutToRect) {
    UIComputedLayout layout;
    layout.x = 5.0f;
    layout.y = 10.0f;
    layout.width = 200.0f;
    layout.height = 100.0f;

    Rect r = layout.toRect();
    EXPECT_FLOAT_EQ(r.x, 5.0f);
    EXPECT_FLOAT_EQ(r.y, 10.0f);
    EXPECT_FLOAT_EQ(r.width, 200.0f);
    EXPECT_FLOAT_EQ(r.height, 100.0f);
}

TEST(UITypesTest, UIStyleDefaults) {
    UIStyle style;
    EXPECT_EQ(style.width.mode, SizeMode::Auto);
    EXPECT_EQ(style.height.mode, SizeMode::Auto);
    EXPECT_EQ(style.flexDirection, FlexDirection::Column);
    EXPECT_EQ(style.justifyContent, JustifyContent::Start);
    EXPECT_EQ(style.alignItems, AlignItems::Start);
    EXPECT_FLOAT_EQ(style.gap, 0.0f);
    EXPECT_EQ(style.fontSize, 20);
    EXPECT_TRUE(style.visible);
    EXPECT_EQ(style.backgroundColor.a, 0); // Transparent by default
}

// ============================================================================
// UIElement Tests
// ============================================================================

TEST(UIElementTest, BasicProperties) {
    UIBox box("test_box");
    EXPECT_EQ(box.getId(), "test_box");
    EXPECT_EQ(box.getType(), UIElementType::Box);
    EXPECT_EQ(box.getChildCount(), 0u);
    EXPECT_EQ(box.getParent(), nullptr);
}

TEST(UIElementTest, TreeStructure) {
    auto parent = std::make_shared<UIBox>("parent");
    auto child1 = std::make_shared<UIBox>("child1");
    auto child2 = std::make_shared<UIBox>("child2");

    parent->addChild(child1);
    parent->addChild(child2);

    EXPECT_EQ(parent->getChildCount(), 2u);
    EXPECT_EQ(child1->getParent(), parent.get());
    EXPECT_EQ(child2->getParent(), parent.get());
}

TEST(UIElementTest, RemoveChild) {
    auto parent = std::make_shared<UIBox>("parent");
    auto child1 = std::make_shared<UIBox>("child1");
    auto child2 = std::make_shared<UIBox>("child2");

    parent->addChild(child1);
    parent->addChild(child2);
    EXPECT_EQ(parent->getChildCount(), 2u);

    parent->removeChild("child1");
    EXPECT_EQ(parent->getChildCount(), 1u);
    EXPECT_EQ(parent->getChildren()[0]->getId(), "child2");
}

TEST(UIElementTest, ClearChildren) {
    auto parent = std::make_shared<UIBox>("parent");
    parent->addChild(std::make_shared<UIBox>("a"));
    parent->addChild(std::make_shared<UIBox>("b"));
    parent->addChild(std::make_shared<UIBox>("c"));
    EXPECT_EQ(parent->getChildCount(), 3u);

    parent->clearChildren();
    EXPECT_EQ(parent->getChildCount(), 0u);
}

TEST(UIElementTest, FindById) {
    auto root = std::make_shared<UIBox>("root");
    auto child = std::make_shared<UIBox>("child");
    auto grandchild = std::make_shared<UIBox>("grandchild");

    root->addChild(child);
    child->addChild(grandchild);

    EXPECT_EQ(root->findById("root"), root.get());
    EXPECT_EQ(root->findById("child"), child.get());
    EXPECT_EQ(root->findById("grandchild"), grandchild.get());
    EXPECT_EQ(root->findById("nonexistent"), nullptr);
}

TEST(UIElementTest, FocusableState) {
    UIBox box("box");
    EXPECT_FALSE(box.isFocusable());
    EXPECT_FALSE(box.isFocused());

    box.setFocusable(true);
    EXPECT_TRUE(box.isFocusable());

    box.setFocused(true);
    EXPECT_TRUE(box.isFocused());
}

TEST(UIElementTest, HoverState) {
    UIBox box("box");
    EXPECT_FALSE(box.isHovered());

    box.setHovered(true);
    EXPECT_TRUE(box.isHovered());
}

// ============================================================================
// UIWidget Tests
// ============================================================================

TEST(UITextTest, TextContent) {
    UIText text("label", "Hello World");
    EXPECT_EQ(text.getText(), "Hello World");
    EXPECT_EQ(text.getType(), UIElementType::Text);

    text.setText("Updated");
    EXPECT_EQ(text.getText(), "Updated");
}

TEST(UITextTest, ContentDimensions) {
    UIText text("t", "Test");
    // Without a renderer, uses rough estimate
    float w = text.getContentWidth();
    EXPECT_GT(w, 0.0f);
    EXPECT_GT(text.getContentHeight(), 0.0f);
}

TEST(UIButtonTest, BasicButton) {
    UIButton btn("mybtn", "Click Me");
    EXPECT_EQ(btn.getLabel(), "Click Me");
    EXPECT_EQ(btn.getType(), UIElementType::Button);
    EXPECT_TRUE(btn.isFocusable()); // Buttons are focusable by default
}

TEST(UIButtonTest, ClickCallback) {
    UIButton btn("btn", "Test");
    bool clicked = false;
    btn.setOnClick([&clicked]() { clicked = true; });

    // Simulate click: set layout, press inside, release inside
    btn.getLayoutMut().x = 0;
    btn.getLayoutMut().y = 0;
    btn.getLayoutMut().width = 100;
    btn.getLayoutMut().height = 30;

    btn.handleMousePress(50, 15);
    EXPECT_TRUE(btn.isPressed());
    EXPECT_FALSE(clicked); // Not clicked yet - only on release

    btn.handleMouseRelease(50, 15);
    EXPECT_FALSE(btn.isPressed());
    EXPECT_TRUE(clicked);
}

TEST(UIButtonTest, ClickOutside) {
    UIButton btn("btn", "Test");
    bool clicked = false;
    btn.setOnClick([&clicked]() { clicked = true; });

    btn.getLayoutMut().x = 0;
    btn.getLayoutMut().y = 0;
    btn.getLayoutMut().width = 100;
    btn.getLayoutMut().height = 30;

    btn.handleMousePress(50, 15);
    EXPECT_TRUE(btn.isPressed());

    // Release outside
    btn.handleMouseRelease(200, 200);
    EXPECT_FALSE(btn.isPressed());
    EXPECT_FALSE(clicked); // Should NOT trigger callback
}

TEST(UISliderTest, ValueClamping) {
    UISlider slider("vol");
    slider.setRange(0.0f, 100.0f);

    slider.setValue(50.0f);
    EXPECT_FLOAT_EQ(slider.getValue(), 50.0f);

    slider.setValue(-10.0f);
    EXPECT_FLOAT_EQ(slider.getValue(), 0.0f); // Clamped to min

    slider.setValue(200.0f);
    EXPECT_FLOAT_EQ(slider.getValue(), 100.0f); // Clamped to max
}

TEST(UISliderTest, DefaultRange) {
    UISlider slider;
    EXPECT_FLOAT_EQ(slider.getMinValue(), 0.0f);
    EXPECT_FLOAT_EQ(slider.getMaxValue(), 1.0f);
    EXPECT_FLOAT_EQ(slider.getValue(), 0.0f);
}

TEST(UIGridTest, Properties) {
    UIGrid grid("inv", 10);
    EXPECT_EQ(grid.getColumns(), 10);
    EXPECT_EQ(grid.getType(), UIElementType::Grid);

    grid.setColumns(5);
    EXPECT_EQ(grid.getColumns(), 5);

    // Zero columns should be clamped to 1
    grid.setColumns(0);
    EXPECT_EQ(grid.getColumns(), 1);
}

TEST(UIGridTest, CellSize) {
    UIGrid grid("g", 4);
    grid.setCellSize(48.0f, 48.0f);
    EXPECT_FLOAT_EQ(grid.getCellWidth(), 48.0f);
    EXPECT_FLOAT_EQ(grid.getCellHeight(), 48.0f);
}

TEST(UIScrollPanelTest, ScrollState) {
    UIScrollPanel scroll("scroll");
    EXPECT_EQ(scroll.getType(), UIElementType::ScrollPanel);
    EXPECT_FLOAT_EQ(scroll.getScrollX(), 0.0f);
    EXPECT_FLOAT_EQ(scroll.getScrollY(), 0.0f);

    scroll.setScroll(10.0f, 20.0f);
    EXPECT_FLOAT_EQ(scroll.getScrollX(), 10.0f);
    EXPECT_FLOAT_EQ(scroll.getScrollY(), 20.0f);
}

// ============================================================================
// UILayout Tests
// ============================================================================

TEST(UILayoutTest, FixedSizeLayout) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Fixed(400.0f);
    root->getStyle().height = UIDimension::Fixed(300.0f);

    layout.computeLayout(root.get(), 800.0f, 600.0f);

    EXPECT_FLOAT_EQ(root->getLayout().width, 400.0f);
    EXPECT_FLOAT_EQ(root->getLayout().height, 300.0f);
}

TEST(UILayoutTest, PercentSizeLayout) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Percent(50.0f);
    root->getStyle().height = UIDimension::Percent(75.0f);

    layout.computeLayout(root.get(), 800.0f, 600.0f);

    EXPECT_FLOAT_EQ(root->getLayout().width, 400.0f);
    EXPECT_FLOAT_EQ(root->getLayout().height, 450.0f);
}

TEST(UILayoutTest, ColumnLayoutPositioning) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Fixed(200.0f);
    root->getStyle().height = UIDimension::Fixed(300.0f);
    root->getStyle().flexDirection = FlexDirection::Column;

    auto child1 = std::make_shared<UIBox>("c1");
    child1->getStyle().width = UIDimension::Fixed(200.0f);
    child1->getStyle().height = UIDimension::Fixed(50.0f);

    auto child2 = std::make_shared<UIBox>("c2");
    child2->getStyle().width = UIDimension::Fixed(200.0f);
    child2->getStyle().height = UIDimension::Fixed(50.0f);

    root->addChild(child1);
    root->addChild(child2);

    layout.computeLayout(root.get(), 800.0f, 600.0f);

    // Children should be stacked vertically
    EXPECT_FLOAT_EQ(child1->getLayout().y, root->getLayout().y);
    EXPECT_FLOAT_EQ(child2->getLayout().y, root->getLayout().y + 50.0f);
}

TEST(UILayoutTest, RowLayoutPositioning) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Fixed(400.0f);
    root->getStyle().height = UIDimension::Fixed(100.0f);
    root->getStyle().flexDirection = FlexDirection::Row;

    auto child1 = std::make_shared<UIBox>("c1");
    child1->getStyle().width = UIDimension::Fixed(100.0f);
    child1->getStyle().height = UIDimension::Fixed(50.0f);

    auto child2 = std::make_shared<UIBox>("c2");
    child2->getStyle().width = UIDimension::Fixed(100.0f);
    child2->getStyle().height = UIDimension::Fixed(50.0f);

    root->addChild(child1);
    root->addChild(child2);

    layout.computeLayout(root.get(), 800.0f, 600.0f);

    // Children should be placed horizontally
    EXPECT_FLOAT_EQ(child1->getLayout().x, root->getLayout().x);
    EXPECT_FLOAT_EQ(child2->getLayout().x, root->getLayout().x + 100.0f);
}

TEST(UILayoutTest, GapBetweenChildren) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Fixed(400.0f);
    root->getStyle().height = UIDimension::Fixed(300.0f);
    root->getStyle().flexDirection = FlexDirection::Column;
    root->getStyle().gap = 10.0f;

    auto c1 = std::make_shared<UIBox>("c1");
    c1->getStyle().width = UIDimension::Fixed(100.0f);
    c1->getStyle().height = UIDimension::Fixed(40.0f);

    auto c2 = std::make_shared<UIBox>("c2");
    c2->getStyle().width = UIDimension::Fixed(100.0f);
    c2->getStyle().height = UIDimension::Fixed(40.0f);

    root->addChild(c1);
    root->addChild(c2);

    layout.computeLayout(root.get(), 800.0f, 600.0f);

    // Second child should be offset by first child height + gap
    float expectedY = c1->getLayout().y + 40.0f + 10.0f;
    EXPECT_FLOAT_EQ(c2->getLayout().y, expectedY);
}

TEST(UILayoutTest, PaddingInContainer) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Fixed(400.0f);
    root->getStyle().height = UIDimension::Fixed(300.0f);
    root->getStyle().padding = UIEdges(20.0f);

    auto child = std::make_shared<UIBox>("child");
    child->getStyle().width = UIDimension::Fixed(100.0f);
    child->getStyle().height = UIDimension::Fixed(50.0f);

    root->addChild(child);
    layout.computeLayout(root.get(), 800.0f, 600.0f);

    // Child should be offset by padding
    EXPECT_FLOAT_EQ(child->getLayout().x, root->getLayout().x + 20.0f);
    EXPECT_FLOAT_EQ(child->getLayout().y, root->getLayout().y + 20.0f);
}

TEST(UILayoutTest, JustifyCenter) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Fixed(400.0f);
    root->getStyle().height = UIDimension::Fixed(300.0f);
    root->getStyle().flexDirection = FlexDirection::Column;
    root->getStyle().justifyContent = JustifyContent::Center;

    auto child = std::make_shared<UIBox>("child");
    child->getStyle().width = UIDimension::Fixed(100.0f);
    child->getStyle().height = UIDimension::Fixed(50.0f);

    root->addChild(child);
    layout.computeLayout(root.get(), 800.0f, 600.0f);

    // Child should be vertically centered
    float expectedY = root->getLayout().y + (300.0f - 50.0f) * 0.5f;
    EXPECT_NEAR(child->getLayout().y, expectedY, 1.0f);
}

TEST(UILayoutTest, AlignItemsCenter) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Fixed(400.0f);
    root->getStyle().height = UIDimension::Fixed(300.0f);
    root->getStyle().flexDirection = FlexDirection::Column;
    root->getStyle().alignItems = AlignItems::Center;

    auto child = std::make_shared<UIBox>("child");
    child->getStyle().width = UIDimension::Fixed(100.0f);
    child->getStyle().height = UIDimension::Fixed(50.0f);

    root->addChild(child);
    layout.computeLayout(root.get(), 800.0f, 600.0f);

    // Child should be horizontally centered
    float expectedX = root->getLayout().x + (400.0f - 100.0f) * 0.5f;
    EXPECT_NEAR(child->getLayout().x, expectedX, 1.0f);
}

TEST(UILayoutTest, AlignItemsStretch) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Fixed(400.0f);
    root->getStyle().height = UIDimension::Fixed(300.0f);
    root->getStyle().flexDirection = FlexDirection::Column;
    root->getStyle().alignItems = AlignItems::Stretch;

    auto child = std::make_shared<UIBox>("child");
    child->getStyle().height = UIDimension::Fixed(50.0f);
    // Width left as auto - should stretch

    root->addChild(child);
    layout.computeLayout(root.get(), 800.0f, 600.0f);

    // Child width should stretch to fill parent
    EXPECT_FLOAT_EQ(child->getLayout().width, 400.0f);
}

TEST(UILayoutTest, GrowDistribution) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Fixed(300.0f);
    root->getStyle().height = UIDimension::Fixed(300.0f);
    root->getStyle().flexDirection = FlexDirection::Column;

    auto fixed = std::make_shared<UIBox>("fixed");
    fixed->getStyle().width = UIDimension::Fixed(100.0f);
    fixed->getStyle().height = UIDimension::Fixed(100.0f);

    auto grow1 = std::make_shared<UIBox>("grow1");
    grow1->getStyle().width = UIDimension::Fixed(100.0f);
    grow1->getStyle().height = UIDimension::Grow(1.0f);

    auto grow2 = std::make_shared<UIBox>("grow2");
    grow2->getStyle().width = UIDimension::Fixed(100.0f);
    grow2->getStyle().height = UIDimension::Grow(1.0f);

    root->addChild(fixed);
    root->addChild(grow1);
    root->addChild(grow2);

    layout.computeLayout(root.get(), 800.0f, 600.0f);

    // Fixed takes 100, remaining 200 split evenly
    EXPECT_FLOAT_EQ(fixed->getLayout().height, 100.0f);
    EXPECT_FLOAT_EQ(grow1->getLayout().height, 100.0f);
    EXPECT_FLOAT_EQ(grow2->getLayout().height, 100.0f);
}

TEST(UILayoutTest, GridLayout) {
    UILayout layout;

    auto grid = std::make_shared<UIGrid>("grid", 3);
    grid->getStyle().width = UIDimension::Fixed(300.0f);
    grid->getStyle().height = UIDimension::Fixed(200.0f);
    grid->setCellSize(90.0f, 90.0f);
    grid->getStyle().gap = 15.0f;

    for (int i = 0; i < 6; ++i) {
        auto cell = std::make_shared<UIBox>("cell" + std::to_string(i));
        grid->addChild(cell);
    }

    layout.computeLayout(grid.get(), 800.0f, 600.0f);

    // First row: cells at x=0, x=105, x=210
    EXPECT_FLOAT_EQ(grid->getChildren()[0]->getLayout().x, grid->getLayout().x);
    EXPECT_FLOAT_EQ(grid->getChildren()[1]->getLayout().x, grid->getLayout().x + 105.0f);
    EXPECT_FLOAT_EQ(grid->getChildren()[2]->getLayout().x, grid->getLayout().x + 210.0f);

    // Second row: y offset by cellHeight + gap
    EXPECT_FLOAT_EQ(grid->getChildren()[3]->getLayout().y, grid->getLayout().y + 105.0f);
}

TEST(UILayoutTest, MinMaxConstraints) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Percent(100.0f);
    root->getStyle().height = UIDimension::Percent(100.0f);
    root->getStyle().maxWidth = 500.0f;
    root->getStyle().minHeight = 200.0f;

    layout.computeLayout(root.get(), 800.0f, 100.0f);

    EXPECT_FLOAT_EQ(root->getLayout().width, 500.0f);   // Capped by maxWidth
    EXPECT_FLOAT_EQ(root->getLayout().height, 200.0f);   // Boosted by minHeight
}

// ============================================================================
// UIInput Tests
// ============================================================================

TEST(UIInputTest, FocusManagement) {
    UIInput uiInput;

    UIButton btn1("btn1", "A");
    UIButton btn2("btn2", "B");

    EXPECT_EQ(uiInput.getFocusedElement(), nullptr);

    uiInput.setFocus(&btn1);
    EXPECT_EQ(uiInput.getFocusedElement(), &btn1);
    EXPECT_TRUE(btn1.isFocused());

    uiInput.setFocus(&btn2);
    EXPECT_EQ(uiInput.getFocusedElement(), &btn2);
    EXPECT_FALSE(btn1.isFocused());
    EXPECT_TRUE(btn2.isFocused());

    uiInput.setFocus(nullptr);
    EXPECT_EQ(uiInput.getFocusedElement(), nullptr);
    EXPECT_FALSE(btn2.isFocused());
}

TEST(UIInputTest, FocusNavigation) {
    UIInput uiInput;

    auto root = std::make_shared<UIBox>("root");
    auto btn1 = std::make_shared<UIButton>("btn1", "A");
    auto btn2 = std::make_shared<UIButton>("btn2", "B");
    auto btn3 = std::make_shared<UIButton>("btn3", "C");

    root->addChild(btn1);
    root->addChild(btn2);
    root->addChild(btn3);

    // Focus next cycles through focusable elements
    uiInput.focusNext(root.get());
    EXPECT_EQ(uiInput.getFocusedElement(), btn1.get());

    uiInput.focusNext(root.get());
    EXPECT_EQ(uiInput.getFocusedElement(), btn2.get());

    uiInput.focusNext(root.get());
    EXPECT_EQ(uiInput.getFocusedElement(), btn3.get());

    // Wraps around
    uiInput.focusNext(root.get());
    EXPECT_EQ(uiInput.getFocusedElement(), btn1.get());
}

TEST(UIInputTest, FocusPrev) {
    UIInput uiInput;

    auto root = std::make_shared<UIBox>("root");
    auto btn1 = std::make_shared<UIButton>("btn1", "A");
    auto btn2 = std::make_shared<UIButton>("btn2", "B");

    root->addChild(btn1);
    root->addChild(btn2);

    // Start at btn2, go prev
    uiInput.setFocus(btn2.get());
    uiInput.focusPrev(root.get());
    EXPECT_EQ(uiInput.getFocusedElement(), btn1.get());

    // Wraps to end
    uiInput.focusPrev(root.get());
    EXPECT_EQ(uiInput.getFocusedElement(), btn2.get());
}

TEST(UIInputTest, SkipsNonFocusable) {
    UIInput uiInput;

    auto root = std::make_shared<UIBox>("root");
    auto box = std::make_shared<UIBox>("box");       // Not focusable
    auto btn = std::make_shared<UIButton>("btn", "A"); // Focusable

    root->addChild(box);
    root->addChild(btn);

    uiInput.focusNext(root.get());
    // Should skip box, land on btn
    EXPECT_EQ(uiInput.getFocusedElement(), btn.get());
}

// ============================================================================
// UISystem Tests (no renderer needed for basic tests)
// ============================================================================

TEST(UISystemTest, ScreenRegistration) {
    UISystem uiSys;
    // Without init, registerScreen should still work for storage

    auto root = std::make_shared<UIBox>("root");
    uiSys.registerScreen("test", root);

    EXPECT_EQ(uiSys.getScreen("test"), root.get());
    EXPECT_FALSE(uiSys.isScreenVisible("test")); // Not visible by default
}

TEST(UISystemTest, ShowHideScreen) {
    UISystem uiSys;

    auto root = std::make_shared<UIBox>("root");
    uiSys.registerScreen("menu", root);

    uiSys.showScreen("menu");
    EXPECT_TRUE(uiSys.isScreenVisible("menu"));

    uiSys.hideScreen("menu");
    EXPECT_FALSE(uiSys.isScreenVisible("menu"));
}

TEST(UISystemTest, RemoveScreen) {
    UISystem uiSys;

    auto root = std::make_shared<UIBox>("root");
    uiSys.registerScreen("temp", root);
    EXPECT_NE(uiSys.getScreen("temp"), nullptr);

    uiSys.removeScreen("temp");
    EXPECT_EQ(uiSys.getScreen("temp"), nullptr);
}

TEST(UISystemTest, FindById) {
    UISystem uiSys;

    auto root = std::make_shared<UIBox>("root");
    auto child = std::make_shared<UIBox>("deep_child");
    root->addChild(child);

    uiSys.registerScreen("test", root);
    uiSys.showScreen("test");

    EXPECT_EQ(uiSys.findById("deep_child"), child.get());
    EXPECT_EQ(uiSys.findById("nonexistent"), nullptr);
}

TEST(UISystemTest, Stats) {
    UISystem uiSys;

    auto root1 = std::make_shared<UIBox>("r1");
    root1->addChild(std::make_shared<UIBox>("c1"));
    root1->addChild(std::make_shared<UIBox>("c2"));

    auto root2 = std::make_shared<UIBox>("r2");

    uiSys.registerScreen("s1", root1);
    uiSys.registerScreen("s2", root2);

    auto stats = uiSys.getStats();
    EXPECT_EQ(stats.screenCount, 2u);
    EXPECT_EQ(stats.visibleScreenCount, 0u);

    uiSys.showScreen("s1");
    stats = uiSys.getStats();
    EXPECT_EQ(stats.visibleScreenCount, 1u);
    EXPECT_EQ(stats.totalElements, 3u); // root + 2 children
}

// ============================================================================
// Complex Layout Integration Tests
// ============================================================================

TEST(UILayoutIntegrationTest, NestedContainers) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Fixed(800.0f);
    root->getStyle().height = UIDimension::Fixed(600.0f);
    root->getStyle().flexDirection = FlexDirection::Column;

    // Header
    auto header = std::make_shared<UIBox>("header");
    header->getStyle().width = UIDimension::Fixed(800.0f);
    header->getStyle().height = UIDimension::Fixed(60.0f);

    // Body (takes remaining space)
    auto body = std::make_shared<UIBox>("body");
    body->getStyle().width = UIDimension::Fixed(800.0f);
    body->getStyle().height = UIDimension::Grow();
    body->getStyle().flexDirection = FlexDirection::Row;

    // Sidebar in body
    auto sidebar = std::make_shared<UIBox>("sidebar");
    sidebar->getStyle().width = UIDimension::Fixed(200.0f);
    sidebar->getStyle().height = UIDimension::Fixed(540.0f);

    // Content in body
    auto content = std::make_shared<UIBox>("content");
    content->getStyle().width = UIDimension::Grow();
    content->getStyle().height = UIDimension::Fixed(540.0f);

    body->addChild(sidebar);
    body->addChild(content);
    root->addChild(header);
    root->addChild(body);

    layout.computeLayout(root.get(), 800.0f, 600.0f);

    // Header at top
    EXPECT_FLOAT_EQ(header->getLayout().y, root->getLayout().y);
    EXPECT_FLOAT_EQ(header->getLayout().height, 60.0f);

    // Body below header
    EXPECT_FLOAT_EQ(body->getLayout().y, root->getLayout().y + 60.0f);
    EXPECT_FLOAT_EQ(body->getLayout().height, 540.0f); // remaining space

    // Sidebar on left
    EXPECT_FLOAT_EQ(sidebar->getLayout().x, body->getLayout().x);
    EXPECT_FLOAT_EQ(sidebar->getLayout().width, 200.0f);
}

TEST(UILayoutIntegrationTest, SpaceBetween) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Fixed(300.0f);
    root->getStyle().height = UIDimension::Fixed(100.0f);
    root->getStyle().flexDirection = FlexDirection::Row;
    root->getStyle().justifyContent = JustifyContent::SpaceBetween;

    auto c1 = std::make_shared<UIBox>("c1");
    c1->getStyle().width = UIDimension::Fixed(50.0f);
    c1->getStyle().height = UIDimension::Fixed(50.0f);

    auto c2 = std::make_shared<UIBox>("c2");
    c2->getStyle().width = UIDimension::Fixed(50.0f);
    c2->getStyle().height = UIDimension::Fixed(50.0f);

    auto c3 = std::make_shared<UIBox>("c3");
    c3->getStyle().width = UIDimension::Fixed(50.0f);
    c3->getStyle().height = UIDimension::Fixed(50.0f);

    root->addChild(c1);
    root->addChild(c2);
    root->addChild(c3);

    layout.computeLayout(root.get(), 800.0f, 600.0f);

    // 3 children of 50px in 300px = 150px used, 150px extra, 75px gap between each
    float rootX = root->getLayout().x;
    EXPECT_FLOAT_EQ(c1->getLayout().x, rootX);
    EXPECT_NEAR(c3->getLayout().x, rootX + 250.0f, 1.0f); // Last child at right edge
}

TEST(UILayoutIntegrationTest, InvisibleChildrenSkipped) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Fixed(300.0f);
    root->getStyle().height = UIDimension::Fixed(200.0f);
    root->getStyle().flexDirection = FlexDirection::Column;

    auto visible = std::make_shared<UIBox>("vis");
    visible->getStyle().width = UIDimension::Fixed(100.0f);
    visible->getStyle().height = UIDimension::Fixed(40.0f);

    auto hidden = std::make_shared<UIBox>("hid");
    hidden->getStyle().width = UIDimension::Fixed(100.0f);
    hidden->getStyle().height = UIDimension::Fixed(40.0f);
    hidden->getStyle().visible = false;

    auto visible2 = std::make_shared<UIBox>("vis2");
    visible2->getStyle().width = UIDimension::Fixed(100.0f);
    visible2->getStyle().height = UIDimension::Fixed(40.0f);

    root->addChild(visible);
    root->addChild(hidden);
    root->addChild(visible2);

    layout.computeLayout(root.get(), 800.0f, 600.0f);

    // visible2 should be directly after visible (hidden one skipped)
    EXPECT_FLOAT_EQ(visible2->getLayout().y, visible->getLayout().y + 40.0f);
}

// ============================================================================
// UIScrollPanel Additional Tests
// ============================================================================

TEST(UIScrollPanelTest, ScrollClamping) {
    auto scroll = std::make_shared<UIScrollPanel>("scroll");
    scroll->getStyle().width = UIDimension::Fixed(200.0f);
    scroll->getStyle().height = UIDimension::Fixed(100.0f);

    // Add children taller than the panel
    for (int i = 0; i < 5; ++i) {
        auto child = std::make_shared<UIBox>("item" + std::to_string(i));
        child->getStyle().width = UIDimension::Fixed(180.0f);
        child->getStyle().height = UIDimension::Fixed(50.0f);
        scroll->addChild(child);
    }

    UILayout layout;
    layout.computeLayout(scroll.get(), 800.0f, 600.0f);

    // Scroll down
    scroll->handleScroll(-5.0f); // Negative delta = scroll down
    EXPECT_GT(scroll->getScrollY(), 0.0f);

    // Scroll should not go negative
    scroll->setScroll(0.0f, 0.0f);
    scroll->handleScroll(5.0f); // Positive delta = scroll up
    EXPECT_FLOAT_EQ(scroll->getScrollY(), 0.0f); // Clamped to 0
}

TEST(UIScrollPanelTest, HandleScrollEvent) {
    auto scroll = std::make_shared<UIScrollPanel>("scroll");
    scroll->getLayoutMut() = {0.0f, 0.0f, 200.0f, 100.0f};

    // Need children taller than the panel for scrolling to work
    auto tall = std::make_shared<UIBox>("tall");
    tall->getLayoutMut() = {0.0f, 0.0f, 180.0f, 300.0f};
    scroll->addChild(tall);

    EXPECT_FLOAT_EQ(scroll->getScrollY(), 0.0f);

    // handleScroll with negative delta scrolls down
    // scroll speed is 30, so delta of -2 results in scrollY += 2 * 30 = 60
    scroll->handleScroll(-2.0f);
    EXPECT_FLOAT_EQ(scroll->getScrollY(), 60.0f);
}

// ============================================================================
// UISlider Arrow Key Tests
// ============================================================================

TEST(UISliderTest, ArrowKeyHandling) {
    UISlider slider("vol");
    slider.setRange(0.0f, 100.0f);
    slider.setValue(50.0f);

    float lastValue = -1.0f;
    slider.setOnChange([&lastValue](float v) { lastValue = v; });

    // Right arrow should increase value
    bool handled = slider.handleKeyPress(static_cast<int>(Key::Right));
    EXPECT_TRUE(handled);
    EXPECT_GT(slider.getValue(), 50.0f);
    EXPECT_FLOAT_EQ(lastValue, slider.getValue());

    // Left arrow should decrease value
    float before = slider.getValue();
    handled = slider.handleKeyPress(static_cast<int>(Key::Left));
    EXPECT_TRUE(handled);
    EXPECT_LT(slider.getValue(), before);

    // Non-arrow key should not be handled
    handled = slider.handleKeyPress(static_cast<int>(Key::Space));
    EXPECT_FALSE(handled);
}

TEST(UISliderTest, ArrowKeyClampingAtBounds) {
    UISlider slider("s");
    slider.setRange(0.0f, 1.0f);

    // At minimum, left arrow shouldn't go below
    slider.setValue(0.0f);
    slider.handleKeyPress(static_cast<int>(Key::Left));
    EXPECT_FLOAT_EQ(slider.getValue(), 0.0f);

    // At maximum, right arrow shouldn't go above
    slider.setValue(1.0f);
    slider.handleKeyPress(static_cast<int>(Key::Right));
    EXPECT_FLOAT_EQ(slider.getValue(), 1.0f);
}

// ============================================================================
// UISystem Additional Tests
// ============================================================================

TEST(UISystemTest, ScreenBlockingAndZOrder) {
    UISystem uiSys;

    auto root1 = std::make_shared<UIBox>("r1");
    auto root2 = std::make_shared<UIBox>("r2");

    uiSys.registerScreen("hud", root1);
    uiSys.registerScreen("menu", root2);

    // Set blocking and z-order
    uiSys.setScreenBlocking("menu", true);
    uiSys.setScreenZOrder("menu", 10);
    uiSys.setScreenZOrder("hud", 0);

    // Blocking screen not visible yet
    EXPECT_FALSE(uiSys.isBlockingInput());

    uiSys.showScreen("menu");
    // Without calling update() (requires Engine), we test the methods exist and don't crash
    EXPECT_TRUE(uiSys.isScreenVisible("menu"));
}

TEST(UISystemTest, DynamicScreenDirtyFlag) {
    UISystem uiSys;

    int buildCount = 0;
    uiSys.registerDynamicScreen("dynamic", [&buildCount]() -> std::shared_ptr<UIElement> {
        buildCount++;
        return std::make_shared<UIBox>("dyn_root");
    });

    // Dynamic screens start dirty, but won't build until visible + update
    EXPECT_EQ(buildCount, 0);

    // Mark dirty should not crash
    uiSys.markScreenDirty("dynamic");
}

TEST(UISystemTest, MultipleScreenFindById) {
    UISystem uiSys;

    auto root1 = std::make_shared<UIBox>("r1");
    auto child1 = std::make_shared<UIBox>("unique_child");
    root1->addChild(child1);

    auto root2 = std::make_shared<UIBox>("r2");

    uiSys.registerScreen("s1", root1);
    uiSys.registerScreen("s2", root2);
    uiSys.showScreen("s1");
    uiSys.showScreen("s2");

    // findById searches across visible screens
    EXPECT_EQ(uiSys.findById("unique_child"), child1.get());
    EXPECT_EQ(uiSys.findById("r2"), root2.get());

    // Hide s1 - should not find its children
    uiSys.hideScreen("s1");
    EXPECT_EQ(uiSys.findById("unique_child"), nullptr);
}

// ============================================================================
// UIElement Hover Tests
// ============================================================================

TEST(UIElementTest, HoverStateFromMouseMove) {
    auto box = std::make_shared<UIBox>("box");
    box->getLayoutMut().x = 10.0f;
    box->getLayoutMut().y = 10.0f;
    box->getLayoutMut().width = 100.0f;
    box->getLayoutMut().height = 50.0f;

    EXPECT_FALSE(box->isHovered());

    // Move mouse inside
    box->handleMouseMove(50.0f, 30.0f);
    EXPECT_TRUE(box->isHovered());

    // Move mouse outside
    box->handleMouseMove(200.0f, 200.0f);
    EXPECT_FALSE(box->isHovered());
}

TEST(UIElementTest, NestedHoverStates) {
    auto parent = std::make_shared<UIBox>("parent");
    parent->getLayoutMut() = {0.0f, 0.0f, 200.0f, 200.0f};

    auto child = std::make_shared<UIBox>("child");
    child->getLayoutMut() = {10.0f, 10.0f, 50.0f, 50.0f};
    parent->addChild(child);

    // Move mouse over child (also inside parent)
    parent->handleMouseMove(25.0f, 25.0f);
    EXPECT_TRUE(parent->isHovered());
    EXPECT_TRUE(child->isHovered());

    // Move mouse outside child but inside parent
    parent->handleMouseMove(100.0f, 100.0f);
    EXPECT_TRUE(parent->isHovered());
    EXPECT_FALSE(child->isHovered());
}

// ============================================================================
// UIButton Hover/Press Visual State Tests
// ============================================================================

TEST(UIButtonTest, HoverAndPressStates) {
    UIButton btn("btn", "Test");
    btn.getLayoutMut() = {0.0f, 0.0f, 100.0f, 30.0f};

    // Initial state
    EXPECT_FALSE(btn.isHovered());
    EXPECT_FALSE(btn.isPressed());

    // Hover
    btn.handleMouseMove(50.0f, 15.0f);
    EXPECT_TRUE(btn.isHovered());
    EXPECT_FALSE(btn.isPressed());

    // Press
    btn.handleMousePress(50.0f, 15.0f);
    EXPECT_TRUE(btn.isPressed());

    // Release
    btn.handleMouseRelease(50.0f, 15.0f);
    EXPECT_FALSE(btn.isPressed());
}

// ============================================================================
// Layout Margin Tests
// ============================================================================

TEST(UILayoutTest, MarginInColumnLayout) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Fixed(200.0f);
    root->getStyle().height = UIDimension::Fixed(300.0f);
    root->getStyle().flexDirection = FlexDirection::Column;

    auto child = std::make_shared<UIBox>("child");
    child->getStyle().width = UIDimension::Fixed(100.0f);
    child->getStyle().height = UIDimension::Fixed(40.0f);
    child->getStyle().margin = UIEdges(10.0f);

    root->addChild(child);
    layout.computeLayout(root.get(), 800.0f, 600.0f);

    // Child width should be 100 (not affected by margin)
    EXPECT_FLOAT_EQ(child->getLayout().width, 100.0f);
    // Child height should be 40 (not affected by margin)
    EXPECT_FLOAT_EQ(child->getLayout().height, 40.0f);
    // Child should be offset by margin
    EXPECT_FLOAT_EQ(child->getLayout().x, root->getLayout().x + 10.0f);
    EXPECT_FLOAT_EQ(child->getLayout().y, root->getLayout().y + 10.0f);
}

TEST(UILayoutTest, MarginInRowLayout) {
    UILayout layout;

    auto root = std::make_shared<UIBox>("root");
    root->getStyle().width = UIDimension::Fixed(300.0f);
    root->getStyle().height = UIDimension::Fixed(100.0f);
    root->getStyle().flexDirection = FlexDirection::Row;

    auto c1 = std::make_shared<UIBox>("c1");
    c1->getStyle().width = UIDimension::Fixed(50.0f);
    c1->getStyle().height = UIDimension::Fixed(40.0f);
    c1->getStyle().margin = UIEdges(5.0f);

    auto c2 = std::make_shared<UIBox>("c2");
    c2->getStyle().width = UIDimension::Fixed(50.0f);
    c2->getStyle().height = UIDimension::Fixed(40.0f);
    c2->getStyle().margin = UIEdges(5.0f);

    root->addChild(c1);
    root->addChild(c2);
    layout.computeLayout(root.get(), 800.0f, 600.0f);

    // c1 width should be exactly 50 (margin not included in element width)
    EXPECT_FLOAT_EQ(c1->getLayout().width, 50.0f);
    // c1 position should include margin
    EXPECT_FLOAT_EQ(c1->getLayout().x, root->getLayout().x + 5.0f);
    // c2 should follow c1 with margins accounted for
    // c2.x = root.x + c1_margin_left + c1_width + c1_margin_right + c2_margin_left
    float expectedC2X = root->getLayout().x + 5.0f + 50.0f + 5.0f + 5.0f;
    EXPECT_FLOAT_EQ(c2->getLayout().x, expectedC2X);
}
