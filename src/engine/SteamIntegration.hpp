#pragma once

#include <string>
#include <cstdint>
#include <functional>

namespace gloaming {

/// Optional Steamworks SDK integration for Steam Deck Verified features.
///
/// All Steam-specific functionality is behind the GLOAMING_STEAM compile flag.
/// When GLOAMING_STEAM is not defined, every method is a safe no-op or returns
/// a sensible default — the engine works identically without the SDK.
///
/// Features provided when Steam is available:
///   - Steam overlay keyboard for text input
///   - Input glyph lookup via SteamInput
///   - Steam overlay detection (for auto-pausing)
///   - Per-frame Steam callback processing
class SteamIntegration {
public:
    /// Initialize the Steam API. Returns false if Steam is not running
    /// or if GLOAMING_STEAM is not defined.
    bool init(uint32_t appId);

    /// Shutdown the Steam API and release resources.
    void shutdown();

    /// Per-frame callback processing.  Must be called once per frame
    /// so Steam callbacks (overlay, keyboard, etc.) are dispatched.
    void update();

    /// Check if the Steam API was successfully initialized.
    bool isAvailable() const;

    // --- On-screen keyboard ---

    /// Show the Steam overlay floating keyboard for text input.
    /// No-op when Steam is unavailable; caller should fall back to
    /// the built-in OnScreenKeyboard.
    void showOnScreenKeyboard(const std::string& description,
                              const std::string& existingText,
                              int maxChars);

    /// Check whether the Steam keyboard has submitted text this frame.
    bool hasKeyboardResult() const;

    /// Retrieve the text submitted by the Steam keyboard.
    /// Only valid when hasKeyboardResult() returns true.
    std::string getKeyboardResult() const;

    // --- Input glyphs ---

    /// Get the filesystem path to the glyph image for a SteamInput action
    /// origin.  Returns an empty string when Steam is unavailable.
    std::string getGlyphPath(int actionOrigin) const;

    // --- Overlay ---

    /// Check if the Steam overlay is currently active.
    /// Games typically pause when the overlay is shown.
    bool isOverlayActive() const;

    // --- Platform detection ---

    /// Check if we are running on a Steam Deck.
    static bool isSteamDeck();

    /// Check if we are running on SteamOS.
    static bool isSteamOS();

private:
    bool m_initialized = false;
    uint32_t m_appId = 0;

    // Keyboard result state
    bool m_hasKeyboardResult = false;
    std::string m_keyboardResult;
};

} // namespace gloaming
