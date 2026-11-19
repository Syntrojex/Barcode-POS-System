# Navigation, scanning preferences, and settings

The application starts on Record Order and provides seven sidebar screens.
These notes describe source behavior; see the [demo readiness gate](demo.md).

## Keyboard navigation

| Key | Current action |
| --- | --- |
| F1 | Open Dashboard. |
| F2 | Open Record Order and focus barcode input. |
| F3 | Open Orders. |
| F4 | Browse products on Record Order; otherwise open Products. |
| F5 | Open Barcode Scanner. |
| F6 | Open Bills. |
| F9 | Open Record Order if needed and start its camera. |
| F10 | Checkout on Record Order; otherwise open Settings. |
| Ctrl+K | Open Dashboard and focus its lookup field. |
| Ctrl+N | Open Record Order and clear the current cart. |

F4 does not open Product Description, despite the current header comment.
Use the visible Description or Details button to inspect a product.
The Scanner screen's Start Scan button starts that screen's camera;
its F9 label differs from the global shortcut, which routes to Record Order.
Dashboard New Order navigates to the draft without clearing an existing cart.

## Apply settings

| Setting | Effect | Persistence |
| --- | --- | --- |
| Dark mode | Reapplies the application stylesheet. | Saved through `QSettings`. |
| Low-stock limit | Changes stock badges and low-stock counts. | Current process only. |
| Auto-add scanned items | Adds a detected product to the cart when enabled. | Current process only. |
| Beep on successful scan | Checkbox value is stored but no sound is implemented. | Current process only. |

Defaults are a low-stock limit of 10, auto-add off, and beep enabled.
The initial dark-mode value is read from `BarcodeHub/SmartPOS` settings.
Tax and discount are per-product PKR amounts loaded from `Product.txt`.
There is no global tax/discount editor in Settings.

## Scanning and explicit Add actions

Manual barcode entry and Browse show a preview; use Add to order to add the item.
With auto-add off, Products Add and Scanner Add last scan to order also only
open the Record Order preview because they share the scanner-style entry path.
With auto-add enabled, that entry path adds one unit after a successful lookup.
Stock validation still applies to additions and quantity increases.

The two camera pages own separate camera widgets and keep running when hidden.
Stop capture before switching to the other camera page to avoid device contention.
Holding the same barcode in view can emit it again after roughly three seconds;
with auto-add enabled, keep the cart quantity visible during a demonstration.

## Source map

- [Navigation and shortcuts](../../Frontend/src/ui/MainWindow.cpp)
- [Settings screen](../../Frontend/src/ui/OtherPages.cpp)
- [Defaults](../../Frontend/include/ui/AppState.h) and [theme persistence](../../Frontend/src/ui/Theme.cpp)
- [Cart entry behavior](../../Frontend/src/ui/RecordOrderPage.cpp)
- [Camera lifecycle and duplicate suppression](../../Frontend/src/ui/Widgets.cpp)
