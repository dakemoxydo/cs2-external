# cs2overlay

Windows C++20 overlay project built with CMake, Visual Studio 2022 and Dear ImGui.

## Prerequisites

- Windows 10/11 x64
- Visual Studio 2022 with Desktop development with C++
- CMake 3.20 or newer
- Git with submodule support

Initialize dependencies from a clean checkout:

```powershell
git submodule update --init --recursive
```

## Build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCS2OVERLAY_MUTATE_BINARY=OFF -DCS2OVERLAY_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The optional `CS2OVERLAY_MUTATE_BINARY` switch is disabled by default because it
changes the PE timestamp/overlay and makes builds non-reproducible. Installable
artifacts include the executable and the two GLB meshes used by Chams:

```powershell
cmake --install build --config Release --prefix dist
```

## Runtime assets and offsets

The application expects `assets/models/tm_phoenix.glb` and
`assets/models/ctm_sas.glb` beside the installed executable. Offset JSON is
loaded from `cache_offsets/` and refreshed over HTTPS only after parsing and
validation succeed. A failed refresh keeps the last known-good offsets.

## Notes

This project interacts with a running game process and requires appropriate
authorization. Runtime integration requires an actual CS2 session and is not
covered by the unit tests; parser, configuration, and lifecycle code should be
tested independently first.
