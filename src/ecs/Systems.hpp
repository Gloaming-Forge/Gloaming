#pragma once

#include "ecs/Registry.hpp"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace gloaming {

// Forward declarations
class Engine;

/// System execution phase
enum class SystemPhase {
    PreUpdate,      // Before main update (input processing, etc.)
    Update,         // Main update (physics, AI, etc.)
    PostUpdate,     // After main update (cleanup, state changes)
    PreRender,      // Before rendering (camera updates, culling)
    Render,         // Main rendering
    PostRender      // After rendering (debug overlays, UI)
};

/// Base class for all ECS systems
class System {
public:
    explicit System(const std::string& name, int priority = 0)
        : m_name(name), m_priority(priority) {}
    virtual ~System() = default;

    // Non-copyable
    System(const System&) = delete;
    System& operator=(const System&) = delete;

    /// Initialize the system (called once when added)
    virtual void init(Registry& registry, Engine& engine) {
        m_registry = &registry;
        m_engine = &engine;
    }

    /// Update the system (called every frame)
    virtual void update(float dt) = 0;

    /// Shutdown the system (called when removed)
    virtual void shutdown() {}

    /// Get system name
    const std::string& getName() const { return m_name; }

    /// Get execution priority (lower = earlier)
    int getPriority() const { return m_priority; }

    /// Enable/disable the system
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

protected:
    Registry& getRegistry() { return *m_registry; }
    const Registry& getRegistry() const { return *m_registry; }
    Engine& getEngine() { return *m_engine; }
    const Engine& getEngine() const { return *m_engine; }

private:
    std::string m_name;
    int m_priority = 0;
    bool m_enabled = true;
    Registry* m_registry = nullptr;
    Engine* m_engine = nullptr;
};

/// System scheduler - manages system registration and execution
class SystemScheduler {
public:
    SystemScheduler() = default;
    ~SystemScheduler() = default;

    // Non-copyable
    SystemScheduler(const SystemScheduler&) = delete;
    SystemScheduler& operator=(const SystemScheduler&) = delete;

    /// Initialize the scheduler
    void init(Registry& registry, Engine& engine) {
        m_registry = &registry;
        m_engine = &engine;
    }

    /// Add a system to a specific phase
    template<typename T, typename... Args>
    T* addSystem(SystemPhase phase, Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = system.get();
        system->init(*m_registry, *m_engine);
        m_phases[phase].push_back(std::move(system));
        sortPhase(phase);
        return ptr;
    }

    /// Add an existing system instance
    void addSystem(SystemPhase phase, std::unique_ptr<System> system) {
        system->init(*m_registry, *m_engine);
        m_phases[phase].push_back(std::move(system));
        sortPhase(phase);
    }

    /// Get a system by type
    template<typename T>
    T* getSystem() {
        for (auto& [phase, systems] : m_phases) {
            for (auto& system : systems) {
                if (T* typed = dynamic_cast<T*>(system.get())) {
                    return typed;
                }
            }
        }
        return nullptr;
    }

    /// Get a system by name
    System* getSystem(const std::string& name) {
        for (auto& [phase, systems] : m_phases) {
            for (auto& system : systems) {
                if (system->getName() == name) {
                    return system.get();
                }
            }
        }
        return nullptr;
    }

    /// Remove a system by name
    bool removeSystem(const std::string& name) {
        for (auto& [phase, systems] : m_phases) {
            auto it = std::find_if(systems.begin(), systems.end(),
                [&name](const auto& sys) { return sys->getName() == name; });
            if (it != systems.end()) {
                (*it)->shutdown();
                systems.erase(it);
                return true;
            }
        }
        return false;
    }

    /// Run all systems in a phase
    void runPhase(SystemPhase phase, float dt) {
        auto it = m_phases.find(phase);
        if (it != m_phases.end()) {
            for (auto& system : it->second) {
                if (system->isEnabled()) {
                    system->update(dt);
                }
            }
        }
    }

    /// Run update phases (PreUpdate, Update, PostUpdate)
    void update(float dt) {
        runPhase(SystemPhase::PreUpdate, dt);
        runPhase(SystemPhase::Update, dt);
        runPhase(SystemPhase::PostUpdate, dt);
    }

    /// Run render phases (PreRender, Render, PostRender)
    void render(float dt) {
        runPhase(SystemPhase::PreRender, dt);
        runPhase(SystemPhase::Render, dt);
        runPhase(SystemPhase::PostRender, dt);
    }

    /// Shutdown all systems
    void shutdown() {
        for (auto& [phase, systems] : m_phases) {
            for (auto& system : systems) {
                system->shutdown();
            }
        }
        m_phases.clear();
    }

    /// Get count of systems in a phase
    size_t getSystemCount(SystemPhase phase) const {
        auto it = m_phases.find(phase);
        return it != m_phases.end() ? it->second.size() : 0;
    }

    /// Get total system count
    size_t getTotalSystemCount() const {
        size_t total = 0;
        for (const auto& [phase, systems] : m_phases) {
            total += systems.size();
        }
        return total;
    }

    /// Enable/disable a system by name
    void setSystemEnabled(const std::string& name, bool enabled) {
        if (System* sys = getSystem(name)) {
            sys->setEnabled(enabled);
        }
    }

private:
    void sortPhase(SystemPhase phase) {
        auto& systems = m_phases[phase];
        std::sort(systems.begin(), systems.end(),
            [](const auto& a, const auto& b) {
                return a->getPriority() < b->getPriority();
            });
    }

    std::unordered_map<SystemPhase, std::vector<std::unique_ptr<System>>> m_phases;
    Registry* m_registry = nullptr;
    Engine* m_engine = nullptr;
};

} // namespace gloaming
