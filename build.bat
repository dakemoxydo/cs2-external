@echo off
chcp 65001 >nul
echo [BUILD] Starting CS2 External build...

if not exist "build" mkdir build
cd build

echo [BUILD] Configuring CMake...
cmake ..

echo [BUILD] Building (Release)...
cmake --build . --config Release

if %ERRORLEVEL% equ 0 (
    echo [BUILD] Success! Executable located at: build\Release\cs2overlay.exe
) else (
    echo [BUILD] Build Failed!
)

cd ..
pause

