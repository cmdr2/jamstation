cmake_minimum_required(VERSION 3.12)
project(GraphTests)

set(CMAKE_CXX_STANDARD 17)

# Add executable
add_executable(test_graph
    test_graph.cpp
    ../src/graph.cpp
    ../src/graph.h
)

# Include directories
target_include_directories(test_graph PRIVATE ../src)