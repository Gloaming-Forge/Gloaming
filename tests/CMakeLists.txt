# GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Test executable
add_executable(gloaming_tests
    test_config.cpp
    test_log.cpp
    test_time.cpp
    test_rendering.cpp
    test_ecs.cpp
    test_world.cpp
    test_physics.cpp
    test_mod.cpp
    test_lighting.cpp
    test_audio.cpp
    test_ui.cpp
    test_gameplay.cpp
    test_entity_spawning.cpp
    test_worldgen.cpp
    test_gameplay_loop.cpp
    test_enemies.cpp
    test_npcs.cpp
    test_scenes_timers_save.cpp
    test_particles_polish.cpp
    test_polish.cpp
    test_input_system.cpp
    test_display_system.cpp
    test_seamlessness.cpp
    test_system_support.cpp
    test_config_persistence.cpp
    test_vec2.cpp
    test_camera.cpp
    test_pathfinding.cpp
    test_state_machine.cpp
    test_grid_movement.cpp
    test_tweens_and_easing.cpp
)

target_link_libraries(gloaming_tests PRIVATE
    gloaming_engine
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(gloaming_tests)
