# Increment 2 inspection and presentation guide

Reviewed on 19 November 2026 against branch `ali`, starting at `2d3c58d`.
These notes describe the checked-in implementation and its verification results;
older planning documents describe intended requirements and can differ from the code.

**Readiness:** all five backend CTest suites passed. The current Qt frontend fails
to compile because `RecordOrderPage.h` does not declare members used by its `.cpp`.
Camera capture and the live checkout UI were not tested. See [verification](verification.md)
and [limitations](limitations.md) before preparing a demonstration.

## Reading map

| Topic | Reference |
| --- | --- |
| Components, dependencies, active and legacy code | [Architecture](architecture.md) |
| Backend build, Qt build, data location, packaging | [Build and run](build.md) |
| Catalogue parsing, validation, lookup, fixture data | [Products](products.md) |
| EAN-8 decoding and camera integration | [Scanner](scanner.md) |
| Cart quantities, stock checks, custom items | [Cart and stock](cart-and-stock.md) |
| Order lifecycle, identifiers, persistence | [Orders](orders.md) |
| Tax, discounts, totals, rounding | [Billing calculations](billing.md) |
| Cash, card, wallet and strategy scope | [Payments](payments.md) |
| Text record formats and encoding | [Records](records.md) |
| Checkout sequence and recovery boundaries | [Checkout](checkout.md) |
| Order history, bills, receipts, dashboard, CSV | [History and reporting](history.md) |
| Navigation, shortcuts and settings | [Settings](settings.md) |
| Executed checks and coverage gaps | [Verification](verification.md) |
| Build blocker and functional limitations | [Limitations](limitations.md) |
| Presentation sequence and worked example | [Demo guide](demo.md) |

## Feature status at inspection

| Area | Evidence |
| --- | --- |
| Products, orders, billing, payment validation | Source reviewed; registered backend tests passed |
| Frontend checkout, history and dashboard | Source reviewed; GUI compilation blocked |
| Camera scanner | Source reviewed; OpenCV/camera runtime not verified |
| Card and wallet payments | Local merchant validation and recording; no gateway integration |
| Barcode generation and image upload | Historical requirements; not active screens in the current POS target |

This review adds documentation only. No application logic, build configuration,
tests, catalogue records or existing transaction files are changed.
