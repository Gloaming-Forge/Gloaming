#pragma once

#include "engine/Input.hpp"
#include "engine/Gamepad.hpp"
#include "engine/InputDeviceTracker.hpp"
#include "rendering/IRenderer.hpp"

#include <string>
#include <unordered_map>

namespace gloaming {

// Forward declaration
class InputActionMap;

/// Visual style for button glyph text/icons.
enum class GlyphStyle {
    Xbox,           // ABXY colored buttons (default for Deck)
    PlayStation,    // Cross/Circle/Square/Triangle
    Nintendo,       // ABXY but swapped layout
    Keyboard,       // Key names ("Space", "E", "Esc")
    SteamDeck,      // Deck-specific with trackpad/grip icons
};

/// Provides human-readable names and atlas regions for button glyphs.
/// Used to show correct prompts (e.g., "Press A" vs "Press Space") based
/// on the active input device and glyph style.
class InputGlyphProvider {
public:
    /// Get the display name for a gamepad button (e.g., "A", "LB", "Start")
    std::string getButtonName(GamepadButton button, GlyphStyle style) const;

    /// Get the display name for a keyboard key (e.g., "Space", "E", "Esc")
    std::string getKeyName(Key key) const;

    /// Get the display name for an action based on the active input device.
    /// Returns the name of the first binding that matches the active device.
    std::string getActionGlyph(const std::string& actionName,
                               const InputActionMap& actions,
                               InputDevice activeDevice,
                               GlyphStyle style = GlyphStyle::Xbox) const;

    /// Get texture region for a button glyph icon (for rendering button prompts).
    /// Returns a 32x32 region in the atlas. Requires a glyph atlas to be loaded.
    Rect getGlyphRegion(GamepadButton button, GlyphStyle style) const;
    Rect getGlyphRegion(Key key) const;

    /// Load a glyph atlas texture
    void loadGlyphAtlas(const std::string& path, GlyphStyle style);

    /// Get/set the current glyph style
    void setGlyphStyle(GlyphStyle style);
    GlyphStyle getGlyphStyle() const;

private:
    GlyphStyle m_currentStyle = GlyphStyle::Xbox;
    std::unordered_map<int, std::string> m_glyphAtlasPaths;
};

} // namespace gloaming
