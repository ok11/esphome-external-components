cmake_minimum_required(VERSION 3.14)
project(esphome_external_components)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable compile_commands.json generation for IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform-specific settings
if(APPLE)
    set(CMAKE_PREFIX_PATH "/opt/homebrew;/usr/local;${CMAKE_PREFIX_PATH}")
endif()

# Component library - climate_ir_woleix
# This ensures component files are included in compile_commands.json
add_library(climate_ir_woleix STATIC
    esphome/components/climate_ir_woleix/climate_ir_woleix.cpp
    esphome/components/climate_ir_woleix/climate_ir_woleix.h
)

# Set include directories for the component
target_include_directories(climate_ir_woleix PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/esphome
    ${CMAKE_CURRENT_SOURCE_DIR}/esphome/components/climate_ir_woleix
)

# Add mock headers as interface library to satisfy component dependencies
# without actual compilation (header-only mocks)
add_library(esphome_core_mocks INTERFACE)
target_include_directories(esphome_core_mocks INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/mocks
)

# Link mocks to the component for header resolution
target_link_libraries(climate_ir_woleix PUBLIC esphome_core_mocks)

# Find Google Test (optional for tests)
find_package(GTest QUIET)

# Enable testing only if GTest is available
if(GTest_FOUND)
    enable_testing()
    add_subdirectory(tests)
    message(STATUS "GTest found - tests will be built")
else()
    message(STATUS "GTest not found - tests will be skipped")
    message(STATUS "To build tests, install GTest: apt-get install libgtest-dev libgmock-dev")
endif()

# Print information about compile_commands.json
message(STATUS "CMAKE_EXPORT_COMPILE_COMMANDS is ON")
message(STATUS "compile_commands.json will be generated at: ${CMAKE_BINARY_DIR}/compile_commands.json")
