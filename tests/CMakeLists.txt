cmake_minimum_required(VERSION 3.14)
project(climate_ir_woleix_tests)

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

# Find Google Test - it's installed via apt
find_package(GTest REQUIRED)

# Include directories for ESPHome mocks and component
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks
  ${CMAKE_CURRENT_SOURCE_DIR}/../esphome
  ${CMAKE_CURRENT_SOURCE_DIR}/../esphome/components/climate_ir_woleix
)

# Create mock library for ESPHome dependencies
add_library(esphome_mocks INTERFACE)
target_include_directories(esphome_mocks INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

# Add the component source files
set(COMPONENT_SOURCES
  ../esphome/components/climate_ir_woleix/climate_ir_woleix.cpp
)

# Create test executable
add_executable(
  climate_ir_woleix_test
  climate_ir_woleix_test.cpp
  ${COMPONENT_SOURCES}
)

target_link_libraries(
  climate_ir_woleix_test
  GTest::gtest_main
  GTest::gmock_main
  esphome_mocks
)

# Enable testing
include(GoogleTest)
gtest_discover_tests(climate_ir_woleix_test)
