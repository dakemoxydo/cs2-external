# Structure And Engineering Rules For `cs2overlay`

This file is the source of truth for the current project layout, runtime model, config rules, render pipeline, and feature boundaries.

Update this file whenever any of the following change:
- folder ownership or subsystem boundaries
- startup or shutdown flow
- process attach or detach behavior
- config persistence or feature enable wiring
- snapshot or telemetry publication
- renderer, overlay, or menu architecture
- runtime assets used by features

## 1. Project Layout

```text
cs2overlay/
|-- CMakeLists.txt
|-- Structure.md
|-- build.bat
|-- imgui.ini
|-- assets/
|   `-- models/
|       |-- tm_phoenix.glb
|       |-- ctm_sas.glb
|       `-- supporting textures...
|-- build/
|   `-- Release/
|       |-- cs2overlay.exe
|       |-- cs2overlay.log
|       |-- cache_offsets/
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
    |   |   |-- app_state.h
    |   |   |-- application.cpp
    |   |   `-- application.h
    |   |-- game/
    |   |   |-- entity_list.h
    |   |   |-- game_manager.cpp
    |   |   |-- game_manager.h
    |   |   |-- game_manager_getters.cpp
    |   |   |-- local_player.h
    |   |   |-- visibility_manager.cpp
    |   |   `-- visibility_manager.h
    |   |-- math/
    |   |   |-- math.cpp
    |   |   `-- math.h
    |   |-- memory/
    |   |   |-- memory_manager.h
    |   |   |-- pattern_scanner.cpp
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
    |       |-- offsets.cpp
    |       |-- offsets.h
    |       |-- structs.h
    |       |-- updater.cpp
    |       `-- updater.h
    |-- features/
    |   |-- aimbot/
    |   |-- bomb/
    |   |-- chams/
    |   |-- debug_overlay/
    |   |-- esp/
    |   |-- feature_base.h
    |   |-- feature_manager.cpp
    |   |-- feature_manager.h
    |   |-- misc/
    |   |-- radar/
    |   |-- rcs/
    |   |-- sound_esp/
    |   `-- triggerbot/
    |-- input/
    |   |-- input_manager.cpp
    |   |-- input_manager.h
    |   `-- keybinds.h
    |-- render/
    |   |-- draw/
    |   |   |-- draw_list.cpp
    |   |   `-- draw_list.h
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

There is one executable boot path:
- `src/main.cpp` constructs `Core::Application`.
- `src/core/application/application.cpp` owns initialization and runtime loops.

The single path must keep these responsibilities aligned:
- stealth setup
- offset update startup behavior
- process attach and detach
- overlay, renderer, and ImGui startup
- feature registration
- config load
- cleanup on failure

Do not add a second startup path without documenting ownership and shutdown semantics.

## 3. Runtime Model

The runtime is split into two long-lived threads.

### Render Thread

Owned by:
- `Core::Application::RenderLoop()`

Responsibilities:
- Windows message pump
- input polling and hotkeys
- menu toggle and close handling
- feature `UpdateAll()`
- overlay move and resize tracking
- frame begin and end
- feature rendering
- ImGui rendering
- FPS pacing and VSync toggling

Rules:
- synthetic mouse input and key state queries stay here
- feature `Update()` is render-thread logic
- feature `Render()` must assume it runs after `GameManager::Update()` published the latest snapshot

### Memory Thread

Owned by:
- the lambda in `src/main.cpp`
- or `Core::Application::MemoryThreadLoop()`

Responsibilities:
- process retry and reattach
- memory reads
- entity reconstruction
- local player state collection
- bomb state collection
- combat telemetry collection
- immutable frame snapshot publication
- UPS pacing

Rules:
- raw memory traversal belongs here
- if process or offsets become invalid, publish empty state instead of leaving stale state alive

## 4. Core Runtime Data Model

### `GameSnapshot`

`Core::GameManager` does not expose live mutable render buffers.

Cross-thread state is published as an immutable `std::shared_ptr<const GameSnapshot>`.

Current snapshot contents include:
- `viewMatrix`
- `players`
- local player spatial state
- local angles, shoot angle, and aim punch
- local shots fired
- local team, scope state, crosshair, and pawn
- bomb info
- local weapon name and range
- raw bullet impacts
- `frameTimeSeconds`
- combat telemetry:
  - `shotEvents`
  - `bulletTraceEvents`
  - `hitEvents`
- `movementAudioEvents`
- per-entity `boneTransforms` (up to `SDK::MAX_GAME_BONES`) when bone reading is enabled

Rules:
- never add a new render-visible gameplay field as a hidden global if it belongs in the frame snapshot
- render-side code must consume snapshots or thin getters over snapshots
- do not return mutable references to internal vectors
- render features must not call `MemoryManager` directly; expensive game reads belong to the memory thread

### Combat Telemetry

Combat-adjacent features now share one event model.

Produced by `GameManager::UpdateCombatTelemetry()`:
- `ShotEvent`
- `BulletTraceEvent`
- `HitEvent`
- `MovementAudioEvent`

Consumed by:
- `ESP` bullet tracers
- `ESP` hitmarker
- `Sound ESP`

Rules:
- new combat-visual features should prefer telemetry events over ad-hoc render-thread heuristics
- if event retention windows change, review tracer, hitmarker, and sound behavior together

## 5. Core Modules

### `src/config/`

Files:
- `config_manager.cpp/.h`
- `settings.h`

Responsibilities:
- own the global settings object
- serialize and deserialize profiles
- apply feature enable state
- expose thread-safe read and mutate helpers

Current behavior:
- configs live under `build/Release/configs/` next to the executable output
- `Config::ReadSettings`, `CopySettings`, `MutateSettings`, and `MutateSettingsVoid` are the supported access patterns
- runtime-only UI state does not belong in persisted config

Critical rules:
- every persistent setting must be added to `BuildRegistry()`
- adding a field to `settings.h` without registry wiring breaks persistence
- UI writes must go through the thread-safe config mutation path

### `src/core/process/`

Responsibilities:
- find and attach `cs2.exe`
- validate handle lifetime
- detach cleanly
- module base lookup
- stealth behavior

Critical rules:
- never retain stale handles across a restart
- if process death is detected, clear process state immediately
- attach retry must let the overlay survive a CS2 restart

### `src/core/game/`

Responsibilities:
- build one coherent frame from memory
- manage local player state
- rebuild player list
- publish snapshot
- generate combat telemetry

Current behavior:
- `GameManager::Update()` runs only on the memory thread
- telemetry and player state are rebuilt each update
- `PublishFrameState()` creates the immutable snapshot consumed by render features
- `VisibilityManager` can supply trace-based visibility, with fallback to spotted state

Critical rules:
- invalid backend data must result in empty publication, not stale publication
- if new gameplay data is shared with render code, add it to `GameSnapshot`
- avoid reintroducing cross-thread raw references

### `src/core/sdk/`

Responsibilities:
- load, parse, validate, and publish offsets
- expose typed wrappers for game objects
- define shared SDK structs used by memory and render code

Current offset model:
- parsed into `SDK::OffsetSet`
- atomically published through `SDK::Offsets`
- copied at the beginning of `GameManager::Update()`

Offset inputs:
- cached JSON lives in `build/Release/cache_offsets/`
- GitHub download is the fallback source

Critical rules:
- if a field is used by wrappers or features, it must exist in `OffsetSet`
- `dwEntityList`, `dwLocalPlayerPawn`, and `dwViewMatrix` remain minimum viable offsets
- hot offset refresh must not mutate global offsets in place

### `src/core/memory/`

Responsibilities:
- validated `Read<T>()`
- optional reads
- raw reads
- chain reads

Critical rules:
- default values from failed reads must always be treated as suspect by callers
- do not spam hot-path logging from here

## 6. Feature Layer

Shared files:
- `feature_base.h`
- `feature_manager.cpp/.h`

Current registered features:
- ESP
- Chams
- Aimbot
- Triggerbot
- Misc
- Bomb
- Radar
- DebugOverlay
- RCSSystem
- SoundEsp

Feature manager behavior:
- registration is factory-based
- `RegisterAll()` is idempotent
- instances are lazy-created when config enables them
- storage is private; configuration uses the `SetEnabled()` facade

Critical rules:
- every new feature must be registered in `FeatureManager::RegisterAll()`
- every enable flag must be reflected in `ConfigManager::ApplySettings()`
- if a feature has persistent settings, they must be in config registry and menu UI

### `ESP`

Responsibilities:
- boxes, labels, health, snap lines
- rounded skeleton
- off-screen indicators
- bullet tracers
- hitmarker

Current behavior:
- tracers and hitmarker are telemetry-driven
- combat visual state is stored on the feature instance, not in render-local statics

### `Sound ESP`

Responsibilities:
- world-space sound wave rendering for movement audio

Current behavior:
- consumes `movementAudioEvents`
- uses snapshot `frameTimeSeconds`
- no longer relies on `ImGui::GetTime()` for animation timing

### `Chams`

Responsibilities:
- load skinned GLB meshes from `assets/models`
- upload meshes to DX11
- read game bone positions
- build skinning matrices
- queue and flush custom mesh draws

Current assets:
- `assets/models/tm_phoenix.glb`
- `assets/models/ctm_sas.glb`

Important constraint:
- current runtime uses bind-pose orientation from GLB and live in-game translation from the game
- if mesh deformation still looks wrong, the next likely source is `gltf_to_game_bone_map`

Critical rules:
- do not assume arbitrary game bone memory layouts without verifying against the project’s current `BoneData` usage
- if chams data becomes invalid, skip the draw instead of pushing garbage matrices to GPU

## 7. Render Layer

### `src/render/overlay/`

Responsibilities:
- locate CS2 window
- create layered transparent overlay
- track game window move and resize

Critical rules:
- `Create()` must not leak registered classes on failure
- `Destroy()` must be safe after partial init
- `UpdatePosition()` must tolerate CS2 disappearing

### `src/render/renderer/`

Responsibilities:
- DX11 device and swap chain
- render target lifecycle
- resize handling
- VSync state

Current behavior:
- `Renderer` uses `ComPtr`
- `HandleResize()` recreates the render target after `ResizeBuffers`
- `GetDevice()` and `GetContext()` are also used by `Chams`

Critical rules:
- partial init failures must be safe to retry
- custom GPU features must preserve and restore render state if they touch DX11 state directly

### `src/render/menu/`

Responsibilities:
- top-level menu
- 4-tab layout: `Visuals`, `Legit`, `Misc`, `Settings`
- reusable UI components
- config management UI

Current architecture:
- left nav rail
- central live preview used primarily by `Visuals`
- right settings pane with cards and grouped controls
- `tab_visuals.cpp` contains `ESP`, `Chams`, `Radar`, `Sound ESP`, and alert-like visual controls

Critical rules:
- menu controls and config registry must evolve together
- keep persistent feature settings out of ad-hoc UI locals
- prefer compact, grouped controls over explanatory filler text

## 8. Input Layer

Files:
- `input_manager.cpp/.h`
- `keybinds.h`

Responsibilities:
- key polling
- one-frame pressed detection
- synthetic mouse deltas and clicks

Critical rules:
- key access must remain bounds-checked
- input helpers remain render-thread only

## 9. Build And Runtime Paths

### Build Inputs

Runtime offset inputs come from:

```text
build/Release/cache_offsets/
```

### Runtime Outputs

Runtime expects:

```text
build/Release/configs/
build/Release/cs2overlay.exe
```

### Assets

Runtime `Chams` assets are loaded from:

```text
assets/models/
```

If mesh filenames or asset folder structure change, update:
- the feature loader in `src/features/chams/chams.cpp`
- this document

### `build.bat`

Responsibilities:
- normalize PATH behavior for MSBuild
- copy offset files into runtime cache
- configure CMake
- build Release

## 10. Current Constraints

- runtime integration still requires a real CS2 session; automated tests cover parsers and pure state logic
- the optional binary mutation step is disabled by default because it breaks reproducible builds
- offset files are still external inputs
- `Chams` currently use GLB bind-pose orientation plus live game translation, not a verified full Source 2 bone rotation pipeline
- final visual correctness of `Chams` depends on mesh export quality and bone mapping quality

These are reasons to centralize and document behavior carefully, not reasons to duplicate logic.

## 11. Mandatory Engineering Rules

### Startup And Shutdown

- every early return after successful process attach must detach
- every early return after overlay creation must destroy the overlay
- every early return after partial renderer creation must call renderer shutdown
- `main.cpp` and `Core::Application::Initialize()` must follow the same cleanup contract

### Process Recovery

- CS2 restart is a normal runtime event
- losing the process must clear published runtime state
- reattach must replace old handles, not stack on top of them

### Config Safety

- loading missing `default` may fall back to defaults
- loading missing non-default config should fail explicitly
- broken config content must not leave partially applied state
- successful `Load()`, `Save()`, and `LoadDefault()` must clear `LastError`

### Snapshot Publication

- render-visible state must be published explicitly as empty when invalid
- do not interpret "no new data" as "keep stale render state"
- cross-thread mutable references are banned

### UI And Registration

- feature registration must stay idempotent
- UI and persistence must stay in sync
- if a feature appears in the menu, its enable logic must be consistent with `ApplySettings()`

## 12. Checklist For Future Changes

### If you add a new config field

- add it to the correct config struct
- add it to `BuildRegistry()`
- wire it into menu UI if user-facing
- choose and document the default

### If you add a new feature

- add the feature files
- register it in `FeatureManager::RegisterAll()`
- wire enable state into `ConfigManager::ApplySettings()`
- add persistent settings to config registry if needed
- update this file if the feature changes subsystem boundaries

### If you change `GameManager`

- verify invalid data publishes empty snapshot
- verify new shared state belongs in `GameSnapshot`
- verify telemetry producers and consumers still agree on timing windows

### If you change renderer or overlay init

- inspect every failure path
- ensure partially created resources are released
- verify retries remain safe
- verify custom DX11 features still restore pipeline state

### If you change `Chams`

- verify mesh asset path still resolves
- verify GLB loader assumptions still match the exported models
- verify bone mapping assumptions still hold
- if deformation math changes, test both T and CT meshes

## 13. Documentation Rule

If any of the following change, update this file in the same task:
- folder layout
- startup or shutdown flow
- process lifecycle
- config persistence or enable wiring
- snapshot or telemetry architecture
- menu architecture
- renderer or overlay behavior
- assets required by runtime features
