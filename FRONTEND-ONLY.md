# BarcodeHub frontend-only build

This package builds the Qt Widgets frontend only. It does not compile OpenCV or
the `Backend/` sources; the UI uses in-memory sample data from
`Frontend/src/ui/AppState.cpp`.

## Recommended build on the supplied Windows installation

Double-click `build-mingw.cmd`. It explicitly configures:

- Qt: `C:\Qt\6.11.0\mingw_64`
- MinGW GCC/G++: `C:\Qt\Tools\mingw1310_64\bin`
- CMake: `C:\Qt\Tools\CMake_64\bin\cmake.exe`
- Ninja: `C:\Qt\Tools\Ninja\ninja.exe`

The script removes ambiguity about the compiler by passing the exact GCC/G++
executables to CMake. If one of these paths differs on your PC, edit the
variables at the top of `build-mingw.cmd`.

## Qt Creator

Open this folder's `CMakeLists.txt`, then select a Kit whose Qt version is
`Qt 6.11.0 MinGW 64-bit`, C compiler is the MinGW 64-bit `gcc.exe`, and C++
compiler is the matching `g++.exe` from `C:\Qt\Tools\mingw1310_64\bin`.

Do not reuse the previous build directory: it contained an MSVC compiler cache.
The supplied archive intentionally excludes `build/` and `.qtcreator/` so that
stale compiler settings are not carried into the new copy.

If Qt Creator still invokes `cl.exe`, it is using an MSVC Kit. The project
CMake code cannot override a compiler that Qt Creator explicitly passes; choose
the MinGW Kit or use `build-mingw.cmd`.

## Manual command line

```bat
"C:\Qt\Tools\CMake_64\bin\cmake.exe" -S . -B build\qt-mingw -G Ninja ^
  -DCMAKE_C_COMPILER=C:\Qt\Tools\mingw1310_64\bin\gcc.exe ^
  -DCMAKE_CXX_COMPILER=C:\Qt\Tools\mingw1310_64\bin\g++.exe ^
  -DCMAKE_MAKE_PROGRAM=C:\Qt\Tools\Ninja\ninja.exe ^
  -DCMAKE_PREFIX_PATH=C:\Qt\6.11.0\mingw_64
"C:\Qt\Tools\CMake_64\bin\cmake.exe" --build build\qt-mingw --parallel
```
