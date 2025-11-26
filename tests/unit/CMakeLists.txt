cmake_minimum_required(VERSION 3.20)

# Signal to parent that we're building tests (must be before project())
set(BUILDING_TESTS TRUE PARENT_SCOPE)

project(climate_ir_woleix_unit_tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable debug symbols and disable optimizations for debugging
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
endif()

if(APPLE)
    set(CMAKE_PREFIX_PATH "/opt/homebrew;/usr/local;${CMAKE_PREFIX_PATH}")
endif()

# Find Google Test - it's installed via apt/brew
find_package(GTest REQUIRED)

# Coverage configuration
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Code coverage enabled")
        set(COVERAGE_FLAGS "--coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_FLAGS}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COVERAGE_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COVERAGE_FLAGS}")
    endif()
endif()

# Create mock library for ESPHome dependencies
add_library(esphome_mocks INTERFACE)
target_include_directories(esphome_mocks INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

# Add the component source files
set(COMPONENT_SOURCES
  ../../esphome/components/climate_ir_woleix/climate_ir_woleix.cpp
  ../../esphome/components/climate_ir_woleix/woleix_ac_state_machine.cpp
)

# Create test executable for climate component
add_executable(
  climate_ir_woleix_test
  climate_ir_woleix_test.cpp
  ${COMPONENT_SOURCES}
)

# Create test executable for state machine
add_executable(
  woleix_ac_state_machine_test
  woleix_ac_state_machine_test.cpp
  ../../esphome/components/climate_ir_woleix/woleix_ac_state_machine.cpp
)

# Set include directories with mocks having highest priority
# Use BEFORE PRIVATE to ensure mocks are searched first, before any inherited paths
target_include_directories(
  climate_ir_woleix_test
  BEFORE PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks
  ${CMAKE_CURRENT_SOURCE_DIR}/../../esphome
  ${CMAKE_CURRENT_SOURCE_DIR}/../../esphome/components/climate_ir_woleix
)

target_link_libraries(
  climate_ir_woleix_test
  GTest::gtest_main
  GTest::gmock_main
  esphome_mocks
)

# Set include directories for state machine test
target_include_directories(
  woleix_ac_state_machine_test
  BEFORE PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks
  ${CMAKE_CURRENT_SOURCE_DIR}/../../esphome
  ${CMAKE_CURRENT_SOURCE_DIR}/../../esphome/components/climate_ir_woleix
)

target_link_libraries(
  woleix_ac_state_machine_test
  GTest::gtest_main
  GTest::gmock_main
  esphome_mocks
)

# Enable testing
include(GoogleTest)
gtest_discover_tests(climate_ir_woleix_test)
gtest_discover_tests(woleix_ac_state_machine_test)

