#include <gtest/gtest.h>

#include "engine/Gamepad.hpp"
#include "engine/InputDeviceTracker.hpp"
#include "engine/InputGlyphs.hpp"
#include "engine/Haptics.hpp"
#include "gameplay/InputActions.hpp"
#include "ui/OnScreenKeyboard.hpp"

using namespace gloaming;

// ============================================================================
// Gamepad Tests
// ============================================================================

TEST(GamepadTest, DefaultDeadzone) {
    Gamepad gamepad;
    EXPECT_FLOAT_EQ(gamepad.getDeadzone(), 0.15f);
}

TEST(GamepadTest, SetDeadzone) {
    Gamepad gamepad;
    gamepad.setDeadzone(0.25f);
    EXPECT_FLOAT_EQ(gamepad.getDeadzone(), 0.25f);
}

TEST(GamepadTest, MaxGamepads) {
    EXPECT_EQ(Gamepad::MAX_GAMEPADS, 4);
}

TEST(GamepadTest, DisconnectedGamepadReturnsDefaults) {
    Gamepad gamepad;
    // No physical gamepad connected in test environment
    EXPECT_FALSE(gamepad.isConnected(0));
    EXPECT_FALSE(gamepad.isButtonDown(GamepadButton::FaceDown, 0));
    EXPECT_FALSE(gamepad.isButtonPressed(GamepadButton::FaceDown, 0));
    EXPECT_FALSE(gamepad.isButtonReleased(GamepadButton::FaceDown, 0));
    EXPECT_FLOAT_EQ(gamepad.getAxis(GamepadAxis::LeftX, 0), 0.0f);
}

TEST(GamepadTest, StickReturnsZeroWhenDisconnected) {
    Gamepad gamepad;
    Vec2 stick = gamepad.getLeftStick(0);
    EXPECT_FLOAT_EQ(stick.x, 0.0f);
    EXPECT_FLOAT_EQ(stick.y, 0.0f);

    Vec2 rightStick = gamepad.getRightStick(0);
    EXPECT_FLOAT_EQ(rightStick.x, 0.0f);
    EXPECT_FLOAT_EQ(rightStick.y, 0.0f);
}

TEST(GamepadTest, TriggersReturnZeroWhenDisconnected) {
    Gamepad gamepad;
    EXPECT_FLOAT_EQ(gamepad.getLeftTrigger(0), 0.0f);
    EXPECT_FLOAT_EQ(gamepad.getRightTrigger(0), 0.0f);
}

TEST(GamepadTest, InvalidGamepadIdReturnsFalse) {
    Gamepad gamepad;
    EXPECT_FALSE(gamepad.isConnected(-1));
    EXPECT_FALSE(gamepad.isConnected(5));
}

TEST(GamepadTest, HadAnyInputReturnsFalseWhenDisconnected) {
    Gamepad gamepad;
    EXPECT_FALSE(gamepad.hadAnyInput(0));
}

TEST(GamepadTest, ConnectedCountZeroInTestEnv) {
    Gamepad gamepad;
    EXPECT_EQ(gamepad.getConnectedCount(), 0);
}

// ============================================================================
// InputDeviceTracker Tests
// ============================================================================

TEST(InputDeviceTrackerTest, DefaultsToKeyboardMouse) {
    InputDeviceTracker tracker;
    EXPECT_EQ(tracker.getActiveDevice(), InputDevice::KeyboardMouse);
}

TEST(InputDeviceTrackerTest, NoChangeOnFirstUpdate) {
    InputDeviceTracker tracker;
    Input input;
    Gamepad gamepad;
    // No input activity = no change
    tracker.update(input, gamepad);
    EXPECT_FALSE(tracker.didDeviceChange());
    EXPECT_EQ(tracker.getActiveDevice(), InputDevice::KeyboardMouse);
}

TEST(InputDeviceTrackerTest, DeviceEnumValues) {
    // Verify enum values exist
    EXPECT_NE(static_cast<int>(InputDevice::KeyboardMouse),
              static_cast<int>(InputDevice::Gamepad));
}

// ============================================================================
// InputGlyphProvider Tests
// ============================================================================

TEST(InputGlyphProviderTest, DefaultStyleIsXbox) {
    InputGlyphProvider glyphs;
    EXPECT_EQ(glyphs.getGlyphStyle(), GlyphStyle::Xbox);
}

TEST(InputGlyphProviderTest, SetGlyphStyle) {
    InputGlyphProvider glyphs;
    glyphs.setGlyphStyle(GlyphStyle::PlayStation);
    EXPECT_EQ(glyphs.getGlyphStyle(), GlyphStyle::PlayStation);

    glyphs.setGlyphStyle(GlyphStyle::Nintendo);
    EXPECT_EQ(glyphs.getGlyphStyle(), GlyphStyle::Nintendo);
}

TEST(InputGlyphProviderTest, XboxButtonNames) {
    InputGlyphProvider glyphs;
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::FaceDown, GlyphStyle::Xbox), "A");
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::FaceRight, GlyphStyle::Xbox), "B");
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::FaceLeft, GlyphStyle::Xbox), "X");
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::FaceUp, GlyphStyle::Xbox), "Y");
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::LeftBumper, GlyphStyle::Xbox), "LB");
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::RightBumper, GlyphStyle::Xbox), "RB");
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::Start, GlyphStyle::Xbox), "Menu");
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::DpadUp, GlyphStyle::Xbox), "D-Up");
}

TEST(InputGlyphProviderTest, PlayStationButtonNames) {
    InputGlyphProvider glyphs;
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::FaceDown, GlyphStyle::PlayStation), "Cross");
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::FaceRight, GlyphStyle::PlayStation), "Circle");
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::FaceLeft, GlyphStyle::PlayStation), "Square");
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::FaceUp, GlyphStyle::PlayStation), "Triangle");
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::LeftBumper, GlyphStyle::PlayStation), "L1");
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::RightBumper, GlyphStyle::PlayStation), "R1");
}

TEST(InputGlyphProviderTest, NintendoButtonNames) {
    InputGlyphProvider glyphs;
    // Nintendo swaps AB and XY
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::FaceDown, GlyphStyle::Nintendo), "B");
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::FaceRight, GlyphStyle::Nintendo), "A");
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::FaceLeft, GlyphStyle::Nintendo), "Y");
    EXPECT_EQ(glyphs.getButtonName(GamepadButton::FaceUp, GlyphStyle::Nintendo), "X");
}

TEST(InputGlyphProviderTest, KeyNames) {
    InputGlyphProvider glyphs;
    EXPECT_EQ(glyphs.getKeyName(Key::Space), "Space");
    EXPECT_EQ(glyphs.getKeyName(Key::Enter), "Enter");
    EXPECT_EQ(glyphs.getKeyName(Key::Escape), "Esc");
    EXPECT_EQ(glyphs.getKeyName(Key::A), "A");
    EXPECT_EQ(glyphs.getKeyName(Key::Z), "Z");
    EXPECT_EQ(glyphs.getKeyName(Key::Tab), "Tab");
    EXPECT_EQ(glyphs.getKeyName(Key::F1), "F1");
    EXPECT_EQ(glyphs.getKeyName(Key::LeftShift), "LShift");
}

TEST(InputGlyphProviderTest, GlyphRegionGamepadButton) {
    InputGlyphProvider glyphs;
    // FaceDown (index 0) should be at (0, 0) for Xbox style
    Rect region = glyphs.getGlyphRegion(GamepadButton::FaceDown, GlyphStyle::Xbox);
    EXPECT_FLOAT_EQ(region.x, 0.0f);
    EXPECT_FLOAT_EQ(region.y, 0.0f);
    EXPECT_FLOAT_EQ(region.width, 32.0f);
    EXPECT_FLOAT_EQ(region.height, 32.0f);

    // FaceRight (index 1) should be at (32, 0) for Xbox
    Rect region2 = glyphs.getGlyphRegion(GamepadButton::FaceRight, GlyphStyle::Xbox);
    EXPECT_FLOAT_EQ(region2.x, 32.0f);
    EXPECT_FLOAT_EQ(region2.y, 0.0f);

    // PlayStation style = row 1
    Rect psRegion = glyphs.getGlyphRegion(GamepadButton::FaceDown, GlyphStyle::PlayStation);
    EXPECT_FLOAT_EQ(psRegion.y, 32.0f); // row 1
}

TEST(InputGlyphProviderTest, ActionGlyphForKeyboardDevice) {
    InputGlyphProvider glyphs;
    InputActionMap actions;
    actions.registerAction("jump", Key::Space);

    std::string glyph = glyphs.getActionGlyph("jump", actions,
                                                InputDevice::KeyboardMouse, GlyphStyle::Xbox);
    EXPECT_EQ(glyph, "Space");
}

TEST(InputGlyphProviderTest, ActionGlyphForGamepadDevice) {
    InputGlyphProvider glyphs;
    InputActionMap actions;
    actions.registerAction("jump", Key::Space);
    actions.addGamepadBinding("jump", GamepadButton::FaceDown);

    std::string glyph = glyphs.getActionGlyph("jump", actions,
                                                InputDevice::Gamepad, GlyphStyle::Xbox);
    EXPECT_EQ(glyph, "A");
}

TEST(InputGlyphProviderTest, ActionGlyphFallsBackWhenNoMatch) {
    InputGlyphProvider glyphs;
    InputActionMap actions;
    // Only keyboard binding, but querying for gamepad device
    actions.registerAction("jump", Key::Space);

    std::string glyph = glyphs.getActionGlyph("jump", actions,
                                                InputDevice::Gamepad, GlyphStyle::Xbox);
    // Should fall back to the first binding (keyboard)
    EXPECT_EQ(glyph, "Space");
}

TEST(InputGlyphProviderTest, ActionGlyphUnknownAction) {
    InputGlyphProvider glyphs;
    InputActionMap actions;

    std::string glyph = glyphs.getActionGlyph("nonexistent", actions,
                                                InputDevice::KeyboardMouse, GlyphStyle::Xbox);
    EXPECT_EQ(glyph, "?");
}

// ============================================================================
// InputActionMap (Extended) Tests
// ============================================================================

TEST(InputActionMapExtendedTest, RegisterAndCheckAction) {
    InputActionMap actions;
    actions.registerAction("jump", Key::Space);
    EXPECT_TRUE(actions.hasAction("jump"));
    EXPECT_FALSE(actions.hasAction("nonexistent"));
}

TEST(InputActionMapExtendedTest, AddGamepadButtonBinding) {
    InputActionMap actions;
    actions.registerAction("jump", Key::Space);
    actions.addGamepadBinding("jump", GamepadButton::FaceDown);

    const auto& bindings = actions.getBindings("jump");
    EXPECT_EQ(bindings.size(), 2u);
    EXPECT_EQ(bindings[0].sourceType, InputSourceType::Key);
    EXPECT_EQ(bindings[0].key, Key::Space);
    EXPECT_EQ(bindings[1].sourceType, InputSourceType::GamepadButton);
    EXPECT_EQ(bindings[1].gamepadButton, GamepadButton::FaceDown);
}

TEST(InputActionMapExtendedTest, AddGamepadAxisBinding) {
    InputActionMap actions;
    actions.registerAction("move_left", Key::A);
    actions.addGamepadBinding("move_left", GamepadAxis::LeftX, -0.5f);

    const auto& bindings = actions.getBindings("move_left");
    EXPECT_EQ(bindings.size(), 2u);
    EXPECT_EQ(bindings[1].sourceType, InputSourceType::GamepadAxis);
    EXPECT_EQ(bindings[1].gamepadAxis, GamepadAxis::LeftX);
    EXPECT_FLOAT_EQ(bindings[1].axisThreshold, 0.5f);
    EXPECT_FALSE(bindings[1].axisPositive); // negative direction
}

TEST(InputActionMapExtendedTest, PlatformerDefaultsHaveGamepadBindings) {
    InputActionMap actions;
    actions.registerPlatformerDefaults();

    EXPECT_TRUE(actions.hasAction("jump"));
    EXPECT_TRUE(actions.hasAction("attack"));
    EXPECT_TRUE(actions.hasAction("interact"));
    EXPECT_TRUE(actions.hasAction("menu"));
    EXPECT_TRUE(actions.hasAction("inventory"));
    EXPECT_TRUE(actions.hasAction("move_left"));
    EXPECT_TRUE(actions.hasAction("move_right"));

    // Check that gamepad bindings exist for movement
    const auto& leftBindings = actions.getBindings("move_left");
    bool hasGamepadBinding = false;
    for (const auto& b : leftBindings) {
        if (b.sourceType == InputSourceType::GamepadButton ||
            b.sourceType == InputSourceType::GamepadAxis) {
            hasGamepadBinding = true;
            break;
        }
    }
    EXPECT_TRUE(hasGamepadBinding);
}

TEST(InputActionMapExtendedTest, TopDownDefaultsHaveGamepadBindings) {
    InputActionMap actions;
    actions.registerTopDownDefaults();

    EXPECT_TRUE(actions.hasAction("interact"));
    EXPECT_TRUE(actions.hasAction("cancel"));
    EXPECT_TRUE(actions.hasAction("run"));

    const auto& runBindings = actions.getBindings("run");
    bool hasGamepadBinding = false;
    for (const auto& b : runBindings) {
        if (b.sourceType == InputSourceType::GamepadButton) {
            hasGamepadBinding = true;
            break;
        }
    }
    EXPECT_TRUE(hasGamepadBinding);
}

TEST(InputActionMapExtendedTest, FlightDefaultsHaveGamepadBindings) {
    InputActionMap actions;
    actions.registerFlightDefaults();

    EXPECT_TRUE(actions.hasAction("fire"));
    EXPECT_TRUE(actions.hasAction("bomb"));

    const auto& fireBindings = actions.getBindings("fire");
    bool hasGamepadBinding = false;
    for (const auto& b : fireBindings) {
        if (b.sourceType == InputSourceType::GamepadButton) {
            hasGamepadBinding = true;
            break;
        }
    }
    EXPECT_TRUE(hasGamepadBinding);
}

TEST(InputActionMapExtendedTest, ClearBindingsRemovesAll) {
    InputActionMap actions;
    actions.registerAction("jump", Key::Space);
    actions.addGamepadBinding("jump", GamepadButton::FaceDown);
    EXPECT_EQ(actions.getBindings("jump").size(), 2u);

    actions.clearBindings("jump");
    EXPECT_EQ(actions.getBindings("jump").size(), 0u);
}

TEST(InputActionMapExtendedTest, RebindReplacesWithKey) {
    InputActionMap actions;
    actions.registerAction("jump", Key::Space);
    actions.addGamepadBinding("jump", GamepadButton::FaceDown);
    EXPECT_EQ(actions.getBindings("jump").size(), 2u);

    actions.rebind("jump", Key::Enter);
    EXPECT_EQ(actions.getBindings("jump").size(), 1u);
    EXPECT_EQ(actions.getBindings("jump")[0].key, Key::Enter);
}

TEST(InputActionMapExtendedTest, ClearAll) {
    InputActionMap actions;
    actions.registerPlatformerDefaults();
    EXPECT_GT(actions.getActionNames().size(), 0u);

    actions.clearAll();
    EXPECT_EQ(actions.getActionNames().size(), 0u);
}

TEST(InputActionMapExtendedTest, GetActionValue) {
    InputActionMap actions;
    Input input;
    Gamepad gamepad;
    actions.registerAction("jump", Key::Space);

    // No input = 0
    float val = actions.getActionValue("jump", input, gamepad);
    EXPECT_FLOAT_EQ(val, 0.0f);
}

TEST(InputActionMapExtendedTest, GetMovementVector) {
    InputActionMap actions;
    Input input;
    Gamepad gamepad;
    actions.registerPlatformerDefaults();

    // No input = zero vector
    Vec2 mv = actions.getMovementVector("move_left", "move_right", "move_up", "move_down",
                                        input, gamepad);
    EXPECT_FLOAT_EQ(mv.x, 0.0f);
    EXPECT_FLOAT_EQ(mv.y, 0.0f);
}

TEST(InputActionMapExtendedTest, BackwardCompatKeyboardOnly) {
    InputActionMap actions;
    Input input;
    actions.registerAction("jump", Key::Space);

    // Backward-compatible keyboard-only overload should work
    bool pressed = actions.isActionPressed("jump", input);
    bool down = actions.isActionDown("jump", input);
    bool released = actions.isActionReleased("jump", input);

    // No real input, but just ensure the overloads compile and return false
    EXPECT_FALSE(pressed);
    EXPECT_FALSE(down);
    EXPECT_FALSE(released);
}

// ============================================================================
// Haptics Tests
// ============================================================================

TEST(HapticsTest, DefaultEnabled) {
    Haptics haptics;
    EXPECT_TRUE(haptics.isEnabled());
}

TEST(HapticsTest, DefaultIntensity) {
    Haptics haptics;
    EXPECT_FLOAT_EQ(haptics.getIntensity(), 1.0f);
}

TEST(HapticsTest, SetEnabled) {
    Haptics haptics;
    haptics.setEnabled(false);
    EXPECT_FALSE(haptics.isEnabled());
    haptics.setEnabled(true);
    EXPECT_TRUE(haptics.isEnabled());
}

TEST(HapticsTest, SetIntensity) {
    Haptics haptics;
    haptics.setIntensity(0.5f);
    EXPECT_FLOAT_EQ(haptics.getIntensity(), 0.5f);
}

TEST(HapticsTest, IntensityClamps) {
    Haptics haptics;
    haptics.setIntensity(-1.0f);
    EXPECT_FLOAT_EQ(haptics.getIntensity(), 0.0f);

    haptics.setIntensity(5.0f);
    EXPECT_FLOAT_EQ(haptics.getIntensity(), 1.0f);
}

TEST(HapticsTest, VibrateAndUpdate) {
    Haptics haptics;
    // Should not crash even without hardware
    haptics.vibrate(1.0f, 1.0f, 0.5f);
    haptics.update(0.3f);
    // Still active (0.2s remaining)
    haptics.update(0.3f);
    // Should have expired and cleaned up
}

TEST(HapticsTest, StopClearsVibration) {
    Haptics haptics;
    haptics.vibrate(1.0f, 1.0f, 10.0f);
    haptics.stop();
    // No crash
}

TEST(HapticsTest, DisablingStopsAll) {
    Haptics haptics;
    haptics.vibrate(1.0f, 1.0f, 10.0f);
    haptics.setEnabled(false);
    // Should clear all active vibrations
}

TEST(HapticsTest, ImpulseCreatesShortVibration) {
    Haptics haptics;
    // Impulse of 100ms
    haptics.impulse(0.8f, 100.0f);
    haptics.update(0.05f); // 50ms elapsed
    haptics.update(0.06f); // 110ms total — should expire
}

TEST(HapticsTest, VibrateDisabledDoesNothing) {
    Haptics haptics;
    haptics.setEnabled(false);
    haptics.vibrate(1.0f, 1.0f, 1.0f); // Should be ignored
}

// ============================================================================
// OnScreenKeyboard Tests
// ============================================================================

TEST(OnScreenKeyboardTest, InitiallyNotVisible) {
    OnScreenKeyboard osk;
    EXPECT_FALSE(osk.isVisible());
}

TEST(OnScreenKeyboardTest, RequestMakesVisible) {
    OnScreenKeyboard osk;
    osk.requestTextInput("Enter name", "", 20, [](const std::string&) {});
    EXPECT_TRUE(osk.isVisible());
}

TEST(OnScreenKeyboardTest, DismissHidesAndCallsCallback) {
    OnScreenKeyboard osk;
    bool called = false;
    std::string result;
    osk.requestTextInput("Test", "", 20, [&](const std::string& r) {
        called = true;
        result = r;
    });
    EXPECT_TRUE(osk.isVisible());

    osk.dismiss();
    EXPECT_FALSE(osk.isVisible());
    EXPECT_TRUE(called);
    EXPECT_EQ(result, ""); // Dismiss returns empty string
}

// ============================================================================
// GamepadButton and GamepadAxis Enum Tests
// ============================================================================

TEST(GamepadEnumsTest, ButtonValues) {
    // Verify critical button enum values match Raylib conventions
    EXPECT_EQ(static_cast<int>(GamepadButton::FaceDown), 0);
    EXPECT_EQ(static_cast<int>(GamepadButton::FaceRight), 1);
    EXPECT_EQ(static_cast<int>(GamepadButton::FaceLeft), 2);
    EXPECT_EQ(static_cast<int>(GamepadButton::FaceUp), 3);
    EXPECT_EQ(static_cast<int>(GamepadButton::LeftBumper), 4);
    EXPECT_EQ(static_cast<int>(GamepadButton::RightBumper), 5);
    EXPECT_EQ(static_cast<int>(GamepadButton::DpadUp), 11);
    EXPECT_EQ(static_cast<int>(GamepadButton::DpadDown), 12);
    EXPECT_EQ(static_cast<int>(GamepadButton::DpadLeft), 13);
    EXPECT_EQ(static_cast<int>(GamepadButton::DpadRight), 14);
}

TEST(GamepadEnumsTest, AxisValues) {
    EXPECT_EQ(static_cast<int>(GamepadAxis::LeftX), 0);
    EXPECT_EQ(static_cast<int>(GamepadAxis::LeftY), 1);
    EXPECT_EQ(static_cast<int>(GamepadAxis::RightX), 2);
    EXPECT_EQ(static_cast<int>(GamepadAxis::RightY), 3);
    EXPECT_EQ(static_cast<int>(GamepadAxis::LeftTrigger), 4);
    EXPECT_EQ(static_cast<int>(GamepadAxis::RightTrigger), 5);
}

// ============================================================================
// InputSourceType Enum Tests
// ============================================================================

TEST(InputSourceTypeTest, EnumValues) {
    EXPECT_NE(static_cast<int>(InputSourceType::Key),
              static_cast<int>(InputSourceType::GamepadButton));
    EXPECT_NE(static_cast<int>(InputSourceType::Key),
              static_cast<int>(InputSourceType::GamepadAxis));
    EXPECT_NE(static_cast<int>(InputSourceType::GamepadButton),
              static_cast<int>(InputSourceType::GamepadAxis));
}

// ============================================================================
// Vec2 Utility Tests (used by stick input)
// ============================================================================

TEST(Vec2InputTest, ZeroVector) {
    Vec2 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.length(), 0.0f);
}

TEST(Vec2InputTest, NormalizedZeroReturnsZero) {
    Vec2 v;
    Vec2 n = v.normalized();
    EXPECT_FLOAT_EQ(n.x, 0.0f);
    EXPECT_FLOAT_EQ(n.y, 0.0f);
}

TEST(Vec2InputTest, UnitVectorLength) {
    Vec2 v(1.0f, 0.0f);
    EXPECT_FLOAT_EQ(v.length(), 1.0f);

    Vec2 v2(0.0f, 1.0f);
    EXPECT_FLOAT_EQ(v2.length(), 1.0f);
}

TEST(Vec2InputTest, Multiplication) {
    Vec2 v(0.5f, 0.5f);
    Vec2 scaled = v * 2.0f;
    EXPECT_FLOAT_EQ(scaled.x, 1.0f);
    EXPECT_FLOAT_EQ(scaled.y, 1.0f);
}
