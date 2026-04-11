# Structure And Engineering Rules For `cs2overlay`

This document is the source of truth for the current project layout, runtime model, UI architecture, and maintenance rules.

Update this file whenever one of the following changes:
- folders or subsystem boundaries change
- startup or shutdown flow changes
- process attach / detach behavior changes
- config persistence changes
- feature registration changes
- overlay, renderer, or menu architecture changes
- offset pipeline changes

## 1. Project Layout

```text
cs2overlay/
|-- CMakeLists.txt
|-- build.bat
|-- Structure.md
|-- QWEN.md
|-- GEMINI.md
|-- offsets/
|   `-- output/
|       |-- offsets.json
|       |-- client_dll.json
|       |-- offsets.hpp
|       `-- client_dll.hpp
|-- build/
|   `-- Release/
|       |-- cs2overlay.exe
|       |-- cache_offsets/
|       |-- configs/
|       `-- ...
|-- external/
|   `-- imgui/
|-- scripts/
|   `-- mutate_signature.ps1
`-- src/
    |-- main.cpp
    |-- config/
    |   |-- config_manager.cpp
    |   |-- config_manager.h
    |   `-- settings.h
    |-- core/
    |   |-- application/
    |   |   |-- application.cpp
    |   |   |-- application.h
    |   |   |-- app_config.h
    |   |   `-- app_state.h
    |   |-- game/
    |   |   |-- entity_list.h
    |   |   |-- game_manager.cpp
    |   |   |-- game_manager.h
    |   |   |-- game_manager_getters.cpp
    |   |   `-- local_player.h
    |   |-- math/
    |   |   |-- math.cpp
    |   |   `-- math.h
    |   |-- memory/
    |   |   |-- memory_manager.h
    |   |   `-- pattern_scanner.h
    |   |-- process/
    |   |   |-- module.cpp
    |   |   |-- module.h
    |   |   |-- process.cpp
    |   |   |-- process.h
    |   |   |-- stealth.cpp
    |   |   `-- stealth.h
    |   `-- sdk/
    |       |-- entity.h
    |       |-- entity_classes.h
    |       |-- offset_applier.cpp
    |       |-- offset_applier.h
    |       |-- offset_file_loader.cpp
    |       |-- offset_file_loader.h
    |       |-- offset_loader.cpp
    |       |-- offset_loader.h
    |       |-- offset_parser.cpp
    |       |-- offset_parser.h
    |       |-- offsets.h
    |       |-- structs.h
    |       `-- updater.h
    |-- features/
    |   |-- aimbot/
    |   |-- bomb/
    |   |-- debug_overlay/
    |   |-- esp/
    |   |-- footsteps_esp/
    |   |-- misc/
    |   |-- radar/
    |   |-- rcs/
    |   `-- triggerbot/
    |-- input/
    |   |-- input_manager.cpp
    |   |-- input_manager.h
    |   `-- keybinds.h
    |-- render/
    |   |-- draw/
    |   |   |-- draw_list.cpp
    |   |   |-- draw_list.h
    |   |   `-- world_to_screen.h
    |   |-- menu/
    |   |   |-- menu.cpp
    |   |   |-- menu.h
    |   |   |-- menu_theme.cpp
    |   |   |-- menu_theme.h
    |   |   |-- tab_legit.cpp
    |   |   |-- tab_legit.h
    |   |   |-- tab_misc.cpp
    |   |   |-- tab_misc.h
    |   |   |-- tab_settings.cpp
    |   |   |-- tab_settings.h
    |   |   |-- tab_visuals.cpp
    |   |   |-- tab_visuals.h
    |   |   |-- ui_components.cpp
    |   |   `-- ui_components.h
    |   |-- overlay/
    |   |   |-- overlay.cpp
    |   |   `-- overlay.h
    |   `-- renderer/
    |       |-- imgui_manager.cpp
    |       |-- imgui_manager.h
    |       |-- renderer.cpp
    |       `-- renderer.h
    `-- utils/
        |-- logger.cpp
        |-- logger.h
        |-- math.h
        |-- string_utils.h
        `-- timer.h
```

## 2. Entry Points

There are two executable boot paths in the repo:

- `src/main.cpp`
  Legacy direct bootstrap path.
  It initializes stealth, offsets, process, overlay, renderer, ImGui, features, config, then runs the memory and render loops directly.

- `src/core/application/application.cpp`
  Structured bootstrap path through `Core::Application`.
  It performs the same high-level work but wraps it in a dedicated application object.

Rules:
- Keep both paths behaviorally aligned.
- If startup, shutdown, config loading, or error handling changes in one path, review the other path in the same change.
- Do not fix only one entry path unless the other is intentionally deprecated.

## 3. Runtime Model

The runtime is split into two main threads.

### Render Thread

Owned by:
- the main loop in `src/main.cpp`
- or `Core::Application::RenderLoop()`

Responsibilities:
- Windows message pump
- menu toggle handling
- input polling
- `FeatureManager::UpdateAll()`
- overlay position updates
- ImGui frame construction
- rendering ESP and menu
- frame pacing and VSync behavior

Rules:
- Input helpers such as `GetAsyncKeyState()` and synthetic mouse actions stay on the render thread.
- Features should assume `Update()` runs on the render thread.
- UI changes must stay cheap enough to run every frame.

### Memory Thread

Owned by:
- the lambda in `src/main.cpp`
- or `Core::Application::MemoryThreadLoop()`

Responsibilities:
- process retry / attach
- live memory reads
- `GameManager` updates
- UPS limiting

Rules:
- Raw memory traversal and entity reconstruction belong here.
- If the target process disappears, runtime state must be cleared instead of leaving stale data visible.

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
- Missing `default` config falls back to in-memory defaults.
- Missing non-default config returns a load error and does not silently replace the current profile.
- Invalid config content resets to defaults, records `LastError`, and continues safely.

Critical maintenance rules:
- Every user-editable setting that should persist must be added to `BuildRegistry()`.
- If a field is added to `settings.h` or a feature config and is not registered, persistence is broken.
- Keep menu controls and config serialization in sync.

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
- `Process::Attach()` finds `cs2.exe`, tries handle theft first, then falls back to `OpenProcess`.
- `Process::GetProcessId()` and `Process::GetHandle()` validate that the underlying process is still alive.
- If the process exits, internal process state is cleared automatically.

Critical maintenance rules:
- Never keep an old `HANDLE` when reattaching to a new process.
- Never keep an old PID after process death or failed attach.
- `module.cpp` must never call `CloseHandle()` on `INVALID_HANDLE_VALUE`.
- Attach retry logic must allow a CS2 restart to recover without restarting the overlay.

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
- Player data is reconstructed into a write buffer and then published to a read buffer.
- Scalar state is protected by `stateMutex`.
- `ClearFrameState()` resets render-visible state when process data becomes invalid.

Critical maintenance rules:
- If the process is gone, publish empty state, not stale state.
- If `client.dll` disappears or the entity list cannot be rebuilt, do not leave old players on screen.
- Writes to values read under `stateMutex` must also be synchronized with that mutex.
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
- `dwEntityList`, `dwLocalPlayerPawn`, and `dwViewMatrix` are the minimum viable offsets.
- If these are missing, the overlay must not pretend it has valid game data.
- Any field used by `entity_classes.h` or feature logic must exist either as parsed data or as an intentional hardcoded fallback in `offsets.h`.

### `src/core/memory/`

Files:
- `memory_manager.h`
- `pattern_scanner.h`

Responsibilities:
- safe address validation
- low-level reads and writes
- raw buffer reads

Critical maintenance rules:
- `Read<T>()` may fail quietly, but callers must treat zero/default return values as potentially invalid.
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
- Feature enable and disable behavior must stay consistent with `ConfigManager::ApplySettings()`.
- If a feature is shown in the menu and has persistent settings, those settings must be present in config serialization.

## 6. Render Layer

### `src/render/overlay/`

Responsibilities:
- find the CS2 window
- create the transparent overlay window
- keep the overlay aligned with the game window

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
- frame begin / end
- VSync state
- ImGui backend

Critical maintenance rules:
- `Renderer::Init()` must be safe to call after a previous partial init failure.
- If `GetBuffer()` or `CreateRenderTargetView()` fails, partially created D3D objects must be released immediately.
- Startup code must destroy overlay and process state if renderer init fails.

### `src/render/menu/`

Responsibilities:
- top-level menu
- tab layout
- config management UI
- theme controls
- offset refresh controls

Current UI architecture:
- a compact top bar with theme and pacing chips
- a left navigation rail with animated tiles
- a right content pane with cards and grouped sections
- reusable primitives in `ui_components.cpp` for cards, nav tiles, chips, toggles, colors, and hotkeys
- a minimalist visual language with rounded corners, soft gradients, and low-text-noise controls

Critical maintenance rules:
- settings UI and config registry must evolve together
- do not add new persistent UI fields without updating config serialization
- keep text short and actionable
- prefer grouping by intent over long explanatory paragraphs

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
- build the Release target

Maintenance rule:
- If runtime cache expectations change, `build.bat`, `Structure.md`, and offset loader logic must be updated together.

## 9. Current Constraints

- There are still two startup implementations: `main.cpp` and `Core::Application`.
- The project relies on external offset files being present or downloadable.
- Environment-specific MSBuild issues can still block local compilation even when the code is correct.

These are not reasons to duplicate logic further. They are reasons to centralize behavior more aggressively over time.

## 10. Rules Added After Recent Bug Fixes

These rules are mandatory because recent bugs came from violating them.

### Startup And Shutdown

- Every early return after successful process attach must detach the process.
- Every early return after overlay creation must destroy the overlay.
- Every early return after partial renderer creation must call renderer shutdown.
- `main.cpp` and `Core::Application::Initialize()` must follow the same cleanup contract.

### Process Recovery

- A CS2 restart must be treated as a normal runtime event.
- Losing the process must clear runtime state.
- Reattach must replace old handles, not stack on top of them.

### Config Safety

- Loading a missing `default` config may fall back to defaults.
- Loading a missing non-default config should report failure.
- Loading a broken config must reset to defaults instead of keeping partially applied values.
- `LastError` must be cleared after successful `Load()`, `Save()`, or `LoadDefault()`.

### State Publication

- Render-visible state must be explicitly published as empty when the backend has no valid frame.
- Never assume "no new data" means "keep old data".

### UI And Registration

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
- render or memory thread ownership
- offset loading pipeline
- menu architecture
