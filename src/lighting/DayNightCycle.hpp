#pragma once

#include "lighting/LightMap.hpp"
#include <cmath>

namespace gloaming {

/// Time-of-day phases
enum class TimeOfDay {
    Dawn,       // Sunrise transition
    Day,        // Full daylight
    Dusk,       // Sunset transition
    Night       // Full night
};

/// Configuration for the day/night cycle
struct DayNightConfig {
    float dayDurationSeconds = 600.0f;  // Total cycle length (10 minutes default)
    float dawnStart = 0.20f;            // Fraction of day when dawn begins
    float dayStart = 0.30f;             // Fraction of day when full day begins
    float duskStart = 0.70f;            // Fraction of day when dusk begins
    float nightStart = 0.80f;           // Fraction of day when full night begins

    TileLight dayColor{255, 255, 240};      // Warm white daylight
    TileLight dawnColor{200, 150, 100};     // Orange-ish sunrise
    TileLight duskColor{180, 120, 80};      // Red-orange sunset
    TileLight nightColor{20, 20, 50};       // Deep blue night
};

/// Manages the day/night cycle and computes ambient sky light
class DayNightCycle {
public:
    DayNightCycle() = default;
    explicit DayNightCycle(const DayNightConfig& config) : m_config(config) {}

    /// Set configuration
    void setConfig(const DayNightConfig& config) { m_config = config; }
    const DayNightConfig& getConfig() const { return m_config; }

    /// Update the cycle
    /// @param dt Delta time in seconds
    void update(float dt) {
        if (m_config.dayDurationSeconds <= 0.0f) return;
        m_time += dt;
        while (m_time >= m_config.dayDurationSeconds) {
            m_time -= m_config.dayDurationSeconds;
            ++m_dayCount;
        }
    }

    /// Get the current normalized time (0.0 to 1.0 representing the full cycle)
    float getNormalizedTime() const {
        return m_config.dayDurationSeconds > 0.0f
            ? m_time / m_config.dayDurationSeconds
            : 0.0f;
    }

    /// Get the current time of day phase
    TimeOfDay getTimeOfDay() const {
        float t = getNormalizedTime();
        if (t < m_config.dawnStart || t >= m_config.nightStart) return TimeOfDay::Night;
        if (t < m_config.dayStart) return TimeOfDay::Dawn;
        if (t < m_config.duskStart) return TimeOfDay::Day;
        return TimeOfDay::Dusk;
    }

    /// Get the current ambient sky light color
    TileLight getSkyColor() const {
        float t = getNormalizedTime();

        if (t < m_config.dawnStart) {
            // Night
            return m_config.nightColor;
        }
        if (t < m_config.dayStart) {
            // Dawn: interpolate night → dawn → day
            float progress = (t - m_config.dawnStart) / (m_config.dayStart - m_config.dawnStart);
            if (progress < 0.5f) {
                // Night → Dawn
                float p = progress * 2.0f;
                return lerpColor(m_config.nightColor, m_config.dawnColor, p);
            } else {
                // Dawn → Day
                float p = (progress - 0.5f) * 2.0f;
                return lerpColor(m_config.dawnColor, m_config.dayColor, p);
            }
        }
        if (t < m_config.duskStart) {
            // Day
            return m_config.dayColor;
        }
        if (t < m_config.nightStart) {
            // Dusk: interpolate day → dusk → night
            float progress = (t - m_config.duskStart) / (m_config.nightStart - m_config.duskStart);
            if (progress < 0.5f) {
                // Day → Dusk
                float p = progress * 2.0f;
                return lerpColor(m_config.dayColor, m_config.duskColor, p);
            } else {
                // Dusk → Night
                float p = (progress - 0.5f) * 2.0f;
                return lerpColor(m_config.duskColor, m_config.nightColor, p);
            }
        }
        // Night
        return m_config.nightColor;
    }

    /// Get the sky brightness factor (0.0 = night, 1.0 = full day)
    float getSkyBrightness() const {
        TileLight sky = getSkyColor();
        return static_cast<float>(sky.maxChannel()) / 255.0f;
    }

    /// Set the time directly (0.0 to dayDuration)
    void setTime(float time) {
        m_time = std::fmod(time, m_config.dayDurationSeconds);
        if (m_time < 0.0f) m_time += m_config.dayDurationSeconds;
    }

    /// Set normalized time (0.0 to 1.0)
    void setNormalizedTime(float t) {
        m_time = std::fmod(t, 1.0f) * m_config.dayDurationSeconds;
    }

    /// Get absolute time in seconds
    float getTime() const { return m_time; }

    /// Get number of completed days
    int getDayCount() const { return m_dayCount; }

    /// Check if it's currently nighttime
    bool isNight() const { return getTimeOfDay() == TimeOfDay::Night; }

    /// Check if it's currently daytime
    bool isDay() const { return getTimeOfDay() == TimeOfDay::Day; }

private:
    static TileLight lerpColor(const TileLight& a, const TileLight& b, float t) {
        t = std::max(0.0f, std::min(1.0f, t));
        return TileLight(
            static_cast<uint8_t>(static_cast<float>(a.r) + (static_cast<float>(b.r) - static_cast<float>(a.r)) * t),
            static_cast<uint8_t>(static_cast<float>(a.g) + (static_cast<float>(b.g) - static_cast<float>(a.g)) * t),
            static_cast<uint8_t>(static_cast<float>(a.b) + (static_cast<float>(b.b) - static_cast<float>(a.b)) * t)
        );
    }

    DayNightConfig m_config;
    float m_time = 0.0f;       // Current time in seconds
    int m_dayCount = 0;        // Number of completed day cycles
};

} // namespace gloaming
