# Testing Guide

This directory contains comprehensive tests for the climate_ir_woleix ESPHome external component.

## Test Structure

```text
tests/
├── unit/                               # Fast, isolated unit tests
│   ├── climate_ir_woleix_test.cpp      # Climate component tests (28 tests)
│   ├── woleix_state_machine_test.cpp   # State machine tests (26 tests)
│   ├── woleix_state_mapper_test.cpp    # State mapper tests (19 tests)
│   ├── woleix_comm_test.cpp            # Communication tests (12 tests)
│   ├── CMakeLists.txt
│   ├── run_tests.sh                    # Quick test runner
│   ├── generate_coverage.sh            # Coverage report generator
│   ├── mocks/                          # Mock ESPHome headers
│   └── README.md
├── integration/                        # End-to-end integration tests
│   ├── docker-compose.yml              # Docker orchestration
│   ├── run_tests.sh                    # Main entry point
│   ├── requirements.txt                # Python dependencies
│   ├── test_configs/                   # ESPHome YAML test configurations
│   │   ├── woleix_test_basic.yaml      # Basic component setup
│   │   ├── woleix_test_full.yaml       # Full-featured configuration
│   │   ├── woleix_test_with_dht.yaml   # With DHT sensor integration
│   │   └── woleix_test_with_reset_button.yaml  # With reset button
│   ├── scripts/                        # Test automation scripts
│   │   ├── test_runner.py              # Python test orchestrator
│   │   └── wait_for_esphome.sh         # Health check script
│   ├── output/                         # Compiled firmware (gitignored)
│   └── README.md
├── CMakeLists.txt
└── README.md                           # This file
```

## Two Types of Tests

### Unit Tests (`tests/unit/`)

**Purpose:** Fast, isolated tests using mocked dependencies

**Test Files:**

- `climate_ir_woleix_test.cpp` - 28 tests for climate component (main controller)
- `woleix_state_manager_test.cpp` - 26 tests for state manager (command generation)
- `woleix_state_mapper_test.cpp` - 19 tests for state mapper (ESPHome↔Woleix conversions)
- `woleix_protocol_handler_test.cpp` - 12 tests for protocol handler (NEC protocol transmission)
- **Total: 85 unit tests**

**Characteristics:**

- Run in seconds (typically < 5 seconds for all 85 tests)
- Test component logic in isolation using mocked dependencies
- Use Google Test framework with Google Mock for mocking
- No external dependencies (beyond C++ compiler & Google Test)
- Perfect for TDD and rapid development cycles
- **>95% line coverage, >95% function coverage**
- Test NEC protocol command generation and IR transmission logic

**When to use:**

- Testing individual functions and methods in isolation
- Verifying state transitions and state machine logic
- Validating NEC IR command generation and command sequences
- Testing circular mode cycling (COOL→DEHUM→FAN→COOL)
- Testing temperature control granularity (1°C steps)
- Verifying state mapping between ESPHome and Woleix formats
- Quick feedback during development (runs in seconds)

**How to run:**

```bash
cd tests/unit
./run_tests.sh

# Or generate coverage report
./generate_coverage.sh
```

See [unit/README.md](unit/README.md) for detailed instructions.

### Integration Tests (`tests/integration/`)

**Purpose:** End-to-end validation with real ESPHome

**Test Configurations:**

- `woleix_test_basic.yaml` - Minimal component setup
- `woleix_test_full.yaml` - Full-featured configuration with web server and status LED
- `woleix_test_with_dht.yaml` - Integration with DHT22 temperature/humidity sensor
- `woleix_test_with_reset_button.yaml` - Configuration with reset button functionality

**Characteristics:**

- Run in minutes (~5-10 minutes first run, ~1-2 minutes subsequent runs)
- Test against actual ESPHome in Docker container
- Validate real compilation and integration
- Test external component loading from mounted directory
- Compilation tests for realistic scenarios
- Automated via Python test runner (`test_runner.py`)

**When to use:**

- Before releasing changes
- Verifying ESPHome compatibility
- Testing component installation/loading
- Validating real-world configurations
- CI/CD pipeline validation

**How to run:**

```bash
cd tests/integration
./run_tests.sh

# Keep containers running for debugging
KEEP_RUNNING=true ./run_tests.sh
```

See [integration/README.md](integration/README.md) for detailed instructions.

## Testing Pyramid

Our test strategy follows the testing pyramid:

```text
       /\
      /  \  Integration Tests (Few, Slow, High Confidence)
     /____\
    /      \
   /  Unit  \ Unit Tests (Many, Fast, Focused)
  /__________\
```

- **Many unit tests**: Fast feedback, test individual components
- **Few integration tests**: Validate end-to-end functionality

## Quick Start

### Run All Tests Locally

```bash
# 1. Run unit tests (fast, ~5 seconds)
cd tests/unit
./run_tests.sh

# 2. Run integration tests (~5-10 minutes first run, ~1-2 minutes subsequent)
cd ../integration
./run_tests.sh
```

### Run Only Unit Tests (Fastest)

```bash
cd tests/unit
./run_tests.sh

# Or with CMake directly
mkdir -p build && cd build
cmake .. && make && ctest --output-on-failure
```

### Run Only Integration Tests

```bash
cd tests/integration
./run_tests.sh

# Keep containers running for debugging
KEEP_RUNNING=true ./run_tests.sh
```

## Prerequisites

### For Unit Tests

- CMake 3.20+
- C++20 compiler (GCC/Clang)
- Google Test
- PlatformIO (for dependency management)

**Install on macOS:**

```bash
brew install cmake googletest
pip install platformio
```

**Install on Ubuntu:**

```bash
sudo apt-get install cmake libgtest-dev build-essential
pip install platformio
```

**Install PlatformIO Dependencies:**

The project uses `platformio_install_deps_locally.py` to install ESPHome dependencies under a stable path for reliable compilation:

```bash
# From project root, activate your virtual environment
source .venv/bin/activate

# Install dependencies for unit tests
./platformio_install_deps_locally.py tests/unit/platformio.ini -l -p

# This installs libraries under ESPHome root instead of global locations
# providing stable include paths for C++ compilation and IDE IntelliSense
```

**Why use platformio_install_deps_locally.py?**

- Provides stable, predictable library paths for external component development
- Better IDE integration (IntelliSense can find headers reliably)
- Installs under ESPHome directory instead of system-wide
- Mirrors how ESPHome itself manages dependencies

See the main [README.md](../README.md#4-platformio-dependencies-for-external-component-development) for more details.

### For Integration Tests

- Docker Desktop (macOS/Windows) or Docker Engine (Linux)
- Docker Compose
- Python 3.12+
- bash
- Python packages: `requests` (installed via requirements.txt)

**Install Docker:**

- macOS/Windows: Install Docker Desktop from https://www.docker.com/products/docker-desktop
- Ubuntu: `sudo apt-get install docker.io docker-compose`

**Install Python dependencies:**

```bash
cd tests/integration
python -m venv .venv
source .venv/bin/activate  # On Windows: .venv\Scripts\activate
pip install -r requirements.txt
```

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Tests

on: [push, pull_request]

jobs:
  unit-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: sudo apt-get install -y cmake libgtest-dev
      - name: Run unit tests
        run: |
          cd tests/unit
          mkdir build && cd build
          cmake .. && make && ctest --output-on-failure
          
  integration-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Run integration tests
        run: |
          cd tests/integration
          ./run_tests.sh
```

## Development Workflow

### When Working on Component Code

1. **Write unit tests first** (TDD approach)

   ```bash
   cd tests/unit/build
   # Edit tests/unit/climate_ir_woleix_test.cpp
   make && ctest
   ```

2. **Implement the feature** in component code

3. **Run unit tests** to verify

   ```bash
   make && ctest --output-on-failure
   ```

4. **Run integration tests** before committing

   ```bash
   cd ../../integration
   ./run_tests.sh
   ```

### Before Submitting Pull Request

1. Ensure all unit tests pass
2. Ensure all integration tests pass
3. Add tests for new features
4. Update test documentation if needed

## Debugging Tests

### Unit Tests

```bash
cd tests/unit/build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Run with debugger
lldb ./climate_ir_woleix_test
# or
gdb ./climate_ir_woleix_test
```

### Integration Tests

```bash
# Keep containers running for inspection
cd tests/integration
KEEP_RUNNING=true ./run_tests.sh

# Inspect ESPHome logs
docker-compose logs -f esphome

# Access ESPHome dashboard
open http://localhost:6052  # macOS
# or
xdg-open http://localhost:6052  # Linux

# Manually compile a specific config
docker exec esphome-test esphome compile /config/test_configs/woleix_test_basic.yaml

# When done
docker-compose down
```

## Test Coverage

To generate coverage reports for unit tests:

```bash
cd tests/unit/build
cmake -DENABLE_COVERAGE=ON ..
make
ctest

# Generate HTML report
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_html

# View report
open coverage_html/index.html  # macOS
xdg-open coverage_html/index.html  # Linux
```

## Writing New Tests

### Adding Unit Tests

**For Climate Component** - Edit `tests/unit/climate_ir_woleix_test.cpp`:

```cpp
TEST_F(WoleixClimateTest, YourNewTest) {
  // Setup
  climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  
  // Execute
  climate->target_temperature = 25.0f;
  climate->call_transmit_state();
  
  // Verify
  EXPECT_EQ(transmitted_data.size(), 1);
}
```

**For State Manager** - Edit `tests/unit/woleix_state_manager_test.cpp`:

```cpp
TEST(WoleixStateManagerTest, YourNewTest) {
  // Setup
  WoleixStateManager state_manager;
  
  // Execute
  state_manager.move_to(WoleixInternalState(WoleixPowerState::ON, 
                                            WoleixMode::COOL, 
                                            25.0f, 
                                            WoleixFanSpeed::LOW));
  
  // Verify
  const auto& commands = state_manager.get_commands();
  EXPECT_EQ(commands.size(), expected_count);
}
```

**For State Mapper** - Edit `tests/unit/woleix_state_mapper_test.cpp`:

```cpp
TEST(StateMapperTest, YourNewTest) {
  // Setup
  WoleixMode woleix_mode = WoleixMode::COOL;
  
  // Execute
  ClimateMode esphome_mode = StateMapper::woleix_to_esphome_mode(woleix_mode);
  
  // Verify
  EXPECT_EQ(esphome_mode, ClimateMode::CLIMATE_MODE_COOL);
}
```

**For Protocol Handler** - Edit `tests/unit/woleix_protocol_handler_test.cpp`:

```cpp
TEST(WoleixProtocolHandlerTest, YourNewTest) {
  // Setup
  MockRemoteTransmitterBase mock_transmitter;
  WoleixProtocolHandler handler([](const std::string&, uint32_t, std::function<void()>){},
                                [](const std::string&){});
  handler.set_transmitter(&mock_transmitter);
  WoleixCommand command(WoleixCommand::Type::POWER, ADDRESS_NEC, 1);
  
  // Expectations - verify NEC protocol transmission
  EXPECT_CALL(mock_transmitter, transmit(testing::_))
      .Times(1);
  
  // Execute
  handler.transmit_(command);
  
  // Verify is handled by the EXPECT_CALL
}
```

### Adding Integration Test Configs

Create a new YAML file in `tests/integration/test_configs/`:

```yaml
esphome:
  name: test-your-scenario
  friendly_name: "Test Your Scenario"
  platformio_options:
    board_build.flash_mode: dio

esp32:
  board: esp32dev
  framework:
    type: arduino

external_components:
  - source: /config/external_components
    components: [ climate_ir_woleix ]

# Add your test configuration here
remote_transmitter:
  pin: GPIO14
  carrier_duty_percent: 50%

climate:
  - platform: climate_ir_woleix
    name: "Test Climate"
    # ... rest of config
```

The test runner will automatically discover and compile all `.yaml` files in `test_configs/`.

## Troubleshooting

### Unit Tests Won't Compile

- Check C++20 support: `g++ --version` or `clang++ --version`
- Verify Google Test installation
- Check include paths in CMakeLists.txt

### Integration Tests Timeout

- Increase `MAX_WAIT_TIME` in scripts
- Check Docker has enough resources
- Verify network connectivity
- Check ESPHome logs: `docker-compose logs esphome`

### Tests Pass Locally But Fail in CI

- Check CI has sufficient resources
- Verify Docker version compatibility
- Check for timing-dependent tests
- Review CI logs carefully

## Test Coverage Details

### Current Coverage (as of latest update)

- **Line Coverage**: >95%
- **Function Coverage**: >95%
- **Total Tests**: 85 passing tests

### Coverage by Component

| Component | Tests | Focus Areas |
| --------- | ----- | ----------- |
| Climate Component | 28 | State synchronization, sensor integration, command transmission |
| State Manager | 26 | Command generation, state transitions, mode cycling |
| State Mapper | 19 | Bidirectional state conversions (ESPHome↔Woleix) |
| Protocol Handler | 12 | NEC protocol transmission, command delays, repeats |

### What's Tested

✅ **Power Control**

- Power on/off transitions
- State reset on power-on
- Default state initialization

✅ **Mode Control**

- Circular mode sequence (COOL→DEHUM→FAN→COOL)
- Mode cycling with minimal commands
- Mode-specific behaviors (e.g., temperature only in COOL)

✅ **Temperature Control**

- 1°C granular adjustments (15-30°C range)
- Multiple temperature step commands
- Temperature clamping to valid range
- Temperature changes only in COOL mode

✅ **Fan Speed Control**

- LOW↔HIGH toggle
- Fan speed state tracking

✅ **State Mapping**

- ESPHome→Woleix conversions
- Woleix→ESPHome conversions
- Mode, fan speed, and power state mappings

✅ **IR Communication**

- NEC protocol command generation
- Command delays (150ms for temp, 200ms for mode)
- Command repeat functionality
- Command queue management

✅ **Sensor Integration**

- Temperature sensor updates
- Humidity sensor updates (optional)
- State publishing

## Protocol Information

### NEC IR Protocol

The component uses the **NEC IR protocol** for all communications:

- **Address**: `0xFB04` (fixed for all Woleix commands)
- **Protocol**: Standard NEC consumer IR protocol
- **Commands**: POWER (0xFB04), TEMP_UP (0xFA05), TEMP_DOWN (0xFE01), MODE (0xF20D), FAN_SPEED (0xF906)

Tests verify that commands are correctly formatted for NEC transmission and include proper delays and repeats.

## Additional Resources

- [Unit Tests README](unit/README.md) - Detailed unit test guide
- [Integration Tests README](integration/README.md) - Detailed integration test guide
- [API Documentation](../API.md) - Complete API reference
- [Main README](../README.md) - Project overview and setup
- [ESPHome Testing Docs](https://esphome.io/guides/contributing.html#testing)
- [Google Test Documentation](https://google.github.io/googletest/)

## Support

For issues or questions about testing:

1. Check this documentation
2. Review test output and logs
3. Open an issue with test details
