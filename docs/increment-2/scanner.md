# Camera and EAN-8 scanner reference

This is a source-inspection reference for the Increment 2 camera path.
Camera hardware and GUI behavior were not runtime-verified for this document.
The current GUI compile check reports a header mismatch; see [verification](verification.md).
Rehearse using a verified build before presenting a live camera demonstration.

## Active source path

The [CMake configuration](../../CMakeLists.txt) selects the backend scanner library.
Its implementation is [CameraScanner.cpp](../../Backend/src/scanner/CameraScanner.cpp)
with [decoder.cpp](../../Backend/src/scanner/decoder.cpp).
The main POS embeds `CameraFeedWidget` from [Widgets.cpp](../../Frontend/src/ui/Widgets.cpp).
Root-level scanner copies and `Frontend/legacy-camera` are separate older sources.
The optional `CameraDemo` executable uses another `CameraWidget` implementation.
Do not infer main-POS image-upload features from the legacy camera widget.

## Frame-to-barcode processing

1. Open camera index 0 by default; Windows requests the DirectShow backend.
2. Request MJPG frames at 640 by 480 and 30 frames per second.
3. Convert a supported color frame to grayscale, or reuse a grayscale frame.
4. Attempt decoding across several thin horizontal bands.
5. If necessary, locate a likely barcode using gradients and morphology, then crop it.
6. When no suitable contour is found, also try a 90-degree rotation.

Requested camera settings depend on what the device actually supports.
The decoder measures 43 alternating stripe runs and reconstructs 67 modules.
It validates EAN-8 guard patterns, matches eight digits, and checks the checksum.
It also tries reversing the bit sequence for the opposite orientation.
The historical method name `decodeSix()` returns the **full eight-digit EAN-8**.
This matches the catalogue barcode keys rather than a six-digit payload.

## Main POS integration

`CameraFeedWidget` requests a frame on a 40 ms Qt timer.
The same detected code is suppressed until more than three seconds have elapsed;
a different code can be emitted immediately.
Stopping the main feed destroys its scanner and releases the camera.
Read failures stop the feed and emit an error message.
The [order page](../../Frontend/src/ui/RecordOrderPage.cpp) looks up the detected code.
Automatic addition to the cart depends on the `autoAddScans` setting.
Typed barcode, ID/acronym lookup, and Browse provide independent catalogue entry paths.

## Presentation limits

Camera builds require OpenCV 4; builds without it report disabled camera support.
This decoder implements EAN-8, not a general QR or every-barcode-format scanner.
No automated camera/decoder test is registered in the current CTest configuration.
The old `tests/regression.cpp` references different APIs and is not a current CTest target.
Source/header capitalization differs in some CMake/include references; verify the build environment.
Do not claim arbitrary image rotation, hardware compatibility, or successful live scanning without rehearsal.
For fallback, enter sample barcode `40001029` and explain the same product-lookup boundary.
