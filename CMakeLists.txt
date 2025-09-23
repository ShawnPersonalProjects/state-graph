cmake_minimum_required(VERSION 3.15)
project(cpp_state_graph CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Code coverage configuration
option(ENABLE_COVERAGE "Enable code coverage" OFF)
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -g -O0")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    elseif(MSVC)
        # For MSVC, we'll use a different approach
        message(STATUS "Code coverage enabled for MSVC - using debug info")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DEBUG /Od")
    endif()
endif()

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_library(state_graph INTERFACE)
target_include_directories(state_graph INTERFACE include ${nlohmann_json_SOURCE_DIR})

add_executable(state_graph_demo src/main.cpp)
target_link_libraries(state_graph_demo PRIVATE state_graph)

enable_testing()
add_executable(state_graph_tests tests/test_state_graph.cpp)
target_link_libraries(state_graph_tests PRIVATE state_graph)
add_test(NAME state_graph_tests COMMAND state_graph_tests)

# Add Google Test based tests
add_executable(test_value tests/test_value_gtest.cpp)
target_link_libraries(test_value PRIVATE state_graph gtest_main)
add_test(NAME test_value COMMAND test_value)

add_executable(test_expression tests/test_expression_gtest.cpp)
target_link_libraries(test_expression PRIVATE state_graph gtest_main)
add_test(NAME test_expression COMMAND test_expression)

add_executable(test_node_edge tests/test_node_edge_gtest.cpp)
target_link_libraries(test_node_edge PRIVATE state_graph gtest_main)
add_test(NAME test_node_edge COMMAND test_node_edge)

add_executable(test_phase_edge tests/test_phase_edge_gtest.cpp)
target_link_libraries(test_phase_edge PRIVATE state_graph gtest_main)
add_test(NAME test_phase_edge COMMAND test_phase_edge)

add_executable(test_state_graph tests/test_state_graph_gtest.cpp)
target_link_libraries(test_state_graph PRIVATE state_graph gtest_main)
add_test(NAME test_state_graph COMMAND test_state_graph)

add_executable(test_multi_phase_state_graph tests/test_multi_phase_state_graph_gtest.cpp)
target_link_libraries(test_multi_phase_state_graph PRIVATE state_graph gtest_main)
add_test(NAME test_multi_phase_state_graph COMMAND test_multi_phase_state_graph)