# VS Code IntelliSense Not Working - Troubleshooting Guide

Even though `compile_commands.json` is generated correctly with all include paths, VS Code IntelliSense isn't finding ESPHome/GTest headers. Follow these steps:

## Quick Fixes (Try in Order)

### 1. Select CMake Kit
**Status bar → No Kit Selected?**
- Press `Cmd+Shift+P` (Mac) or `Ctrl+Shift+P` (Windows/Linux)
- Type: `CMake: Select a Kit`
- Choose your compiler (e.g., "Clang 14.0.0" or "GCC")

### 2. Reload C++ IntelliSense
- Press `Cmd+Shift+P`
- Type: `C/C++: Reset IntelliSense Database`
- Wait for indexing to complete (watch bottom right)

### 3. Restart CMake Configuration
- Press `Cmd+Shift+P`
- Type: `CMake: Delete Cache and Reconfigure`
- This forces CMake Tools to re-read everything

### 4. Check Configuration Provider
- Press `Cmd+Shift+P`
- Type: `C/C++: Edit Configurations (UI)`
- Scroll down to "Configuration Provider"
- Should show: `ms-vscode.cmake-tools`
- If not, select it from dropdown

### 5. Verify Active Configuration
Open any `.cpp` file and check:
- Bottom right corner should show: `{` icon with configuration name
- Click it → Should show "Active configuration: CMake Tools"
- If it shows something else, select "CMake Tools"

## Deep Diagnostics

### Check if CMake Tools is Active
1. Open Command Palette (`Cmd+Shift+P`)
2. Type: `CMake: View CMake Cache`
3. If it opens → CMake is configured ✓
4. If error → Run `CMake: Configure`

### Check C++ Extension Output
1. View → Output (or `Cmd+Shift+U`)
2. Dropdown: Select "C/C++"
3. Look for errors about configuration provider
4. Should see: "Using configuration provider: CMake Tools"

### Verify compile_commands.json is Being Used
1. Open a component file (e.g., `climate_ir_woleix.cpp`)
2. View → Output → Select "C/C++"
3. Search for "compile_commands.json"
4. Should show path to your build/compile_commands.json

## Nuclear Option (If Nothing Works)

### Complete Reset
```bash
# Close VS Code first, then:

# 1. Clear VS Code workspace cache
rm -rf .vscode/.cmaketools.json
rm -rf build/

# 2. Reconfigure
cp .env.example .env
# Edit .env with your paths

# 3. Reopen VS Code and let CMake Tools auto-configure
```

### Check Extensions
Make sure these are installed and enabled:
- **CMake Tools** (`ms-vscode.cmake-tools`)
- **C/C++** (`ms-vscode.cpptools`)
- **C++ TestMate** (optional, for testing)

## Common Issues and Solutions

### Issue: "Cannot find source file: esphome/core/log.h"
**Cause:** External paths not in compile_commands.json
**Fix:** Check `.env` file has correct `ESPHOME_PATH`, then reconfigure CMake

### Issue: IntelliSense works in some files but not others
**Cause:** Configuration provider switched to wrong mode
**Fix:**
1. Open the file with issues
2. Click `{` icon in bottom right
3. Select "CMake Tools" configuration

### Issue: Red squiggles but code compiles fine
**Cause:** C++ extension cache out of sync
**Fix:** `C/C++: Reset IntelliSense Database`

### Issue: "No Configuration Provider Available"
**Cause:** CMake Tools not providing configuration
**Fix:**
1. Check `.vscode/settings.json` has:
   ```json
   "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"
   ```
2. Run `CMake: Configure`
3. Reload VS Code window

## Verification Steps

After fixes, verify IntelliSense works:

1. **Open** `esphome/components/climate_ir_woleix/climate_ir_woleix.cpp`
2. **Hover** over `#include "esphome/core/log.h"`
   - Should show file path, not error
3. **Right-click** on `esphome::climate::ClimateMode` → "Go to Definition" (F12)
   - Should jump to ESPHome source (or mock)
4. **Type** `esphome::`
   - Should show autocomplete suggestions

## Still Not Working?

Check these settings in `.vscode/settings.json`:

```json
{
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "cmake.configureOnOpen": true,
    "cmake.sourceDirectory": "${workspaceFolder}",

    // These should NOT be set (conflicts with CMake Tools):
    // "C_Cpp.default.compileCommands": "${workspaceFolder}/compile_commands.json",
    // "C_Cpp.default.includePath": [...]
}
```

The key is to let CMake Tools be the **sole** configuration provider.

## Debug Information to Share

If still having issues, collect this info:

1. **CMake configure output:**
   ```bash
   cmake -S . -B build 2>&1 | grep -E "(Added|Loaded|ESPHOME)"
   ```

2. **Include paths in compile database:**
   ```bash
   grep -o '\-I[^ ]*' build/compile_commands.json | sort -u
   ```

3. **C++ extension diagnostics:**
   - Output panel → C/C++
   - Look for lines mentioning "configuration provider"

4. **Active configuration:**
   - Open a .cpp file
   - Click `{` icon in status bar
   - Screenshot the active configuration
