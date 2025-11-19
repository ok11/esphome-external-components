cmake_minimum_required(VERSION 3.20)
project(climate_ir_woleix_all_tests)

# This is the root CMakeLists.txt for all tests
# It orchestrates unit tests via CMake
# Integration tests are run separately via Docker (see integration/README.md)

message(STATUS "Configuring climate_ir_woleix tests")

# Add unit tests subdirectory
add_subdirectory(unit)

message(STATUS "")
message(STATUS "====================================================================")
message(STATUS "Test Configuration Complete")
message(STATUS "====================================================================")
message(STATUS "Unit Tests: Configured via CMake (in tests/unit/)")
message(STATUS "  To build: cd tests/unit/build && cmake .. && make")
message(STATUS "  To run:   ctest --output-on-failure")
message(STATUS "")
message(STATUS "Integration Tests: Run via Docker (in tests/integration/)")
message(STATUS "  See tests/integration/README.md for instructions")
message(STATUS "  Quick run: cd tests/integration && ./run_tests.sh")
message(STATUS "====================================================================")
