@echo off
REM Symlinks this example mod into the engine's mods\ directory and launches the engine.
REM Run from anywhere — the script resolves paths relative to itself.

setlocal

set "SCRIPT_DIR=%~dp0"
pushd "%SCRIPT_DIR%\..\.."
set "ENGINE_ROOT=%CD%"
popd
set "MOD_NAME=simple-platformer"

REM Ensure the engine has been built
if not exist "%ENGINE_ROOT%\build\Release\gloaming.exe" (
    if not exist "%ENGINE_ROOT%\build\Debug\gloaming.exe" (
        echo Engine binary not found in %ENGINE_ROOT%\build\
        echo Build the engine first:
        echo   cmake -B build
        echo   cmake --build build --config Release
        exit /b 1
    )
)

REM Create mods\ directory if it doesn't exist
if not exist "%ENGINE_ROOT%\mods" mkdir "%ENGINE_ROOT%\mods"

REM Symlink the example into mods\ (requires elevated prompt on older Windows)
if not exist "%ENGINE_ROOT%\mods\%MOD_NAME%" (
    mklink /D "%ENGINE_ROOT%\mods\%MOD_NAME%" "%SCRIPT_DIR%."
    if errorlevel 1 (
        echo Failed to create symlink. On Windows, either:
        echo   1. Run this script as Administrator, or
        echo   2. Enable Developer Mode in Settings ^> Update ^& Security ^> For Developers
        exit /b 1
    )
    echo Linked %MOD_NAME% into mods\
)

REM Launch the engine from the engine root (so config.json is found)
cd /d "%ENGINE_ROOT%"
if exist "build\Release\gloaming.exe" (
    "build\Release\gloaming.exe"
) else (
    "build\Debug\gloaming.exe"
)
