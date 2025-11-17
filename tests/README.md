# Unit Tests for Woleix Climate IR Component

This directory contains unit tests for the `climate_ir_woleix` ESPHome external component using Google Test framework.

## Overview

The tests validate the IR command encoding logic and state machine behavior without requiring actual hardware. This allows for:

- Fast development iteration
- Regression testing
- Verification of IR timing accuracy
- State transition validation

## What's Being Tested

### Core Functionality

- **Power On/Off**: Verifies correct power command transmission
- **Temperature Control**: Tests temperature increase/decrease commands
- **Mode Changes**: Validates mode switching (cool, heat, etc.)
- **Fan Speed**: Tests fan speed changes
- **State Persistence**: Ensures component remembers last state to minimize IR commands

### IR Protocol Validation

- Correct timing sequences for each command
- Proper carrier frequency (38.03 kHz)
- Accurate mark/space durations

## Project Structure

```text
tests/
├── CMakeLists.txt                    # Build configuration
├── climate_ir_woleix_test.cpp        # Test cases
├── run_tests.sh                      # Build and run script
└── mocks/                            # ESPHome API mocks
    └── esphome/
        ├── core/
        │   ├── log.h                 # Logging mock
        │   ├── hal.h                 # Hardware abstraction mock
        │   └── optional.h            # Optional type mock
        └── components/
            ├── climate/
            │   └── climate_mode.h    # Climate enums and types
            └── climate_ir/
                └── climate_ir.h      # ClimateIR base class mock
```

## Prerequisites

- CMake 3.14 or higher
- C++17 compatible compiler (GCC, Clang, or MSVC)
- Internet connection (for first build to download Google Test)

## Building and Running

### Quick Start

```bash
cd tests
./run_tests.sh
```

### Manual Build

```bash
cd tests
mkdir build
cd build
cmake ..
cmake --build .
./climate_ir_woleix_test
```

### Running Specific Tests

```bash
# Run only power-related tests
./climate_ir_woleix_test --gtest_filter="*Power*"

# Run with verbose output
./climate_ir_woleix_test --gtest_verbose

# List all available tests
./climate_ir_woleix_test --gtest_list_tests
```

## Adding New Tests

To add new test cases:

1. Open `climate_ir_woleix_test.cpp`
2. Add a new TEST_F within the appropriate section:

```cpp
TEST_F(WoleixClimateTest, YourTestName) {
  // Setup
  climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  
  // Execute
  climate->call_transmit_state();
  
  // Verify
  ASSERT_EQ(transmitted_data.size(), 1);
  EXPECT_TRUE(check_timings_match(transmitted_data[0], get_power_timings()));
}
```

## Test Output Example

```text
[==========] Running 12 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 12 tests from WoleixClimateTest
[ RUN      ] WoleixClimateTest.TraitsConfiguredCorrectly
[       OK ] WoleixClimateTest.TraitsConfiguredCorrectly (0 ms)
[ RUN      ] WoleixClimateTest.TurningOnFromOffSendsPowerCommand
[D][climate_ir_woleix.climate] Sending Power command
[       OK ] WoleixClimateTest.TurningOnFromOffSendsPowerCommand (1 ms)
...
[==========] 12 tests from 1 test suite ran. (15 ms total)
[  PASSED  ] 12 tests.
```

## Mocking Strategy

The tests use lightweight mocks that simulate ESPHome's behavior:

- **RemoteTransmitter**: Captures IR data instead of actually transmitting
- **ClimateIR Base**: Provides state variables and transmitter access
- **Logging**: Prints to stdout for debugging
- **Delay**: No-op (tests run instantly)

This approach allows testing the component's logic in isolation without ESP32 dependencies.

## Continuous Integration

You can integrate these tests into CI/CD:

```yaml
# .github/workflows/test.yml
name: Unit Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Run Tests
        run: |
          cd tests
          ./run_tests.sh
```

## Troubleshooting

### CMake can't find compiler

```bash
export CXX=g++
export CC=gcc
```

### Google Test download fails

Check your internet connection. Google Test is fetched automatically during CMake configuration.

### Tests fail after component changes

Update the expected timing constants in the test file if you've modified the IR protocol timings in the component.

## Future Improvements

Potential enhancements:

- [ ] Test coverage for swing mode
- [ ] Test coverage for timer functions
- [ ] Parameterized tests for temperature ranges
- [ ] Performance benchmarks
- [ ] Mock receiver for decode testing
