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
    if exist "build\Release\cache_offsets\offsets.json" if exist "build\Release\cache_offsets\client_dll.json" (
        echo [OFFSET] offsets/output/ not found; using existing build cache.
    ) else (
        echo [OFFSET] No offsets/output/ found, skipping offset copy.
        echo [OFFSET] Place your dumper output folder at: offsets\output\
    )
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

:: Check for Visual Studio and select generator.
:: Prefer vswhere (installed with VS) so we also catch VS Enterprise,
:: non-C: installs and custom locations. Falls back to hardcoded paths.
set GENERATOR=
set VS_YEAR=
set VS_INSTALL_DIR=

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
        set "VS_INSTALL_DIR=%%i"
    )
)
if defined VS_INSTALL_DIR (
    if exist "%VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat" (
        echo "%VS_INSTALL_DIR%" | findstr /C:"2022" >nul && (
            set GENERATOR=-G "Visual Studio 17 2022"
            set VS_YEAR=2022
        )
        if not defined GENERATOR (
            echo "%VS_INSTALL_DIR%" | findstr /C:"2019" >nul && (
                set GENERATOR=-G "Visual Studio 16 2019"
                set VS_YEAR=2019
            )
        )
        if not defined GENERATOR (
            :: Unknown edition/year: let CMake auto-detect the generator.
            set VS_YEAR=detected
        )
        goto vs_found
    )
)

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
:: Initialize the MSVC build environment (sets PATH/INCLUDE/LIB for the
:: current shell). Without this, the documented PATH normalization never ran.
if defined VS_INSTALL_DIR (
    if exist "%VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat" (
        call "%VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    )
) else (
    :: Resolve vcvars from the hardcoded generator paths above.
    if "%VS_YEAR%"=="2022" (
        if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
            call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
        ) else (
            call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
        )
    )
)
echo.

if not exist "build" mkdir build
cd /d "%~dp0build"

set "PATH=%PATH%"

:: CMake caches absolute source paths. Recreate only stale generated metadata
:: when the project was moved or opened from a different workspace path.
set "EXPECTED_SOURCE=%~dp0"
set "EXPECTED_SOURCE=%EXPECTED_SOURCE:~0,-1%"
set "EXPECTED_SOURCE=%EXPECTED_SOURCE:\=/%"
set "CACHED_SOURCE="
if exist "CMakeCache.txt" for /f "tokens=2 delims==" %%A in ('findstr /b "CMAKE_HOME_DIRECTORY:INTERNAL=" "CMakeCache.txt"') do set "CACHED_SOURCE=%%A"
if defined CACHED_SOURCE if /I not "%CACHED_SOURCE%"=="%EXPECTED_SOURCE%" (
    echo [BUILD] Removing stale CMake cache...
    del /f /q "CMakeCache.txt" >nul 2>&1
    if exist "CMakeFiles" rmdir /s /q "CMakeFiles"
    if exist "_deps" rmdir /s /q "_deps"
)
if not exist "CMakeCache.txt" if exist "_deps" rmdir /s /q "_deps"

echo [BUILD] Configuring CMake...
cmake %GENERATOR% -A x64 -S "%~dp0." -B "%~dp0build" -DCS2OVERLAY_MUTATE_BINARY=OFF -DCS2OVERLAY_BUILD_TESTS=ON
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    echo.
    pause
    exit /b 1
)

echo [BUILD] Building (Release)...
cmake --build . --config Release --parallel

echo.
if %ERRORLEVEL% equ 0 (
    echo [BUILD] Success! Executable located at: build\Release\cs2overlay.exe
    :: Stage runtime assets (Chams GLB meshes + PNT textures) next to the exe.
    if exist "%~dp0assets\models" (
        if not exist "build\Release\assets\models" mkdir "build\Release\assets\models"
        xcopy /Y /E /I "%~dp0assets\models" "build\Release\assets\models" >nul
        echo [ASSET] Copied assets/models to build\Release\assets\models
    ) else (
        echo [ASSET] assets/models not found; Chams models will be missing.
    )
) else (
    echo [BUILD] Build Failed! Check errors above.
)

echo.
cd /d "%~dp0"
pause
