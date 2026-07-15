cmake_minimum_required(VERSION 3.24)
project(cs2overlay VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CS2OVERLAY_MUTATE_BINARY "Randomize the built executable (breaks reproducible builds)" OFF)
option(CS2OVERLAY_BUILD_TESTS "Build project unit tests" ON)

# ── Dependencies ──────────────────────────────────────────────────────────────

# nlohmann::json via FetchContent
include(FetchContent)

# CMP0135: Use actual file timestamp for extracted content (avoid warning)
cmake_policy(SET CMP0135 NEW)
FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# ── ImGui sources ─────────────────────────────────────────────────────────────

set(IMGUI_DIR "external/imgui")
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${IMGUI_DIR}/imgui.cpp")
  message(FATAL_ERROR
    "Dear ImGui is missing. Run: git submodule update --init --recursive")
endif()
# NOTE: If you add or remove ImGui files, re-run CMake configuration.
set(IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_dx11.cpp"
)

# NOTE: If you add or remove source files in src/, re-run CMake configuration.
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
)

add_executable(cs2overlay ${SOURCES} ${IMGUI_SOURCES})

target_compile_definitions(cs2overlay PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

if(MSVC)
  target_compile_options(cs2overlay PRIVATE /W4 /utf-8 /permissive-)
endif()

target_include_directories(cs2overlay PRIVATE
    src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Add minimal linking for windows
target_link_libraries(cs2overlay PRIVATE user32 d3d11 d3dcompiler dxgi dwmapi wininet ntdll psapi dxguid nlohmann_json::nlohmann_json)

# ── Post-build: mutate EXE signature so each build has a unique SHA256 ──
set(MUTATE_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/mutate_signature.ps1")
if(CS2OVERLAY_MUTATE_BINARY AND EXISTS "${MUTATE_SCRIPT}")
  add_custom_command(
    TARGET cs2overlay POST_BUILD
    COMMAND powershell -ExecutionPolicy Bypass -NonInteractive
            -File "${MUTATE_SCRIPT}"
            -ExePath "$<TARGET_FILE:cs2overlay>"
    COMMENT "Mutating EXE signature..."
    VERBATIM
  )
endif()

include(GNUInstallDirs)
install(TARGETS cs2overlay RUNTIME DESTINATION .)
# Install the whole models directory so the Chams textures (PNGs) are
# shipped alongside the GLB meshes.
install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/models"
    DESTINATION assets
    FILES_MATCHING PATTERN "*.*"
)

include(CTest)
if(BUILD_TESTING AND CS2OVERLAY_BUILD_TESTS AND EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
  add_subdirectory(tests)
endif()
