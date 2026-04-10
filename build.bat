@echo off
setlocal
chcp 65001 >nul
echo [BUILD] Starting CS2 External build...
echo.

:: Step 1: Copy offsets from offsets/output/ to build output
if exist "offsets\output\offsets.json" (
    echo [OFFSET] Found offsets in offsets/output/
    if not exist "build\Release\cache_offsets" mkdir "build\Release\cache_offsets"
    copy /Y "offsets\output\offsets.json" "build\Release\cache_offsets\offsets.json" >nul
    echo [OFFSET] Copied offsets.json to cache_offsets/
    if exist "offsets\output\client_dll.json" (
        copy /Y "offsets\output\client_dll.json" "build\Release\cache_offsets\client_dll.json" >nul
        echo [OFFSET] Copied client_dll.json to cache_offsets/
    )
    if exist "offsets\output\client_dll.hpp" (
        copy /Y "offsets\output\client_dll.hpp" "build\Release\cache_offsets\client_dll.hpp" >nul
        echo [OFFSET] Copied client_dll.hpp to cache_offsets/
    )
    if exist "offsets\output\offsets.hpp" (
        copy /Y "offsets\output\offsets.hpp" "build\Release\cache_offsets\offsets.hpp" >nul
        echo [OFFSET] Copied offsets.hpp to cache_offsets/
    )
) else (
    echo [OFFSET] No offsets/output/ found, skipping offset copy.
    echo [OFFSET] Place your dumper output folder at: offsets\output\
)

echo.

:: Check for CMake
echo [CHECK] Checking for CMake...
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake not found! Please install CMake and add it to PATH.
    echo [ERROR] Download from: https://cmake.org/download/
    echo.
    pause
    exit /b 1
)
echo [CHECK] CMake found:
cmake --version | findstr /C:"cmake version"

:: Check for Visual Studio and select generator
set GENERATOR=
set VS_YEAR=

if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set GENERATOR=-G "Visual Studio 17 2022"
    set VS_YEAR=2022
    goto vs_found
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    set GENERATOR=-G "Visual Studio 17 2022"
    set VS_YEAR=2022
    goto vs_found
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    set GENERATOR=-G "Visual Studio 17 2022"
    set VS_YEAR=2022
    goto vs_found
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set GENERATOR=-G "Visual Studio 16 2019"
    set VS_YEAR=2019
    goto vs_found
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    set GENERATOR=-G "Visual Studio 16 2019"
    set VS_YEAR=2019
    goto vs_found
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    set GENERATOR=-G "Visual Studio 16 2019"
    set VS_YEAR=2019
    goto vs_found
)

:vs_not_found
echo [ERROR] Visual Studio 2019 or 2022 not found!
echo [ERROR] Please install Visual Studio with C++ desktop development workload.
echo [ERROR] Download from: https://visualstudio.microsoft.com/downloads/
echo.
pause
exit /b 1

:vs_found
echo [CHECK] Visual Studio %VS_YEAR% found, using generator: %GENERATOR%
echo.

if not exist "build" mkdir build
cd /d "%~dp0build"

echo [BUILD] Configuring CMake...
cmake %GENERATOR% -A x64 ..
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    echo.
    pause
    exit /b 1
)

echo [BUILD] Building (Release)...
cmake --build . --config Release

echo.
if %ERRORLEVEL% equ 0 (
    echo [BUILD] Success! Executable located at: build\Release\cs2overlay.exe
) else (
    echo [BUILD] Build Failed! Check errors above.
)

echo.
cd /d "%~dp0"
pause
