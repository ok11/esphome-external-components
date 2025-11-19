# Unit Tests

This directory contains unit tests for the climate_ir_woleix component. These tests use Google Test/Mock and run against mocked ESPHome dependencies for fast, isolated testing.

## Structure

```text
unit/
├── climate_ir_woleix_test.cpp  # Test cases
├── CMakeLists.txt              # Build configuration
├── mocks/                      # Mock ESPHome headers
└── README.md                   # This file
```

## Prerequisites

- CMake 3.20 or higher
- Google Test (installed via package manager)
- C++20 compatible compiler

### Installing Dependencies

**macOS:**

```bash
brew install cmake googletest
```

**Ubuntu/Debian:**

```bash
sudo apt-get install cmake libgtest-dev
```

## Running Unit Tests

### Quick Run

From the `tests/unit/` directory:

```bash
mkdir -p build && cd build
cmake ..
make
ctest --output-on-failure
```

### With Coverage

```bash
mkdir -p build && cd build
cmake -DENABLE_COVERAGE=ON ..
make
ctest
# Generate coverage report
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
lcov --list coverage.info
```

### Debug Mode

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
# Run with debugger
lldb ./climate_ir_woleix_test
# or
gdb ./climate_ir_woleix_test
```

## Testing Philosophy

### Control Path Testing

Unit tests focus on the **control path** - verifying what IR commands are transmitted in response to state changes:

✅ **What we test:**
- Target temperature changes → Verify temp up/down IR commands sent
- Mode changes (OFF → COOL) → Verify power, mode IR commands sent
- Fan speed changes → Verify fan speed IR commands sent
- Power on/off → Verify power IR command sent

❌ **What we don't test:**
- Current temperature updates (sensor input, no IR transmission)
- Current humidity updates (sensor input, no IR transmission)
- Display updates (UI concern, not control logic)

This approach ensures tests validate the component's **command generation logic** without testing the ESPHome base `Climate` class behavior.

### Mock Usage

Two types of test fixtures demonstrate different testing approaches:

1. **`WoleixClimateTest`** - Uses real transmitter with callbacks
   - Captures transmitted IR data for detailed verification
   - Validates IR timing data matches expected patterns
   - Best for testing IR protocol correctness

2. **`WoleixClimateMockTest`** - Uses GMock's `MockRemoteTransmitter`
   - Uses `EXPECT_CALL` to verify method calls
   - Tests call counts and sequences
   - Best for testing transmission behavior

**Example GMock test:**
```cpp
TEST_F(WoleixClimateMockTest, PowerOnCallsTransmitMultipleTimes) {
  EXPECT_CALL(*mock_transmitter, transmit_raw(_))
      .Times(3);  // Power, Mode, Speed commands
  
  climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  climate->call_transmit_state();
}
```

## Test Categories

### Traits Configuration Tests

- Verify temperature ranges (15-30°C)
- Check supported modes and features
- Validate current temperature/humidity support flags

### Power Control Tests

- Power on/off commands
- State transitions
- Verify power command sent exactly once

### Temperature Control Tests

- Target temperature increase/decrease
- Command sequence validation
- Multiple degree changes generate multiple commands

### Mode Control Tests

- Mode switching (cool, heat, dry, fan)
- Mode command generation
- Verify mode commands sent on state change

### Fan Control Tests

- Fan speed changes
- Fan mode commands
- Speed command verification

### Complex Scenarios

- Complete state transitions (OFF → COOL with temp & fan settings)
- Multiple simultaneous changes
- Verify correct command ordering

### GMock Verification Tests

- Exact call count verification (`.Times(n)`)
- Minimum call count (`.Times(AtLeast(n))`)
- No-call verification (`.Times(0)`)
- Argument inspection with `.WillOnce(Invoke(...))`

## Writing New Tests

Tests follow the Google Test framework pattern:

```cpp
TEST_F(WoleixClimateTest, YourTestName) {
  // Setup
  climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  
  // Execute
  climate->target_temperature = 25.0f;
  climate->call_transmit_state();
  
  // Verify
  EXPECT_EQ(transmitted_data.size(), 1);
  EXPECT_TRUE(check_timings_match(transmitted_data[0], expected_timings));
}
```

## Mocks

The `mocks/` directory contains lightweight mock implementations of ESPHome headers:

- `esphome/components/climate/` - Climate component interfaces
- `esphome/components/climate_ir/` - Climate IR base classes
- `esphome/core/` - Core ESPHome utilities

These mocks allow testing without the full ESPHome framework.

## CI Integration

Unit tests are designed to run quickly in CI environments. They should complete in under 10 seconds and require no external dependencies beyond the compiler and Google Test.

## Troubleshooting

**CMake can't find Google Test:**

- Ensure Google Test is installed via package manager
- On macOS, check `/opt/homebrew` and `/usr/local` paths
- Set `CMAKE_PREFIX_PATH` if needed

**Linking errors:**

- Verify C++20 support in your compiler
- Check that all mock headers are present

**Test failures:**

- Run with `--output-on-failure` for detailed error messages
- Use Debug build for more information
- Check timing tolerances in test assertions
