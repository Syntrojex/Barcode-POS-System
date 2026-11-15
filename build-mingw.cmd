@echo off
setlocal

set "QT_ROOT=C:\Qt\6.11.0\mingw_64"
set "MINGW_ROOT=C:\Qt\Tools\mingw1310_64"
set "CMAKE=C:\Qt\Tools\CMake_64\bin\cmake.exe"
set "NINJA=C:\Qt\Tools\Ninja\ninja.exe"

if not exist "%QT_ROOT%\lib\cmake\Qt6\Qt6Config.cmake" (
    echo ERROR: Qt 6.11.0 MinGW installation was not found at:
    echo %QT_ROOT%
    pause
    exit /b 1
)
if not exist "%MINGW_ROOT%\bin\g++.exe" (
    echo ERROR: MinGW 64-bit compiler was not found at:
    echo %MINGW_ROOT%\bin\g++.exe
    pause
    exit /b 1
)
if not exist "%CMAKE%" (
    echo ERROR: CMake was not found at:
    echo %CMAKE%
    pause
    exit /b 1
)
if not exist "%NINJA%" (
    echo ERROR: Ninja was not found at:
    echo %NINJA%
    echo Install Ninja using Qt Maintenance Tool, or add ninja.exe to PATH.
    pause
    exit /b 1
)

set "PATH=%MINGW_ROOT%\bin;%QT_ROOT%\bin;C:\Qt\Tools\Ninja;%PATH%"
set "BUILD_DIR=%~dp0build\qt-mingw"

echo.
echo Configuring BarcodeHub with Qt 6.11.0 MinGW 64-bit...
"%CMAKE%" -S "%~dp0" -B "%BUILD_DIR%" -G Ninja ^
  -DCMAKE_BUILD_TYPE=Debug ^
  "-DCMAKE_C_COMPILER=%MINGW_ROOT%\bin\gcc.exe" ^
  "-DCMAKE_CXX_COMPILER=%MINGW_ROOT%\bin\g++.exe" ^
  "-DCMAKE_MAKE_PROGRAM=%NINJA%" ^
  "-DCMAKE_PREFIX_PATH=%QT_ROOT%"

if errorlevel 1 (
    echo.
    echo CMake configuration failed. Check the paths above.
    pause
    exit /b 1
)

echo.
echo Building BarcodeHub...
"%CMAKE%" --build "%BUILD_DIR%" --parallel
if errorlevel 1 (
    echo.
    echo Build failed.
    pause
    exit /b 1
)

echo.
echo SUCCESS: %BUILD_DIR%\BarcodeHub.exe
pause
