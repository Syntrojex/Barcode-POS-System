# BarcodeHub - frontend only

Qt 6 Widgets only. No OpenCV and no `Backend/` code is compiled; every screen runs on
in-memory sample data (`Frontend/src/ui/AppState.cpp`).

## Build

| Compiler | How |
|---|---|
| Qt MinGW | double-click `build-mingw.cmd` (edit the paths at the top if Qt is elsewhere), or open `CMakeLists.txt` in Qt Creator with a **MinGW 64-bit** kit |
| MSVC | double-click `build-msvc.cmd` (edit `QT_MSVC` at the top), or open `CMakeLists.txt` in Qt Creator / Visual Studio with the **MSVC 2022 64-bit** kit |

Delete the `build/` folder when you switch compilers - CMake caches the compiler.
`Run-BarcodeHub.cmd` starts the last build.

## Shortcuts
F1 Dashboard - F2 Record Order - F3 Orders - F4 Products (Browse on Record Order) -
F5 Scanner - F6 Bills - F9 simulated scan - F10 Settings (Checkout on Record Order) -
Ctrl+K search - Ctrl+N new order.

The old camera widget that needs the backend is parked in `Frontend/legacy-camera/` (not built).
