#pragma once

#include "ecs/Entity.hpp"

#include <functional>
#include <vector>
#include <cstdint>

namespace gloaming {

// Forward declarations
class Registry;

/// Unique identifier for a timer
using TimerId = uint32_t;

/// Invalid timer ID sentinel
constexpr TimerId InvalidTimerId = 0;

/// A scheduled timer entry
struct TimerEntry {
    TimerId id = InvalidTimerId;
    float delay = 0.0f;          // Time between firings (for repeating) or total delay (for one-shot)
    float remaining = 0.0f;      // Time remaining until next firing
    std::function<void()> callback;
    bool repeating = false;      // True for timer.every(), false for timer.after()
    bool cancelled = false;      // Marked for removal
    Entity entity = NullEntity;  // If not NullEntity, auto-cancel when entity is destroyed
    bool paused = false;         // Paused timers don't tick
};

/// Timer / Scheduler system.
///
/// Provides delayed and repeating callbacks for mods:
///   timer.after(seconds, callback)      — one-shot delayed call
///   timer.every(seconds, callback)      — repeating call
///   timer.cancel(id)                    — cancel a pending timer
///   timer.after_for(entity, secs, cb)   — entity-scoped one-shot
///   timer.every_for(entity, secs, cb)   — entity-scoped repeating
///
/// Timers are paused when the game is paused.
/// Entity-scoped timers auto-cancel when their entity is destroyed.
class TimerSystem {
public:
    TimerSystem() = default;

    /// Schedule a one-shot timer
    /// @param delay Seconds until the callback fires
    /// @param callback Function to call
    /// @return Timer ID for cancellation
    TimerId after(float delay, std::function<void()> callback);

    /// Schedule a repeating timer
    /// @param interval Seconds between firings
    /// @param callback Function to call each interval
    /// @return Timer ID for cancellation
    TimerId every(float interval, std::function<void()> callback);

    /// Schedule an entity-scoped one-shot timer.
    /// Auto-cancels if the entity is destroyed.
    TimerId afterFor(Entity entity, float delay, std::function<void()> callback);

    /// Schedule an entity-scoped repeating timer.
    /// Auto-cancels if the entity is destroyed.
    TimerId everyFor(Entity entity, float interval, std::function<void()> callback);

    /// Cancel a timer by ID
    /// @return true if the timer existed and was cancelled
    bool cancel(TimerId id);

    /// Cancel all timers scoped to a specific entity
    /// @return number of timers cancelled
    int cancelAllForEntity(Entity entity);

    /// Update all timers. Call once per frame.
    /// @param dt Delta time in seconds
    /// @param registry ECS registry for entity validation
    /// @param gamePaused If true, all timers are paused
    void update(float dt, Registry& registry, bool gamePaused = false);

    /// Remove all timers
    void clear();

    /// Get number of active timers
    size_t activeCount() const;

    /// Get total timers ever created (for debugging)
    uint32_t totalCreated() const { return m_nextId - 1; }

    /// Pause/unpause a specific timer
    bool setPaused(TimerId id, bool paused);

private:
    TimerId allocateId();

    std::vector<TimerEntry> m_timers;
    TimerId m_nextId = 1; // Start at 1 so 0 is the invalid sentinel
};

} // namespace gloaming
