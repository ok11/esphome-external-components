cmake_minimum_required(VERSION 3.14)
project(esphome_external_components)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable compile_commands.json generation for IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Debug build if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Enable debug symbols and disable optimizations for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
    message(STATUS "Debug build enabled with symbols (-g -O0)")
endif()

# Platform-specific settings
if(APPLE)
    set(CMAKE_PREFIX_PATH "/opt/homebrew;/usr/local;${CMAKE_PREFIX_PATH}")
endif()

# ============================================================================
# External ESPHome Dependencies Configuration
# ============================================================================
# The component needs access to ESPHome core and PlatformIO libraries.
# These paths can be set via:
#   1. .env file in project root (recommended for portability)
#   2. Environment variables: export ESPHOME_PATH=/path/to/esphome
#   3. CMake command line: cmake -DESPHOME_PATH=/path/to/esphome ..
#   4. VS Code settings.json: "cmake.configureSettings"
#   5. Auto-detection from common locations

# Load .env file if it exists (for portable configuration)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.env")
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.env" ENV_CONTENTS)
    foreach(LINE ${ENV_CONTENTS})
        # Skip comments and empty lines
        if(NOT LINE MATCHES "^[ \t]*#" AND NOT LINE MATCHES "^[ \t]*$")
            # Parse KEY=VALUE
            if(LINE MATCHES "^([A-Za-z_][A-Za-z0-9_]*)=(.*)$")
                set(ENV_KEY "${CMAKE_MATCH_1}")
                set(ENV_VALUE "${CMAKE_MATCH_2}")

                # Remove quotes if present
                string(REGEX REPLACE "^['\"]|['\"]$" "" ENV_VALUE "${ENV_VALUE}")

                # Only set if not already defined (command line takes precedence)
                if(NOT DEFINED ${ENV_KEY})
                    set(${ENV_KEY} "${ENV_VALUE}")
                    message(STATUS "Loaded from .env: ${ENV_KEY}=${ENV_VALUE}")
                endif()
            endif()
        endif()
    endforeach()
endif()

# Try to find ESPHome installation path
if(NOT DEFINED ESPHOME_PATH)
    # Check environment variable
    if(DEFINED ENV{ESPHOME_PATH})
        set(ESPHOME_PATH $ENV{ESPHOME_PATH})
        message(STATUS "Using ESPHOME_PATH from environment: ${ESPHOME_PATH}")
    else()
        # Try common locations
        set(POSSIBLE_ESPHOME_PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/../esphome"
            "$ENV{HOME}/esphome"
            "/Users/$ENV{USER}/Library/CloudStorage/OneDrive-SAPSE/workspace/ok11/esphome"
        )
        foreach(PATH ${POSSIBLE_ESPHOME_PATHS})
            if(EXISTS "${PATH}")
                set(ESPHOME_PATH "${PATH}")
                message(STATUS "Auto-detected ESPHOME_PATH: ${ESPHOME_PATH}")
                break()
            endif()
        endforeach()
    endif()
endif()

# Try to find PlatformIO dependencies path
if(NOT DEFINED PLATFORMIO_LIBDEPS_PATH)
    if(DEFINED ENV{PLATFORMIO_LIBDEPS_PATH})
        set(PLATFORMIO_LIBDEPS_PATH $ENV{PLATFORMIO_LIBDEPS_PATH})
        message(STATUS "Using PLATFORMIO_LIBDEPS_PATH from environment: ${PLATFORMIO_LIBDEPS_PATH}")
    elseif(DEFINED ESPHOME_PATH AND EXISTS "${ESPHOME_PATH}/.pio/libdeps")
        # Try to find in ESPHome installation
        file(GLOB PIO_PLATFORMS "${ESPHOME_PATH}/.pio/libdeps/*")
        if(PIO_PLATFORMS)
            list(GET PIO_PLATFORMS 0 FIRST_PLATFORM)
            set(PLATFORMIO_LIBDEPS_PATH "${FIRST_PLATFORM}")
            message(STATUS "Auto-detected PLATFORMIO_LIBDEPS_PATH: ${PLATFORMIO_LIBDEPS_PATH}")
        endif()
    endif()
endif()

# Collect all include paths for external dependencies
set(EXTERNAL_INCLUDE_PATHS "")

if(DEFINED ESPHOME_PATH)
    # Convert to absolute path if relative
    if(NOT IS_ABSOLUTE "${ESPHOME_PATH}")
        get_filename_component(ESPHOME_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${ESPHOME_PATH}" ABSOLUTE)
        message(STATUS "Resolved ESPHOME_PATH to absolute: ${ESPHOME_PATH}")
    endif()

    if(EXISTS "${ESPHOME_PATH}")
        list(APPEND EXTERNAL_INCLUDE_PATHS "${ESPHOME_PATH}")
        message(STATUS "Added ESPHome include path: ${ESPHOME_PATH}")
    else()
        message(WARNING "ESPHOME_PATH is set but does not exist: ${ESPHOME_PATH}")
    endif()
endif()

if(DEFINED PLATFORMIO_LIBDEPS_PATH)
    # Convert to absolute path if relative
    if(NOT IS_ABSOLUTE "${PLATFORMIO_LIBDEPS_PATH}")
        get_filename_component(PLATFORMIO_LIBDEPS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORMIO_LIBDEPS_PATH}" ABSOLUTE)
        message(STATUS "Resolved PLATFORMIO_LIBDEPS_PATH to absolute: ${PLATFORMIO_LIBDEPS_PATH}")
    endif()

    if(EXISTS "${PLATFORMIO_LIBDEPS_PATH}")
        # Add the base libdeps path first
        list(APPEND EXTERNAL_INCLUDE_PATHS "${PLATFORMIO_LIBDEPS_PATH}")

        # Add common PlatformIO libraries
        if(EXISTS "${PLATFORMIO_LIBDEPS_PATH}/ArduinoJson/src")
            list(APPEND EXTERNAL_INCLUDE_PATHS "${PLATFORMIO_LIBDEPS_PATH}/ArduinoJson/src")
            message(STATUS "Added ArduinoJson include path")
        endif()
        # Add other common libraries as needed
        # if(EXISTS "${PLATFORMIO_LIBDEPS_PATH}/OtherLib/src")
        #     list(APPEND EXTERNAL_INCLUDE_PATHS "${PLATFORMIO_LIBDEPS_PATH}/OtherLib/src")
        # endif()
        message(STATUS "Added PlatformIO libraries path: ${PLATFORMIO_LIBDEPS_PATH}")
    else()
        message(WARNING "PLATFORMIO_LIBDEPS_PATH is set but does not exist: ${PLATFORMIO_LIBDEPS_PATH}")
    endif()
endif()

# Warn if external paths not configured
if(NOT DEFINED ESPHOME_PATH)
    message(WARNING "ESPHOME_PATH not set. External ESPHome dependencies won't be available for IntelliSense.")
    message(WARNING "Set via CMake: cmake -DESPHOME_PATH=/path/to/esphome ..")
    message(WARNING "Or via environment: export ESPHOME_PATH=/path/to/esphome")
    message(WARNING "Or via VS Code settings.json: \"cmake.configureSettings\": { \"ESPHOME_PATH\": \"/path/to/esphome\" }")
endif()

# ============================================================================

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
    ${EXTERNAL_INCLUDE_PATHS}  # Add external ESPHome and PlatformIO paths
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
