#include "engine/InputGlyphs.hpp"
#include "gameplay/InputActions.hpp"

namespace gloaming {

std::string InputGlyphProvider::getButtonName(GamepadButton button, GlyphStyle style) const {
    switch (style) {
        case GlyphStyle::Xbox:
        case GlyphStyle::SteamDeck:
            switch (button) {
                case GamepadButton::FaceDown:    return "A";
                case GamepadButton::FaceRight:   return "B";
                case GamepadButton::FaceLeft:    return "X";
                case GamepadButton::FaceUp:      return "Y";
                case GamepadButton::LeftBumper:   return "LB";
                case GamepadButton::RightBumper:  return "RB";
                case GamepadButton::Select:       return "View";
                case GamepadButton::Start:        return "Menu";
                case GamepadButton::Guide:        return "Guide";
                case GamepadButton::LeftThumb:    return "LS";
                case GamepadButton::RightThumb:   return "RS";
                case GamepadButton::DpadUp:       return "D-Up";
                case GamepadButton::DpadDown:     return "D-Down";
                case GamepadButton::DpadLeft:     return "D-Left";
                case GamepadButton::DpadRight:    return "D-Right";
            }
            break;

        case GlyphStyle::PlayStation:
            switch (button) {
                case GamepadButton::FaceDown:    return "Cross";
                case GamepadButton::FaceRight:   return "Circle";
                case GamepadButton::FaceLeft:    return "Square";
                case GamepadButton::FaceUp:      return "Triangle";
                case GamepadButton::LeftBumper:   return "L1";
                case GamepadButton::RightBumper:  return "R1";
                case GamepadButton::Select:       return "Share";
                case GamepadButton::Start:        return "Options";
                case GamepadButton::Guide:        return "PS";
                case GamepadButton::LeftThumb:    return "L3";
                case GamepadButton::RightThumb:   return "R3";
                case GamepadButton::DpadUp:       return "D-Up";
                case GamepadButton::DpadDown:     return "D-Down";
                case GamepadButton::DpadLeft:     return "D-Left";
                case GamepadButton::DpadRight:    return "D-Right";
            }
            break;

        case GlyphStyle::Nintendo:
            switch (button) {
                case GamepadButton::FaceDown:    return "B";
                case GamepadButton::FaceRight:   return "A";
                case GamepadButton::FaceLeft:    return "Y";
                case GamepadButton::FaceUp:      return "X";
                case GamepadButton::LeftBumper:   return "L";
                case GamepadButton::RightBumper:  return "R";
                case GamepadButton::Select:       return "Minus";
                case GamepadButton::Start:        return "Plus";
                case GamepadButton::Guide:        return "Home";
                case GamepadButton::LeftThumb:    return "LS";
                case GamepadButton::RightThumb:   return "RS";
                case GamepadButton::DpadUp:       return "D-Up";
                case GamepadButton::DpadDown:     return "D-Down";
                case GamepadButton::DpadLeft:     return "D-Left";
                case GamepadButton::DpadRight:    return "D-Right";
            }
            break;

        case GlyphStyle::Keyboard:
            // Shouldn't be used for gamepad buttons, return Xbox-style names
            return getButtonName(button, GlyphStyle::Xbox);
    }
    return "?";
}

std::string InputGlyphProvider::getKeyName(Key key) const {
    switch (key) {
        case Key::A: return "A"; case Key::B: return "B"; case Key::C: return "C";
        case Key::D: return "D"; case Key::E: return "E"; case Key::F: return "F";
        case Key::G: return "G"; case Key::H: return "H"; case Key::I: return "I";
        case Key::J: return "J"; case Key::K: return "K"; case Key::L: return "L";
        case Key::M: return "M"; case Key::N: return "N"; case Key::O: return "O";
        case Key::P: return "P"; case Key::Q: return "Q"; case Key::R: return "R";
        case Key::S: return "S"; case Key::T: return "T"; case Key::U: return "U";
        case Key::V: return "V"; case Key::W: return "W"; case Key::X: return "X";
        case Key::Y: return "Y"; case Key::Z: return "Z";
        case Key::Num0: return "0"; case Key::Num1: return "1"; case Key::Num2: return "2";
        case Key::Num3: return "3"; case Key::Num4: return "4"; case Key::Num5: return "5";
        case Key::Num6: return "6"; case Key::Num7: return "7"; case Key::Num8: return "8";
        case Key::Num9: return "9";
        case Key::Space: return "Space";
        case Key::Enter: return "Enter";
        case Key::Escape: return "Esc";
        case Key::Backspace: return "Backspace";
        case Key::Tab: return "Tab";
        case Key::Delete: return "Del";
        case Key::Insert: return "Ins";
        case Key::Home: return "Home";
        case Key::End: return "End";
        case Key::PageUp: return "PgUp";
        case Key::PageDown: return "PgDn";
        case Key::Up: return "Up";
        case Key::Down: return "Down";
        case Key::Left: return "Left";
        case Key::Right: return "Right";
        case Key::LeftShift: return "LShift";
        case Key::RightShift: return "RShift";
        case Key::LeftControl: return "LCtrl";
        case Key::RightControl: return "RCtrl";
        case Key::LeftAlt: return "LAlt";
        case Key::RightAlt: return "RAlt";
        case Key::F1: return "F1"; case Key::F2: return "F2"; case Key::F3: return "F3";
        case Key::F4: return "F4"; case Key::F5: return "F5"; case Key::F6: return "F6";
        case Key::F7: return "F7"; case Key::F8: return "F8"; case Key::F9: return "F9";
        case Key::F10: return "F10"; case Key::F11: return "F11"; case Key::F12: return "F12";
        case Key::Minus: return "-";
        case Key::Equal: return "=";
        case Key::LeftBracket: return "[";
        case Key::RightBracket: return "]";
        case Key::Backslash: return "\\";
        case Key::Semicolon: return ";";
        case Key::Apostrophe: return "'";
        case Key::Comma: return ",";
        case Key::Period: return ".";
        case Key::Slash: return "/";
        case Key::GraveAccent: return "`";
    }
    return "?";
}

std::string InputGlyphProvider::getActionGlyph(const std::string& actionName,
                                               const InputActionMap& actions,
                                               InputDevice activeDevice,
                                               GlyphStyle style) const {
    const auto& bindings = actions.getBindings(actionName);
    if (bindings.empty()) return "?";

    // Find the first binding that matches the active device
    for (const auto& binding : bindings) {
        if (activeDevice == InputDevice::KeyboardMouse &&
            binding.sourceType == InputSourceType::Key) {
            return getKeyName(binding.key);
        }
        if (activeDevice == InputDevice::Gamepad) {
            if (binding.sourceType == InputSourceType::GamepadButton) {
                return getButtonName(binding.gamepadButton, style);
            }
            if (binding.sourceType == InputSourceType::GamepadAxis) {
                // Return a descriptive name for axis bindings
                switch (binding.gamepadAxis) {
                    case GamepadAxis::LeftX:
                        return binding.axisPositive ? "LS Right" : "LS Left";
                    case GamepadAxis::LeftY:
                        return binding.axisPositive ? "LS Down" : "LS Up";
                    case GamepadAxis::RightX:
                        return binding.axisPositive ? "RS Right" : "RS Left";
                    case GamepadAxis::RightY:
                        return binding.axisPositive ? "RS Down" : "RS Up";
                    case GamepadAxis::LeftTrigger:  return "LT";
                    case GamepadAxis::RightTrigger: return "RT";
                }
            }
        }
    }

    // Fallback: return the first binding regardless of device type
    const auto& first = bindings[0];
    if (first.sourceType == InputSourceType::Key) {
        return getKeyName(first.key);
    }
    if (first.sourceType == InputSourceType::GamepadButton) {
        return getButtonName(first.gamepadButton, style);
    }
    return "?";
}

Rect InputGlyphProvider::getGlyphRegion(GamepadButton button, GlyphStyle style) const {
    // Atlas layout: 16 buttons per row, 32x32 pixels each
    // Row 0 = Xbox, Row 1 = PlayStation, Row 2 = Nintendo, Row 3 = SteamDeck
    int row = 0;
    switch (style) {
        case GlyphStyle::Xbox:        row = 0; break;
        case GlyphStyle::PlayStation:  row = 1; break;
        case GlyphStyle::Nintendo:     row = 2; break;
        case GlyphStyle::SteamDeck:    row = 3; break;
        case GlyphStyle::Keyboard:     row = 0; break; // fallback
    }

    int col = static_cast<int>(button);
    return Rect(static_cast<float>(col * 32), static_cast<float>(row * 32), 32.0f, 32.0f);
}

Rect InputGlyphProvider::getGlyphRegion(Key key) const {
    // Keyboard glyph atlas layout: keys laid out in order of the enum value
    // For simplicity, use a grid of 16 columns, 32x32 each
    int index = static_cast<int>(key) % 256;
    int col = index % 16;
    int row = index / 16;
    return Rect(static_cast<float>(col * 32), static_cast<float>(row * 32), 32.0f, 32.0f);
}

void InputGlyphProvider::loadGlyphAtlas(const std::string& path, GlyphStyle style) {
    m_glyphAtlasPaths[static_cast<int>(style)] = path;
}

void InputGlyphProvider::setGlyphStyle(GlyphStyle style) {
    m_currentStyle = style;
}

GlyphStyle InputGlyphProvider::getGlyphStyle() const {
    return m_currentStyle;
}

} // namespace gloaming
