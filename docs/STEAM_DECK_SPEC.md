# Steam Deck Readiness Specification

**Engine:** Gloaming v0.5.0 → v0.6.0
**Target:** Steam Deck Verified badge
**Stage:** 19 — Steam Deck Support
**Implementation Status:** Complete — all systems implemented and tested

---

## 1. Overview

This document specifies every engine component that must be added or modified to achieve Steam Deck Verified status. The Steam Deck Verified program evaluates games across four categories — **Input**, **Display**, **Seamlessness**, and **System Support** — all of which must pass for the badge. This spec maps each Valve requirement to concrete engine work.

### 1.1 Steam Deck Hardware Reference

| Spec | Value |
|------|-------|
| **Display (LCD)** | 7" 1280×800 @ 60 Hz |
| **Display (OLED)** | 7.4" 1280×800 @ 90 Hz |
| **GPU** | AMD RDNA 2, 8 CUs, 1.6 TFLOPS |
| **CPU** | AMD Zen 2, 4c/8t, 2.4–3.5 GHz |
| **RAM** | 16 GB LPDDR5 |
| **OS** | SteamOS 3.x (Arch Linux, KDE Plasma) |
| **Input** | 2 thumbsticks, D-pad, ABXY, 2 bumpers, 2 triggers, 4 back grip buttons, 2 trackpads, gyroscope, touchscreen |
| **Audio** | Stereo speakers, 3.5mm jack, Bluetooth |
| **Docked output** | Up to 4K@60Hz / 1080p@120Hz via USB-C DisplayPort 1.4 |

### 1.2 Engine Gaps Summary (All Resolved)

| Verified Category | Status | Resolution |
|-------------------|--------|------------|
| **Input** | Done | Gamepad class, InputDeviceTracker, InputGlyphs, OnScreenKeyboard, Haptics, unified InputActionMap with gamepad bindings. |
| **Display** | Done | ViewportScaler with multiple scale modes, UIScaling with minimum font size enforcement, 1280×800 support. |
| **Seamlessness** | Done | Suspend/resume handling via focus detection, SIGTERM graceful exit, SeamlessnessLuaBindings. |
| **System Support** | Done | Optional SteamIntegration, Linux CI build, native Raylib/OpenGL on SteamOS. |

---

## 2. Input System (Stage 19A)

The largest body of work. The current `Input` class only handles keyboard and mouse. The `InputActionMap` binds actions exclusively to `Key` enums. Steam Deck requires full gamepad support with correct glyphs.

### 2.1 Gamepad Input Layer

**Files to create:**
- `src/engine/Gamepad.hpp` — Gamepad abstraction
- `src/engine/Gamepad.cpp` — Raylib gamepad backend

**Gamepad enum and state:**

```cpp
enum class GamepadButton : int {
    // Face buttons (Xbox layout — matches Deck)
    FaceDown    = 0,   // A / Cross
    FaceRight   = 1,   // B / Circle
    FaceLeft    = 2,   // X / Square
    FaceUp      = 3,   // Y / Triangle

    // Shoulder buttons
    LeftBumper  = 4,
    RightBumper = 5,

    // Center buttons
    Select      = 6,   // Back / View
    Start       = 7,   // Start / Menu
    Guide       = 8,   // Steam button

    // Stick clicks
    LeftThumb   = 9,
    RightThumb  = 10,

    // D-pad
    DpadUp      = 11,
    DpadDown    = 12,
    DpadLeft    = 13,
    DpadRight   = 14,
};

enum class GamepadAxis : int {
    LeftX       = 0,
    LeftY       = 1,
    RightX      = 2,
    RightY      = 3,
    LeftTrigger = 4,
    RightTrigger = 5,
};
```

**Gamepad class interface:**

```cpp
class Gamepad {
public:
    void update();

    // Connection state
    bool isConnected(int gamepadId = 0) const;
    int  getConnectedCount() const;

    // Button queries
    bool isButtonPressed(GamepadButton button, int gamepadId = 0) const;
    bool isButtonDown(GamepadButton button, int gamepadId = 0) const;
    bool isButtonReleased(GamepadButton button, int gamepadId = 0) const;

    // Axis queries (returns -1.0 to 1.0 for sticks, 0.0 to 1.0 for triggers)
    float getAxis(GamepadAxis axis, int gamepadId = 0) const;

    // Convenience — applies deadzone and returns normalized direction
    Vec2  getLeftStick(int gamepadId = 0) const;
    Vec2  getRightStick(int gamepadId = 0) const;
    float getLeftTrigger(int gamepadId = 0) const;
    float getRightTrigger(int gamepadId = 0) const;

    // Configuration
    void setDeadzone(float deadzone);      // Default: 0.15
    float getDeadzone() const;

private:
    float m_deadzone = 0.15f;
    static constexpr int MAX_GAMEPADS = 4;
};
```

**Implementation notes:**
- Wraps Raylib's `IsGamepadButtonPressed()`, `IsGamepadButtonDown()`, `IsGamepadButtonReleased()`, `GetGamepadAxisMovement()`, `IsGamepadAvailable()`, `GetGamepadAxisCount()`.
- Deadzone is applied as a radial deadzone on stick axes (not per-axis) to prevent drift.
- Triggers use a linear deadzone.

### 2.2 Unified Input Binding System

**File to modify:** `src/gameplay/InputActions.hpp`

The `InputBinding` struct and `InputActionMap` must support gamepad buttons and axes alongside keyboard keys.

**New InputBinding:**

```cpp
enum class InputSourceType {
    Key,            // Keyboard key
    GamepadButton,  // Gamepad digital button
    GamepadAxis,    // Gamepad analog axis (treated as digital with threshold)
};

struct InputBinding {
    InputSourceType sourceType = InputSourceType::Key;

    // Keyboard binding
    Key key = Key::Space;
    bool requireShift = false;
    bool requireCtrl = false;
    bool requireAlt = false;

    // Gamepad binding
    GamepadButton gamepadButton = GamepadButton::FaceDown;
    GamepadAxis   gamepadAxis = GamepadAxis::LeftX;
    float         axisThreshold = 0.5f;  // Axis value at which it counts as "pressed"
    bool          axisPositive = true;   // true = positive direction, false = negative
};
```

**InputActionMap changes:**
- `registerAction()` accepts mixed bindings (keyboard + gamepad in the same action).
- `isActionPressed/Down/Released()` checks both `Input` (keyboard/mouse) and `Gamepad` state.
- New method `getActiveInputDevice()` returns which device last produced input (for glyph switching).
- Preset methods updated:

```cpp
void registerPlatformerDefaults() {
    // Keyboard bindings (existing)
    registerAction("move_left",  Key::A);
    addBinding("move_left", Key::Left);
    // Gamepad bindings (new)
    addGamepadBinding("move_left", GamepadAxis::LeftX, -0.5f);  // Left stick left
    addGamepadBinding("move_left", GamepadButton::DpadLeft);

    registerAction("jump", Key::Space);
    addGamepadBinding("jump", GamepadButton::FaceDown);  // A button

    registerAction("attack", Key::Z);
    addGamepadBinding("attack", GamepadButton::FaceRight);  // B button

    registerAction("interact", Key::E);
    addGamepadBinding("interact", GamepadButton::FaceUp);  // Y button

    registerAction("menu", Key::Escape);
    addGamepadBinding("menu", GamepadButton::Start);

    registerAction("inventory", Key::Tab);
    addGamepadBinding("inventory", GamepadButton::Select);
    // ... etc.
}
```

### 2.3 Analog Input Actions

Some gameplay needs analog values, not just digital pressed/down/released (e.g., variable-speed movement, camera aiming).

**New methods on InputActionMap:**

```cpp
/// Get analog value for an action (0.0–1.0 for digital, raw axis for analog).
/// Falls back to 1.0 if a keyboard key is held, or axis value if gamepad.
float getActionValue(const std::string& name, const Input& input,
                     const Gamepad& gamepad) const;

/// Get 2D vector for a movement pair (e.g., "move_left"/"move_right"/"move_up"/"move_down").
/// Returns normalized direction with analog magnitude from sticks.
Vec2 getMovementVector(const std::string& leftAction, const std::string& rightAction,
                       const std::string& upAction, const std::string& downAction,
                       const Input& input, const Gamepad& gamepad) const;
```

### 2.4 Active Input Device Detection

Required for glyph switching (Deck Verified requirement: show controller glyphs when using controller, keyboard glyphs when using keyboard).

```cpp
enum class InputDevice {
    KeyboardMouse,
    Gamepad
};

class InputDeviceTracker {
public:
    void update(const Input& input, const Gamepad& gamepad);

    /// Which device was most recently used?
    InputDevice getActiveDevice() const;

    /// Did the active device change this frame?
    bool didDeviceChange() const;

private:
    InputDevice m_activeDevice = InputDevice::KeyboardMouse;
    bool m_changed = false;
};
```

**Logic:** Any keyboard/mouse event switches to `KeyboardMouse`. Any gamepad button or significant axis movement switches to `Gamepad`. Hysteresis prevents flickering (require sustained input on new device before switching).

### 2.5 Input Glyph System

Deck Verified requires on-screen glyphs to match the active controller. When the player is using the Deck's physical controls, the game must show Xbox-style (ABXY) or Deck-specific button icons — never keyboard glyphs.

**File to create:** `src/engine/InputGlyphs.hpp`

```cpp
enum class GlyphStyle {
    Xbox,           // ABXY colored buttons (default for Deck)
    PlayStation,    // Cross/Circle/Square/Triangle
    Nintendo,       // ABXY but swapped layout
    Keyboard,       // Key names ("Space", "E", "Esc")
    SteamDeck,      // Deck-specific with trackpad/grip icons
};

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
    /// Requires a glyph atlas texture to be loaded.
    Rect getGlyphRegion(GamepadButton button, GlyphStyle style) const;
    Rect getGlyphRegion(Key key) const;

    /// Load a glyph atlas texture
    void loadGlyphAtlas(const std::string& path, GlyphStyle style);
};
```

**Lua API:**

```lua
-- Get glyph for an action based on current input device
local glyph = input.get_glyph("jump")  -- Returns "A" or "Space" depending on device

-- Check active device
if input.active_device() == "gamepad" then
    -- Show controller-specific UI
end
```

### 2.6 UI Navigation with Gamepad

The current `UIInput` system uses mouse position and keyboard Tab for focus navigation. For Steam Deck, the UI must be fully navigable with the gamepad.

**File to modify:** `src/ui/UIInput.hpp` and `src/ui/UIInput.cpp`

**Additions:**
- D-pad / left stick navigation: Up/Down moves focus vertically, Left/Right horizontally.
- A button = confirm/click the focused element.
- B button = back/cancel (close current screen, navigate back).
- Bumpers (LB/RB) = tab switching between panels/categories.
- Automatic focus assignment: when a screen opens with no mouse movement, auto-focus the first focusable element.
- Focus wrapping: focus wraps from last to first element (and vice versa).
- Spatial navigation: for grid layouts, navigate based on spatial position rather than DOM order.

**UIInput extended interface:**

```cpp
class UIInput {
public:
    // ... existing methods ...

    /// Update with gamepad support
    bool update(UIElement* root, const Input& input, const Gamepad& gamepad,
                InputDevice activeDevice);

    /// Enable/disable gamepad navigation
    void setGamepadNavigationEnabled(bool enabled);

    /// Set the spatial navigation mode
    void setSpatialNavigation(bool enabled);

    /// Navigate focus in a direction (for D-pad/stick)
    void navigateFocus(UIElement* root, int dx, int dy);

    /// Confirm the focused element (A button)
    void confirmFocus();

    /// Cancel / go back (B button)
    void cancelFocus();

private:
    bool m_gamepadNavEnabled = true;
    bool m_spatialNav = false;
    float m_navRepeatDelay = 0.4f;  // Initial delay before auto-repeat
    float m_navRepeatRate = 0.1f;   // Rate of auto-repeat
    float m_navTimer = 0.0f;
};
```

### 2.7 On-Screen Keyboard

Deck Verified requires that when text input is needed, an on-screen keyboard appears automatically. Valve provides two Steamworks API functions for this:
- `ISteamUtils::ShowFloatingGamepadTextInput()` — overlay keyboard
- `ISteamUtils::ShowGamepadTextInput()` — modal keyboard with callback

**Approach:**
- When Steamworks SDK is available, use the Steam overlay keyboard via `ShowFloatingGamepadTextInput()`.
- When Steamworks SDK is not available (development/non-Steam builds), provide a built-in fallback on-screen keyboard navigable by gamepad.

**File to create:** `src/ui/OnScreenKeyboard.hpp`

```cpp
class OnScreenKeyboard {
public:
    /// Request text input. Shows Steam overlay keyboard if available,
    /// otherwise shows built-in keyboard UI.
    /// callback is invoked with the result string when the user confirms.
    void requestTextInput(const std::string& description,
                          const std::string& existingText,
                          int maxChars,
                          std::function<void(const std::string&)> callback);

    /// Check if the keyboard is currently visible
    bool isVisible() const;

    /// Update (for built-in keyboard input processing)
    void update(const Input& input, const Gamepad& gamepad);

    /// Render the built-in keyboard (no-op when using Steam overlay)
    void render(IRenderer* renderer);

    /// Dismiss without confirming
    void dismiss();
};
```

**Lua API:**

```lua
-- Request text input from the player
ui.request_text_input("Enter world name:", "", 32, function(text)
    if text then
        world.set_name(text)
    end
end)
```

### 2.8 Rumble / Haptic Feedback

The Steam Deck has HD haptics in both trackpads and standard haptic motors. Raylib does not expose haptics directly, but SteamInput API provides `TriggerHapticPulse()` and `TriggerVibration()`.

**File to create:** `src/engine/Haptics.hpp`

```cpp
class Haptics {
public:
    /// Vibrate the gamepad (intensity 0.0–1.0, duration in seconds)
    void vibrate(float leftIntensity, float rightIntensity, float duration,
                 int gamepadId = 0);

    /// Short impulse vibration (e.g., landing, hitting)
    void impulse(float intensity, float durationMs = 100.0f, int gamepadId = 0);

    /// Stop vibration
    void stop(int gamepadId = 0);

    /// Update (ticks down active vibrations)
    void update(float dt);

    /// Enable/disable globally (user preference)
    void setEnabled(bool enabled);
    bool isEnabled() const;
};
```

**Lua API:**

```lua
haptics.vibrate(0.5, 0.5, 0.2)   -- Both motors, 50%, 200ms
haptics.impulse(0.8, 50)          -- Quick impact, 80%, 50ms
```

### 2.9 Lua Bindings for Input

**File to modify:** `src/gameplay/GameplayLuaBindings.cpp`

New Lua APIs to expose:

```lua
-- Gamepad state
input.is_gamepad_connected(id?)         -- bool
input.gamepad_button_pressed(button)    -- bool
input.gamepad_button_down(button)       -- bool
input.gamepad_axis(axis)                -- float
input.left_stick()                      -- {x, y}
input.right_stick()                     -- {x, y}
input.left_trigger()                    -- float
input.right_trigger()                   -- float

-- Active device
input.active_device()                   -- "keyboard" | "gamepad"
input.on_device_change(callback)        -- fires when device switches

-- Analog action values
input.action_value(name)                -- float (0.0–1.0)
input.movement_vector()                 -- {x, y} normalized with analog magnitude

-- Glyphs
input.get_glyph(action_name)            -- string glyph name for current device
input.get_glyph_style()                 -- current glyph style name
input.set_glyph_style(style)            -- "xbox", "playstation", "keyboard", "deck"

-- Gamepad binding from Lua
input.add_gamepad_binding(action, button_or_axis, ...)

-- Haptics
haptics.vibrate(left, right, duration)
haptics.impulse(intensity, duration_ms?)
haptics.stop()
```

---

## 3. Display System (Stage 19B)

### 3.1 Native Resolution Support

The Steam Deck display is 1280×800. The engine currently defaults to 1280×720.

**File to modify:** `src/engine/Window.hpp`, `src/engine/Window.cpp`

**Changes:**
- Add `1280x800` as a recognized resolution.
- On SteamOS / Linux, auto-detect the native resolution and use it as the default.
- `config.json` already supports `window.width` / `window.height`, so mods can override.
- The engine must handle any resolution, not just 16:9 or 16:10.

### 3.2 Aspect Ratio Handling

Different resolutions (16:9 at 1280×720, 16:10 at 1280×800, arbitrary when docked) require proper handling so the game doesn't stretch or cut off content.

**File to create:** `src/rendering/ViewportScaler.hpp`

```cpp
enum class ScaleMode {
    /// Scales to fill the window; content may be cropped on one axis.
    FillCrop,

    /// Scales to fit within the window; may have letterbox/pillarbox bars.
    FitLetterbox,

    /// Stretches to fill exactly (distorts aspect ratio — not recommended).
    Stretch,

    /// Expand the game world to fill the extra space (show more of the world).
    Expand,
};

struct ViewportConfig {
    int designWidth = 1280;      // The resolution the game is designed for
    int designHeight = 720;
    ScaleMode scaleMode = ScaleMode::Expand;
    Color letterboxColor = Color::Black();
};

class ViewportScaler {
public:
    void configure(const ViewportConfig& config);

    /// Call each frame with the actual window size.
    /// Computes the viewport rectangle and scale factor.
    void update(int windowWidth, int windowHeight);

    /// Get the computed viewport (where to render within the window)
    Rect getViewport() const;

    /// Get the effective game resolution (may differ from design if Expand mode)
    int getEffectiveWidth() const;
    int getEffectiveHeight() const;

    /// Get the scale factor from design resolution to screen pixels
    float getScale() const;

    /// Convert screen coordinates to game coordinates (for input)
    Vec2 screenToGame(Vec2 screenPos) const;

    /// Convert game coordinates to screen coordinates
    Vec2 gameToScreen(Vec2 gamePos) const;

    /// Render letterbox/pillarbox bars (call after game rendering)
    void renderBars(IRenderer* renderer) const;
};
```

**Integration:**
- The `Camera` class must use the `ViewportScaler`'s effective resolution for its screen size, not the raw window size.
- Mouse/touch input coordinates must be transformed through `screenToGame()` before use.
- The `ScaleMode::Expand` mode is recommended for platformers — it shows slightly more of the world vertically on 16:10 screens, which is a natural fit.

### 3.3 UI Scaling and Minimum Font Size

Deck Verified requires that the smallest text is at least 9px at 1280×800. Valve recommends 12px.

**File to create:** `src/ui/UIScaling.hpp`

```cpp
struct UIScalingConfig {
    float baseScale = 1.0f;       // Global UI scale multiplier
    int   minFontSize = 12;       // Floor for any rendered font size (pixels)
    float dpiScale = 1.0f;        // Auto-detected from display DPI (future)
};

class UIScaling {
public:
    void configure(const UIScalingConfig& config);

    /// Apply scale to a font size (enforces minimum)
    int scaleFontSize(int designSize) const;

    /// Apply scale to a dimension (padding, margin, widget size)
    float scaleDimension(float designValue) const;

    /// Apply scale to a position (for layout offsets)
    Vec2 scalePosition(Vec2 designPos) const;

    /// Get the effective scale factor
    float getScale() const;

    /// Auto-detect appropriate scale from screen size
    void autoDetect(int screenWidth, int screenHeight);
};
```

**Integration:**
- The `UILayout` engine applies `UIScaling` to all dimensions during layout computation.
- The `IRenderer::drawText()` calls are routed through `UIScaling::scaleFontSize()`.
- Mods can override the scale via config or Lua:

```lua
ui.set_scale(1.2)        -- 20% larger UI
ui.set_min_font_size(14) -- Enforce larger minimum
```

### 3.4 Fullscreen and Exclusive Mode

**File to modify:** `src/engine/Window.hpp`, `src/engine/Window.cpp`

**Additions:**

```cpp
enum class FullscreenMode {
    Windowed,
    Fullscreen,           // Exclusive fullscreen (changes display mode)
    BorderlessFullscreen,  // Borderless window at desktop resolution (preferred for Deck)
};

// New methods on Window:
void setFullscreenMode(FullscreenMode mode);
FullscreenMode getFullscreenMode() const;
```

**Notes:**
- Steam Deck games run in borderless fullscreen by default under Gamescope (SteamOS compositor). The engine should not fight this — default to `BorderlessFullscreen` when running on SteamOS.
- When docked, the user may want different resolution/mode than handheld. Valve recommends **not** syncing display settings between configurations.

### 3.5 Refresh Rate Awareness

Steam Deck LCD = 60 Hz, OLED = up to 90 Hz. Docked = up to 120 Hz.

**File to modify:** `src/engine/Window.hpp`, `src/engine/Time.hpp`

**Changes:**
- Expose the monitor refresh rate: `int Window::getRefreshRate() const;`
- VSync already respects the display refresh rate (Raylib handles this).
- Add an optional frame rate cap: `void Time::setTargetFPS(int fps);` (for battery life management on Deck).
- Expose to Lua:

```lua
window.get_refresh_rate()   -- 60, 90, or 120
window.set_target_fps(60)   -- Cap for battery life
```

---

## 4. Seamlessness (Stage 19C)

### 4.1 No Unsupported Hardware/OS Warnings

**Requirement:** The game must never display warnings about unsupported Linux distributions, GPUs, or OS versions.

**Action:** Audit all engine initialization code and mod APIs to ensure no platform-specific warnings are emitted. The engine currently has no such warnings (Raylib handles platform abstraction), but this must be enforced as a policy for mods.

**Mod guidelines to enforce (in documentation):**
- Mods must not check for or warn about specific OS/GPU.
- Engine should strip or block such checks in the Lua sandbox.

### 4.2 Suspend and Resume (Sleep/Wake)

The Steam Deck suspends and resumes like a handheld console. Games must handle this gracefully.

**File to modify:** `src/engine/Engine.cpp`

**Behavior on suspend/resume:**
- Audio must pause on suspend and resume on wake without glitches.
- Game time (`Time`) must handle large `dt` gaps from suspend — clamp `dt` to a maximum (e.g., 0.1s) to prevent physics explosions.
- Save state should be preserved (the OS handles process state, but the engine should auto-save when it detects a suspend event if possible).
- Texture and GPU resources: Raylib + OpenGL contexts are preserved by SteamOS Gamescope, so no special handling is needed.

**Detection:** On Linux, suspend triggers the window losing focus. The engine should:
1. Detect focus loss via Raylib's `IsWindowFocused()`.
2. Pause audio via `AudioSystem::pause()`.
3. On focus regain, resume audio and clamp the next frame's `dt`.

```cpp
// In Engine::processInput() or Engine::update():
if (!IsWindowFocused()) {
    m_audioSystem->pause();
    m_wasSuspended = true;
} else if (m_wasSuspended) {
    m_audioSystem->resume();
    m_time.clampNextDelta(MAX_DELTA);  // Prevent physics explosion
    m_wasSuspended = false;
}
```

### 4.3 Graceful Exit

**Requirement:** The game should save and exit cleanly when the user presses the Steam button → Exit Game, or when SteamOS sends a termination signal.

**Action:**
- Handle `SIGTERM` on Linux to trigger a graceful shutdown (auto-save + clean exit).
- The existing `Window::shouldClose()` loop already handles the close request from the window manager.

---

## 5. System Support (Stage 19D)

### 5.1 Native Linux Build

The engine already builds on Linux (CMake + GCC/Clang + Raylib). This section formalizes the requirements.

**Build verification:**
- Ensure the CMake build succeeds on GCC 11+ and Clang 13+ on Linux.
- All dependencies (Raylib, EnTT, sol2, spdlog, nlohmann_json, Lua) are platform-agnostic.
- Raylib uses OpenGL on Linux, which is supported by Mesa/RADV on Steam Deck.

**CI additions (CMakeLists.txt or GitHub Actions):**
- Add a Linux build target to CI alongside Windows.
- Test on Ubuntu 22.04 LTS (Valve-recommended Linux dev environment).
- Optional: Test on Arch Linux / Manjaro (closest to SteamOS).

### 5.2 Steamworks SDK Integration (Optional)

The Steamworks SDK is optional — the engine should work without it (for development, non-Steam builds, and open-source distribution). When present, it enables Steam-specific features needed for the Verified badge.

**File to create:** `src/engine/SteamIntegration.hpp`, `src/engine/SteamIntegration.cpp`

**CMake option:** `option(GLOAMING_STEAM "Enable Steamworks SDK integration" OFF)`

**Conditional compilation:** All Steam-specific code is behind `#ifdef GLOAMING_STEAM`.

**Interface:**

```cpp
class SteamIntegration {
public:
    /// Initialize the Steam API. Returns false if Steam is not running.
    bool init(uint32_t appId);

    /// Shutdown
    void shutdown();

    /// Per-frame callback processing
    void update();

    /// Check if Steam is available
    bool isAvailable() const;

    // --- On-screen keyboard ---
    /// Show the Steam overlay keyboard for text input
    void showOnScreenKeyboard(const std::string& description,
                              const std::string& existingText,
                              int maxChars);

    /// Check if the Steam keyboard submitted text
    bool hasKeyboardResult() const;
    std::string getKeyboardResult() const;

    // --- Input glyphs ---
    /// Get the glyph texture path for a Steam Input action origin
    std::string getGlyphPath(int actionOrigin) const;

    // --- Overlay ---
    /// Check if the Steam overlay is active (should pause game)
    bool isOverlayActive() const;
};
```

**Dependency management:**
- The Steamworks SDK is not open-source and cannot be fetched via FetchContent.
- It should be provided as a local path: `set(STEAMWORKS_SDK_DIR "" CACHE PATH "Path to Steamworks SDK")`
- When `GLOAMING_STEAM` is ON and the SDK path is set, link against `steam_api` / `steam_api64`.

### 5.3 Audio on Linux

The engine uses Raylib which uses miniaudio, which supports ALSA, PulseAudio, and PipeWire — all present on SteamOS. No additional work needed for basic audio.

**Verification needed:** Test that positional audio, music crossfade, and concurrent sound playback work correctly under PipeWire (SteamOS's default audio server).

### 5.4 Graphics API

The engine uses Raylib with OpenGL. On Steam Deck:
- OpenGL 4.6 is supported via Mesa's RadeonSI/RADV drivers.
- Vulkan would be preferred for better performance and battery life, but Raylib currently uses OpenGL. This is acceptable for Verified status — many Verified games use OpenGL.

**Future consideration (post-Stage 19):**
- Raylib 5.5 has experimental Vulkan support. Monitor for stable Vulkan backend.
- The abstract `IRenderer` interface is already designed for backend swaps.

---

## 6. Configuration and Persistence (Stage 19E)

### 6.1 Platform-Aware Default Config

**File to modify:** `src/engine/Config.cpp`, `config.json`

The engine should detect the platform and apply appropriate defaults:

```json
{
    "window": {
        "width": 1280,
        "height": 800,
        "title": "Gloaming",
        "fullscreen": true,
        "fullscreen_mode": "borderless",
        "vsync": true
    },
    "input": {
        "gamepad_enabled": true,
        "gamepad_deadzone": 0.15,
        "glyph_style": "auto",
        "rumble_enabled": true,
        "rumble_intensity": 1.0
    },
    "display": {
        "scale_mode": "expand",
        "ui_scale": 1.0,
        "min_font_size": 12
    },
    "performance": {
        "target_fps": 0
    }
}
```

**Auto-detection logic:**

```cpp
bool isSteamDeck() {
    // SteamOS sets the SteamDeck environment variable
    const char* val = std::getenv("SteamDeck");
    return val != nullptr && std::string(val) == "1";
}

bool isSteamOS() {
    const char* val = std::getenv("SteamOS");
    return val != nullptr;
}
```

When running on Steam Deck:
- Default to 1280×800 borderless fullscreen.
- Default to gamepad as primary input.
- Default to `GlyphStyle::Xbox` (Deck uses ABXY labeling).
- Default to target FPS of 60 (battery life).

### 6.2 Per-Device Settings Persistence

Valve recommends not syncing display settings between devices (handheld vs docked vs desktop).

**Approach:**
- Store settings in a device-specific config file: `config.local.json` (gitignored, not synced via Steam Cloud).
- Steam Cloud syncs save files but not display/input config.

---

## 7. Testing Requirements (Stage 19F)

### 7.1 Unit Tests

New test files:

| Test File | Covers |
|-----------|--------|
| `tests/test_gamepad.cpp` | Gamepad input detection, deadzone, button/axis queries |
| `tests/test_input_bindings.cpp` | Unified bindings, mixed keyboard+gamepad, analog values |
| `tests/test_input_glyphs.cpp` | Glyph provider, device switching, style selection |
| `tests/test_viewport_scaler.cpp` | All scale modes, coordinate transforms, aspect ratios |
| `tests/test_ui_scaling.cpp` | Font size scaling, minimum enforcement, dimension scaling |
| `tests/test_ui_gamepad_nav.cpp` | Gamepad UI navigation, spatial focus, wrap-around |
| `tests/test_on_screen_keyboard.cpp` | Keyboard show/hide, text callback, built-in fallback |
| `tests/test_suspend_resume.cpp` | Focus loss/gain, audio pause/resume, dt clamping |
| `tests/test_steam_integration.cpp` | Steam API mock, init/shutdown, overlay detection |

### 7.2 Integration Testing

Manual testing checklist (to be run on actual Steam Deck hardware or SteamOS VM):

- [ ] Game launches in fullscreen at 1280×800
- [ ] All menus navigable with gamepad only (no keyboard/mouse needed)
- [ ] Glyphs switch correctly when switching between keyboard and gamepad
- [ ] On-screen keyboard appears when text input is required
- [ ] Game suspends/resumes cleanly (no audio glitch, no physics explosion)
- [ ] Game exits cleanly from Steam overlay exit command
- [ ] All text is readable at arm's length (12px minimum)
- [ ] Game maintains 60 FPS during normal gameplay
- [ ] Audio works (music, SFX, positional)
- [ ] Analog stick movement is smooth with no drift at rest
- [ ] D-pad navigates menus
- [ ] No Linux/GPU unsupported warnings appear
- [ ] Docked mode works at different resolutions (1080p, 4K)

### 7.3 Performance Targets

| Metric | Target | Notes |
|--------|--------|-------|
| **Handheld FPS** | 60 FPS stable | At 1280×800, typical platformer scene |
| **Docked FPS** | 60 FPS | At 1080p |
| **Input latency** | < 16ms | One frame at 60 FPS |
| **Boot time** | < 5s to gameplay | Cold start to playable |
| **RAM usage** | < 2 GB | Leave headroom for OS |
| **Battery life** | 3+ hours | With target FPS cap at 60 |

---

## 8. Implementation Plan

### 8.1 File Summary

**New files (11):**

| File | Purpose |
|------|---------|
| `src/engine/Gamepad.hpp` | Gamepad abstraction |
| `src/engine/Gamepad.cpp` | Raylib gamepad backend |
| `src/engine/InputGlyphs.hpp` | Button glyph provider |
| `src/engine/InputGlyphs.cpp` | Glyph logic and atlas lookup |
| `src/engine/InputDeviceTracker.hpp` | Active device detection |
| `src/engine/InputDeviceTracker.cpp` | Device switch logic |
| `src/engine/Haptics.hpp` | Vibration/rumble abstraction |
| `src/engine/Haptics.cpp` | Haptics implementation |
| `src/engine/SteamIntegration.hpp` | Optional Steamworks wrapper |
| `src/engine/SteamIntegration.cpp` | Steam API calls |
| `src/rendering/ViewportScaler.hpp` | Resolution/aspect ratio handling |
| `src/rendering/ViewportScaler.cpp` | Viewport computation |
| `src/ui/UIScaling.hpp` | UI dimension/font scaling |
| `src/ui/UIScaling.cpp` | Scaling logic |
| `src/ui/OnScreenKeyboard.hpp` | On-screen keyboard |
| `src/ui/OnScreenKeyboard.cpp` | Built-in + Steam overlay keyboard |

**Modified files (12):**

| File | Changes |
|------|---------|
| `src/engine/Input.hpp` | Add `Gamepad` include and accessor |
| `src/engine/Engine.hpp` | Add `Gamepad`, `Haptics`, `InputDeviceTracker`, `ViewportScaler`, `SteamIntegration` members |
| `src/engine/Engine.cpp` | Initialize new systems, suspend/resume logic, SIGTERM handler |
| `src/engine/Window.hpp` | `FullscreenMode` enum, `getRefreshRate()`, `setFullscreenMode()` |
| `src/engine/Window.cpp` | Fullscreen mode implementation, refresh rate query |
| `src/engine/Time.hpp` | `setTargetFPS()`, `clampNextDelta()` |
| `src/gameplay/InputActions.hpp` | `InputSourceType`, gamepad bindings, analog action values |
| `src/gameplay/GameplayLuaBindings.cpp` | Gamepad, glyph, haptics, viewport Lua bindings |
| `src/ui/UIInput.hpp` | Gamepad navigation methods |
| `src/ui/UIInput.cpp` | Gamepad navigation implementation |
| `config.json` | New `input`, `display`, `performance` sections |
| `CMakeLists.txt` | New source files, optional Steamworks SDK linkage |

### 8.2 Staged Implementation Order

The work is ordered to allow incremental testing. Each sub-stage is independently testable.

| Sub-stage | Work | Depends On |
|-----------|------|-----------|
| **19A-1** | `Gamepad` class + Raylib backend | — |
| **19A-2** | Unified `InputBinding` (keyboard + gamepad) | 19A-1 |
| **19A-3** | `InputDeviceTracker` | 19A-1 |
| **19A-4** | `InputGlyphProvider` | 19A-2, 19A-3 |
| **19A-5** | UI gamepad navigation (`UIInput` changes) | 19A-1 |
| **19A-6** | `OnScreenKeyboard` (built-in fallback) | 19A-5 |
| **19A-7** | `Haptics` | 19A-1 |
| **19A-8** | Input Lua bindings | 19A-1 through 19A-7 |
| **19B-1** | `ViewportScaler` | — |
| **19B-2** | `UIScaling` + min font size | — |
| **19B-3** | Window fullscreen modes + refresh rate | — |
| **19B-4** | Platform auto-detection + config defaults | 19B-1, 19B-3 |
| **19C-1** | Suspend/resume handling | — |
| **19C-2** | Graceful exit (SIGTERM) | — |
| **19D-1** | Linux CI build verification | — |
| **19D-2** | `SteamIntegration` (optional) | 19A-6 |
| **19E-1** | Config file updates | 19A-1, 19B-1 |
| **19F-1** | Unit tests for all new systems | All above |
| **19F-2** | Manual Deck testing checklist | All above |

### 8.3 Estimated Scope

| Category | New Code (estimated) | Modified Code (estimated) |
|----------|---------------------|--------------------------|
| Input (19A) | ~1,800 lines | ~500 lines |
| Display (19B) | ~600 lines | ~200 lines |
| Seamlessness (19C) | ~100 lines | ~80 lines |
| System Support (19D) | ~400 lines | ~50 lines |
| Config (19E) | ~50 lines | ~30 lines |
| Tests (19F) | ~2,000 lines | — |
| **Total** | **~4,950 lines** | **~860 lines** |

---

## 9. Deck Verified Checklist Cross-Reference

Final verification that every Valve requirement is addressed:

### Input

| Requirement | Solution |
|-------------|----------|
| Controller support with access to all content | Gamepad class + unified InputActionMap with gamepad bindings (§2.1, §2.2) |
| Default controller config works without user adjustment | Platform-aware defaults auto-enable gamepad (§6.1) |
| Glyphs match controller (ABXY or Deck names) | InputGlyphProvider + InputDeviceTracker (§2.4, §2.5) |
| No keyboard/mouse glyphs when using controller | Device tracker suppresses wrong glyphs (§2.4) |
| On-screen keyboard for text input | OnScreenKeyboard with Steam overlay + fallback (§2.7) |

### Display

| Requirement | Solution |
|-------------|----------|
| Supports 1280×800 or 1280×720 | Config defaults + ViewportScaler (§3.1, §3.2) |
| Text legible at 12" distance, min 9px (recommended 12px) | UIScaling with minFontSize enforcement (§3.3) |

### Seamlessness

| Requirement | Solution |
|-------------|----------|
| No unsupported OS/GPU warnings | Policy enforcement, no platform checks in engine or mods (§4.1) |
| Launcher navigable with controller (if present) | No launcher — engine boots directly to game (§4.1) |

### System Support

| Requirement | Solution |
|-------------|----------|
| Runs on SteamOS (natively or via Proton) | Native Linux build with Raylib/OpenGL (§5.1) |
| Anti-cheat compatible (if applicable) | No anti-cheat used (§5.2) |

---

## 10. References

- [Steam Deck Compatibility Review Process](https://partner.steamgames.com/doc/steamdeck/compat)
- [Getting Your Game Ready for Steam Deck](https://partner.steamgames.com/doc/steamdeck/recommendations)
- [Steam Deck Tech Specs](https://www.steamdeck.com/en/tech)
- [Developing for SteamOS and Linux](https://partner.steamgames.com/doc/store/application/platforms/linux)
- [Steamworks SDK Documentation](https://partner.steamgames.com/doc/sdk)
- [Steam Deck FAQ for Developers](https://partner.steamgames.com/doc/steamdeck/faq)
- [Raylib Gamepad API](https://www.raylib.com/cheatsheet/cheatsheet.html)
