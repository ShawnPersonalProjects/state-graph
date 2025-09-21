# C++ State Graph

## Overview

The C++ State Graph project implements a template-based graph structure where each node represents a state and each edge represents the condition to transition to another state. This project is designed to facilitate the modeling of state machines and similar structures using a flexible and extensible graph representation.

## Project Structure

- **include/graph/**: Contains the header files for the graph implementation.
  - **state_graph.hpp**: Declares the `StateGraph` template class, which manages the graph structure, including methods for adding nodes and edges, and loading the graph from configuration files.
  - **node.hpp**: Defines the `Node` class template, representing a state in the graph with properties for the node's identifier and associated data.
  - **edge.hpp**: Defines the `Edge` class template, representing a transition condition between nodes, including properties for the source node, destination node, and the condition for the transition.

- **src/**: Contains the source files for the application.
  - **main.cpp**: The entry point of the application, initializing the graph, loading the configuration from a file, and demonstrating the graph functionality.
  - **state_graph.cpp**: Implements the methods declared in `state_graph.hpp`, including logic for adding nodes and edges and loading the graph from configuration files.
  - **json_config_loader.cpp**: Implements the `JsonConfigLoader` class, responsible for reading graph configuration from JSON files and populating the `StateGraph` instance.
  
- **config/**: Contains sample graph configuration files.
  - **sample_graph.json**: A sample graph configuration in JSON format, defining nodes and edges for the graph.
  - **sample_graph.yaml**: A sample graph configuration in YAML format, defining nodes and edges for the graph.

- **tests/**: Contains unit tests for the project.
  - **test_main.cpp**: The main test runner for the unit tests in the project.
  - **state_graph_tests.cpp**: Unit tests for the `StateGraph` class, verifying the functionality of adding nodes and edges and loading from configuration files.
  - **config_loading_tests.cpp**: Unit tests for the `JsonConfigLoader` class, verifying that the configuration files are loaded correctly into the graph.

- **CMakeLists.txt**: The configuration file for CMake, specifying the build instructions for the project, including source files, include directories, and dependencies.

## Building the Project

To build the project, follow these steps:

1. Ensure you have CMake installed on your system.
2. Navigate to the project root directory.
3. Create a build directory:
   ```
   mkdir build
   cd build
   ```
4. Run CMake to configure the project:
   ```
   cmake ..
   ```
5. Build the project:
   ```
   cmake --build .
   ```

## Running the Application

After building the project, you can run the application by executing the generated binary in the build directory.

## Running Tests

To run the tests, you can use the following command in the build directory:
```
ctest
```

## License

This project is licensed under the MIT License. See the LICENSE file for more details.