#pragma once

#include <cstdint>

namespace gloaming {

class Time {
public:
    /// Call once per frame with the raw frame time.
    void update(double rawDeltaTime);

    /// Seconds elapsed since last frame (clamped).
    double deltaTime() const { return m_deltaTime; }

    /// Total seconds since Time was created.
    double elapsedTime() const { return m_elapsedTime; }

    /// Number of frames since Time was created.
    uint64_t frameCount() const { return m_frameCount; }

    /// Approximate frames per second (smoothed).
    double fps() const { return m_fps; }

private:
    double   m_deltaTime   = 0.0;
    double   m_elapsedTime = 0.0;
    uint64_t m_frameCount  = 0;
    double   m_fps         = 0.0;

    // Simple exponential moving average for FPS
    double m_fpsAccumulator = 0.0;
    int    m_fpsSamples     = 0;

    static constexpr double MAX_DELTA = 0.25; // Clamp to avoid spiral of death
};

} // namespace gloaming
