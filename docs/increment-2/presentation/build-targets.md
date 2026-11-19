# Build options and active source map

Use this reference when explaining which parts of Increment 2 a build or test
actually exercises. It describes the checked-in CMake configuration; target
availability does not mean that the target compiled successfully.

## Option combinations

| Configuration | Available components | External requirements |
| --- | --- | --- |
| `BACKEND_ONLY=ON`, `CAMERA=OFF`, `BUILD_TESTING=ON` | Backend libraries and five test executables | C++17 compiler, CMake |
| `BACKEND_ONLY=ON`, `CAMERA=ON` | Backend plus scanner library; tests if enabled | OpenCV 4, even without Qt |
| `BACKEND_ONLY=OFF`, `CAMERA=OFF` | Backend, Qt UI, BarcodeHub; tests if enabled | Qt Widgets and compatible compiler |
| `BACKEND_ONLY=OFF`, `CAMERA=ON` | Above plus scanner and optional CameraDemo | Qt Widgets and OpenCV 4 |
| `CAMERA=AUTO` | Scanner only when OpenCV is discovered | Camera can be absent without configuration failure |
| `BUILD_TESTING=OFF` | Omits the five registered test executables | Does not repair or bypass GUI compilation |

`CAMERA` defaults to `ON`; `BACKEND_ONLY` defaults to `OFF`. OpenCV discovery
precedes the backend-only return. Specify `CAMERA=OFF` explicitly when testing
the backend without OpenCV. `CAMERA=OFF` still allows manual product lookup.

## Targets and responsibilities

| Target | Responsibility |
| --- | --- |
| `product_core` | Catalogue model, parsing, validation and stock |
| `order_core` | Cart, order records, details and order states |
| `common_core` | Shared record-text and date helpers |
| `billing_core` | Totals, payment validation, codec and bill repository |
| `integration_core` | Product-to-order snapshot adapter |
| `backend_core` | Interface target joining backend dependencies |
| `scanner_core` | OpenCV capture, localization and EAN-8 decoder |
| `barcode_ui` / `BarcodeHub` | Qt pages/widgets and application entry point |
| `CameraDemo` | Standalone camera widget, when camera support is enabled |

CTest names are `product`, `order`, `integration`, `checkout`, and `billing`.
Their executables are `product_tests`, `order_tests`, `integration_test`,
`checkout_test`, and `billing_tests`. None is a live GUI/camera acceptance test.

## Distinguish active and older sources

| Feature boundary | Selected by current CMake | Older or separate material |
| --- | --- | --- |
| Application entry | `Frontend/src/main.cpp` | `Frontend/src/ui/main.cpp` |
| Main camera view | `CameraFeedWidget` in `Widgets.cpp` | `POSWindow.cpp` and `Frontend/legacy-camera/` |
| Scanner implementation | `Backend/src/scanner/CameraScanner.cpp`, `decoder.cpp` | Root-level scanner/decoder copies |
| Optional camera window | `tests/camera_demo.cpp`, UI `CameraWidget.cpp` | Not registered in CTest |
| Generator regression | No generator target in current configuration | `tests/regression.cpp` references absent historical APIs |

The CMake scanner source and some includes use `cameraScanner` casing while
tracked files use `CameraScanner`; case-sensitive environments need review.
The backend passed five tests. The initial `1b16a70` GUI build failed on stale
`RecordOrderPage` declarations; a separate correction at `883425c` allowed the
Qt build with `CAMERA=OFF` to pass. Camera-enabled builds and GUI interaction
remain unverified. See [verification](../verification.md).

Sources: [CMakeLists.txt](../../../CMakeLists.txt),
[presets](../../../CMakePresets.json), [current main](../../../Frontend/src/main.cpp),
and [older regression](../../../tests/regression.cpp).
