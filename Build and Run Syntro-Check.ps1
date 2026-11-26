Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$project = $PSScriptRoot
$appPath = Join-Path $project 'build\qt-mingw\BarcodeHub.exe'
$qtRoot = if ($env:QT_INSTALL_ROOT) { $env:QT_INSTALL_ROOT } else { 'C:\Qt' }
$qtKit = Join-Path $qtRoot '6.11.0\mingw_64\bin'
$compiler = Join-Path $qtRoot 'Tools\mingw1310_64\bin'
$cmakeBin = Join-Path $qtRoot 'Tools\CMake_64\bin'
$ninjaBin = Join-Path $qtRoot 'Tools\Ninja'
$env:Path = "$qtKit;$compiler;$cmakeBin;$ninjaBin;$env:Path"

$launchLock = [Threading.Mutex]::new($false, 'Local\SyntroCheckBuildAndRun')
$ownsLock = $false
try {
    try { $ownsLock = $launchLock.WaitOne(0) }
    catch [Threading.AbandonedMutexException] { $ownsLock = $true }
    if (!$ownsLock) {
        throw 'Another Syntro-Check build is already waiting or running.'
    }

    if (!(Test-Path -LiteralPath (Join-Path $qtKit 'Qt6Core.dll'))) {
        throw "Qt 6.11.0 MinGW was not found in $qtKit. Set QT_INSTALL_ROOT to your Qt installation."
    }
    $running = @(Get-Process -Name BarcodeHub -ErrorAction SilentlyContinue |
        Where-Object { $_.Path -eq $appPath })
    if ($running.Count) {
        Write-Host 'Close the current Syntro-Check window normally. The build will start automatically after it closes.' -ForegroundColor Yellow
        foreach ($instance in $running) { $instance.WaitForExit() }
    }

    Set-Location -LiteralPath $project
    Write-Host 'Configuring Syntro-Check...'
    & cmake --preset qt-mingw
    if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed (exit $LASTEXITCODE)." }

    Write-Host 'Building Syntro-Check...'
    & cmake --build --preset qt-mingw-debug --parallel 4
    if ($LASTEXITCODE -ne 0) { throw "The build failed (exit $LASTEXITCODE)." }

    Write-Host 'Opening Syntro-Check...'
    Start-Process -FilePath $appPath -WorkingDirectory (Split-Path -Parent $appPath) -WindowStyle Normal
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    $null = Read-Host 'Press Enter to close'
    exit 1
} finally {
    if ($ownsLock) { $launchLock.ReleaseMutex() }
    $launchLock.Dispose()
}
