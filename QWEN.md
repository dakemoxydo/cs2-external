# QWEN Project Guide For `cs2overlay`

This file is a compact AI-oriented working guide for the current project state.

Primary source of truth:
- `Structure.md`

If this file conflicts with `Structure.md`, update this file and follow `Structure.md`.

## 1. What This Project Is

`cs2overlay` is a C++20 external CS2 overlay application with:
- Win32 process management
- DX11 transparent overlay rendering
- ImGui menu
- async offset loading
- separate memory and render loops

The codebase contains two startup flows:
- `src/main.cpp`
- `src/core/application/application.cpp`

They must stay behaviorally aligned until one is intentionally removed.

## 2. High-Level Architecture

Dependency direction:

```text
config -> used by almost every runtime layer
core   -> no dependency on render/features business logic
features -> depend on core and render/draw
render -> may coordinate features, but should not own game-memory logic
```

Hard rules:
- Do not move render concerns into `core/`.
- Do not move game-memory traversal into `render/`.
- Do not make features read CS2 memory ad hoc if `GameManager` or SDK wrappers should own that data.

## 3. Thread Ownership

### Render Thread

Owns:
- Windows message pump
- input polling
- menu open/close
- `FeatureManager::UpdateAll()`
- all `SendInput` / `GetAsyncKeyState` driven logic
- overlay rendering

Rule:
- Anything involving synthetic input or real-time key checks stays on the render thread.

### Memory Thread

Owns:
- attach retry loop
- game memory reads
- entity rebuild
- `GameManager::Update()`

Rule:
- If process state becomes invalid, publish empty frame state instead of keeping stale values alive.

## 4. Critical Runtime Rules

### Startup / Shutdown

- Every early return after attach must detach the process.
- Every early return after overlay creation must destroy the overlay.
- Every partial renderer failure must release D3D resources.
- `main.cpp` and `Application::Initialize()` must stay in sync on cleanup behavior.

### Process Recovery

- A CS2 restart is a normal runtime event, not a fatal one.
- Old `PID` and `HANDLE` values must never survive failed attach or dead process state.
- Reattach must replace old handles instead of stacking new state on top.

### Game State Publication

- `GameManager` is responsible for publishing render-safe state.
- If entity list rebuild fails, render-visible state must be cleared.
- Never rely on "no update happened" as permission to keep old data visible.

## 5. Config Rules

Configs live next to the executable in:

```text
configs/
```

Rules:
- `default` and `default.json` must resolve to the same config file.
- Missing config file must fall back to defaults, not hard fail.
- Invalid config content must reset to defaults safely.
- Any user-facing setting that should persist must be added to `BuildRegistry()` in `src/config/config_manager.cpp`.

If you add a setting and forget `BuildRegistry()`, that is a bug.

## 6. Feature Rules

Features live under:

```text
src/features/<feature_name>/
```

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

Rules:
- Each feature must fit the `IFeature` lifecycle.
- `FeatureManager::RegisterAll()` must stay idempotent.
- Feature enable/disable logic must remain compatible with `ConfigManager::ApplySettings()`.
- Persistent feature settings must be represented in both:
  - `settings.h`
  - `BuildRegistry()`

## 7. Memory And Offset Rules

Use:
- `MemoryManager::Read<T>()`
- `MemoryManager::ReadOptional<T>()`
- `MemoryManager::ReadChain<T>()`
- `MemoryManager::ReadRaw()`

Do not:
- call `NtReadVirtualMemory` directly outside `MemoryManager`
- hardcode dumper offsets inside feature logic

Offset pipeline:
1. read from `cache_offsets/`
2. if invalid, try GitHub
3. parse JSON first, HPP second
4. apply into `SDK::Offsets`

Critical offsets:
- `dwEntityList`
- `dwLocalPlayerPawn`
- `dwViewMatrix`

If these are missing, do not assume ESP data is valid.

## 8. Render And Overlay Rules

Overlay:
- must tolerate CS2 window disappearing
- must not leak registered window classes on failure
- must support repeated create/destroy cycles safely

Renderer:
- must be safe after partial init failure
- must release partially created resources immediately on failure

## 9. Rules That Prevent Recently Fixed Bugs

- Do not keep stale process state after CS2 exits.
- Do not keep stale players or local state after frame reconstruction fails.
- Do not add persistent UI settings without serialization support.
- Do not register features twice.
- Do not leave pending UI state stuck after future completion.
- Do not read array-backed input state without key bounds checks.

## 10. Change Checklist

Before finishing a change, verify:
- startup and shutdown are symmetrical
- process recovery still works conceptually
- config persistence still matches visible settings
- render thread still owns input logic
- memory thread still owns game-memory reconstruction
- `Structure.md`, `QWEN.md`, and `GEMINI.md` remain aligned if rules changed
