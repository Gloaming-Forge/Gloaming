#include "engine/Time.hpp"

#include <algorithm>

namespace gloaming {

void Time::update(double rawDeltaTime) {
    m_deltaTime = std::min(rawDeltaTime, MAX_DELTA);
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

} // namespace gloaming
