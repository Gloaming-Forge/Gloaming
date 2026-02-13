#include "engine/Profiler.hpp"
#include "engine/Log.hpp"

#include <algorithm>

namespace gloaming {

// =============================================================================
// ScopedZone — stores const char* to avoid per-frame allocation
// =============================================================================

Profiler::ScopedZone::ScopedZone(Profiler& profiler, const char* name)
    : m_profiler(profiler), m_name(name) {
    m_profiler.beginZone(m_name);
}

Profiler::ScopedZone::~ScopedZone() {
    m_profiler.endZone(m_name);
}

// =============================================================================
// Profiler
// =============================================================================

Profiler::Profiler() {
    m_history.resize(kHistorySize, 0.0f);
    m_activeZones.reserve(8); // Typical engine has < 10 zones
}

void Profiler::beginFrame() {
    if (!m_enabled) return;
    m_frameStart = Clock::now();
}

void Profiler::endFrame() {
    if (!m_enabled) return;

    auto now = Clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(now - m_frameStart).count();
    m_frameTimeMs = elapsed;
    ++m_frameCount;

    // Exponential moving average
    if (m_frameCount == 1) {
        m_avgFrameTimeMs = elapsed;
    } else {
        constexpr double smoothing = 0.05;
        m_avgFrameTimeMs = m_avgFrameTimeMs * (1.0 - smoothing) + elapsed * smoothing;
    }

    m_minFrameTimeMs = std::min(m_minFrameTimeMs, elapsed);
    m_maxFrameTimeMs = std::max(m_maxFrameTimeMs, elapsed);

    // Record in ring buffer
    m_history[m_historyIndex] = static_cast<float>(elapsed);
    m_historyIndex = (m_historyIndex + 1) % kHistorySize;
}

void Profiler::beginZone(const std::string& name) {
    if (!m_enabled) return;

    // Warn on overlapping zone with the same name (Issue #4)
    for (const auto& active : m_activeZones) {
        if (active.name == name) {
            LOG_WARN("Profiler: overlapping beginZone('{}') — previous measurement will be lost", name);
            break;
        }
    }

    // Check if zone already active (overlap) — update start time
    for (auto& active : m_activeZones) {
        if (active.name == name) {
            active.start = Clock::now();
            return;
        }
    }

    m_activeZones.push_back(ActiveZone{name, Clock::now()});
}

void Profiler::endZone(const std::string& name) {
    if (!m_enabled) return;

    // Linear search through small vector (typically < 10 entries)
    for (auto it = m_activeZones.begin(); it != m_activeZones.end(); ++it) {
        if (it->name == name) {
            auto now = Clock::now();
            double elapsed = std::chrono::duration<double, std::milli>(now - it->start).count();

            // Swap-and-pop for O(1) removal
            if (it != m_activeZones.end() - 1) {
                *it = std::move(m_activeZones.back());
            }
            m_activeZones.pop_back();

            auto& stats = getOrCreateZone(name);
            stats.lastTimeMs = elapsed;
            stats.sampleCount++;

            if (stats.sampleCount == 1) {
                stats.avgTimeMs = elapsed;
            } else {
                stats.avgTimeMs = stats.avgTimeMs * (1.0 - ProfileZoneStats::kSmoothing)
                                + elapsed * ProfileZoneStats::kSmoothing;
            }

            stats.minTimeMs = std::min(stats.minTimeMs, elapsed);
            stats.maxTimeMs = std::max(stats.maxTimeMs, elapsed);
            return;
        }
    }

    // Zone was never started — ignore silently
}

Profiler::ScopedZone Profiler::scopedZone(const char* name) {
    return ScopedZone(*this, name);
}

ProfileZoneStats Profiler::getZoneStats(const std::string& name) const {
    auto it = m_zoneIndexMap.find(name);
    if (it != m_zoneIndexMap.end()) {
        return m_zoneStatsVec[it->second];
    }
    return ProfileZoneStats{name};
}

void Profiler::setTargetFPS(int fps) {
    if (fps > 0) {
        m_frameBudgetMs = 1000.0 / static_cast<double>(fps);
    }
}

double Profiler::frameBudgetUsage() const {
    if (m_frameBudgetMs <= 0.0) return 0.0;
    return m_frameTimeMs / m_frameBudgetMs;
}

void Profiler::reset() {
    m_frameTimeMs    = 0.0;
    m_avgFrameTimeMs = 0.0;
    m_minFrameTimeMs = 1e9;
    m_maxFrameTimeMs = 0.0;
    m_frameCount     = 0;

    m_activeZones.clear();
    m_zoneStatsVec.clear();
    m_zoneIndexMap.clear();

    std::fill(m_history.begin(), m_history.end(), 0.0f);
    m_historyIndex = 0;
}

ProfileZoneStats& Profiler::getOrCreateZone(const std::string& name) {
    auto it = m_zoneIndexMap.find(name);
    if (it != m_zoneIndexMap.end()) {
        return m_zoneStatsVec[it->second];
    }

    size_t index = m_zoneStatsVec.size();
    m_zoneIndexMap[name] = index;
    m_zoneStatsVec.push_back(ProfileZoneStats{name});
    return m_zoneStatsVec.back();
}

} // namespace gloaming
