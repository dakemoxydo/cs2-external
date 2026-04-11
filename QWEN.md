# CS2 Overlay - Project Context

## Project Overview

CS2 External Overlay — a C++20 external overlay application for Counter-Strike 2 built with ImGui and DirectX 11. It provides game enhancement features such as ESP, Aimbot, Triggerbot, RCS, Radar, and more via external memory reading.

### Architecture Summary

- **Dual-threaded runtime**: Render thread (ImGui, DirectX 11, UI, input) + Memory thread (process attach, game state reads, GameManager updates)
- **Two entry points**: `src/main.cpp` (legacy direct bootstrap) and `src/core/application/application.cpp` (structured `Core::Application` object). Both must stay behaviorally aligned.
- **Offset-driven**: Loads game offsets from JSON/HPP files produced by a CS2 dumper. The offset pipeline caches files in `build/Release/cache_offsets/`.
- **Layered design**: Core (process/memory/game/SDK) → Features (ESP, aimbot, etc.) → Render (overlay/renderer/menu/ImGui) → Config/ Input layers.

### Key Technologies

- **C++20** with MSVC (Visual Studio 2019/2022)
- **CMake 3.20+** for build configuration
- **ImGui** (embedded, in `external/imgui/`)
- **DirectX 11** for rendering
- **WinHTTP / WinINet** for network operations (offset downloads)
- **nlohmann/json** for JSON parsing (fetched via CMake FetchContent)
- **Windows API** for process/memory access, stealth, and overlay window management

---

## Directory Structure (High-Level)

```
cs2overlay/
├── CMakeLists.txt          # Build configuration
├── build.bat               # Automated build script (Windows)
├── Structure.md            # Architecture documentation (PRIMARY REFERENCE)
├── QWEN.md                 # This file — AI agent context
├── offsets/
│   └── output/             # CS2 dumper output (offsets.json, client_dll.json, etc.)
├── external/
│   └── imgui/              # ImGui library (embedded)
├── scripts/
│   └── mutate_signature.ps1  # Post-build EXE signature mutation
├── src/
│   ├── main.cpp            # Legacy entry point
│   ├── config/             # ConfigManager, settings, serialization
│   ├── core/
│   │   ├── application/    # Structured Application class (alternative entry)
│   │   ├── game/           # GameManager, entity lists, local player
│   │   ├── math/           # Math utilities
│   │   ├── memory/         # Memory manager, pattern scanner
│   │   ├── process/        # Process attach/detach, module lookup, stealth
│   │   └── sdk/            # Offset loading, parsing, application, SDK structs
│   ├── features/           # Game features (aimbot, esp, radar, rcs, triggerbot, etc.)
│   ├── input/              # Input manager, keybinds, synthetic mouse
│   ├── render/
│   │   ├── draw/           # Draw list, world-to-screen
│   │   ├── menu/           # ImGui menu UI (tabs, components, theme)
│   │   ├── overlay/        # Transparent overlay window management
│   │   └── renderer/       # D3D11 device/swapchain, ImGui integration
│   └── utils/              # Logger, math, string utilities, timer
└── build/                  # CMake build output (gitignored)
```

For the full canonical structure, see `Structure.md`.

---

## Building and Running

### Prerequisites

- **Windows** (x64)
- **Visual Studio 2019 or 2022** with C++ Desktop Development workload
- **CMake 3.20+** in PATH
- **CS2 offset files** from a dumper (placed in `offsets/output/`)

### Quick Build (Recommended)

Run the provided build script:

```bat
build.bat
```

This script will:
1. Copy offset files from `offsets/output/` to `build/Release/cache_offsets/`
2. Detect Visual Studio version and configure CMake
3. Build the Release target
4. Run post-build EXE signature mutation

The output executable: `build/Release/cs2overlay.exe`

### Manual Build

```bat
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

### Runtime Expectations

- The overlay looks for `cs2.exe` process and attaches automatically
- If CS2 is not running, it retries every 5 seconds
- Config files stored in `build/Release/configs/`
- Offset cache stored in `build/Release/cache_offsets/`
- Press **INSERT** to toggle the menu
- Press **END** to exit the application

---

## Development Conventions

### Code Style

- C++20, MSVC `/W4` warnings
- Snake case for files, PascalCase for classes/namespaces, camelCase for functions/variables
- Headers use `#pragma once`
- Namespaced design: `Core::`, `Render::`, `Features::`, `Config::`, `Input::`, `SDK::`

### Critical Rules (From Structure.md)

These are non-negotiable and were added after actual bugs:

1. **Startup/Shutdown**: Every early return after successful resource acquisition must clean up that resource. Process attach → overlay creation → renderer init all must have matching teardown on failure.
2. **Process Recovery**: CS2 restart is a normal event. Losing the process clears runtime state. Reattach replaces old handles (never stacks).
3. **Config Safety**: Missing default config falls back to defaults. Missing non-default config reports failure. Broken configs reset to defaults. `LastError` cleared after successful operations.
4. **State Publication**: When backend has no valid frame, explicitly publish empty state — never assume "no new data" means "keep old data".
5. **Idempotent Registration**: `FeatureManager::RegisterAll()` must be idempotent. Never duplicate registrations.
6. **Two Entry Paths**: Changes to startup, shutdown, config loading, or error handling in one entry path must be applied to the other.

### When Adding New Features

1. Add feature files under `src/features/<feature_name>/`
2. Register in `FeatureManager::RegisterAll()` (keep it idempotent)
3. Wire enabled flag into `ConfigManager::ApplySettings()`
4. Add persistent settings to config registry (`BuildRegistry()`)
5. Ensure menu reads/writes the config

### When Adding New Config Fields

1. Add field to the appropriate config struct in `settings.h` or feature config
2. Register in `BuildRegistry()` in `config_manager.cpp`
3. Ensure menu reads/writes it
4. Define a sensible default value

### Thread Safety

- `Config::SettingsMutex` (shared_mutex) protects all config reads/writes
- GameManager uses double-buffered render data with mutex protection
- Input helpers (`GetAsyncKeyState()`, `SendInput()`) must stay on the **render thread only**
- Memory reads happen on the **memory thread**

---

## Key Files Reference

| File | Purpose |
|------|---------|
| `Structure.md` | **Primary architecture reference** — update this on any structural change |
| `CMakeLists.txt` | Build config: ImGui sources, dependencies (nlohmann/json), linking, post-build mutation |
| `build.bat` | Automated build: offset copying, VS detection, CMake configure + build |
| `src/main.cpp` | Legacy direct bootstrap entry point |
| `src/core/application/` | Structured Application class (alternative bootstrap) |
| `src/config/config_manager.cpp` | Config serialization/deserialization, `BuildRegistry()` |
| `src/config/settings.h` | Global settings struct, feature configs, performance settings |
| `src/features/feature_manager.h` | Feature registration, lazy initialization, update/render dispatch |
| `src/core/process/process.cpp` | Process attach/detach, handle management, CS2 discovery |
| `src/core/sdk/offset_loader.cpp` | Offset file loading (JSON → HPP parsing) |
| `src/render/overlay/overlay.cpp` | Transparent overlay window, CS2 window tracking |
| `src/render/renderer/renderer.cpp` | D3D11 device/swapchain, frame begin/end, VSync |

---

## Feature List

| Feature | Description |
|---------|-------------|
| **ESP** | Entity visualization (boxes, health, weapons, bones, distance) |
| **Aimbot** | Aim assistance with configurable smoothing and FOV |
| **Triggerbot** | Auto-fire when crosshair is on target |
| **RCS** | Recoil Control System (automatic mouse compensation) |
| **Radar** | 2D minimap overlay showing entity positions |
| **Bomb** | Bomb timer and placement tracking |
| **FootstepsEsp** | Audio-based footsteps visualization |
| **Misc** | Miscellaneous utilities (bhop, crosshair, etc.) |
| **DebugOverlay** | Debug information overlay (FPS, UPS, process state) |

---

## Useful Commands

```bat
:: Build the project
build.bat

:: Clean rebuild (delete build/ first)
rmdir /s /q build && build.bat

:: Run CMake configuration only
cd build && cmake --build . --config Release

:: Check CMake version
cmake --version

:: Find Visual Studio installation
where /R "C:\Program Files\Microsoft Visual Studio" vcvars64.bat
```

---

## Common Issues

| Problem | Solution |
|---------|----------|
| CMake not found | Install CMake and add to PATH |
| Visual Studio not detected | Install VS 2019/2022 with C++ Desktop Development |
| Missing offsets | Place dumper output in `offsets/output/` or run build.bat |
| EXE signature mutation fails | Check `scripts/mutate_signature.ps1` execution policy |
| Process attach fails | Ensure CS2 is running and overlay has appropriate permissions |

---

## Documentation Rule

If any of the following change, update `Structure.md` in the same task:
- Folder layout
- Startup flow
- Process lifecycle
- Config persistence rules
- Feature registration rules
- Render or memory thread ownership
- Offset loading pipeline
- Menu architecture
