cmake_minimum_required(VERSION 3.12)
project(JamCore VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set the build directory name to build
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

# Find Windows SDK
if(WIN32)
    # Get Windows SDK include path
    get_filename_component(WINDOWS_SDK_INCLUDE "$ENV{WindowsSdkDir}Include/$ENV{WindowsSDKVersion}" ABSOLUTE)
    if(EXISTS "${WINDOWS_SDK_INCLUDE}")
        include_directories("${WINDOWS_SDK_INCLUDE}/um")
        include_directories("${WINDOWS_SDK_INCLUDE}/shared")
    endif()
endif()

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
