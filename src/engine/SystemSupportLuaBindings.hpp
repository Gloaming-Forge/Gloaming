#pragma once

#include <sol/sol.hpp>

namespace gloaming {

class Engine;

/// Registers all Stage 19D Lua APIs: System Support (Steam integration,
/// platform detection).
///
/// Provides:
///   steam.is_available()          — check if Steamworks SDK is active
///   steam.is_overlay_active()     — check if Steam overlay is open
///   steam.show_keyboard(desc, existing, max) — show Steam keyboard
///   steam.has_keyboard_result()   — check for keyboard submission
///   steam.get_keyboard_result()   — get submitted text
///
///   platform.is_steam_deck()      — running on Steam Deck?
///   platform.is_steam_os()        — running on SteamOS?
///   platform.is_linux()           — running on Linux?
///   platform.is_windows()         — running on Windows?
void bindSystemSupportAPI(sol::state& lua, Engine& engine);

} // namespace gloaming
