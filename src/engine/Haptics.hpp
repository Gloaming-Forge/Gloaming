#pragma once

#include <vector>

namespace gloaming {

/// Manages gamepad vibration/haptic feedback.
/// Supports timed vibrations that auto-stop, and a global enable/disable toggle.
class Haptics {
public:
    /// Vibrate the gamepad (intensity 0.0–1.0, duration in seconds)
    void vibrate(float leftIntensity, float rightIntensity, float duration,
                 int gamepadId = 0);

    /// Short impulse vibration (e.g., landing, hitting)
    void impulse(float intensity, float durationMs = 100.0f, int gamepadId = 0);

    /// Stop vibration on a specific gamepad
    void stop(int gamepadId = 0);

    /// Update (ticks down active vibrations)
    void update(float dt);

    /// Enable/disable globally (user preference)
    void setEnabled(bool enabled);
    bool isEnabled() const;

    /// Set global intensity multiplier (0.0–1.0)
    void setIntensity(float intensity);
    float getIntensity() const;

private:
    struct ActiveVibration {
        int gamepadId = 0;
        float leftIntensity = 0.0f;
        float rightIntensity = 0.0f;
        float remaining = 0.0f; // seconds remaining
    };

    void applyVibration(int gamepadId, float left, float right);
    void stopVibration(int gamepadId);

    std::vector<ActiveVibration> m_active;
    bool m_enabled = true;
    float m_intensity = 1.0f;
};

} // namespace gloaming
