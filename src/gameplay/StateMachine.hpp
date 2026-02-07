#pragma once

#include "ecs/Systems.hpp"
#include "ecs/Components.hpp"

#include <string>
#include <unordered_map>
#include <functional>

namespace gloaming {

/// Callbacks for a state in the state machine.
/// All callbacks receive the entity and delta time.
struct StateCallbacks {
    std::function<void(Entity)> onEnter;
    std::function<void(Entity, float)> onUpdate;
    std::function<void(Entity)> onExit;
};

/// A finite state machine component for entity behaviors.
/// Each entity can have its own independent state machine with named states.
///
/// Note on scaling: This component stores std::function callbacks and a string map,
/// which makes it heavier than typical ECS components. This is acceptable for
/// moderate numbers of AI entities (dozens to low hundreds). For games with thousands
/// of stateful entities, consider a shared FSM definition table that entities
/// reference by ID instead of storing callbacks per-entity.
///
/// Transition guards (e.g., "can only transition from X to Y") are not built in.
/// Transitions are purely imperative via setState(). Implement guards in your
/// onUpdate callbacks if needed.
///
/// Usage example (from Lua):
///   -- Define states for an NPC
///   fsm.addState(entity, "idle", { onEnter=..., onUpdate=..., onExit=... })
///   fsm.addState(entity, "patrol", { ... })
///   fsm.addState(entity, "chase", { ... })
///   fsm.setState(entity, "idle")
struct StateMachine {
    std::unordered_map<std::string, StateCallbacks> states;
    std::string currentState;
    std::string previousState;
    float stateTime = 0.0f;         // Time spent in current state

    StateMachine() = default;

    /// Add or replace a named state
    void addState(const std::string& name, StateCallbacks callbacks) {
        states[name] = std::move(callbacks);
    }

    /// Check if a state exists
    bool hasState(const std::string& name) const {
        return states.count(name) > 0;
    }

    /// Get current state name
    const std::string& getCurrentState() const { return currentState; }

    /// Get time spent in current state
    float getStateTime() const { return stateTime; }
};

/// System that updates all entities with StateMachine components.
/// Handles state transitions and calls state callbacks.
class StateMachineSystem : public System {
public:
    StateMachineSystem() : System("StateMachineSystem", 5) {}

    void update(float dt) override {
        getRegistry().each<StateMachine>(
            [dt](Entity entity, StateMachine& fsm) {
                if (fsm.currentState.empty()) return;

                auto it = fsm.states.find(fsm.currentState);
                if (it != fsm.states.end() && it->second.onUpdate) {
                    it->second.onUpdate(entity, dt);
                }
                fsm.stateTime += dt;
            }
        );
    }

    /// Transition an entity's state machine to a new state.
    /// Calls onExit on the old state and onEnter on the new state.
    static void setState(StateMachine& fsm, Entity entity, const std::string& newState) {
        if (newState == fsm.currentState) return;
        if (!fsm.hasState(newState)) return;

        // Exit current state
        if (!fsm.currentState.empty()) {
            auto it = fsm.states.find(fsm.currentState);
            if (it != fsm.states.end() && it->second.onExit) {
                it->second.onExit(entity);
            }
        }

        // Transition
        fsm.previousState = fsm.currentState;
        fsm.currentState = newState;
        fsm.stateTime = 0.0f;

        // Enter new state
        auto it = fsm.states.find(newState);
        if (it != fsm.states.end() && it->second.onEnter) {
            it->second.onEnter(entity);
        }
    }
};

} // namespace gloaming
