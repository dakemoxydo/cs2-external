# CS2 Overlay — Project Context

## Project Overview

**CS2 Overlay** is an external cheat/overlay software for the game Counter-Strike 2. It operates as a separate process without injecting DLLs into the game, which enhances safety and reduces detection risk.

The project is written in **C++20** and uses **CMake** as its build system. It features a transparent DirectX 11 overlay rendered on top of the CS2 game window, powered by **ImGui** for UI rendering.

### Key Technologies
- **C++20** (standard)
- **CMake 3.20+** (build system)
- **DirectX 11** (rendering backend)
- **ImGui** (UI framework, vendored in `external/imgui/`)
- **Win32 API** (window management, process handling)
- **nlohmann/json** (JSON parsing, fetched via FetchContent)
- **WinHTTP/WinINet** (network operations for GitHub offset updates)

### Architecture

The codebase follows a strict **layered architecture** with unidirectional dependencies:

```
src/
├── main.cpp              — Thin entry point (~10 lines)
├── core/                 — Core engine layer
│   ├── application/      — Application lifecycle (Application class, state, config)
│   ├── memory/           — Memory reading (NtReadVirtualMemory wrappers)
│   ├── process/          — Process attachment, module base addresses, stealth (PEB spoofing)
│   ├── game/             — Game entities, GameManager (double-buffered entity list)
│   ├── sdk/              — Offsets loading/parsing/applying pipeline
│   ├── math/             — Vectors, angles, matrices, WorldToScreen
│   └── input/            — Mouse/keyboard input handling
├── features/             — Game features layer (all inherit IFeature)
│   ├── aimbot/           — Auto-aim with FOV, smooth, target bone selection
│   ├── esp/              — Box ESP, health bars, names, weapons, skeleton, snaplines
│   ├── footsteps_esp/    — Footstep visualization (walking/jumping/landing circles)
│   ├── triggerbot/       — Auto-fire on crosshair target
│   ├── rcs/              — Recoil Control System (standalone)
│   ├── misc/             — AWP crosshair and other utilities
│   ├── bomb/             — Bomb timer and carrier tracking
│   ├── radar/            — 2D minimap radar
│   ├── debug_overlay/    — Debug visualization
│   ├── feature_base.h    — IFeature interface
│   └── feature_manager.h/cpp — Factory pattern, lazy init, feature coordination
├── render/               — Rendering and UI layer
│   ├── overlay/          — Transparent overlay window management
│   ├── renderer/         — DirectX 11 init, ImGui integration
│   ├── draw/             — Drawing primitives (lines, boxes, circles, etc.)
│   └── menu/             — Main menu UI (7 themes, modular tabs)
├── config/               — Configuration layer
│   ├── config_manager.h/cpp — Config registry with auto-serialization
│   └── settings.h        — GlobalSettings struct with all parameters
└── utils/                — Utilities
    ├── logger.h/cpp      — Logging (Debug/Info/Warn/Error levels)
    ├── string_utils.h    — String helpers
    ├── math.h            — Math helpers
    └── timer.h           — Timing utilities
```

### Multithreading Model

The application uses **two main threads**:

1. **Memory Thread** — Runs in a loop at configurable UPS (64–240). Reads CS2 memory, updates `GameManager`. Supports VSync-synced UPS mode.
2. **Render Thread** — Main thread. Handles Windows message pump, overlay rendering, feature logic (aimbot angles, triggerbot), input polling, and menu UI.

**Double-buffering** is used for the entity list to avoid race conditions between threads — no explicit locks needed for entity data access.

### Offset System (3-Stage Pipeline)

Offsets are loaded via a structured pipeline:

```
FileLoader → Parser → Applier
```

**File formats supported:**
- **Priority:** JSON (`offsets.json` + `client_dll.json`)
- **Fallback:** HPP (`offsets.hpp` + `client_dll.hpp`) — parsed via regex `#define`

**Data flow:**
1. **Build-time:** `build.bat` copies files from `offsets/output/` → `build/Release/cache_offsets/`
2. **Runtime:** Executable reads from `cache_offsets/` next to the `.exe`
3. **GitHub fallback:** If `cache_offsets/` is missing or invalid, downloads from `a2x/cs2-dumper`
4. **UI:** "Update Offsets from GitHub" / "Reload Offsets from Disk" buttons in Settings tab

**Critical validation fields:** `dwEntityList`, `dwLocalPlayerPawn`, `dwViewMatrix` — if any is 0, ESP will not work.

---

## Building and Running

### Prerequisites
- **Visual Studio 2019 or 2022** (Community, Professional, or BuildTools) with C++ desktop development workload
- **CMake 3.20+** in PATH

### Build Command

```bat
build.bat
```

This script:
1. Copies offset files from `offsets/output/` to `build/Release/cache_offsets/`
2. Detects Visual Studio version and configures CMake
3. Builds the Release configuration
4. Outputs `build\Release\cs2overlay.exe`

### Manual CMake Build

```bat
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

### Running

Run `build\Release\cs2overlay.exe` while CS2 is running. The overlay will auto-detect the CS2 window position and size.

- **INSERT** — Toggle menu
- **END** — Close application

### Providing Offsets

Place dumper output files (from [cs2-dumper](https://github.com/a2x/cs2-dumper)) into `offsets/output/`:
- `offsets.json` + `client_dll.json` (preferred), or
- `offsets.hpp` + `client_dll.hpp` (fallback)

Then rebuild.

---

## Development Conventions

### Code Style
- **C++20** with modern features
- **Header-only inline variables** for offsets (C++17 inline semantics)
- **MSVC `/W4`** warning level
- `NOMINMAX` and `WIN32_LEAN_AND_MEAN` defined to avoid Windows header pollution

### Architecture Patterns

1. **Config Registry** — Parameters are registered in `BuildRegistry()` for automatic save/load serialization.
2. **Feature UI (Open-Closed)** — Each feature renders its own UI via `RenderUI()`. The menu delegates, never hardcodes feature settings.
3. **Factory Pattern** — Features are registered as factories; instances are created only on first enable (lazy init).
4. **Double-Buffering** — Entity list written to one buffer, read from another — lock-free.
5. **Thin Entry Point** — `main.cpp` is minimal. All logic lives in the `Application` class.

### Memory Reading
- `MemoryManager::Read<T>()` — Silently returns default-constructed `T{}` on failure
- `MemoryManager::ReadOptional<T>()` — Returns `std::optional<T>`
- `MemoryManager::ReadBatch()` — Batch reading for contiguous memory blocks
- `MemoryManager::ReadRaw()` — Raw byte buffer reading
- **Do not** add verbose logging to `NtRead` operations

### Feature Interface

All features inherit `IFeature` (`feature_base.h`):
- `Update()` — Logic (called from render thread)
- `Render(DrawList&)` — Overlay rendering
- `RenderUI()` — Settings UI in menu
- `GetName()` — Feature name
- `OnEnable()` / `OnDisable()` — Enable/disable hooks
- `Initialize()` — Lazy initialization (called once on first enable)

### Important Notes
- `GameManager::SetScreenSize()` is called **every frame** from the render loop
- `Overlay::UpdatePosition()` is called **every frame**
- WorldToScreen uses `Overlay::GetGameWidth/Height()`
- Feature Manager `UpdateAll()` contains `GetAsyncKeyState`/`SendInput` — must only be called from the **render thread**
- `build.bat` copies offsets before building; the executable reads from `cache_offsets/` at runtime
- `.gitignore` excludes all `build*/` directories
- Post-build script mutates EXE signature for unique SHA256 per build (anti-detection)

### File Naming
- Use snake_case for file names (e.g., `feature_manager.h`, `config_manager.cpp`)
- Headers and sources use `.h` / `.cpp` extensions
