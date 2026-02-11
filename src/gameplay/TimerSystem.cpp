#include "gameplay/TimerSystem.hpp"
#include "ecs/Registry.hpp"
#include "engine/Log.hpp"

#include <algorithm>

namespace gloaming {

TimerId TimerSystem::after(float delay, std::function<void()> callback) {
    TimerEntry entry;
    entry.id = allocateId();
    entry.delay = delay;
    entry.remaining = delay;
    entry.callback = std::move(callback);
    entry.repeating = false;
    m_timers.push_back(std::move(entry));
    return m_timers.back().id;
}

TimerId TimerSystem::every(float interval, std::function<void()> callback) {
    if (interval <= 0.0f) {
        LOG_WARN("TimerSystem::every: interval must be > 0");
        return InvalidTimerId;
    }
    TimerEntry entry;
    entry.id = allocateId();
    entry.delay = interval;
    entry.remaining = interval;
    entry.callback = std::move(callback);
    entry.repeating = true;
    m_timers.push_back(std::move(entry));
    return m_timers.back().id;
}

TimerId TimerSystem::afterFor(Entity entity, float delay, std::function<void()> callback) {
    TimerEntry entry;
    entry.id = allocateId();
    entry.delay = delay;
    entry.remaining = delay;
    entry.callback = std::move(callback);
    entry.repeating = false;
    entry.entity = entity;
    m_timers.push_back(std::move(entry));
    return m_timers.back().id;
}

TimerId TimerSystem::everyFor(Entity entity, float interval, std::function<void()> callback) {
    if (interval <= 0.0f) {
        LOG_WARN("TimerSystem::everyFor: interval must be > 0");
        return InvalidTimerId;
    }
    TimerEntry entry;
    entry.id = allocateId();
    entry.delay = interval;
    entry.remaining = interval;
    entry.callback = std::move(callback);
    entry.repeating = true;
    entry.entity = entity;
    m_timers.push_back(std::move(entry));
    return m_timers.back().id;
}

bool TimerSystem::cancel(TimerId id) {
    for (auto& timer : m_timers) {
        if (timer.id == id && !timer.cancelled) {
            timer.cancelled = true;
            return true;
        }
    }
    return false;
}

int TimerSystem::cancelAllForEntity(Entity entity) {
    int count = 0;
    for (auto& timer : m_timers) {
        if (timer.entity == entity && !timer.cancelled) {
            timer.cancelled = true;
            ++count;
        }
    }
    return count;
}

void TimerSystem::update(float dt, Registry& registry, bool gamePaused) {
    // First pass: auto-cancel timers whose entities no longer exist
    for (auto& timer : m_timers) {
        if (!timer.cancelled && timer.entity != NullEntity) {
            if (!registry.valid(timer.entity)) {
                timer.cancelled = true;
            }
        }
    }

    // Second pass: tick timers and fire callbacks
    for (auto& timer : m_timers) {
        if (timer.cancelled) continue;
        if (gamePaused || timer.paused) continue;

        timer.remaining -= dt;

        if (timer.remaining <= 0.0f) {
            // Fire callback
            if (timer.callback) {
                try {
                    timer.callback();
                } catch (const std::exception& ex) {
                    LOG_ERROR("Timer {} callback error: {}", timer.id, ex.what());
                }
            }

            if (timer.repeating) {
                // Reset for next interval, accounting for overshoot
                timer.remaining += timer.delay;
                // Prevent runaway if dt >> delay
                if (timer.remaining <= 0.0f) {
                    timer.remaining = timer.delay;
                }
            } else {
                timer.cancelled = true; // One-shot: mark for removal
            }
        }
    }

    // Third pass: remove cancelled timers
    m_timers.erase(
        std::remove_if(m_timers.begin(), m_timers.end(),
            [](const TimerEntry& t) { return t.cancelled; }),
        m_timers.end());
}

void TimerSystem::clear() {
    m_timers.clear();
}

size_t TimerSystem::activeCount() const {
    size_t count = 0;
    for (const auto& timer : m_timers) {
        if (!timer.cancelled) ++count;
    }
    return count;
}

bool TimerSystem::setPaused(TimerId id, bool paused) {
    for (auto& timer : m_timers) {
        if (timer.id == id && !timer.cancelled) {
            timer.paused = paused;
            return true;
        }
    }
    return false;
}

TimerId TimerSystem::allocateId() {
    return m_nextId++;
}

} // namespace gloaming
