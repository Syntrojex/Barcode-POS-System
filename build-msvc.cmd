@echo off
setlocal
rem ---- Edit QT_MSVC if your Qt MSVC kit lives somewhere else ----
set "QT_MSVC=C:\Qt\6.11.0\msvc2022_64"
if not exist "%QT_MSVC%\lib\cmake\Qt6\Qt6Config.cmake" (
    echo ERROR: Qt MSVC kit not found at %QT_MSVC%
    echo Edit QT_MSVC at the top of build-msvc.cmd
    pause
    exit /b 1
)
set "BUILD_DIR=%~dp0build\msvc"
cmake -S "%~dp0." -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 "-DCMAKE_PREFIX_PATH=%QT_MSVC%"
if errorlevel 1 ( pause & exit /b 1 )
cmake --build "%BUILD_DIR%" --config Release --parallel
if errorlevel 1 ( pause & exit /b 1 )
echo.
echo SUCCESS: %BUILD_DIR%\Release\BarcodeHub.exe
start "" "%BUILD_DIR%\Release\BarcodeHub.exe"
