#include "engine/Haptics.hpp"

#include <raylib.h>
#include <algorithm>
#include <cmath>

namespace gloaming {

void Haptics::vibrate(float leftIntensity, float rightIntensity, float duration,
                      int gamepadId) {
    if (!m_enabled || duration <= 0.0f) return;

    // Remove existing vibration for this gamepad
    stop(gamepadId);

    ActiveVibration vib;
    vib.gamepadId = gamepadId;
    vib.leftIntensity = std::clamp(leftIntensity, 0.0f, 1.0f);
    vib.rightIntensity = std::clamp(rightIntensity, 0.0f, 1.0f);
    vib.remaining = duration;
    m_active.push_back(vib);

    applyVibration(gamepadId,
                   vib.leftIntensity * m_intensity,
                   vib.rightIntensity * m_intensity);
}

void Haptics::impulse(float intensity, float durationMs, int gamepadId) {
    vibrate(intensity, intensity, durationMs / 1000.0f, gamepadId);
}

void Haptics::stop(int gamepadId) {
    m_active.erase(
        std::remove_if(m_active.begin(), m_active.end(),
                       [gamepadId](const ActiveVibration& v) {
                           return v.gamepadId == gamepadId;
                       }),
        m_active.end());
    stopVibration(gamepadId);
}

void Haptics::update(float dt) {
    if (!m_enabled) return;

    for (auto it = m_active.begin(); it != m_active.end(); ) {
        it->remaining -= dt;
        if (it->remaining <= 0.0f) {
            stopVibration(it->gamepadId);
            it = m_active.erase(it);
        } else {
            ++it;
        }
    }
}

void Haptics::setEnabled(bool enabled) {
    m_enabled = enabled;
    if (!enabled) {
        // Stop all active vibrations
        for (auto& vib : m_active) {
            stopVibration(vib.gamepadId);
        }
        m_active.clear();
    }
}

bool Haptics::isEnabled() const {
    return m_enabled;
}

void Haptics::setIntensity(float intensity) {
    m_intensity = std::clamp(intensity, 0.0f, 1.0f);
}

float Haptics::getIntensity() const {
    return m_intensity;
}

void Haptics::applyVibration(int gamepadId, float left, float right) {
    // Raylib doesn't have a direct vibration API, but we can use the platform-specific
    // SetGamepadVibration if available (Raylib 5.5+).
    // For now, use the Raylib function if it exists, otherwise this is a no-op
    // that will be replaced by SteamInput API when Steamworks is integrated.
#if defined(SUPPORT_GAMEPAD_VIBRATION)
    SetGamepadVibration(gamepadId,
                        static_cast<unsigned short>(left * 65535.0f),
                        static_cast<unsigned short>(right * 65535.0f));
#else
    (void)gamepadId;
    (void)left;
    (void)right;
#endif
}

void Haptics::stopVibration(int gamepadId) {
    applyVibration(gamepadId, 0.0f, 0.0f);
}

} // namespace gloaming
