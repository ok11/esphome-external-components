# Testing Guide

This directory contains comprehensive tests for the climate_ir_woleix ESPHome external component.

## Test Structure

```text
tests/
├── unit/                          # Fast, isolated unit tests
│   ├── climate_ir_woleix_test.cpp
│   ├── CMakeLists.txt
│   ├── mocks/                     # Mock ESPHome headers
│   └── README.md
├── integration/                   # End-to-end integration tests
│   ├── docker-compose.yml
│   ├── Dockerfile.esphome
│   ├── test_configs/              # ESPHome YAML configs
│   ├── test_runner.py             # Main test orchestrator
│   ├── scripts/                   # Helper scripts
│   └── README.md
├── CMakeLists.txt
└── README.md                      # This file
```

## Two Types of Tests

### Unit Tests (`tests/unit/`)

**Purpose:** Fast, isolated tests using mocked dependencies

**Characteristics:**

- Run in seconds
- Test component logic in isolation
- Use Google Test/Mock framework
- No external dependencies (beyond compiler & GTest)
- Perfect for TDD and rapid development

**When to use:**

- Testing individual functions and methods
- Verifying state transitions
- Validating IR command generation
- Quick feedback during development

**How to run:**

```bash
cd tests/unit
mkdir -p build && cd build
cmake ..
make
ctest --output-on-failure
```

See [unit/README.md](unit/README.md) for detailed instructions.

### Integration Tests (`tests/integration/`)

**Purpose:** End-to-end validation with real ESPHome

**Characteristics:**

- Run in minutes
- Test against actual ESPHome in Docker
- Validate real compilation and integration
- Test external component loading
- Smoke tests for realistic scenarios

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
# 1. Run unit tests
cd tests/unit
mkdir -p build && cd build
cmake .. && make && ctest

# 2. Run integration tests
cd ../../integration
./run_tests.sh
```

### Run Only Unit Tests (Fastest)

```bash
cd tests/unit/build
ctest --output-on-failure
```

### Run Only Integration Tests

```bash
cd tests/integration
./run_tests.sh
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

- Docker
- Docker Compose
- Python 3.8+
- bash

**Install Docker:**

- macOS/Windows: Install Docker Desktop
- Ubuntu: `sudo apt-get install docker.io docker-compose`

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
open http://localhost:6052

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

Edit `tests/unit/climate_ir_woleix_test.cpp`:

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

### Adding Integration Test Configs

Create a new YAML file in `tests/integration/test_configs/`:

```yaml
esphome:
  name: test-your-scenario
  # ... rest of config
```

The test runner will automatically discover and test it.

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

## Additional Resources

- [Unit Tests README](unit/README.md) - Detailed unit test guide
- [Integration Tests README](integration/README.md) - Detailed integration test guide
- [ESPHome Testing Docs](https://esphome.io/guides/contributing.html#testing)
- [Google Test Documentation](https://google.github.io/googletest/)

## Support

For issues or questions about testing:

1. Check this documentation
2. Review test output and logs
3. Open an issue with test details
