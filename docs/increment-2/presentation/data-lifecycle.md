# Data lifetime and reload behavior

Use this map to explain what survives navigation, checkout, and application exit.
Initial review used `1b16a70`; the GUI build with `CAMERA=OFF` passes at `883425c`.
GUI/camera interactions are unexecuted. Storage uses text files and one QSettings preference.

## Ownership and lifetime

| Value | Owner/location | When it changes or reloads | After restart |
| --- | --- | --- | --- |
| Active cart | RecordOrderPage memory | Explicit add/remove/quantity actions; clear; successful checkout | Lost |
| Customer/contact fields | RecordOrderPage widgets | Cashier edits; initial sample defaults | Initial defaults return |
| Product catalogue/stock cache | ProductManager and AppState memory | Startup load, checkout deduction, rollback reload | Loaded from Product.txt |
| Inventory | Selected Product.txt | Successful save during checkout | Retained |
| Orders and details | Selected Order.txt/OrderDetail.txt | Order creation/update and recovery operations | Retained |
| Bills | Selected Bill.txt | Successful billing append | Retained |
| History display cache | AppState memory | Startup and successful checkout reconstruction | Rebuilt from files |
| Cash tendered | Current OrderRecord | Current payment dialog | Not restored from records |
| Low-stock threshold | Settings memory | Apply button | Defaults to 10 |
| Auto-add preference | Settings memory | Apply button | Defaults to false |
| Beep preference | Settings memory | Apply button; no audio action implemented | Defaults to true |
| Dark mode | BarcodeHub/SmartPOS QSettings | Apply when changed | Retained |
| Reorder markers | AppState in-memory set | Dormant reorder methods | Lost; no active reorder UI |

## Navigation is not a reset

Changing pages retains the current draft and customer/contact fields. Dashboard
New Order navigates to Record Order without clearing an existing cart.
Ctrl+N or Clear empties the cart, but it also retains customer/contact fields.
Successful checkout clears the cart after receipt dismissal and retains those
fields too. Check the customer's name before the next transaction.

## External file edits are not live updates

There is no active file watcher or visible Reload button. Normal catalogue
loading happens at startup; stock rollback also reloads it. Successful checkout
refreshes the catalogue cache from its existing in-memory manager, rather than
reading arbitrary external catalogue edits anew. History is reconstructed at
startup and after checkout, so navigation alone does not reread record files.

For rehearsal data changes, close the application, prepare a complete copied
data set, then restart with the intended directory. Avoid editing files while
a session is using its cache: a later save can replace external edits with the
session's values. Concurrent application instances have no file-lock protocol.

## Startup and daily values

Data-directory selection looks for Product.txt; it does not validate all four
files before choosing a candidate. Product-load issues and history-load errors
follow different reporting paths. See [history diagnostics](history-diagnostics.md).
Daily sales/order counters are calculated during history reconstruction, with
no midnight refresh timer. Restart or checkout can change the displayed day totals.

Sources: [AppState](../../../Frontend/src/ui/AppState.cpp),
[draft lifecycle](../../../Frontend/src/ui/RecordOrderPage.cpp),
[navigation](../../../Frontend/src/ui/MainWindow.cpp),
[settings controls](../../../Frontend/src/ui/OtherPages.cpp),
and [theme settings](../../../Frontend/src/ui/Theme.cpp).
