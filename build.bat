@echo off
chcp 65001 >nul
echo [BUILD] Starting CS2 External build...

:: Check for CMake
echo [CHECK] Checking for CMake...
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake not found! Please install CMake and add it to PATH.
    echo [ERROR] Download from: https://cmake.org/download/
    pause
    exit /b 1
)
echo [CHECK] CMake found: 
cmake --version | findstr /C:"cmake version"

:: Check for Visual Studio and select generator
set GENERATOR=
set VS_YEAR=

:: Check VS 2022
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
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    set GENERATOR=-G "Visual Studio 17 2022"
    set VS_YEAR=2022
    goto vs_found
)

:: Check VS 2019
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
pause
exit /b 1

:vs_found
echo [CHECK] Visual Studio %VS_YEAR% found, using generator: %GENERATOR%

if not exist "build" mkdir build
cd build

echo [BUILD] Configuring CMake...
cmake %GENERATOR% -A x64 ..

echo [BUILD] Building (Release)...
cmake --build . --config Release

if %ERRORLEVEL% equ 0 (
    echo [BUILD] Success! Executable located at: build\Release\cs2overlay.exe
) else (
    echo [BUILD] Build Failed!
)

cd ..
pause
