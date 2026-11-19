# BarcodeHub evaluation: build, run, test and explain

Prepared on 19 November 2026 for the local Increment 2 evaluation. Application
source baseline: `514aa7e`. Test data and temporary test programs are under the
ignored `build` folder; application business logic was not changed.

## Open the camera-enabled application

Double-click [Run-Camera-Evaluation.cmd](../../Run-Camera-Evaluation.cmd).
It opens the freshly built application with its separate evaluation data:

```text
D:\Github-Repositories\Barcode-POS-System\build\evaluation-camera\BarcodeHub.exe
D:\Github-Repositories\Barcode-POS-System\build\evaluation-camera\data
```

The original `Run-BarcodeHub.cmd` configures new builds with `CAMERA=OFF`.
That explains the screenshot's "Camera support is disabled" message. The new
evaluation executable was explicitly configured with `CAMERA=ON`, compiled,
linked, and supplied with Qt and OpenCV runtime DLLs. Opening the old executable
can reproduce the old message even though the new build supports a camera.

Evaluation checkout updates only the selected evaluation data copy. The original
repository `data` directory is not the checkout destination. Repeated evaluation
sales reduce the evaluation stock further; record starting stock before a test.

## What to read

- [Manual test checklist: 68 cases](manual-testing.md): exact UI steps, sample
  values, negative cases, restart and settings checks.
- [C++ file-by-file map](backend-file-map.md): every `.cpp`, active versus
  historical files, module ownership, and the complete execution flow.
- [Scanner execution results](scanner-results.md): 89 image checks, real camera
  open/capture/reopen results, and the live recognition attempt.
- [Qt interface test results](ui-test-results.md): observed automated widget
  results, test limitations, and the isolated fixture used.

## Build and test again on this machine

Run PowerShell from the repository root. This uses the installed Qt compiler
and an existing OpenCV cache built with that same compiler; it does not rebuild
OpenCV or modify the sibling repository.

```powershell
$env:PATH = 'C:/Qt/Tools/mingw1310_64/bin;C:/Qt/Tools/Ninja;C:/Qt/6.11.0/mingw_64/bin;' + $env:PATH
cmake -S . -B build/evaluation-camera -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe -DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/Ninja/ninja.exe -DCMAKE_PREFIX_PATH=C:/Qt/6.11.0/mingw_64 -DOpenCV_DIR=D:/Github-Repositories/Bar-Code-System-CPP/build/deps/opencv/x64/mingw/lib -DCAMERA=ON -DBUILD_TESTING=ON
cmake --build build/evaluation-camera --parallel 4
ctest --test-dir build/evaluation-camera --output-on-failure
& 'C:/Qt/6.11.0/mingw_64/bin/windeployqt.exe' --release --no-translations --compiler-runtime build/evaluation-camera/BarcodeHub.exe
& ./Run-Camera-Evaluation.cmd
```

The MinGW Debug application links the installed release Qt libraries, so the
successful deployment command uses `windeployqt --release`. An initial
`--debug` deployment attempt could not find a debug platform plugin; the
release-runtime deployment succeeded. The latest CTest run passed all five
suites in 1.28 seconds. CameraDemo also compiled in this camera-enabled build.

If this OpenCV cache is absent on a different PC, the existing
[Windows build script](../../scripts/build-windows.ps1) can download/build the
dependency and assemble a portable application. That full script was not run
for this evaluation because a matching cache already existed.

## A ten-minute evaluation sequence

1. **Explain the architecture.** The Qt GUI accepts input; AppState coordinates
   catalogue, order and billing services; four local text files store data.
2. **Show Products.** Search `40001029`, open Details, explain price, unit,
   tax, discount, available stock and the barcode image.
3. **Show the camera.** Use only one camera panel at a time. Click Start camera,
   hold the label upright with vertical bars and visible white side margins,
   and watch for the full eight-digit code. The supported format is EAN-8.
   If recognition fails, stop capture and use typed lookup for the sale.
4. **Build the sale explicitly.** On Record Order enter milk `40001029`, press
   Enter, click Add to order, increase milk quantity to two; enter bread
   `40002149`, press Enter, click Add to order once.
5. **Explain the arithmetic.** Subtotal 940, discount 20, tax 18, payable
   **PKR 938**. These values assume the supplied catalogue prices.
6. **Show validation.** In payment, PKR 900 is insufficient. PKR 1,000 pays
   the bill and produces **PKR 62 change**. Use evaluation data for confirmation.
7. **Show evidence.** Read the receipt; locate the order in Orders and its bill
   in Bills. For the untouched starting fixture, milk stock changes 42→40 and
   bread 34→33. Two bill item rows share a single bill ID and order ID.
8. **Show alternatives.** Card and Wallet require merchant names. These are
   locally recorded payment modes; there is no bank authorization connection.
9. **Show reports and preferences.** Orders supports CSV export; Dashboard shows
   today's counters. Apply the low-stock threshold or dark mode in Settings.
10. **Explain persistence and limits.** Restart restores saved sales/stock, while
    the cart is lost. Receipts are text views, without printer/PDF integration.

## Explain the whole system in one minute

"BarcodeHub is a C++17 desktop point-of-sale system built with Qt Widgets.
Products are loaded from Product.txt. A cashier types or scans an EAN-8 barcode
to find a product, checks its description and stock, and builds a cart. The
order module validates quantities and creates an eight-digit order ID. The
billing module calculates per-unit tax and discount, validates the selected
cash/card/wallet payment, creates a nine-digit bill ID, and saves the sale.
The GUI coordinator saves stock changes and attempts rollback if saving fails.
Orders, Bills and Dashboard display the saved results. Storage is local text
files; payment processing is simulated locally rather than connected to a bank."

## Functionality checklist for the evaluator

Use the detailed manual guide for individual acceptance steps. A feature can
have passing backend/widget evidence while still needing human interaction or
physical-camera verification.

- [ ] Startup and all seven pages.
- [ ] Product search, description, barcode image and load issues.
- [ ] Camera start, video, live barcode recognition, stop and restart.
- [ ] Manual barcode/ID/acronym lookup and unknown-code handling.
- [ ] Explicit Add, auto-add preference and repeat-scan quantity behavior.
- [ ] Cart plus/minus, removal, Clear and retained customer fields.
- [ ] Zero-stock and combined-quantity rejection.
- [ ] Custom item entry, price, quantity and cancellation.
- [ ] Subtotal, tax, discount and payable calculations.
- [ ] Cash shortfall, exact payment, change and payment cancellation.
- [ ] Card and Wallet merchant validation and local recording.
- [ ] Receipt IDs and amounts; completed-sale draft reset.
- [ ] Order/detail/bill persistence and product stock persistence.
- [ ] Orders search/filter/details and CSV export.
- [ ] Bills search and historical receipts.
- [ ] Dashboard daily counters, hourly chart and low-stock indicators.
- [ ] Settings Apply, dark mode persistence and session-only preferences.
- [ ] Context-dependent keyboard shortcuts.
- [ ] Restart with the same data copy.

## Scope to state accurately

- The active scanner reads EAN-8, even though one legacy API is named decodeSix.
- The active POS target does not provide the historical generator/image-upload UI.
- Backend order code allows fractional weight quantities; the GUI uses integers.
- Card/Wallet validate merchant text and save records without external settlement.
- Settings' beep flag has no implemented scan audio action.
- Hold/resume drafts, refunds, login/roles and inventory purchasing are not active
  cashier workflows in this implementation.
- Temporary files and rollback attempts are not a crash-safe transaction across
  the four files, and there is no multi-process checkout locking.
- The live camera probe captured frames but decoded no label during its timed
  attempt. Do not present fixture-image success as a successful physical scan.
