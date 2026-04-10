# Structure And Engineering Rules For `cs2overlay`

This document is the current source of truth for the project structure, runtime model, and maintenance rules.

It should be updated when:
- folders or subsystems change
- feature lifecycle changes
- config fields are added or removed
- process attach / detach behavior changes
- overlay / renderer initialization rules change

## 1. Project Layout

```text
cs2overlay/
|-- CMakeLists.txt
|-- build.bat
|-- offsets/
|   `-- output/
|       |-- offsets.json
|       |-- client_dll.json
|       |-- offsets.hpp
|       `-- client_dll.hpp
|-- scripts/
|   `-- mutate_signature.ps1
|-- external/
|   `-- imgui/
|-- src/
|   |-- main.cpp
|   |-- config/
|   |   |-- config_manager.cpp
|   |   |-- config_manager.h
|   |   `-- settings.h
|   |-- core/
|   |   |-- application/
|   |   |-- game/
|   |   |-- math/
|   |   |-- memory/
|   |   |-- process/
|   |   `-- sdk/
|   |-- features/
|   |   |-- aimbot/
|   |   |-- bomb/
|   |   |-- debug_overlay/
|   |   |-- esp/
|   |   |-- footsteps_esp/
|   |   |-- misc/
|   |   |-- radar/
|   |   |-- rcs/
|   |   `-- triggerbot/
|   |-- input/
|   |-- render/
|   |   |-- draw/
|   |   |-- menu/
|   |   |-- overlay/
|   |   `-- renderer/
|   `-- utils/
|-- build/
|   `-- Release/
|       |-- cs2overlay.exe
|       |-- configs/
|       `-- cache_offsets/
|           |-- offsets.json
|           |-- client_dll.json
|           |-- offsets.hpp
|           `-- client_dll.hpp
`-- Structure.md
```

## 2. Entry Points

There are currently two executable flows in the repo:

- `src/main.cpp`
  Legacy direct bootstrap path.
  It initializes stealth, offsets, process, overlay, renderer, ImGui, features, config, then runs memory and render loops directly.

- `src/core/application/application.cpp`
  Structured bootstrap path through `Core::Application`.
  It performs the same high-level work but wraps it in a dedicated application object.

Important:
- Both paths must stay behaviorally aligned.
- If startup, shutdown, config loading, or error handling is changed in one path, the other path must be reviewed immediately.
- Do not fix only one entry path unless the other is intentionally deprecated in the same change.

## 3. Runtime Architecture

The project is split into two main threads:

### Render Thread

Owned by:
- `main.cpp` main loop
- or `Core::Application::RenderLoop()`

Responsibilities:
- Windows message pump
- menu toggle handling
- input polling
- calling `FeatureManager::UpdateAll()`
- overlay position updates
- ImGui frame construction
- rendering ESP and menu
- FPS limiting and VSync handling

Rules:
- Any code that calls `GetAsyncKeyState`, `SendInput`, or interacts with user input must stay on the render thread.
- Features should assume `Update()` runs on the render thread.

### Memory Thread

Owned by:
- lambda in `main.cpp`
- or `Core::Application::MemoryThreadLoop()`

Responsibilities:
- periodic attach retry to `cs2.exe`
- reading live game memory
- updating `GameManager`
- UPS limiting

Rules:
- Raw memory traversal and entity reconstruction belong here.
- If the target process disappears, this thread must reset runtime state instead of leaving stale data visible.

## 4. Core Modules

### `src/config/`

Files:
- `config_manager.cpp/.h`
- `settings.h`

Responsibilities:
- own global runtime settings
- serialize and deserialize configs
- apply settings to feature enable state

Current behavior:
- Configs are stored under `configs/` next to the executable.
- Config names are normalized so `default` and `default.json` refer to the same file.
- Missing config file is not a fatal error.
- Missing config now means: load default in-memory settings and continue.
- Invalid config content means: reset to defaults, record `LastError`, and continue safely.

Critical maintenance rule:
- Every setting that is user-editable and expected to persist must be added to `BuildRegistry()`.
- If a new field is added to `settings.h` or any feature config and is not registered, persistence is considered broken.

### `src/core/process/`

Files:
- `process.cpp/.h`
- `module.cpp/.h`
- `stealth.cpp/.h`

Responsibilities:
- process discovery
- attach / detach lifecycle
- target handle ownership
- module base lookup
- stealth behavior

Current behavior:
- `Process::Attach()` finds `cs2.exe`, tries handle theft first, falls back to `OpenProcess`.
- `Process::GetProcessId()` and `Process::GetHandle()` validate whether the underlying process is still alive.
- If the process has exited, internal process state is cleared automatically.
- If attach fails because `cs2.exe` is not running, old process state must not survive.

Critical maintenance rules:
- Never keep an old `HANDLE` when reattaching to a new process.
- Never keep an old `PID` after process death or failed attach.
- `module.cpp` must never call `CloseHandle()` on `INVALID_HANDLE_VALUE`.
- Any attach retry logic must be written so a CS2 restart can recover without restarting the overlay.

### `src/core/game/`

Files:
- `game_manager.cpp/.h`
- `game_manager_getters.cpp`
- `entity_list.h`
- `local_player.h`

Responsibilities:
- live game snapshot assembly
- cached local player state
- cached entity state
- double-buffered render data
- frustum culling flags

Current behavior:
- `GameManager::Update()` is driven by the memory thread.
- Player data is reconstructed into a write buffer, then published to a read buffer.
- Scalar state is protected by `stateMutex`.
- `ClearFrameState()` now resets render-visible state when process data becomes invalid.

Critical maintenance rules:
- If process is gone, `GameManager` must publish empty state, not stale state.
- If `client.dll` disappears or entity list cannot be rebuilt, do not leave old players on screen.
- Writes to values that are read under `stateMutex` must also be synchronized with that mutex.
- Render-side code must consume getters, not backend mutable state directly.

### `src/core/sdk/`

Files:
- `offsets.h`
- `offset_file_loader.cpp/.h`
- `offset_parser.cpp/.h`
- `offset_applier.cpp/.h`
- `offset_loader.cpp/.h`
- `entity.h`
- `entity_classes.h`
- `structs.h`
- `updater.h`

Responsibilities:
- load offset sources
- parse offsets
- apply offsets into `SDK::Offsets`
- expose typed wrappers around game objects

Offset pipeline:
1. Load from `cache_offsets/` next to the executable.
2. If cache is missing or invalid, try GitHub fallback.
3. Parse JSON first, HPP second.
4. Apply parsed values to `SDK::Offsets`.

Critical maintenance rules:
- `dwEntityList`, `dwLocalPlayerPawn`, and `dwViewMatrix` are minimum viable offsets.
- If these are missing, the overlay must not pretend it has valid game data.
- Any field used by `entity_classes.h` or feature logic must exist either as parsed data or as an intentional internal hardcoded fallback in `offsets.h`.

### `src/core/memory/`

Files:
- `memory_manager.h`
- `pattern_scanner.h`

Responsibilities:
- safe address validation
- low-level reads and writes
- raw buffer reads

Critical maintenance rules:
- `Read<T>()` is allowed to fail quietly, but callers must treat zero/default return values as potentially invalid.
- Do not add noisy logging inside hot-path memory reads.

## 5. Feature Layer

Folder:
- `src/features/`

Shared files:
- `feature_base.h`
- `feature_manager.cpp/.h`

Current feature set:
- Aimbot
- Bomb
- DebugOverlay
- ESP
- FootstepsEsp
- Misc
- Radar
- RCSSystem
- Triggerbot

Feature manager behavior:
- features are registered as factories
- instances are created lazily
- `RegisterAll()` must be idempotent

Critical maintenance rules:
- `RegisterAll()` must not duplicate registrations if called more than once.
- `UpdateAll()` is render-thread only.
- Feature enable/disable behavior must remain consistent with `ConfigManager::ApplySettings()`.
- If a feature is shown in the menu and has persistent settings, those settings must be present in config registry.

## 6. Render Layer

### `src/render/overlay/`

Responsibilities:
- find the CS2 window
- create transparent overlay window
- keep overlay aligned with the game window

Current behavior:
- class registration is validated
- failed overlay creation unregisters the class
- destroy path unregisters the class even if window creation previously failed

Critical maintenance rules:
- `Create()` must not leak a registered class on failure.
- `Destroy()` must tolerate partial initialization.
- `UpdatePosition()` must not assume CS2 is still alive.

### `src/render/renderer/`

Responsibilities:
- DirectX 11 device and swap chain
- frame begin/end
- VSync state
- ImGui backend

Critical maintenance rules:
- `Renderer::Init()` must be safe to call after a previous partial init failure.
- If `GetBuffer()` or `CreateRenderTargetView()` fails, partially created D3D objects must be released immediately.
- Startup code must destroy overlay/process state if renderer init fails.

### `src/render/menu/`

Responsibilities:
- top-level menu
- tab layout
- config management UI
- theme controls
- offset update buttons

Current behavior:
- config list initializes lazily
- offset update status distinguishes pending / success / failure

Critical maintenance rules:
- settings UI and config registry must evolve together
- do not add new persistent UI fields without updating config serialization

## 7. Input Layer

Files:
- `input_manager.cpp/.h`
- `keybinds.h`

Responsibilities:
- key state polling
- one-frame pressed detection
- synthetic mouse movement and clicks

Critical maintenance rules:
- virtual key access must always be bounds-checked
- input helpers must remain render-thread only

## 8. Build And Runtime Paths

### Build Inputs

The project expects CS2 dumper files under:

```text
offsets/output/
```

### Build Output

Runtime expects:

```text
build/Release/cache_offsets/
build/Release/configs/
```

### `build.bat`

Responsibilities:
- copy offset files from `offsets/output/` into runtime cache
- configure CMake
- build Release target

Maintenance rule:
- If runtime cache expectations change, `build.bat`, `Structure.md`, and offset loader logic must be updated together.

## 9. Current Known Constraints

- There are two startup implementations: `main.cpp` and `Core::Application`.
- The project currently relies on external offset files being present or downloadable.
- Environment-specific MSBuild issues can still block local compilation even when the code is correct.

These are not reasons to duplicate logic further. They are reasons to centralize behavior more aggressively over time.

## 10. Rules Added After Recent Bug Fixes

These rules are mandatory because recent bugs came from violating them.

### Startup And Shutdown

- Every early return after successful process attach must detach the process.
- Every early return after overlay creation must destroy the overlay.
- Every early return after partial renderer creation must call renderer shutdown.
- `main.cpp` and `Application::Initialize()` must follow the same cleanup contract.

### Process Recovery

- A CS2 restart must be treated as a normal runtime event.
- Losing the process must clear runtime state.
- Reattach must replace old handles, not stack on top of them.

### Config Safety

- Loading a missing config must not leave the application in a half-configured state.
- Loading a broken config must reset to defaults instead of keeping partially applied values.
- `LastError` must be cleared after successful `Load()`, `Save()`, or `LoadDefault()`.

### State Publication

- Render-visible state must be explicitly published as empty when the backend has no valid frame.
- Never assume "no new data" means "keep old data".

### Registration And UI

- Factory registration functions must be idempotent.
- Menu state that reads from futures must update the pending flag when the future completes.
- Lists shown in UI should initialize themselves when safe instead of requiring a manual refresh for first use.

## 11. Checklist For Future Changes

When changing the project, verify all relevant items below.

### If you add a new config field

- add the field to the correct config struct
- add it to `BuildRegistry()`
- ensure the menu reads and writes it
- decide what the default value should be

### If you add a new feature

- add the feature files
- register it in `FeatureManager::RegisterAll()`
- ensure registration stays idempotent
- wire its enabled flag into `ConfigManager::ApplySettings()`
- add its persistent settings to config registry if needed

### If you change process attach logic

- test the mental model for:
  - CS2 not started yet
  - CS2 starts later
  - CS2 closes while overlay is alive
  - CS2 restarts with a new PID

### If you change overlay or renderer init

- inspect all failure paths
- ensure every partially acquired resource is released
- ensure later retries are still safe

### If you change `GameManager`

- decide what happens when data is invalid
- verify empty state is published correctly
- verify thread-safe reads still use the intended lock / buffer model

## 12. Documentation Rule

If any of the following change, update this file in the same task:
- folder layout
- startup flow
- process lifecycle
- config persistence rules
- feature registration rules
- render / memory thread ownership
- offset loading pipeline
