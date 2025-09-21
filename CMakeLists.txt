cmake_minimum_required(VERSION 3.15)
project(cpp_state_graph CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(state_graph INTERFACE)
target_include_directories(state_graph INTERFACE include ${nlohmann_json_SOURCE_DIR})

add_executable(state_graph_demo src/main.cpp)
target_link_libraries(state_graph_demo PRIVATE state_graph)

enable_testing()
add_executable(state_graph_tests tests/test_state_graph.cpp)
target_link_libraries(state_graph_tests PRIVATE state_graph)
add_test(NAME state_graph_tests COMMAND state_graph_tests)