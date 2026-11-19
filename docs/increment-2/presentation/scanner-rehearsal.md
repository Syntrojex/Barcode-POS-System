# Camera rehearsal and acceptance worksheet

These cases are **planned and unexecuted**. No camera hardware or GUI behavior
was verified during this documentation work. The initial `1b16a70` GUI build
failed on mismatched `RecordOrderPage` members; a separate header correction
at `883425c` allowed the Qt build with `CAMERA=OFF` to pass. A camera-enabled
build and live capture remain unverified. See [verification](../verification.md)
before using this worksheet for a live presentation.

## Prepare one capture session

1. Use a verified camera-enabled build and disposable application data.
2. Confirm that the selected data folder contains the sample catalogue.
3. Stop camera capture on the other POS page before starting this feed.
4. Use the catalogue image for `40001029` with visible white margins.
5. Begin with horizontal, well-lit bars and keep the image steady.
6. Leave automatic scan addition off to separate preview from explicit Add.

The main feed opens camera index 0. It requests 640 by 480 MJPG frames at
30 FPS; these settings are requests, not proof of device capabilities.
The Qt capture timer runs every 40 ms and performs decoding on that callback.

## Record observed outcomes

| Case | Expected from source | Observed |
| --- | --- | --- |
| Start with working device | Live frames replace the stopped-camera view | Not run |
| Present `40001029` | Eight-digit code is emitted and milk lookup succeeds | Not run |
| Keep same code visible for under three seconds | Repeated code emission is suppressed | Not run |
| Keep same code visible beyond three seconds | A later detection may emit again | Not run |
| Replace with `40002149` immediately | Different code can emit without waiting | Not run |
| Present a blank white image | No valid barcode detection | Not run |
| Present bars with a wrong check digit | Decoder rejects that candidate | Not run |
| Present a vertical code | Conditional rotation may decode; success is not guaranteed | Not run |
| Stop the main feed | Timer stops, scanner is destroyed, camera is released | Not run |
| Capture fails after start | Feed stops and emits a scan-error message | Not run |
| Build with `CAMERA=OFF` | Start reports disabled camera support | Not run |

The decoder is EAN-8-specific. QR, EAN-13, arbitrary skew, damaged labels,
blurred frames, and every camera model are outside established coverage.
The 90-degree retry occurs only when contour localization finds no suitable
box, so vertical-code handling depends on the surrounding image too.

## Use the manual fallback

Stop capture, type `40001029` on Record Order, press Enter, and explicitly
click Add to order. This exercises catalogue lookup and the cart without
establishing that camera decoding worked. If the presentation machine cannot
run the GUI, use the backend evidence in the [demo guide](../demo.md).

The optional CameraDemo uses a different widget with no repeat suppression;
its behavior does not establish the main POS feed's acceptance results.

Sources: [main camera feed](../../../Frontend/src/ui/Widgets.cpp),
[capture/localization](../../../Backend/src/scanner/CameraScanner.cpp),
[decoder](../../../Backend/src/scanner/decoder.cpp),
[order page](../../../Frontend/src/ui/RecordOrderPage.cpp), and
[CameraDemo widget](../../../Frontend/src/ui/CameraWidget.cpp).
