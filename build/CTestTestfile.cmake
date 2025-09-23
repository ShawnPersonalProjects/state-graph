# CMake generated Testfile for 
# Source directory: C:/Users/Shawn/Dev/Learning/c++/graph/state-graph
# Build directory: C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(state_graph_tests "C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/build/state_graph_tests.exe")
set_tests_properties(state_graph_tests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/CMakeLists.txt;40;add_test;C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/CMakeLists.txt;0;")
add_test(test_value "C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/build/test_value.exe")
set_tests_properties(test_value PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/CMakeLists.txt;45;add_test;C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/CMakeLists.txt;0;")
add_test(test_expression "C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/build/test_expression.exe")
set_tests_properties(test_expression PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/CMakeLists.txt;49;add_test;C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/CMakeLists.txt;0;")
add_test(test_node_edge "C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/build/test_node_edge.exe")
set_tests_properties(test_node_edge PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/CMakeLists.txt;53;add_test;C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/CMakeLists.txt;0;")
add_test(test_phase_edge "C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/build/test_phase_edge.exe")
set_tests_properties(test_phase_edge PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/CMakeLists.txt;57;add_test;C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/CMakeLists.txt;0;")
add_test(test_state_graph "C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/build/test_state_graph.exe")
set_tests_properties(test_state_graph PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/CMakeLists.txt;61;add_test;C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/CMakeLists.txt;0;")
add_test(test_multi_phase_state_graph "C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/build/test_multi_phase_state_graph.exe")
set_tests_properties(test_multi_phase_state_graph PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/CMakeLists.txt;65;add_test;C:/Users/Shawn/Dev/Learning/c++/graph/state-graph/CMakeLists.txt;0;")
subdirs("_deps/googletest-build")
