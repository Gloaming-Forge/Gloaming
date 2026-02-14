#include "engine/SteamIntegration.hpp"
#include "engine/Log.hpp"

#include <cstdlib>

#ifdef GLOAMING_STEAM
#include <steam/steam_api.h>
#endif

namespace gloaming {

bool SteamIntegration::init(uint32_t appId) {
    m_appId = appId;

#ifdef GLOAMING_STEAM
    // Set the app ID for development builds that don't have steam_appid.txt
    if (appId != 0) {
        // SteamAPI_RestartAppIfNecessary would normally go here for release
        // builds, but during development we skip it.
    }

    if (!SteamAPI_Init()) {
        LOG_WARN("SteamAPI_Init() failed — Steam is not running or the app ID is invalid");
        m_initialized = false;
        return false;
    }

    m_initialized = true;
    LOG_INFO("Steamworks SDK initialized (appId={})", appId);

    // Log whether we're on Steam Deck
    if (SteamUtils() && SteamUtils()->IsSteamRunningOnSteamDeck()) {
        LOG_INFO("Running on Steam Deck (detected via SteamUtils)");
    }

    return true;
#else
    LOG_INFO("Steamworks SDK not compiled in (GLOAMING_STEAM not defined) — Steam features disabled");
    m_initialized = false;
    return false;
#endif
}

void SteamIntegration::shutdown() {
#ifdef GLOAMING_STEAM
    if (m_initialized) {
        SteamAPI_Shutdown();
        LOG_INFO("Steamworks SDK shut down");
    }
#endif
    m_initialized = false;
}

void SteamIntegration::update() {
#ifdef GLOAMING_STEAM
    if (m_initialized) {
        SteamAPI_RunCallbacks();
    }
#endif

    // Clear one-shot keyboard result from previous frame
    m_hasKeyboardResult = false;
}

bool SteamIntegration::isAvailable() const {
    return m_initialized;
}

void SteamIntegration::showOnScreenKeyboard(const std::string& description,
                                            const std::string& existingText,
                                            int maxChars) {
#ifdef GLOAMING_STEAM
    if (!m_initialized) return;

    // Use the floating gamepad text input — it overlays on top of the game
    // without taking over the full screen.
    if (SteamUtils()) {
        bool shown = SteamUtils()->ShowFloatingGamepadTextInput(
            k_EFloatingGamepadTextInputModeModeSingleLine,
            0, 0, 0, 0  // Position and size (0 = system default)
        );
        if (!shown) {
            LOG_WARN("Steam floating keyboard could not be shown");
        }
    }
#else
    (void)description;
    (void)existingText;
    (void)maxChars;
#endif
}

bool SteamIntegration::hasKeyboardResult() const {
    return m_hasKeyboardResult;
}

std::string SteamIntegration::getKeyboardResult() const {
    return m_keyboardResult;
}

std::string SteamIntegration::getGlyphPath(int actionOrigin) const {
#ifdef GLOAMING_STEAM
    if (!m_initialized) return "";

    if (SteamInput()) {
        const char* path = SteamInput()->GetGlyphPNGForActionOrigin(
            static_cast<EInputActionOrigin>(actionOrigin),
            k_ESteamInputGlyphSize_Small, 0);
        return path ? std::string(path) : "";
    }
#else
    (void)actionOrigin;
#endif
    return "";
}

bool SteamIntegration::isOverlayActive() const {
#ifdef GLOAMING_STEAM
    if (m_initialized && SteamUtils()) {
        return SteamUtils()->IsOverlayEnabled() && SteamUtils()->BOverlayNeedsPresent();
    }
#endif
    return false;
}

bool SteamIntegration::isSteamDeck() {
    const char* val = std::getenv("SteamDeck");
    return val != nullptr && std::string(val) == "1";
}

bool SteamIntegration::isSteamOS() {
    const char* val = std::getenv("SteamOS");
    return val != nullptr;
}

} // namespace gloaming
