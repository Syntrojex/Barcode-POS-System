# Scanner execution results - 19 November 2026

The current scanner source passed **89 isolated checks**, and camera index 0
successfully opened through DirectShow and returned 640 by 480 BGR frames.
A user-coordinated live scan then read 832 frames over 30.27 seconds but decoded
no barcode. Camera release/reopen also passed. No camera image was saved.

## Scope and toolchain

The probe compiled the checked-in [CameraScanner implementation](../../Backend/src/scanner/CameraScanner.cpp)
and [decoder](../../Backend/src/scanner/decoder.cpp) directly. The execution
checkout was `514aa7e`; the probe did not alter application code or CMake.
Its C++ harness and output remain local ignored artifacts under
`build/evaluation-probes/`, separate from the five registered CTest suites.

The compiler was `C:/Qt/Tools/mingw1310_64/bin/g++.exe` (GNU 13.1.0).
OpenCV 4.13.0 was reused from an existing matching-compiler installation at
`D:/Github-Repositories/Bar-Code-System-CPP/build/deps/opencv`.
No dependency download, OpenCV rebuild, or sibling-repository mutation was needed.

## Observed image and decoder results

Each image case required `frameRead()` to return success and exactly match the
full eight-digit barcode in the shipped PNG's filename. All 14 catalogue PNGs
were tested in every image group below.

| Group | Passed / attempted |
| --- | ---: |
| Original BGR images | 14 / 14 |
| Grayscale conversions | 14 / 14 |
| BGRA conversions | 14 / 14 |
| 90-degree clockwise rotations | 14 / 14 |
| 180-degree rotations | 14 / 14 |
| Horizontal mirrors | 14 / 14 |
| Blank image, empty frame, stale-output clearing | 3 / 3 |
| Synthetic valid EAN-8 and wrong-checksum EAN-8 | 2 / 2 |
| **Total** | **89 / 89** |

The synthetic pair used exact 67-module EAN-8 layouts with white margins:
`40001029` decoded, while `40001020` was rejected with the bad-checksum flag.
The invalid-checksum image changes only the last digit's valid bar pattern.
Blank/empty frames returned false and cleared prior output; the empty frame
reported `Camera stopped sending frames`. `decodeSix("bad", output)` also
cleared a prior output string rather than returning stale digits.

## Actual camera result

The separate `--camera` invocation called the application's `openCamera()`
and then `readFrame()` once. It printed:

```text
CAMERA_OPEN PASS
CAMERA_FRAME PASS dimensions=640x480 channels=3
CAMERA_PROBE_EXIT=0
```

The scanner destructor released the camera when the process exited. No frame
was encoded, displayed, saved, or sent elsewhere by this probe. The earlier
absence of an inventory entry did not predict the actual device-open result.

## Live recognition attempt and camera lifecycle

After the user confirmed the webcam was connected and a barcode was ready,
the extended probe opened index 0 and repeatedly called the application's
`readFrame()` and `frameRead()` until a code was found or 30 seconds elapsed.
The expected example was `40001029`; any actually decoded code would have
been printed. The observed output was:

```text
LIVE_OPEN PASS scan_seconds=30
LIVE_SCAN NO_CODE code= expected=40001029 frames=832 seconds=30.2715 fps=27.4846 read_ok=1
LIVE_REOPEN PASS frame=PASS
```

The live probe exited 4 because the recognition window ended without a code.
Frame capture itself succeeded. Its camera object was destroyed before a new
scanner reopened the same device and successfully read another frame. The
measured rate is approximately 27.48 frames per second for this invocation,
including capture/decoding and startup overhead; it is not a device guarantee.
This attempt does not establish why recognition failed. Fixture PNG success
does not establish equivalent lighting, focus, scale or alignment at the lens.

## Commands used

Run from the repository root. The local probe source was generated for this
evaluation and is not an application source or committed test target.

```powershell
& 'C:/Qt/Tools/mingw1310_64/bin/g++.exe' -std=c++17 -O0 -g build/evaluation-probes/scanner_probe.cpp Backend/src/scanner/CameraScanner.cpp Backend/src/scanner/decoder.cpp -IBackend/include/scanner -I'D:/Github-Repositories/Bar-Code-System-CPP/build/deps/opencv/include' -L'D:/Github-Repositories/Bar-Code-System-CPP/build/deps/opencv/x64/mingw/lib' -lopencv_videoio4130 -lopencv_imgcodecs4130 -lopencv_imgproc4130 -lopencv_core4130 -lole32 -loleaut32 -lstrmiids -o build/evaluation-probes/scanner_probe.exe
$env:PATH = 'C:/Qt/Tools/mingw1310_64/bin;D:/Github-Repositories/Bar-Code-System-CPP/build/deps/opencv/x64/mingw/bin;' + $env:PATH
& ./build/evaluation-probes/scanner_probe.exe data/barcodes
& ./build/evaluation-probes/scanner_probe.exe --camera
& ./build/evaluation-probes/scanner_probe.exe --camera-scan
```

Image checks and the single-frame camera check exited zero. The live
recognition attempt exited 4 as described above. Local logs are
`build/evaluation-probes/scanner-images.log`, `scanner-camera.log`, and
`scanner-live.log`.

## Remaining presentation checks

These results do not test the Qt camera feed, repeated-scan suppression,
automatic cart addition, extended sessions beyond the 30-second attempt,
disconnect/reconnect recovery, lighting variation, or arbitrary skew. Recognition
of the presented barcode through the lens was attempted but did not succeed.
The successful rotated fixture cases do not remove the source's conditional
rotation limitation. Use the [camera worksheet](../increment-2/presentation/scanner-rehearsal.md)
to record any later GUI/hardware demonstration separately.
