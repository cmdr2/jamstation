cmake_minimum_required(VERSION 3.12)
project(JamCore VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

add_executable(JamCore)

target_sources(JamCore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graph.h
)

target_link_libraries(JamCore PRIVATE
    ole32
    user32
    Avrt
)

add_subdirectory(tests)
