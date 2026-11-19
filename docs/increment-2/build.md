# Build, data location and packaging

Run commands from the repository root. CMake 3.21+, a C++17 compiler and Ninja
are needed for the commands below. The verified Windows backend compiler was
GNU 13.1.0. See [verification](verification.md) for the actual results.

## Backend without Qt or OpenCV

```powershell
cmake -S . -B build/increment2-audit-backend -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBACKEND_ONLY=ON -DCAMERA=OFF -DBUILD_TESTING=ON
cmake --build build/increment2-audit-backend --parallel 4
ctest --test-dir build/increment2-audit-backend --output-on-failure
```

Use `CAMERA=OFF` explicitly: the default is `ON`, and OpenCV discovery happens
before CMake exits for `BACKEND_ONLY`. The registered tests are product, order,
integration, checkout and billing. Tests do not require a camera.

## Qt build diagnostic

Qt Widgets and a compatible compiler are required. These paths describe the
reviewed Windows installation; adjust them for another machine.

```powershell
cmake -S . -B build/increment2-audit-gui -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCAMERA=OFF -DBUILD_TESTING=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.11.0/mingw_64 -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe
cmake --build build/increment2-audit-gui --parallel 4
```

At the original baseline, configuration succeeded but compilation failed at
`RecordOrderPage.cpp` because member declarations differed from its header.
After the separate header repair at `883425c`, the same camera-disabled build
passed and linked `BarcodeHub.exe`. See [follow-up results](presentation/review-results.md).
Interactive launch and checkout were not tested.

`CAMERA=ON` requires OpenCV 4 components `core`, `imgproc`, `imgcodecs`, `videoio`.
`CAMERA=AUTO` searches optionally; `OFF` skips OpenCV. Camera-enabled builds also
have filename case inconsistencies on case-sensitive platforms; see [scanner](scanner.md).

## Data selection and preservation

[AppState::locateDataDir](../../Frontend/src/ui/AppState.cpp) first checks the
`BARCODEHUB_DATA` environment variable, then executable-relative `data` folders,
the working directory, and the compiled source directory. A candidate must
contain `Product.txt`; an invalid override falls through to other candidates.
Use a separate copy of the entire `data` directory for any future GUI rehearsal,
because a successful checkout persists inventory and transaction changes.

[cmake/seed-data.cmake](../../cmake/seed-data.cmake) copies only missing starter
files. Rebuilding does not intentionally reset an existing data directory.

[scripts/build-windows.ps1](../../scripts/build-windows.ps1) detects a Qt MinGW
kit, builds OpenCV when needed, builds/tests the app, deploys Qt and OpenCV DLLs,
and assembles `build/portable/BarcodeHub`. This full packaging path was inspected
but not executed in this review. Deployment targets such as `BarcodeHub_deploy`
are optional; an executable alone is not a verified portable package.
