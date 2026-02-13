#include "ui/OnScreenKeyboard.hpp"

#include <raylib.h>
#include <algorithm>
#include <cctype>

namespace gloaming {

void OnScreenKeyboard::requestTextInput(const std::string& description,
                                         const std::string& existingText,
                                         int maxChars,
                                         std::function<void(const std::string&)> callback) {
    m_description = description;
    m_text = existingText;
    m_maxChars = maxChars;
    m_callback = std::move(callback);
    m_visible = true;
    m_cursorRow = 1;
    m_cursorCol = 0;
    m_shiftActive = false;
}

bool OnScreenKeyboard::isVisible() const {
    return m_visible;
}

void OnScreenKeyboard::update(const Input& input, const Gamepad& gamepad, float dt) {
    if (!m_visible) return;

    // Handle direct keyboard typing via Raylib's character input queue.
    // GetCharPressed() returns Unicode codepoints properly handling shift/modifiers,
    // unlike looping over raw key codes which can't distinguish 'a' from 'A'.
    int charCode = GetCharPressed();
    while (charCode > 0) {
        if (charCode >= 32 && charCode < 127 && static_cast<int>(m_text.size()) < m_maxChars) {
            m_text += static_cast<char>(charCode);
        }
        charCode = GetCharPressed();
    }

    // Keyboard shortcuts
    if (input.isKeyPressed(Key::Backspace) && !m_text.empty()) {
        m_text.pop_back();
    }
    if (input.isKeyPressed(Key::Enter)) {
        confirm();
        return;
    }
    if (input.isKeyPressed(Key::Escape)) {
        dismiss();
        return;
    }

    // Gamepad navigation
    int dx = 0, dy = 0;
    if (gamepad.isButtonPressed(GamepadButton::DpadLeft))  dx = -1;
    if (gamepad.isButtonPressed(GamepadButton::DpadRight)) dx = 1;
    if (gamepad.isButtonPressed(GamepadButton::DpadUp))    dy = -1;
    if (gamepad.isButtonPressed(GamepadButton::DpadDown))  dy = 1;

    // Left stick navigation with repeat
    Vec2 stick = gamepad.getLeftStick();
    bool stickActive = (stick.lengthSquared() > 0.25f);
    if (stickActive) {
        if (!m_navHeld) {
            // First input
            if (stick.x < -0.5f) dx = -1;
            if (stick.x > 0.5f)  dx = 1;
            if (stick.y < -0.5f) dy = -1;
            if (stick.y > 0.5f)  dy = 1;
            m_navHeld = true;
            m_navTimer = m_navRepeatDelay;
        } else {
            m_navTimer -= dt;
            if (m_navTimer <= 0.0f) {
                if (stick.x < -0.5f) dx = -1;
                if (stick.x > 0.5f)  dx = 1;
                if (stick.y < -0.5f) dy = -1;
                if (stick.y > 0.5f)  dy = 1;
                m_navTimer = m_navRepeatRate;
            }
        }
    } else {
        m_navHeld = false;
    }

    if (dx != 0 || dy != 0) {
        moveCursor(dx, dy);
    }

    // Gamepad button actions
    if (gamepad.isButtonPressed(GamepadButton::FaceDown)) {  // A = press key
        pressSelectedKey();
    }
    if (gamepad.isButtonPressed(GamepadButton::FaceRight)) { // B = backspace
        if (!m_text.empty()) m_text.pop_back();
    }
    if (gamepad.isButtonPressed(GamepadButton::FaceUp)) {    // Y = shift
        m_shiftActive = !m_shiftActive;
    }
    if (gamepad.isButtonPressed(GamepadButton::Start)) {     // Start = confirm
        confirm();
    }
    if (gamepad.isButtonPressed(GamepadButton::Select)) {    // Select = cancel
        dismiss();
    }
}

void OnScreenKeyboard::render(IRenderer* renderer) {
    if (!m_visible || !renderer) return;

    int screenW = renderer->getScreenWidth();
    int screenH = renderer->getScreenHeight();

    // Background overlay
    renderer->drawRectangle(Rect(0, 0, static_cast<float>(screenW), static_cast<float>(screenH)),
                       Color(0, 0, 0, 160));

    // Keyboard panel
    float panelW = 500.0f;
    float panelH = 300.0f;
    float panelX = (screenW - panelW) / 2.0f;
    float panelY = screenH - panelH - 20.0f;

    renderer->drawRectangle(Rect(panelX, panelY, panelW, panelH), Color(30, 30, 40, 240));
    renderer->drawRectangle(Rect(panelX, panelY, panelW, 2.0f), Color(100, 150, 255, 255));

    // Description
    renderer->drawText(m_description.c_str(),
                       {panelX + 10.0f, panelY + 8.0f}, 14, Color(180, 180, 200, 255));

    // Text input field
    float fieldY = panelY + 30.0f;
    renderer->drawRectangle(Rect(panelX + 10.0f, fieldY, panelW - 20.0f, 24.0f),
                       Color(10, 10, 20, 255));
    std::string displayText = m_text + "_";
    renderer->drawText(displayText.c_str(),
                       {panelX + 14.0f, fieldY + 4.0f}, 16, Color::White());

    // Key grid
    float keySize = 40.0f;
    float keyPad = 4.0f;
    float gridStartX = panelX + (panelW - COLS * (keySize + keyPad)) / 2.0f;
    float gridStartY = fieldY + 36.0f;

    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            char ch = getKeyAt(row, col);
            if (ch == ' ' && row != 4) continue; // Skip padding

            float kx = gridStartX + col * (keySize + keyPad);
            float ky = gridStartY + row * (keySize + keyPad);

            bool selected = (row == m_cursorRow && col == m_cursorCol);
            Color bgColor = selected ? Color(100, 150, 255, 255) : Color(50, 50, 70, 255);
            Color textColor = selected ? Color::White() : Color(200, 200, 220, 255);

            renderer->drawRectangle(Rect(kx, ky, keySize, keySize), bgColor);

            // Label
            std::string label;
            if (row == 4) {
                if (col == 0) label = "SPC";
                else if (col == 1) label = "DEL";
                else if (col == 2) label = "SH";
                else if (col == 3) label = "OK";
                else continue;
            } else {
                label = std::string(1, ch);
            }

            float textX = kx + (keySize - label.size() * 7.0f) / 2.0f;
            float textY = ky + (keySize - 14.0f) / 2.0f;
            renderer->drawText(label.c_str(), {textX, textY}, 14, textColor);
        }
    }

    // Button hints
    renderer->drawText("[A] Type  [B] Delete  [Y] Shift  [Start] Confirm  [Select] Cancel",
                       {panelX + 10.0f, panelY + panelH - 20.0f}, 12,
                       Color(120, 120, 140, 255));
}

void OnScreenKeyboard::dismiss() {
    m_visible = false;
    if (m_callback) {
        m_callback("");
        m_callback = nullptr;
    }
}

void OnScreenKeyboard::confirm() {
    m_visible = false;
    if (m_callback) {
        auto cb = std::move(m_callback);
        m_callback = nullptr;
        cb(m_text);
    }
}

void OnScreenKeyboard::moveCursor(int dx, int dy) {
    m_cursorRow = std::clamp(m_cursorRow + dy, 0, ROWS - 1);

    int maxCol = (m_cursorRow == 4) ? 3 : COLS - 1;
    m_cursorCol = std::clamp(m_cursorCol + dx, 0, maxCol);
}

void OnScreenKeyboard::pressSelectedKey() {
    if (m_cursorRow == 4) {
        // Bottom row special keys
        switch (m_cursorCol) {
            case 0: // Space
                if (static_cast<int>(m_text.size()) < m_maxChars) m_text += ' ';
                break;
            case 1: // Backspace
                if (!m_text.empty()) m_text.pop_back();
                break;
            case 2: // Shift
                m_shiftActive = !m_shiftActive;
                break;
            case 3: // Enter/confirm
                confirm();
                break;
        }
        return;
    }

    char ch = getKeyAt(m_cursorRow, m_cursorCol);
    if (ch != '\0' && static_cast<int>(m_text.size()) < m_maxChars) {
        m_text += ch;
    }
}

char OnScreenKeyboard::getKeyAt(int row, int col) const {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) return '\0';
    char ch = LAYOUT[row][col];
    if (m_shiftActive && ch >= 'a' && ch <= 'z') {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return ch;
}

} // namespace gloaming
