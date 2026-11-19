# Portable application rehearsal worksheet

This is an unexecuted packaging and launch checklist. Packaging was inspected
from source and was not run for this documentation change. The initial
`1b16a70` GUI compilation failed; a separate correction at `883425c` allowed
the Qt build with `CAMERA=OFF` to pass. Camera-enabled builds, portable launch
and live GUI interaction remain unverified. Record the source revision of
any existing executable before using it as presentation evidence.

## Identify the compiler and dependencies

The Windows helper searches for a Qt 6 MinGW kit and reads its configured GCC
major/minor version to find a matching compiler. Pass explicit `QtRoot` and
`MinGWRoot` values if automatic selection does not identify the intended kit.
Do not mix binaries built for unrelated compiler ABIs.

The script requests OpenCV 4.13.0 and builds `core`, `imgproc`, `imgcodecs`,
and `videoio` when its dependency cache is missing. Windows capture uses
DirectShow; the script disables FFmpeg and Media Foundation. This package
configuration does not establish support for every camera or video format.

Packaging is a build operation: it can download dependencies, modify cached
OpenCV source for MinGW compatibility, run tests, copy DLLs, and create files.
It is not a read-only inspection command.

## Check the proposed package contents

The helper assembles `build/portable/BarcodeHub`. After a successful build,
inspect the resulting folder before transferring it to the presentation PC.

| Item | Purpose / acceptance question | Status |
| --- | --- | --- |
| `BarcodeHub.exe` | Does this correspond to the intended source revision? | Not checked |
| Qt runtime DLLs | Were dependencies copied by the matching `windeployqt`? | Not checked |
| Qt platform plugin | Can the app start with the deployed Windows plugin? | Not checked |
| Compiler runtime DLLs | Are the matching MinGW runtimes available? | Not checked |
| OpenCV runtime DLLs | Are camera-enabled dependencies beside the executable? | Not checked |
| `data/Product.txt` | Does the selected data folder load the intended catalogue? | Not checked |
| Barcode PNGs | Do product detail images resolve from the packaged data? | Not checked |
| Other transaction data files | Are rehearsal records separate from live records? | Not checked |
| `licenses/` | Are application, OpenCV and available dependency notices included? | Not checked |

The seed script copies only missing starter files; rebuilding an existing
package preserves its transaction files and stock. It does not reset the demo.
For repeatable rehearsal, prepare a separate copy of the complete data folder.

## Rehearse the transferred folder

1. Record the source revision and the package's build date before transfer.
2. Launch from the copied folder without relying on the developer's Qt shell.
3. Confirm the actual data selection; `BARCODEHUB_DATA` can override defaults.
4. Check Products and one barcode image before creating a transaction.
5. Use the [scanner worksheet](scanner-rehearsal.md) if camera capture is needed.
6. Follow the [transaction walkthrough](../demo.md) using disposable records.
7. Close and reopen to verify the saved order, bill and stock in that data copy.

A successful backend CTest run does not verify transferred DLLs, device access,
Qt rendering, or GUI persistence. Record those outcomes only after observing them.

Sources: [Windows helper](../../../scripts/build-windows.ps1),
[deployment targets](../../../CMakeLists.txt),
[seed policy](../../../cmake/seed-data.cmake), and
[data discovery](../../../Frontend/src/ui/AppState.cpp).
