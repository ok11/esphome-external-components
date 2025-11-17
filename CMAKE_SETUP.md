# CMake Setup for IntelliSense

This project uses CMake to generate `compile_commands.json` for VS Code IntelliSense support.

## Quick Start

### 1. Configure External Dependencies

The component requires access to ESPHome core and PlatformIO libraries. Configure the paths using **one** of these methods:

#### Option A: VS Code Settings (Recommended)

Edit `.vscode/settings.json` and add:

```json
{
    "cmake.configureSettings": {
        "ESPHOME_PATH": "/path/to/your/esphome",
        "PLATFORMIO_LIBDEPS_PATH": "/path/to/your/esphome/.pio/libdeps/esp8266-arduino"
    }
}
```

See `.vscode/settings.local.json.example` for a complete example.

#### Option B: Environment Variables

```bash
export ESPHOME_PATH="/path/to/your/esphome"
export PLATFORMIO_LIBDEPS_PATH="/path/to/your/esphome/.pio/libdeps/esp8266-arduino"
```

Add to your `~/.bashrc` or `~/.zshrc` to make permanent.

#### Option C: CMake Command Line

```bash
cmake -S . -B build \
  -DESPHOME_PATH=/path/to/your/esphome \
  -DPLATFORMIO_LIBDEPS_PATH=/path/to/your/esphome/.pio/libdeps/esp8266-arduino
```

### 2. Build the Project

```bash
# Configure
cmake -S . -B build

# Build components (tests require GTest)
cmake --build build

# Or use VS Code tasks (Ctrl/Cmd+Shift+B)
```

### 3. Verify IntelliSense

Open a component file (e.g., `esphome/components/climate_ir_woleix/climate_ir_woleix.cpp`) and check:
- No red squiggles on ESPHome includes
- Go to Definition works (F12) on ESPHome symbols
- Auto-completion works for ESPHome types

## What Gets Included in compile_commands.json

When properly configured, the compilation database includes:

```json
{
  "command": "/usr/bin/c++ -I/path/to/component -I/path/to/esphome -I/path/to/ArduinoJson ...",
  "file": "climate_ir_woleix.cpp"
}
```

These include paths enable IntelliSense to:
- Resolve ESPHome core headers (`esphome/core/*.h`)
- Resolve climate components (`esphome/components/climate/*.h`)
- Resolve PlatformIO libraries (ArduinoJson, etc.)
- Resolve test mocks (`tests/mocks/*`)

## Auto-Detection

CMake will try to auto-detect paths from these common locations:
- `../esphome` (parent directory)
- `$HOME/esphome`
- PlatformIO libraries inside detected ESPHome installation

If auto-detection fails, you'll see a warning:
```
CMake Warning: ESPHOME_PATH not set. External ESPHome dependencies won't be available for IntelliSense.
```

## Testing

When GTest is installed, tests are automatically built:

```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Or use VS Code Test Explorer (C++ TestMate extension)
```

Test files are also included in `compile_commands.json` for full IntelliSense support.

## Troubleshooting

### IntelliSense shows red squiggles on ESPHome includes

Check that:
1. ESPHOME_PATH is set correctly
2. The path exists and contains ESPHome source code
3. CMake configured successfully (check Output panel → CMake)
4. `build/compile_commands.json` contains the correct include paths

### compile_commands.json not found

Make sure:
1. CMake has been configured at least once
2. The symlink exists: `ls -l compile_commands.json`
3. VS Code CMake Tools extension is installed

### External paths not being used

Check VS Code settings:
1. `"C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"`
2. `"cmake.sourceDirectory": "${workspaceFolder}"`
3. CMake kit is selected (bottom status bar)

## VS Code Extensions Required

- **CMake Tools** (`ms-vscode.cmake-tools`) - CMake integration
- **C/C++** (`ms-vscode.cpptools`) - IntelliSense support
- **C++ TestMate** (optional) - Test runner integration
