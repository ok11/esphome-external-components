# Unit Tests for Climate IR Woleix Component

This directory contains unit tests for the `climate_ir_woleix` ESPHome component.

## Prerequisites

- `CMake` 3.20 or higher
- Google Test (`GTest`) framework
- C++20 compatible compiler
- `lcov` (for code coverage)

### Installing Dependencies

**macOS (Homebrew):**

```bash
brew install cmake googletest lcov
```

**Ubuntu/Debian:**

```bash
sudo apt-get update
sudo apt-get install cmake libgtest-dev lcov
```

## Running Tests

### Basic Test Execution

To build and run the tests:

```bash
cd tests/unit
./run_tests.sh
```

This will:

1. Create a build directory
2. Configure the project with CMake (with coverage enabled)
3. Build the test executable
4. Run the tests

### Generating Code Coverage Reports

After running the tests, generate a coverage report:

```bash
cd tests/unit
./generate_coverage.sh
```

This will:

1. Capture coverage data using `lcov`
2. Filter out system headers and test files
3. Generate an HTML coverage report
4. Display a coverage summary

The HTML report will be available at: `tests/unit/build/coverage/html/index.html`

To view the report:

```bash
open build/coverage/html/index.html  # macOS
xdg-open build/coverage/html/index.html  # Linux
```

### Manual Build (without coverage)

If you want to build without coverage:

```bash
cd tests/unit
mkdir -p build && cd build
cmake ..
cmake --build .
./climate_ir_woleix_test
```

### Manual Build (with coverage)

To manually build with coverage enabled:

```bash
cd tests/unit
mkdir -p build && cd build
cmake -DENABLE_COVERAGE=ON ..
cmake --build .
./climate_ir_woleix_test
```

## Test Structure

- `climate_ir_woleix_test.cpp` - Main test file containing 19 test cases
- `mocks/` - Mock implementations of ESPHome dependencies
  - `esphome/components/sensor/sensor.h` - Mock sensor with callback support for testing humidity sensors
  - `esphome/components/climate_ir/climate_ir.h` - Mock ClimateIR base class
  - Other mock headers for ESPHome core functionality
- `CMakeLists.txt` - CMake configuration for tests

## Test Coverage

Current test coverage (as of latest run):

- **96.3% line coverage** (103 of 107 lines)
- **95.7% function coverage** (22 of 23 functions)
- **84.1% branch coverage** (37 of 44 branches)

### Test Categories:

1. **Traits Configuration** - Verifies temperature bounds and supported features
2. **Power On/Off** - Tests power state transitions and commands
3. **Temperature Changes** - Tests temperature increase/decrease commands
4. **Mode Changes** - Tests switching between cooling, heating, etc.
5. **Fan Speed Changes** - Tests fan mode transitions
6. **Complex State Transitions** - Tests complete state changes
7. **Humidity Sensor Callbacks** - Tests humidity sensor integration and state updates

## Coverage Configuration

Code coverage is controlled by the `ENABLE_COVERAGE` CMake option:

- When enabled, the compiler is configured with `--coverage -fprofile-arcs -ftest-coverage` flags
- Coverage data (`.gcda` and `.gcno` files) is generated during test execution
- lcov is used to process the coverage data and generate reports
- The `generate_coverage.sh` script handles inconsistent coverage data warnings automatically

## Troubleshooting

### Coverage Not Collected

If coverage data is not being collected, ensure:

1. Tests are built with `-DENABLE_COVERAGE=ON`
2. Tests are actually executed (coverage data is only generated during execution)
3. lcov is installed and available in PATH
4. The compiler supports coverage flags (GCC or Clang)

### Build Errors

If you encounter build errors:

1. Ensure all dependencies are installed
2. Try cleaning the build directory: `rm -rf build`
3. Check that your compiler supports C++20

### Test Failures

If tests fail:

1. Check the test output for specific failure messages
2. Verify that the component source files are in the expected location
3. Ensure mock implementations match the expected interfaces
