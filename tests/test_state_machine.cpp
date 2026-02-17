#include <gtest/gtest.h>
#include "gameplay/StateMachine.hpp"

using namespace gloaming;

// =============================================================================
// StateMachine Component Tests
// =============================================================================

class StateMachineTest : public ::testing::Test {
protected:
    StateMachine fsm;
    Entity testEntity = static_cast<Entity>(42);

    void addBasicStates() {
        fsm.addState("idle", {});
        fsm.addState("walk", {});
        fsm.addState("attack", {});
    }
};

TEST_F(StateMachineTest, InitialState) {
    EXPECT_TRUE(fsm.getCurrentState().empty());
    EXPECT_FLOAT_EQ(fsm.getStateTime(), 0.0f);
}

TEST_F(StateMachineTest, AddState) {
    fsm.addState("idle", {});
    EXPECT_TRUE(fsm.hasState("idle"));
    EXPECT_FALSE(fsm.hasState("walk"));
}

TEST_F(StateMachineTest, AddMultipleStates) {
    addBasicStates();
    EXPECT_TRUE(fsm.hasState("idle"));
    EXPECT_TRUE(fsm.hasState("walk"));
    EXPECT_TRUE(fsm.hasState("attack"));
}

TEST_F(StateMachineTest, HasStateNonExistent) {
    EXPECT_FALSE(fsm.hasState("nonexistent"));
}

TEST_F(StateMachineTest, ReplaceState) {
    int callCount = 0;
    fsm.addState("idle", {.onEnter = [&](Entity) { callCount = 1; }});
    fsm.addState("idle", {.onEnter = [&](Entity) { callCount = 2; }});

    StateMachineSystem::setState(fsm, testEntity, "idle");
    EXPECT_EQ(callCount, 2); // Second definition should be used
}

// =============================================================================
// State Transitions
// =============================================================================

TEST_F(StateMachineTest, SetState) {
    addBasicStates();
    StateMachineSystem::setState(fsm, testEntity, "idle");
    EXPECT_EQ(fsm.getCurrentState(), "idle");
}

TEST_F(StateMachineTest, SetStateNonExistent) {
    addBasicStates();
    StateMachineSystem::setState(fsm, testEntity, "idle");
    StateMachineSystem::setState(fsm, testEntity, "nonexistent");
    // Should remain in "idle" because "nonexistent" doesn't exist
    EXPECT_EQ(fsm.getCurrentState(), "idle");
}

TEST_F(StateMachineTest, SetStateSameStateNoOp) {
    int enterCount = 0;
    fsm.addState("idle", {.onEnter = [&](Entity) { enterCount++; }});

    StateMachineSystem::setState(fsm, testEntity, "idle");
    EXPECT_EQ(enterCount, 1);

    // Setting to same state should be a no-op
    StateMachineSystem::setState(fsm, testEntity, "idle");
    EXPECT_EQ(enterCount, 1);
}

TEST_F(StateMachineTest, StateTimeResets) {
    addBasicStates();
    StateMachineSystem::setState(fsm, testEntity, "idle");
    fsm.stateTime = 5.0f; // Simulate some time passing

    StateMachineSystem::setState(fsm, testEntity, "walk");
    EXPECT_FLOAT_EQ(fsm.getStateTime(), 0.0f);
}

TEST_F(StateMachineTest, PreviousStateTracked) {
    addBasicStates();
    StateMachineSystem::setState(fsm, testEntity, "idle");
    StateMachineSystem::setState(fsm, testEntity, "walk");
    EXPECT_EQ(fsm.previousState, "idle");
    EXPECT_EQ(fsm.getCurrentState(), "walk");
}

TEST_F(StateMachineTest, MultiplePreviousStates) {
    addBasicStates();
    StateMachineSystem::setState(fsm, testEntity, "idle");
    StateMachineSystem::setState(fsm, testEntity, "walk");
    StateMachineSystem::setState(fsm, testEntity, "attack");
    EXPECT_EQ(fsm.previousState, "walk");
    EXPECT_EQ(fsm.getCurrentState(), "attack");
}

// =============================================================================
// State Callbacks
// =============================================================================

TEST_F(StateMachineTest, OnEnterCalled) {
    bool entered = false;
    fsm.addState("idle", {.onEnter = [&](Entity e) {
        entered = true;
        EXPECT_EQ(e, testEntity);
    }});

    StateMachineSystem::setState(fsm, testEntity, "idle");
    EXPECT_TRUE(entered);
}

TEST_F(StateMachineTest, OnExitCalled) {
    bool exited = false;
    fsm.addState("idle", {.onExit = [&](Entity e) {
        exited = true;
        EXPECT_EQ(e, testEntity);
    }});
    fsm.addState("walk", {});

    StateMachineSystem::setState(fsm, testEntity, "idle");
    EXPECT_FALSE(exited);

    StateMachineSystem::setState(fsm, testEntity, "walk");
    EXPECT_TRUE(exited);
}

TEST_F(StateMachineTest, TransitionCallbackOrder) {
    std::vector<std::string> callOrder;

    fsm.addState("idle", {
        .onExit = [&](Entity) { callOrder.push_back("idle_exit"); }
    });
    fsm.addState("walk", {
        .onEnter = [&](Entity) { callOrder.push_back("walk_enter"); }
    });

    StateMachineSystem::setState(fsm, testEntity, "idle");
    StateMachineSystem::setState(fsm, testEntity, "walk");

    ASSERT_EQ(callOrder.size(), 2u);
    EXPECT_EQ(callOrder[0], "idle_exit");
    EXPECT_EQ(callOrder[1], "walk_enter");
}

TEST_F(StateMachineTest, NullCallbacksSafe) {
    fsm.addState("idle", {}); // No callbacks
    fsm.addState("walk", {}); // No callbacks

    // Should not crash
    EXPECT_NO_THROW(StateMachineSystem::setState(fsm, testEntity, "idle"));
    EXPECT_NO_THROW(StateMachineSystem::setState(fsm, testEntity, "walk"));
}

TEST_F(StateMachineTest, PartialCallbacksSafe) {
    // Only onEnter, no onExit or onUpdate
    fsm.addState("idle", {.onEnter = [](Entity) {}});
    fsm.addState("walk", {.onExit = [](Entity) {}});

    EXPECT_NO_THROW(StateMachineSystem::setState(fsm, testEntity, "idle"));
    EXPECT_NO_THROW(StateMachineSystem::setState(fsm, testEntity, "walk"));
}

// =============================================================================
// Initial Empty State Transitions
// =============================================================================

TEST_F(StateMachineTest, TransitionFromEmptyState) {
    // Starting from empty current state
    fsm.addState("idle", {});
    StateMachineSystem::setState(fsm, testEntity, "idle");
    EXPECT_EQ(fsm.getCurrentState(), "idle");
    EXPECT_TRUE(fsm.previousState.empty());
}
