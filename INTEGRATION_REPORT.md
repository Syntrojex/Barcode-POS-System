# BarcodeHub — frontend/backend integration report

## Result

The C++17 Product, Order, Integration and Billing modules now build together. The Qt UI has been rewired to invoke the backend order creation, stock saving and billing checkout services, then reconstruct order history and bills from the persisted files on startup.

**Verified in this environment:** backend CMake configure/build, Product tests, Order tests, Product→Order integration test, and full Order→Billing checkout test (4/4 passing). Also tested that repackaging does **not** overwrite an existing `Product.txt`.

**Not verified in this environment:** Qt Widgets GUI compilation/runtime, physical camera scanning, and Windows/MinGW portable deployment. Qt and OpenCV development dependencies were unavailable; the results above must not be interpreted as a successful GUI or camera test.

## Changed and fixed

- `CMakeLists.txt`: replaced stale missing-header checks and skipped-module fallbacks with explicit source lists for the **actual** ProductModel/ProductStorage/ProductValidation modules, Order, Billing, and ProductAdapter. The missing order header code is no longer silently skipped. Restored full backend tests and CTest registration. GUI main and legacy camera entry points are handled separately so there is no duplicate `main()`.
- `Backend/include/order/Order.h`, `OrderDetail.h`, `Backend/src/order/Order.cpp`, `OrderDetail.cpp`: reconstructed the zero-byte model files to conform to `OrderManager.cpp`, with status/payment enums, serialization conversions and field validation. These are compatible reconstructions from the existing implementation and tests, not a recovery of your friend's original content.
- `Backend/include/order/OrderManager.h` and its implementation: added next-ID preview and a safe way to discard unpaid, newly created orders on failed checkout.
- `Backend/src/integration/ProductAdapter.cpp` and `.h`: use the real current Product model and its string measurement-unit API.
- `Frontend/include/ui/AppState.h`, `Frontend/src/ui/AppState.cpp`: correct Product APIs, backend order/cart/bill bridging, file-backed history, real dashboard totals, error propagation, inventory deduction and rollback attempt on failed checkout. Removed hardcoded sample orders and invented sales baseline.
- `Frontend/src/ui/RecordOrderPage.cpp`: checkout now uses the integrated backend, only clears the draft after confirmed success; passes real product IDs and payment details. Unique manual item identifiers no longer reuse a removed item's ID.
- `Frontend/src/ui/Dialogs.cpp`, `.h`: card/wallet payments collect a merchant name for backend validation; receipt shows a real bill number and no longer invents a cashier/store identity.
- `Frontend/src/ui/OtherPages.cpp`: bills display the actual Bill.txt ID rather than a made-up "BILL-" prefix; removed the nonfunctional "Resume" button and fake scan selection.
- `Frontend/src/ui/DashboardPage.cpp`: sales and hourly revenue reflect saved paid orders; removed sample chart revenue and fake purchase-order creation messages. Reorder markers are explicitly session-only.
- `Frontend/src/ui/Widgets.cpp`, `.h`: replaced the animated mock camera with a real OpenCV camera feed and barcode detection **when compiled with `CAMERA=ON`**; otherwise displays a disabled-camera message. The scanner no longer silently cycles sample products.
- `Backend/src/scanner/decoder.cpp`: return full checksum-validated 8-digit EAN-8 so scanned values can actually match `Product.txt` (which stores 8-digit codes), rather than stripping two digits to six.
- `Backend/src/product/ProductStorage.cpp`: corrected Windows overwrite fallback so a failed rename does not delete the existing Product.txt first.
- `Backend/include/product/Models.h`: replaced old divergent model definitions and missing includes with references to the canonical model headers.
- `scripts/build-windows.ps1`: enable the optional camera for the full Windows build (`-DCAMERA=ON`), deploy OpenCV DLLs and seed only missing data files.
- `cmake/seed-data.cmake`: new idempotent initial-data deployment script; existing Product/Order/OrderDetail/Bill records are **not** replaced during another build/deploy.
- `tests/product_tests.cpp`, `tests/integration_test.cpp`: repaired stale API references and registered tests. Added `tests/checkout_test.cpp` to cover payment failure, successful payment, bill persistence and preventing double payment.

## Run/build

Backend checks without Qt:

```bash
cmake -S . -B build/backend -DBACKEND_ONLY=ON -DBUILD_TESTING=ON
cmake --build build/backend --parallel
ctest --test-dir build/backend --output-on-failure
```

Qt GUI without camera (requires installed Qt 6 or 5 Widgets):

```bash
cmake -S . -B build/ui -DCAMERA=OFF -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build build/ui --parallel
```

With OpenCV camera support:

```bash
cmake -S . -B build/ui-camera -DCAMERA=ON -DOpenCV_DIR=/path/to/OpenCVConfig-directory -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build build/ui-camera --parallel
```

On Windows, `scripts/build-windows.ps1` can prepare Qt/MinGW and OpenCV for the camera-enabled portable build. Install a writable copy of the portable folder. The executable's `data/` directory is the persistent store; alternatively, set `BARCODEHUB_DATA` to a folder with `Product.txt` before launch. Do not delete your `data/` folder when updating the app.

## Remaining limitations

- The GUI and physical camera have **not** been compiled or run here; confirm against the team's real Qt/MinGW/OpenCV installation.
- Current Order.txt/OrderDetail.txt/Bill.txt formats do not persist VIP tier, register/shift, cash tendered, or manual item's descriptive name. A manual item's acronym, quantity and price survive on the bill; after restart the custom item's long description is not recoverable.
- Existing held/in-progress orders are viewable, but editing/resuming them in the UI is not implemented; the fake Resume action was removed. The UI currently creates and pays new orders in one checkout workflow.
- Reorder markers are session-only and do **not** create purchase orders.
- The UI quantity stepper uses whole units, even though the backend order module supports fractional weight quantities.
- Text-file writes across all four files are not a single ACID transaction; rollback is attempted and failures are surfaced, but simultaneous cashiers/processes and power loss between writes require a transactional database or journal.
- The dynamic hourly graph covers 08:00–20:59; transactions outside that window still count toward the correct daily sales total.
