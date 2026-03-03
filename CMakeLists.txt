cmake_minimum_required(VERSION 3.20)
project(cs2overlay VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IMGUI_DIR "external/imgui")
file(GLOB IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_dx11.cpp"
)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(cs2overlay ${SOURCES} ${IMGUI_SOURCES})

target_include_directories(cs2overlay PRIVATE 
    src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Add minimal linking for windows
target_link_libraries(cs2overlay PRIVATE user32 d3d11 dxgi dwmapi wininet)
