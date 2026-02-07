#pragma once

#include "ui/UIElement.hpp"
#include "ui/UILayout.hpp"
#include "ui/UIInput.hpp"
#include "ui/UIWidgets.hpp"

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <vector>

namespace gloaming {

// Forward declarations
class IRenderer;
class Input;
class Engine;

/// Callback for Lua-driven UI builders.
/// Called each frame to rebuild the UI tree from Lua state.
using UIBuilderCallback = std::function<std::shared_ptr<UIElement>()>;

/// Configuration for the UI system
struct UISystemConfig {
    bool enabled = true;
};

/// Main UI system coordinator.
/// Manages named UI screens/layers, performs layout, handles input, and renders.
///
/// Usage flow:
/// 1. Register UI screens (e.g., "hud", "main_menu", "inventory")
/// 2. Show/hide screens as needed
/// 3. Each frame: update() processes input and rebuilds Lua-driven UIs, render() draws
class UISystem {
public:
    UISystem() = default;

    /// Initialize the UI system
    bool init(Engine& engine);

    /// Shutdown
    void shutdown();

    /// Per-frame update: process input, rebuild dynamic UIs, compute layout
    void update(float dt);

    /// Render all visible UI screens
    void render();

    // --- Screen management ---

    /// Register a static UI tree (built once, updated manually)
    void registerScreen(const std::string& name, std::shared_ptr<UIElement> root);

    /// Register a dynamic UI screen built by a callback (called every frame when visible)
    void registerDynamicScreen(const std::string& name, UIBuilderCallback builder);

    /// Remove a screen
    void removeScreen(const std::string& name);

    /// Show a screen (makes it visible and interactive)
    void showScreen(const std::string& name);

    /// Hide a screen
    void hideScreen(const std::string& name);

    /// Check if a screen is visible
    bool isScreenVisible(const std::string& name) const;

    /// Get a screen's root element (for modification)
    UIElement* getScreen(const std::string& name);

    /// Find an element by ID across all visible screens
    UIElement* findById(const std::string& id);

    // --- Input state ---

    /// Check if the UI consumed input this frame (game should ignore its input)
    bool didConsumeInput() const { return m_uiInput.didConsumeInput(); }

    /// Check if any blocking screen is visible (e.g., menu, inventory)
    bool isBlockingInput() const { return m_blockingScreenVisible; }

    // --- Configuration ---
    const UISystemConfig& getConfig() const { return m_config; }
    void setEnabled(bool enabled) { m_config.enabled = enabled; }

    // --- Stats ---
    struct Stats {
        size_t screenCount = 0;
        size_t visibleScreenCount = 0;
        size_t totalElements = 0;
    };
    Stats getStats() const;

private:
    struct ScreenEntry {
        std::string name;
        std::shared_ptr<UIElement> root;
        UIBuilderCallback builder; // If set, root is rebuilt each frame
        bool visible = false;
        bool blocking = false;     // If true, blocks game input when visible
        int zOrder = 0;            // Higher = rendered on top
    };

    /// Count elements in a subtree
    size_t countElements(const UIElement* element) const;

    /// Sort screens by z-order for rendering
    std::vector<ScreenEntry*> getSortedVisibleScreens();

    std::unordered_map<std::string, ScreenEntry> m_screens;
    UILayout m_layout;
    UIInput m_uiInput;
    UISystemConfig m_config;

    Engine* m_engine = nullptr;
    IRenderer* m_renderer = nullptr;
    bool m_blockingScreenVisible = false;
};

} // namespace gloaming
