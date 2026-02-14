#pragma once

#include <cstdint>

namespace gloaming {

class Time {
public:
    /// Call once per frame with the raw frame time.
    void update(double rawDeltaTime);

    /// Seconds elapsed since last frame (clamped).
    double deltaTime() const { return m_deltaTime; }

    /// Raw (unclamped) delta from the last update.  Useful for detecting
    /// OS-level suspend: a raw delta >> MAX_DELTA indicates the process
    /// was frozen (e.g., Steam Deck sleep).
    double rawDeltaTime() const { return m_rawDeltaTime; }

    /// Total seconds since Time was created.
    double elapsedTime() const { return m_elapsedTime; }

    /// Number of frames since Time was created.
    uint64_t frameCount() const { return m_frameCount; }

    /// Approximate frames per second (smoothed).
    double fps() const { return m_fps; }

    /// Set a target frame rate (0 = uncapped / vsync only).
    /// Note: calls Raylib's SetTargetFPS(), which requires an initialized
    /// Raylib window.  Do not call from unit tests without a window context.
    void setTargetFPS(int fps);

    /// Get the current target FPS (0 = uncapped).
    int getTargetFPS() const { return m_targetFPS; }

    /// Force the next frame's delta to be clamped to this value.
    /// Useful after suspend/resume to prevent physics explosions.
    void clampNextDelta(double maxDelta);

private:
    double   m_deltaTime    = 0.0;
    double   m_rawDeltaTime = 0.0;
    double   m_elapsedTime  = 0.0;
    uint64_t m_frameCount  = 0;
    double   m_fps         = 0.0;
    int      m_targetFPS   = 0;

    // Simple exponential moving average for FPS
    double m_fpsAccumulator = 0.0;
    int    m_fpsSamples     = 0;

    // One-shot clamp for the next frame
    double m_nextDeltaClamp = 0.0;

    static constexpr double MAX_DELTA = 0.25; // Clamp to avoid spiral of death
};

} // namespace gloaming
