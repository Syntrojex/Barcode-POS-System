# Known limitations at the inspection baseline

Reviewed at `2d3c58d` on 19 November 2026. Only the GUI build failure below was
compiler-reproduced; other findings are from source inspection unless stated.
The five backend CTest suites passed. No functional fixes are included.

## Historical presentation blocker - resolved at `883425c`

**At the original baseline the Qt GUI did not compile.**
[RecordOrderPage.cpp](../../Frontend/src/ui/RecordOrderPage.cpp) used members
absent from [RecordOrderPage.h](../../Frontend/include/ui/RecordOrderPage.h).
A separate commit, `883425c`, restored the declarations. A subsequent Qt build
with `CAMERA=OFF` passed. See [follow-up results](presentation/review-results.md).
The source fix was not part of the documentation changes. Camera support and
interactive checkout still require runtime verification.

## Functional boundaries

| Finding | Consequence / source |
| --- | --- |
| Card/wallet validate merchant text locally | No bank authorization or settlement; [PaymentMethod.cpp](../../Backend/src/billing/PaymentMethod.cpp) |
| UI quantities and catalogue stock are integers | Backend fractional `weight` support is not available end-to-end in the GUI; [AppState.h](../../Frontend/include/ui/AppState.h) |
| Products/Scanner Add routes use the scan path | With auto-add disabled, they preview; use the Record Order Add button; [MainWindow.cpp](../../Frontend/src/ui/MainWindow.cpp) |
| No persisted draft/hold/resume UI | Cart is memory-only, despite backend order states; [RecordOrderPage.cpp](../../Frontend/src/ui/RecordOrderPage.cpp) |
| Receipt is a text dialog | No implemented printer/PDF export; [Dialogs.cpp](../../Frontend/src/ui/Dialogs.cpp) |
| Settings mostly last only for the session | Dark mode uses QSettings; beep flag has no scan action; [OtherPages.cpp](../../Frontend/src/ui/OtherPages.cpp) |
| Navigation does not stop a running camera | Stop one page's camera before opening the other; [MainWindow.cpp](../../Frontend/src/ui/MainWindow.cpp) |
| Daily chart covers 08:00-20:00 | Other-hour sales are outside the chart; Pending also includes archived orders; [DashboardPage.cpp](../../Frontend/src/ui/DashboardPage.cpp) |

## Data and edge cases

- Catalogue parsing accepts some non-finite numeric strings: `strtod` and the
  price rule lack a finite-value check. Later order validation rejects them.
  See [ProductStorage.cpp](../../Backend/src/product/ProductStorage.cpp) and
  [ProductValidation.cpp](../../Backend/src/product/ProductValidation.cpp).
- IDs/barcodes must be unique, but acronyms are not checked for uniqueness.
  The GUI merges cart rows by acronym; duplicate acronyms can combine distinct
  products. The shipped fixture uses distinct acronyms.
- A zero-payable bill can pass calculation/payment validation, but a Paid order
  requires positive payment, so checkout fails. See [Order.cpp](../../Backend/src/order/Order.cpp).
- Stock, order, detail and bill files are updated separately. Temporary-file
  replacement and rollback provide best-effort recovery, not a crash-safe
  multi-file transaction. No process locking exists; see [checkout.md](checkout.md).
- Bill parsing accepts finite amounts without verifying all business constraints
  or repeated header consistency. Per-line tax and complete historical display
  names are not snapshotted; see [records.md](records.md) and [history.md](history.md).
- CSV export wraps fields in quotes without escaping embedded quotes, which can
  produce malformed CSV. See [OtherPages.cpp](../../Frontend/src/ui/OtherPages.cpp).

## Build and documentation differences

The camera source/header capitalization differs between Git filenames and some
includes/CMake paths, which can fail on case-sensitive systems. The main POS
does not expose the historical barcode generator or image-upload screens.
The product acronym rule defaults to 10 characters although the increment plan
specifies 8. Older product/billing README claims include obsolete paths and
overstated persistence guarantees; these inspection notes describe current code.
