@echo off
setlocal enabledelayedexpansion
title BarcodeHub POS and Billing System Launcher

:: Always run from the project root directory
cd /d "%~dp0"

echo ====================================================
echo  BarcodeHub POS and Billing System - Auto Build and Run
echo  Author: Ali Abdullah (25L-3022)
echo ====================================================
echo.

:: Detect toolchain paths
set "QT_ROOT=C:\Qt\6.11.0\mingw_64"
set "QT_TOOLS=C:\Qt\Tools"
set "MINGW_BIN="
set "CMAKE_BIN="
set "NINJA_BIN="

if exist "%QT_TOOLS%\mingw1310_64\bin\g++.exe" (
    set "MINGW_BIN=%QT_TOOLS%\mingw1310_64\bin"
) else if exist "C:\mingw64\bin\g++.exe" (
    set "MINGW_BIN=C:\mingw64\bin"
)

if exist "%QT_TOOLS%\CMake_64\bin\cmake.exe" (
    set "CMAKE_BIN=%QT_TOOLS%\CMake_64\bin"
) else (
    where cmake.exe >nul 2>&1 && set "CMAKE_BIN="
)

if exist "%QT_TOOLS%\Ninja\ninja.exe" (
    set "NINJA_BIN=%QT_TOOLS%\Ninja"
) else (
    where ninja.exe >nul 2>&1 && set "NINJA_BIN="
)

:: Ensure tools are in PATH
if defined MINGW_BIN set "PATH=!MINGW_BIN!;!PATH!"
if exist "%QT_ROOT%\bin" set "PATH=%QT_ROOT%\bin;!PATH!"
if defined CMAKE_BIN set "PATH=!CMAKE_BIN!;!PATH!"
if defined NINJA_BIN set "PATH=!NINJA_BIN!;!PATH!"

set "BUILD_DIR=build\qt-mingw"
set "EXE=%BUILD_DIR%\BarcodeHub.exe"

:: Step 1: Configure CMake if build directory does not exist
if not exist "%BUILD_DIR%\build.ninja" (
    echo [*] Initializing project configuration with Qt 6...
    cmake -S . -B "%BUILD_DIR%" -G Ninja -DCAMERA=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="%QT_ROOT%"
    if errorlevel 1 (
        echo.
        echo [ERROR] CMake configuration failed. Please check paths.
        pause
        exit /b 1
    )
)

:: Step 2: Build project (fast incremental build - picks up any code changes automatically)
echo [*] Building BarcodeHub POS System...
cmake --build "%BUILD_DIR%" --parallel
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed! Check compiler error messages above.
    pause
    exit /b 1
)

:: Step 3: Launch BarcodeHub POS application
echo [*] Launching BarcodeHub POS System...

:: Ensure desktop shortcut with custom icon exists on this PC
powershell -NoProfile -Command "$desktop = [Environment]::GetFolderPath('Desktop'); $lnk = Join-Path $desktop 'BarcodeHub POS System.lnk'; if (!(Test-Path $lnk)) { $ws = New-Object -ComObject WScript.Shell; $s = $ws.CreateShortcut($lnk); $s.TargetPath = '%~dp0Run-BarcodeHub.cmd'; $s.WorkingDirectory = '%~dp0'; $s.IconLocation = '%~dp0assets\pos_icon.ico,0'; $s.Description = 'BarcodeHub Point of Sale (POS) and Billing System'; $s.Save() }" >nul 2>&1

start "" "%EXE%"

:: Exit launcher window cleanly
exit /b 0
