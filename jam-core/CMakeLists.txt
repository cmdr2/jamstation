cmake_minimum_required(VERSION 3.12)
project(JamCore VERSION 1.0.0)

# warn if JUCE_HOME environment variable is not set
if(NOT DEFINED ENV{JUCE_HOME})
    message(FATAL_ERROR "Please set the JUCE_HOME environment variable to point to your JUCE installation.")
endif()

add_subdirectory($ENV{JUCE_HOME} "juce")

# set the build directory name to build
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

juce_add_gui_app(JamCore
    PRODUCT_NAME "JamCore"
)

target_sources(JamCore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MainComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MainComponent.cpp
)

target_link_libraries(JamCore PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_processors_headless
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

juce_generate_juce_header(JamCore)
