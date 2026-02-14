#include "engine/Time.hpp"

#include <algorithm>
#include <raylib.h>

namespace gloaming {

void Time::update(double rawDeltaTime) {
    m_rawDeltaTime = rawDeltaTime;

    double clampLimit = MAX_DELTA;

    // Apply one-shot clamp if set (e.g., after suspend/resume)
    if (m_nextDeltaClamp > 0.0) {
        clampLimit = m_nextDeltaClamp;
        m_nextDeltaClamp = 0.0;
    }

    m_deltaTime = std::min(rawDeltaTime, clampLimit);
    m_elapsedTime += m_deltaTime;
    m_frameCount++;

    // Smooth FPS using a rolling average over ~60 samples
    m_fpsAccumulator += m_deltaTime;
    m_fpsSamples++;
    if (m_fpsSamples >= 60) {
        m_fps = static_cast<double>(m_fpsSamples) / m_fpsAccumulator;
        m_fpsAccumulator = 0.0;
        m_fpsSamples = 0;
    }
}

void Time::setTargetFPS(int fps) {
    m_targetFPS = std::max(fps, 0);
    SetTargetFPS(m_targetFPS);
}

void Time::clampNextDelta(double maxDelta) {
    m_nextDeltaClamp = maxDelta;
}

} // namespace gloaming
