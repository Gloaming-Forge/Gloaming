#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <cstdint>

namespace gloaming {

/// Per-zone timing statistics.
struct ProfileZoneStats {
    std::string name;
    double lastTimeMs  = 0.0;
    double avgTimeMs   = 0.0;
    double minTimeMs   = 1e9;
    double maxTimeMs   = 0.0;
    uint64_t sampleCount = 0;

    /// Smoothing factor for the exponential moving average (lower = smoother).
    static constexpr double kSmoothing = 0.1;
};

/// Lightweight performance profiler with named zone timing, frame budget
/// tracking, and a ring buffer of recent frame times for graphing.
///
/// Usage:
///   profiler.beginFrame();
///   { auto z = profiler.scopedZone("Physics"); ... }
///   { auto z = profiler.scopedZone("Render");  ... }
///   profiler.endFrame();
class Profiler {
public:
    /// RAII zone guard. Calls endZone() on destruction.
    class ScopedZone {
    public:
        ScopedZone(Profiler& profiler, const std::string& name);
        ~ScopedZone();

        ScopedZone(const ScopedZone&) = delete;
        ScopedZone& operator=(const ScopedZone&) = delete;

    private:
        Profiler& m_profiler;
        std::string m_name;
    };

    Profiler();

    /// Call at the very start of each frame.
    void beginFrame();

    /// Call at the very end of each frame (records total frame time).
    void endFrame();

    /// Start timing a named zone.
    void beginZone(const std::string& name);

    /// Stop timing a named zone and accumulate stats.
    void endZone(const std::string& name);

    /// Create a RAII scoped zone.
    ScopedZone scopedZone(const std::string& name);

    // ---- Query API ----

    /// Get stats for a specific zone (returns empty stats if not found).
    ProfileZoneStats getZoneStats(const std::string& name) const;

    /// Get all zone stats in insertion order.
    const std::vector<ProfileZoneStats>& getAllZoneStats() const { return m_zoneStatsVec; }

    /// Total frame time of the last completed frame (ms).
    double frameTimeMs() const { return m_frameTimeMs; }

    /// Smoothed average frame time (ms).
    double avgFrameTimeMs() const { return m_avgFrameTimeMs; }

    /// Minimum frame time observed (ms).
    double minFrameTimeMs() const { return m_minFrameTimeMs; }

    /// Maximum frame time observed (ms).
    double maxFrameTimeMs() const { return m_maxFrameTimeMs; }

    /// Total frame count since profiler creation.
    uint64_t frameCount() const { return m_frameCount; }

    // ---- Frame budget ----

    /// Set target FPS for budget calculation (default: 60).
    void setTargetFPS(int fps);

    /// Target frame time in ms (e.g. 16.67 for 60 FPS).
    double frameBudgetMs() const { return m_frameBudgetMs; }

    /// Fraction of the frame budget used last frame (0.0–1.0+).
    double frameBudgetUsage() const;

    // ---- Frame history (for graphs) ----

    /// Ring buffer of the last N frame times (ms). Size = kHistorySize.
    static constexpr size_t kHistorySize = 120;
    const std::vector<float>& frameTimeHistory() const { return m_history; }
    size_t historyIndex() const { return m_historyIndex; }

    /// Reset all statistics.
    void reset();

    /// Whether profiling is currently active. When disabled, beginZone/endZone
    /// are no-ops for zero overhead.
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    void toggle() { m_enabled = !m_enabled; }

private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    bool m_enabled = true;

    // Frame-level timing
    TimePoint m_frameStart;
    double m_frameTimeMs    = 0.0;
    double m_avgFrameTimeMs = 0.0;
    double m_minFrameTimeMs = 1e9;
    double m_maxFrameTimeMs = 0.0;
    uint64_t m_frameCount   = 0;

    // Frame budget
    double m_frameBudgetMs = 16.6667; // 60 FPS

    // Per-zone timing
    struct ActiveZone {
        TimePoint start;
    };
    std::unordered_map<std::string, ActiveZone> m_activeZones;

    // Zone statistics — vector for ordered iteration, map for O(1) lookup
    std::vector<ProfileZoneStats> m_zoneStatsVec;
    std::unordered_map<std::string, size_t> m_zoneIndexMap;

    ProfileZoneStats& getOrCreateZone(const std::string& name);

    // Frame time history ring buffer
    std::vector<float> m_history;
    size_t m_historyIndex = 0;
};

} // namespace gloaming
