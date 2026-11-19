# Increment 2 requirement evidence

Use this table with the [increment plan](../../../Project%20Planning/Increment-2.md).
"Source" means the implementation was inspected. "Backend test" means a
registered CTest suite passed; it does not imply that a Qt widget or camera ran.
The Qt build now passes with `CAMERA=OFF` at `883425c`; every screen-oriented row
still needs interactive verification. The earlier header mismatch was repaired
in a separate existing commit, outside this documentation work.

| Planned responsibility | Implementation evidence | Verification and qualification |
| --- | --- | --- |
| Read `Product.txt` | `ProductFileLoader`, `ProductParser`, `ProductManager` | Product suite passes; active parser accepts multiword names and can retain accepted rows from a partially invalid file. |
| Product model | `Product` in `product/ProductModel.h` | Product and integration suites pass. Stock is represented as an integer. |
| Barcode and product-ID search | `ProductManager::searchByCode`, `searchByID` | Backend tests pass; GUI lookup is source-reviewed. |
| Product descriptions | `ProductDescriber`, `Dialogs::productDescription` | Backend description and GUI dialog source reviewed; screen not exercised. |
| Available-stock validation | `StockValidator`, `OrderManager::validateProduct` | Product, order and integration tests pass; concurrent writers remain unsupported. |
| Camera-based reading | `CameraScanner`, decoder, `CameraFeedWidget` | Source only; EAN-8 pipeline, hardware not verified. |
| Scanner-to-POS integration | `barcodeDetected` to `RecordOrderPage::showCode` | Source only; auto-add setting controls preview versus addition. |
| Camera error handling | `CameraFeedWidget::setRunning` and error signal | Source only; camera startup can fail, and manual entry is the fallback. |
| Reuse earlier barcode reading | `decodeSix`/`decode` scanner API | Active decoder handles eight-digit EAN-8 despite the historical function name. |
| Cart additions/quantity/removal | Backend `OrderManager` and GUI `RecordOrderPage` | Backend suite passes; GUI uses integer quantities and its own draft cart. |
| Eight-digit Order IDs | `OrderManager::generateOrderId` | Order suite passes; daily sequence is limited to 99. |
| Save order header | `OrderManager::createOrder`, `writeOrdersTo` | Order and checkout suites pass for normal persistence. |
| Save order item quantities | `OrderDetail`, `writeDetailsTo` | Order and integration suites pass; details do not snapshot prices. |
| POS GUI | `MainWindow` and seven pages | Source inspected; camera-disabled Qt build passes at `883425c`; interactive checks pending. |
| Calculate bills | `BillCalculator::calculate` | Billing/checkout tests pass; tax and discount are PKR amounts per unit. |
| Description button | Record Order Description / Products Details | Source only; F4 opens product browsing rather than this description dialog. |
| Save `Bill.txt` | `BillRecordCodec`, `BillRepository` | Billing and checkout suites pass; one row per item, grouped by bill ID. |
| Payment modes | `CashPayment`, `CardPayment`, `WalletPayment` | Billing tests pass; card/wallet record merchant text without bank authorization. |
| Modular design | Product rules, stock policies, payment strategies, adapter | Interfaces are present; adding a new payment enum still requires parser/UI work. |

## Source entry points

- [Product model](../../../Backend/include/product/ProductModel.h),
  [loading](../../../Backend/src/product/ProductStorage.cpp), and
  [validation](../../../Backend/src/product/ProductValidation.cpp).
- [Order manager](../../../Backend/src/order/OrderManager.cpp) and
  [product adapter](../../../Backend/src/integration/ProductAdapter.cpp).
- [Billing calculation](../../../Backend/src/billing/BillCalculator.cpp),
  [checkout service](../../../Backend/src/billing/BillingService.cpp), and
  [payment strategies](../../../Backend/src/billing/PaymentMethod.cpp).
- [Camera](../../../Backend/src/scanner/CameraScanner.cpp),
  [decoder](../../../Backend/src/scanner/decoder.cpp), and
  [active GUI target definition](../../../CMakeLists.txt).
- [Main window](../../../Frontend/src/ui/MainWindow.cpp),
  [Record Order](../../../Frontend/src/ui/RecordOrderPage.cpp), and
  [application persistence bridge](../../../Frontend/src/ui/AppState.cpp).

## Scope questions to anticipate

Barcode generation and image upload appear in older project material, but are
not active screens in the current POS target. Printing/PDF receipts, bank
settlement, login/roles, refunds, and a persisted hold/resume draft workflow are
not demonstrated by this implementation. Keep those claims separate from the
requirements satisfied by the backend and the source-reviewed UI.
