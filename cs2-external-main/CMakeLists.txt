cmake_minimum_required(VERSION 3.20)
project(cs2overlay VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencies ──────────────────────────────────────────────────────────────

# nlohmann::json via FetchContent
include(FetchContent)
FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

# libcurl via find_package (optional, not required - using WinHTTP instead)
find_package(CURL QUIET)

# ── ImGui sources ─────────────────────────────────────────────────────────────

set(IMGUI_DIR "external/imgui")
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
  target_compile_options(cs2overlay PRIVATE /W4)
endif()

target_include_directories(cs2overlay PRIVATE
    src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Add minimal linking for windows
target_link_libraries(cs2overlay PRIVATE user32 d3d11 d3dcompiler dxgi dwmapi wininet ntdll psapi dxguid nlohmann_json::nlohmann_json)
if(CURL_FOUND)
    target_link_libraries(cs2overlay PRIVATE CURL::libcurl)
endif()

# ── Post-build: mutate EXE signature so each build has a unique SHA256 ──
set(MUTATE_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/mutate_signature.ps1")
if(EXISTS "${MUTATE_SCRIPT}")
  add_custom_command(
    TARGET cs2overlay POST_BUILD
    COMMAND powershell -ExecutionPolicy Bypass -NonInteractive
            -File "${MUTATE_SCRIPT}"
            -ExePath "$<TARGET_FILE:cs2overlay>"
    COMMENT "Mutating EXE signature..."
    VERBATIM
  )
else()
  message(STATUS "mutate_signature.ps1 not found, skipping post-build signature mutation")
endif()
