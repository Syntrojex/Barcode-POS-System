@echo off
rem Launches the most recent build (MinGW first, then MSVC).
if exist "%~dp0build\qt-mingw\BarcodeHub.exe" ( start "" "%~dp0build\qt-mingw\BarcodeHub.exe" & exit /b 0 )
if exist "%~dp0build\msvc\Release\BarcodeHub.exe" ( start "" "%~dp0build\msvc\Release\BarcodeHub.exe" & exit /b 0 )
echo Build first: build-mingw.cmd  or  build-msvc.cmd
pause
