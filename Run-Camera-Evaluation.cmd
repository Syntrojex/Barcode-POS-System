@echo off
setlocal
cd /d "%~dp0"
set "EVALUATION_DIR=%~dp0build\evaluation-camera"
if not exist "%EVALUATION_DIR%\BarcodeHub.exe" (
    echo Camera evaluation build is missing.
    echo Follow docs\evaluation\README.md to configure and build it first.
    pause
    exit /b 1
)
if not exist "%EVALUATION_DIR%\data\Product.txt" (
    echo Evaluation data is missing. Rebuild BarcodeHub to seed the data folder.
    pause
    exit /b 1
)
rem The separate build data folder preserves the repository's original records.
set "BARCODEHUB_DATA=%EVALUATION_DIR%\data"
start "" /D "%EVALUATION_DIR%" "%EVALUATION_DIR%\BarcodeHub.exe"
exit /b 0
