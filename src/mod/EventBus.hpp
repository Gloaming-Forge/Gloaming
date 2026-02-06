#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <any>

namespace gloaming {

/// Event data container — wraps arbitrary key-value pairs for Lua interop
class EventData {
public:
    EventData() = default;

    void setString(const std::string& key, const std::string& value) { m_strings[key] = value; }
    void setFloat(const std::string& key, float value) { m_floats[key] = value; }
    void setInt(const std::string& key, int value) { m_ints[key] = value; }
    void setBool(const std::string& key, bool value) { m_bools[key] = value; }

    std::string getString(const std::string& key, const std::string& def = "") const {
        auto it = m_strings.find(key);
        return it != m_strings.end() ? it->second : def;
    }
    float getFloat(const std::string& key, float def = 0.0f) const {
        auto it = m_floats.find(key);
        return it != m_floats.end() ? it->second : def;
    }
    int getInt(const std::string& key, int def = 0) const {
        auto it = m_ints.find(key);
        return it != m_ints.end() ? it->second : def;
    }
    bool getBool(const std::string& key, bool def = false) const {
        auto it = m_bools.find(key);
        return it != m_bools.end() ? it->second : def;
    }

    bool hasString(const std::string& key) const { return m_strings.count(key) > 0; }
    bool hasFloat(const std::string& key) const { return m_floats.count(key) > 0; }
    bool hasInt(const std::string& key) const { return m_ints.count(key) > 0; }
    bool hasBool(const std::string& key) const { return m_bools.count(key) > 0; }

private:
    std::unordered_map<std::string, std::string> m_strings;
    std::unordered_map<std::string, float> m_floats;
    std::unordered_map<std::string, int> m_ints;
    std::unordered_map<std::string, bool> m_bools;
};

/// Handler ID for unsubscribing
using EventHandlerId = uint64_t;

/// Event handler callback — returns true to cancel the event (prevent further handlers)
using EventHandler = std::function<bool(const EventData&)>;

/// Event bus for loose-coupled mod communication
class EventBus {
public:
    EventBus() = default;

    /// Subscribe to an event. Lower priority = called first.
    /// Returns a handler ID for later unsubscription.
    EventHandlerId on(const std::string& eventName, EventHandler handler, int priority = 0) {
        EventHandlerId id = m_nextId++;
        m_handlers[eventName].push_back({id, priority, std::move(handler)});
        sortHandlers(eventName);
        return id;
    }

    /// Unsubscribe a handler by ID
    bool off(EventHandlerId id) {
        for (auto& [name, handlers] : m_handlers) {
            auto it = std::find_if(handlers.begin(), handlers.end(),
                [id](const HandlerEntry& entry) { return entry.id == id; });
            if (it != handlers.end()) {
                handlers.erase(it);
                return true;
            }
        }
        return false;
    }

    /// Unsubscribe all handlers for an event
    void offAll(const std::string& eventName) {
        m_handlers.erase(eventName);
    }

    /// Emit an event, calling all handlers in priority order.
    /// Returns true if the event was cancelled by a handler.
    bool emit(const std::string& eventName, const EventData& data = {}) {
        auto it = m_handlers.find(eventName);
        if (it == m_handlers.end()) return false;

        // Copy handlers to allow safe modification during iteration
        auto handlers = it->second;
        for (const auto& handler : handlers) {
            if (handler.callback(data)) {
                return true;  // Event cancelled
            }
        }
        return false;
    }

    /// Get the number of handlers for an event
    size_t handlerCount(const std::string& eventName) const {
        auto it = m_handlers.find(eventName);
        return it != m_handlers.end() ? it->second.size() : 0;
    }

    /// Clear all handlers
    void clear() {
        m_handlers.clear();
    }

private:
    struct HandlerEntry {
        EventHandlerId id;
        int priority;
        EventHandler callback;
    };

    void sortHandlers(const std::string& eventName) {
        auto& handlers = m_handlers[eventName];
        std::stable_sort(handlers.begin(), handlers.end(),
            [](const HandlerEntry& a, const HandlerEntry& b) {
                return a.priority < b.priority;
            });
    }

    std::unordered_map<std::string, std::vector<HandlerEntry>> m_handlers;
    EventHandlerId m_nextId = 1;
};

} // namespace gloaming
