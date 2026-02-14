#include "engine/SystemSupportLuaBindings.hpp"
#include "engine/Engine.hpp"
#include "engine/Log.hpp"

namespace gloaming {

void bindSystemSupportAPI(sol::state& lua, Engine& engine) {
    // -----------------------------------------------------------------------
    // steam.* — Steamworks SDK wrapper
    // -----------------------------------------------------------------------
    sol::table steamApi = lua.create_named_table("steam");

    // steam.is_available() -> bool
    steamApi["is_available"] = [&engine]() -> bool {
        return engine.getSteamIntegration().isAvailable();
    };

    // steam.is_overlay_active() -> bool
    steamApi["is_overlay_active"] = [&engine]() -> bool {
        return engine.getSteamIntegration().isOverlayActive();
    };

    // steam.show_keyboard(description, existing_text, max_chars)
    steamApi["show_keyboard"] = [&engine](const std::string& description,
                                          const std::string& existingText,
                                          int maxChars) {
        engine.getSteamIntegration().showOnScreenKeyboard(
            description, existingText, maxChars);
    };

    // steam.has_keyboard_result() -> bool
    steamApi["has_keyboard_result"] = [&engine]() -> bool {
        return engine.getSteamIntegration().hasKeyboardResult();
    };

    // steam.get_keyboard_result() -> string
    steamApi["get_keyboard_result"] = [&engine]() -> std::string {
        return engine.getSteamIntegration().getKeyboardResult();
    };

    // -----------------------------------------------------------------------
    // platform.* — Platform detection
    // -----------------------------------------------------------------------
    sol::table platformApi = lua.create_named_table("platform");

    // platform.is_steam_deck() -> bool
    platformApi["is_steam_deck"] = []() -> bool {
        return SteamIntegration::isSteamDeck();
    };

    // platform.is_steam_os() -> bool
    platformApi["is_steam_os"] = []() -> bool {
        return SteamIntegration::isSteamOS();
    };

    // platform.is_linux() -> bool
    platformApi["is_linux"] = []() -> bool {
#ifdef __linux__
        return true;
#else
        return false;
#endif
    };

    // platform.is_windows() -> bool
    platformApi["is_windows"] = []() -> bool {
#ifdef _WIN32
        return true;
#else
        return false;
#endif
    };
}

} // namespace gloaming
