# Increment 2 presentation walkthrough - 20 November 2026

Present product lookup, scanning, order management, and billing as one workflow using four local text files.
The following GUI steps are a rehearsal plan, not a record of a successful live run.
The [presentation pack](presentation/README.md) adds viva questions and acceptance cases.

## Readiness gate

The original inspection found a frontend header/source mismatch in `RecordOrderPage`.
The separate header repair at `883425c` resolves that compile failure. A subsequent
Qt 6.11.0/GNU 13.1 build with `CAMERA=OFF` passed and linked `BarcodeHub.exe`.
No interactive GUI run followed. Rehearse checkout using copied data before the
presentation; camera capture requires a separately verified camera-enabled build.
See [follow-up results](presentation/review-results.md). This documentation work
did not make the source repair.

The backend audit passed all five CTest tests: product, order, integration, checkout,
and billing. That result covers those automated checks, not Qt screens or a camera.
If the GUI build remains blocked, use the backend evidence and source walkthrough.

## Manual-entry demonstration, after the gate passes

1. Open Products and identify milk `40001029` and bread `40002149`.
2. Open Details to show name, unit, price, tax, discount, stock, and barcode image.
3. Open Record Order. Replace the prefilled demo customer/contact values.
4. Type milk barcode `40001029`, press Enter, then click Add to order.
5. Increase milk quantity to two. Type bread `40002149` and explicitly add it once.
6. Explain the totals using the checked-in [product fixture](../../data/Product.txt).

| Item | Quantity | Unit price | Unit tax | Unit discount | Payable |
| --- | ---: | ---: | ---: | ---: | ---: |
| Olpers milk | 2 | 340 | 0 | 10 | 660 |
| National bread | 1 | 260 | 18 | 0 | 278 |
| Total (PKR) | 3 | | | | **938** |

Subtotal is PKR 940, discount PKR 20, tax PKR 18, and payable PKR 938.
Use current product values if the runtime data differs from this fixture.

7. Open Checkout. Show that insufficient cash disables confirmation.
8. Enter PKR 1,000, explain PKR 62 change, and confirm only in disposable demo data.
9. Show the receipt, then locate the saved order and its real bill ID in Orders/Bills.
10. Show the stock reduction and updated Dashboard totals for today's transaction.

## Additional talking points and fallbacks

- Card/Wallet require a merchant name and record payment locally; no bank is contacted.
- Show zero-stock Cola `40011721` in Products and explain why adding is disabled.
- Camera: stop the other page's capture, start one camera, and present a sample barcode.
- Camera fallback: type the barcode; manual entry exercises the same lookup/cart flow.
- Auto-add is off by default; explicit Add avoids confusing a preview with a cart line.
- Receipt is a text viewer; do not promise printing, PDF export, refunds, or draft resume.

For a backend-only fallback, show the CTest summary and explain the assertions in
[checkout tests](../../tests/checkout_test.cpp) and [billing tests](../../tests/billing_tests.cpp).
These commands passed all five tests in 4.22 seconds on the audit's Windows toolchain:

```powershell
cmake -S . -B build/increment2-audit-backend -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBACKEND_ONLY=ON -DCAMERA=OFF -DBUILD_TESTING=ON
cmake --build build/increment2-audit-backend --parallel 4
ctest --test-dir build/increment2-audit-backend --output-on-failure
```

Show the [build definitions](../../CMakeLists.txt) to distinguish backend tests from Qt.
Use the [history guide](history.md) and [settings guide](settings.md) for scope questions.

Checkout writes inventory and order/billing records; use a separate data copy for rehearsal.
