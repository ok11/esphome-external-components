# ESPHome External Components - Woleix Climate IR

Custom ESPHome component for controlling Woleix air conditioners via infrared remote control.

## 📁 Project Structure

```text
esphome-external-components/
├── esphome/
│   └── components/
│       └── climate_ir_woleix/          # Main component directory
│           ├── __init__.py             # Python configuration interface
│           ├── climate.py              # ESPHome climate platform
│           ├── climate_ir_woleix.h     # C++ header file
│           ├── climate_ir_woleix.cpp   # C++ implementation
│           └── LICENSE                 # Component license
├── tests/                              # Test suite
│   ├── unit/                           # C++ unit tests
│   │   ├── climate_ir_woleix_test.cpp  # Test implementation
│   │   ├── CMakeLists.txt              # Test build configuration
│   │   ├── run_tests.sh                # Test execution script
│   │   ├── generate_coverage.sh        # Coverage report generator
│   │   └── mocks/                      # Mock ESPHome headers
│   ├── integration/                    # Integration tests
│   │   ├── test_configs/               # ESPHome test configurations
│   │   ├── test_runner.py              # Test orchestration script
│   │   ├── run_tests.sh                # Integration test runner
│   │   └── docker-compose.yml          # Docker environment
│   └── README.md                       # Testing documentation
├── .vscode/                            # VS Code configuration
│   ├── settings.json                   # Editor settings
│   ├── tasks.json                      # Build tasks
│   ├── launch.json                     # Debug configurations
│   └── c_cpp_properties.json           # C++ IntelliSense config
├── platformio_install_deps_locally.py  # PlatformIO dependency installer
├── prepare_esphome.sh                  # ESPHome setup helper script
├── .venv/                              # Python virtual environment
├── CMakeLists.txt                      # Root CMake configuration
├── .gitignore                          # Git ignore rules
├── .env                                # Environment variables (not in git)
└── README.md                           # This file
```

## 🛠️ Development Setup

### Prerequisites

- **Operating System**: macOS, Linux, or Windows with WSL
- **Git**: For version control
- **Python 3.9+**: For ESPHome development
- **C++ Compiler**: For building and testing
- **CMake 3.16+**: For build system
- **VS Code**: Recommended IDE

### 1. Clone the Repository

```bash
git clone https://github.com/ok11/esphome-external-components.git
cd esphome-external-components
```

### 2. Python Development Environment Setup

#### Create Virtual Environment

```bash
# Create .venv directory
python3 -m venv .venv

# Activate virtual environment
# On macOS/Linux:
source .venv/bin/activate

# On Windows (WSL):
source .venv/bin/activate
```

#### Install ESPHome

```bash
# Upgrade pip first
pip install --upgrade pip

# Install ESPHome and dependencies
pip install esphome

# Verify installation
esphome version
```

#### Configure VS Code Python Interpreter

1. Open Command Palette: `Cmd+Shift+P` (macOS) or `Ctrl+Shift+P` (Windows/Linux)
2. Type: `Python: Select Interpreter`
3. Choose: `.venv/bin/python` or `./esphome-external-components/.venv/bin/python`
4. Reload VS Code window if needed

### 3. C++ Development Environment Setup

#### Install Required Tools

**macOS:**

```bash
# Install Xcode Command Line Tools (includes clang/clang++)
xcode-select --install

# Install CMake via Homebrew
brew install cmake

# Install Google Test (optional, for local testing)
brew install googletest
```

**Linux (Ubuntu/Debian):**

```bash
# Install build essentials
sudo apt update
sudo apt install build-essential cmake

# Install Google Test
sudo apt install libgtest-dev
cd /usr/src/gtest
sudo cmake CMakeLists.txt
sudo make
sudo cp lib/*.a /usr/lib
```

**Windows (WSL):**

```bash
# Follow Linux instructions above in WSL terminal
```

#### Verify C++ Tools

```bash
# Check compiler
g++ --version
# or
clang++ --version

# Check CMake
cmake --version

# Should see CMake 3.20 or higher
```

### 4. PlatformIO Dependencies (For External Component Development)

The `platformio_install_deps_locally.py` script installs PlatformIO libraries into the ESPHome installation directory, providing stable include and library paths for external component compilation. It is called from `prepare_esphome.sh` before `CMake` steps, so you basically don't bother, but it is of course also possible to call it separately.

#### Why Use This Script?

- **Stable Paths**: Unlike ESPHome's global `platformio_install_deps.py` (which installs to system-wide locations), this script installs dependencies under the ESPHome root directory
- **Predictable Includes**: Ensures consistent library locations for IDE IntelliSense and compilation
- **Development Workflow**: Essential for external component development where you need access to ESPHome's internal dependencies

#### Usage

```bash
# Activate virtual environment first
source .venv/bin/activate

# Install all dependencies from a platformio.ini file
./platformio_install_deps_locally.py /path/to/esphome/platformio.ini

# Ii is also possible to install packages separately (as opposed to the above "everything mentioned in the ini file")
#   -l, --libraries <list of libraries>  Install library dependencies
#   -p, --platforms <list of platforms>  Install platform dependencies
#   -t, --tools     <list of tools>      Install tool dependencies
```

### 5. Building and Testing C++ Code

#### Build Tests

```bash
# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build the tests
cmake --build .

# Run tests
ctest --output-on-failure

# Or run the test executable directly
./tests/climate_ir_woleix_test
```

#### Alternative: Use Test Script

```bash
# From project root
cd tests
./run_tests.sh
```

### 6. VS Code Configuration

The project includes pre-configured VS Code settings:

- **`.vscode/settings.json`**: Python interpreter, CMake settings
- **`.vscode/tasks.json`**: Build tasks (Cmd+Shift+B)
- **`.vscode/launch.json`**: Debug configurations
- **`.vscode/c_cpp_properties.json`**: C++ IntelliSense configuration

#### Install Recommended VS Code Extensions

1. **Python** (ms-python.python)
2. **C/C++** (ms-vscode.cpptools)
3. **CMake Tools** (ms-vscode.cmake-tools)
4. **ESPHome Snippets** (optional)

## 🚀 Using the Component

### In Your ESPHome Configuration

```yaml
external_components:
  - source: github://ok11/esphome-external-components
    components: [ climate_ir_woleix ]

# Remote transmitter setup
remote_transmitter:
  pin: GPIO20
  carrier_duty_percent: 50%
  id: ir_transmitter

# Temperature and humidity sensor
sensor:
  - platform: dht
    model: DHT11
    pin: GPIO4
    temperature:
      name: "Room Temperature"
      id: room_temp
    humidity:
      name: "Room Humidity"
      id: room_humidity
    update_interval: 60s

# Climate control
climate:
  - platform: climate_ir_woleix
    name: "Air Conditioner"
    transmitter_id: ir_transmitter
    sensor: room_temp              # Temperature sensor (required)
    humidity_sensor: room_humidity  # Humidity sensor (optional)
```

## 🧪 Running Tests

### Unit Tests

#### Quick Start

```bash
# Build and run all tests
cd tests/unit
./run_tests.sh

# Generate coverage report
./generate_coverage.sh

# View coverage report
open build/coverage/html/index.html  # macOS
xdg-open build/coverage/html/index.html  # Linux
```

#### Using VSCode Tasks

Press `Cmd+Shift+P` (Mac) or `Ctrl+Shift+P` (Windows/Linux), then:

1. Type "Tasks: Run Task"
2. Select:
   - **"Unit Tests: Run Tests"** - Run tests only
   - **"Unit Tests: Generate Coverage Report"** - Run tests + generate coverage
   - **"Unit Tests: Open Coverage Report"** - Run tests + generate + open report in browser

#### Manual Testing

```bash
cd tests/unit
mkdir -p build && cd build
cmake -DENABLE_COVERAGE=ON ..
cmake --build .
./climate_ir_woleix_test
```

See [tests/unit/README.md](tests/unit/README.md) for detailed testing documentation.

### Linting Python Code

```bash
# Activate virtual environment
source .venv/bin/activate

# Install linting tools
pip install pylint black

# Run pylint
pylint esphome/components/climate_ir_woleix/__init__.py

# Format with black
black esphome/components/climate_ir_woleix/__init__.py
```

## 📝 Development Workflow

### Adding New Features

1. **Update C++ Header** (`climate_ir_woleix.h`)
   - Add method declarations
   - Add member variables

2. **Update C++ Implementation** (`climate_ir_woleix.cpp`)
   - Implement new methods
   - Add IR timing data if needed

3. **Update Python Configuration** (`__init__.py`)
   - Add configuration parameters to `CONFIG_SCHEMA`
   - Add code generation in `to_code()`

4. **Write Tests** (`tests/climate_ir_woleix_test.cpp`)
   - Add unit tests for new functionality

5. **Test Locally**

   ```bash
   # Build and test C++
   cd tests && ./run_tests.sh
   
   # Test with ESPHome
   esphome compile your-config.yaml
   ```

## 🐛 Troubleshooting

### Python Import Errors

If you see Pylint errors about missing ESPHome imports:

1. Ensure `.venv` is activated
2. Select correct Python interpreter in VS Code
3. Reload VS Code window

### C++ Build Errors

```bash
# Clean build directory
rm -rf build
mkdir build
cd build
cmake ..
cmake --build .
```

### CMake Not Finding Compiler

```bash
# Explicitly set compiler
export CC=gcc
export CXX=g++
cmake ..
```

## 📚 Resources

- [ESPHome Documentation](https://esphome.io/)
- [ESPHome External Components Guide](https://esphome.io/components/external_components.html)
- [Climate Component Reference](https://esphome.io/components/climate/index.html)
- [Google Test Documentation](https://google.github.io/googletest/)

## 📄 License

See [LICENSE](esphome/components/climate_ir_woleix/LICENSE) file in the component directory.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add/update tests
5. Submit a pull request

## 📧 Contact

- GitHub: [@ok11](https://github.com/ok11)
- Issues: [GitHub Issues](https://github.com/ok11/esphome-external-components/issues)
