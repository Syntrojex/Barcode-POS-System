# Increment 2 manual evaluation guide — 19 November 2026

This is a repeatable test procedure for BarcodeHub's current Qt interface.
Every case below starts **Pending**. Expected results come from source inspection;
they become actual results only after a tester performs and records the action.
Build success and backend tests do not establish that a GUI or camera case passed.
The live-test operator should record observations separately using the case IDs.

## Prepare a reproducible session

1. Record the tested commit, executable path, build configuration, camera option,
   Windows version, and test time. Record any source changes during testing.
2. Use a complete disposable copy of `data`, including its barcode PNGs. Preserve
   an untouched copy for resetting between cases. Point `BARCODEHUB_DATA` at the
   copied directory **before launching**; it must contain `Product.txt`.
3. Verify the runtime catalogue starts with milk stock 42 and bread stock 34.
   Clear transaction files only in a dedicated fixture prepared for that purpose;
   otherwise record initial order counts/sales and evaluate the change in totals.
4. Start one application instance. Stop camera capture before switching between
   Record Order and Barcode Scanner camera panels.
5. Replace the sample customer and phone with `Evaluation Cash` and `03001234567`.
   Default sample identity text is not evidence of login/authentication.
6. Capture before/after evidence for checkout. A real checkout writes inventory,
   order headers, order details, and bills in the selected copied directory.

If `BARCODEHUB_DATA` lacks Product.txt, directory selection can fall back to a
different candidate. Confirm the chosen runtime directory rather than assuming
the repository's files and runtime files are identical.

## Reference fixture and arithmetic

| Product | Barcode | Price | Tax/unit | Discount/unit | Initial stock |
| --- | --- | ---: | ---: | ---: | ---: |
| Olpers Full Cream Milk 1L | `40001029` | 340 | 0 | 10 | 42 |
| National Whole Wheat Bread | `40002149` | 260 | 18 | 0 | 34 |
| Farm Fresh Eggs Dozen | `40007359` | 390 | 0 | 0 | 4 |
| Coca Cola 1.5L | `40011721` | 230 | 18 | 0 | 0 |

The main checkout fixture is **two milk plus one bread**:

- Subtotal: `2 × 340 + 260 = PKR 940`.
- Discount: `2 × 10 = PKR 20`; tax: `1 × 18 = PKR 18`.
- Payable: `940 - 20 + 18 = PKR 938`.
- Cash tendered PKR 1,000 gives change PKR 62.
- After successful checkout milk stock becomes **40** and bread stock **33**.

These exact values require an untouched catalogue fixture. Record actual starting
stock when rerunning a checkout; each additional transaction deducts stock again.

## Startup, navigation, and products

| ID | Steps | Expected result | Status |
| --- | --- | --- | --- |
| NAV-01 | Launch the current executable | Record Order opens; empty cart; seven sidebar destinations are present | Pending |
| NAV-02 | Visit Dashboard, Record Order, Orders, Products, Barcode Scanner, Bills, Settings | Each destination opens without crash; title/subtitle follow the page | Pending |
| NAV-03 | Add a draft item, visit Dashboard, use New Order | Existing cart is retained; Dashboard New Order is navigation | Pending |
| PRD-01 | Products: search `milk`, `OLPMILK`, then `40001029` | Relevant milk record appears; exact values match the runtime catalogue | Pending |
| PRD-02 | Open milk Details | Description shows product fields, stock status, and barcode image when path resolves | Pending |
| PRD-03 | Products: search `zzzz-no-match` | Empty result set, no stale matched row | Pending |
| PRD-04 | Products: find eggs and cola | Eggs is Low at default threshold 10; cola is Out and its Add is disabled | Pending |
| PRD-05 | With auto-add off, Products: click milk Add | Record Order opens and previews milk; explicit Add to order is still needed | Pending |
| PRD-06 | Dashboard lookup: enter `40001029`, then an actual saved order ID | Product code opens Products search; an unmatched product code is routed to Orders search | Pending |

Catalogue text search covers name/acronym/barcode. The exact code lookup additionally
supports numeric product ID (`1`) and acronym (`OLPMILK`). Record a mismatch if a
screen's placeholder promises broader text matching than the actual search provides.

## Manual entry, cart, custom items, and stock

| ID | Steps | Expected result | Status |
| --- | --- | --- | --- |
| CART-01 | Empty draft; type `40001029`, press Enter | Milk preview appears; cart stays empty until Add to order | Pending |
| CART-02 | Click Description, close it, click Add to order | Correct product description; one milk in cart, payable PKR 330 | Pending |
| CART-03 | Press milk plus once; enter bread barcode, Enter, Add to order | Two milk/one bread; three item units; totals 940/20/18/938 | Pending |
| CART-04 | Press bread minus at quantity one | Bread row disappears and totals return to milk-only values | Pending |
| CART-05 | Add bread again, then use its trash button | Only bread is removed; milk quantity remains two | Pending |
| CART-06 | Use Browse/F4 on Record Order; select bread | Product preview changes; choosing a product does not add it automatically | Pending |
| CART-07 | Empty input, Enter; then unknown code `99999999`, Enter | Informative toast; no new cart row; unknown lookup clears preview | Pending |
| CART-08 | Start a disposable draft; add eggs and increase to four, then press plus | Four is allowed; fifth is rejected; total stays PKR 1,560 | Pending |
| CART-09 | Try to add zero-stock cola through manual lookup | Product can be described; Add to order is disabled | Pending |
| CART-10 | Custom item: description `Evaluation service`, price PKR 25.50 | Manual line quantity one; total PKR 25.50, zero default tax/discount | Pending |
| CART-11 | Cancel custom description; repeat with blank description | Neither attempt adds a row | Pending |
| CART-12 | Enter a custom description, then cancel price | No custom row is added | Pending |
| CART-13 | Increase custom quantity to two | Total PKR 51.00; no catalogue inventory entry changes | Pending |
| CART-14 | Clear/Ctrl+N on a disposable draft | Cart and preview clear; customer/contact are retained | Pending |

Use Clear between custom/stock cases and the main payment fixture. Current UI
quantity controls use whole integers even for products labeled `weight`.
Duplicate catalogue acronyms can merge different products into one cart row;
the shipped fixture has distinct acronyms. Do not alter it during acceptance runs.

## Cash checkout, cancellation, and persisted evidence

Recreate the main two-milk/one-bread fixture with starting stock 42/34. Use the
customer `Evaluation Cash`. Record the displayed order ID before payment.

| ID | Steps | Expected result | Status |
| --- | --- | --- | --- |
| PAY-01 | Empty draft: press F10 | Empty-order toast; no payment dialog or saved order | Pending |
| PAY-02 | Main fixture: Checkout/F10 | Instant Payment dialog shows PKR 938; Cash initially selected | Pending |
| PAY-03 | Set tender to PKR 900 | Short by PKR 38; Confirm Payment disabled | Pending |
| PAY-04 | Set tender to PKR 938 | Confirmation enabled; change PKR 0 | Pending |
| PAY-05 | Cancel payment | Draft remains; stocks/order counts/files unchanged | Pending |
| PAY-06 | Reopen; tender PKR 1,000; Confirm Payment | Receipt opens with real order and bill IDs; payable 938, tender 1,000, change 62 | Pending |
| PAY-07 | Close receipt | Cart clears; customer/contact stay; next order ID appears | Pending |
| PAY-08 | Products: inspect milk and bread | Stocks are now milk 40, bread 33 | Pending |
| PAY-09 | Inspect copied transaction files read-only | Matching order is paid; detail rows contain milk quantity two/bread one; bill stores totals 940/18/20/938 | Pending |

Keep the generated order/bill IDs for history and restart cases. Receipt is a text
viewer: no print/PDF action is implemented. Card/wallet are local payment records,
not bank authorization or settlement.

## Card, wallet, and custom-item checkout

Run each case on its own draft. Keep the main cash receipt separate so later
dashboard comparisons account for these additional sales.

| ID | Steps | Expected result | Status |
| --- | --- | --- | --- |
| MODE-01 | One milk, Checkout, Card; leave merchant blank, then spaces | Confirmation disabled | Pending |
| MODE-02 | Card: merchant `HBL`; Confirm Payment | Paid order/bill for PKR 330; Card recorded; milk stock decreases by one; no tender/change shown | Pending |
| MODE-03 | One bread, Checkout, Wallet; merchant blank | Confirmation disabled | Pending |
| MODE-04 | Wallet: merchant `JazzCash`; Confirm Payment | Paid order/bill for PKR 278; Wallet recorded; bread stock decreases by one | Pending |
| MODE-05 | Checkout a PKR 25.50 custom item with exact cash | Paid custom line; bill retained; catalogue inventory unchanged | Pending |
| MODE-06 | Open each resulting order's details/receipt | Payment mode and amount match the corresponding transaction | Pending |

Merchant text is validated again by the backend; enabled UI confirmation alone
does not establish that every possible merchant value will be accepted.

## Orders, bills, CSV, dashboard, and restart

| ID | Steps | Expected result | Status |
| --- | --- | --- | --- |
| HIST-01 | Orders: All; search saved cash order ID, then customer/phone | Correct row found; details match three units and PKR 938 | Pending |
| HIST-02 | Orders: clear search; choose Paid, In Progress, On Hold | Rows follow selected status; empty tabs are acceptable for a paid-only fixture | Pending |
| HIST-03 | Orders: View details then Receipt | Detail SUBTOTAL is unit-price × qty; receipt separately shows tax/discount and final total | Pending |
| HIST-04 | Bills: search saved bill ID, order ID, customer | Paid bill appears with actual bill number and payable PKR 938 | Pending |
| HIST-05 | Filter Orders to one row; Export CSV to a disposable output file | CSV contains all loaded orders, regardless of active search/status filter | Pending |
| HIST-06 | Cancel Export CSV save dialog | No export file created and no success toast | Pending |
| DASH-01 | After main cash transaction only, compare Dashboard to baseline | Today's paid revenue rises 938; today's order count rises one | Pending |
| DASH-02 | Inspect average/recent orders/hourly chart | Average uses paid orders; recent list includes newest order; chart includes it only at hours 08–20 | Pending |
| LIFE-01 | Leave one unpaid draft line; close; choose No | Application stays open with draft intact | Pending |
| LIFE-02 | Close; choose Yes; restart same data copy | Paid history/bills and stock persist; unpaid draft is lost | Pending |
| LIFE-03 | Reopen cash receipt after restart | Payable remains 938; cash tender/change are not restored | Pending |
| LIFE-04 | Reopen custom receipt after restart | Saved price/quantity/totals persist; custom name falls back to saved acronym | Pending |

CSV embedded quotes are not escaped; this can produce malformed exported text for
names containing quotes. An empty history screen can indicate a malformed record
load rather than empty files: inspect warning output and all transaction files.

## Settings and keyboard checks

| ID | Steps | Expected result | Status |
| --- | --- | --- | --- |
| SET-01 | Settings: low-stock limit 5, Apply | Eggs at stock four remains Low; chips at six is OK; zero-stock cola remains Out | Pending |
| SET-02 | Enable auto-add, Apply; Products Add milk | One milk enters cart through the shared scan-style route | Pending |
| SET-03 | With auto-add on, manually type a barcode and Enter | Manual entry still previews; explicit Add is required | Pending |
| SET-04 | Toggle dark mode, Apply; visit all pages | Theme changes; record any unreadable controls as visual defects | Pending |
| SET-05 | Restart | Dark mode persists; low-stock limit returns to 10; auto-add returns off; beep checkbox returns enabled | Pending |
| KEY-01 | F1/F2/F3/F5/F6 | Dashboard/Record Order/Orders/Scanner/Bills open | Pending |
| KEY-02 | F4 on Record Order, then on another page | Browse chooser on Record Order; Products elsewhere | Pending |
| KEY-03 | F10 on Record Order, then on another page | Checkout/empty-cart toast on Record Order; Settings elsewhere | Pending |
| KEY-04 | Ctrl+K; Ctrl+N on disposable draft | Dashboard lookup focused; then Record Order draft cleared | Pending |
| KEY-05 | F9 from Scanner | Routes to Record Order and starts that page's camera, rather than Scanner's local Start Scan button | Pending |

The beep checkbox currently has no sound implementation; record that limitation
without treating a silent scan as a failed decoder. Active shortcut routing is in
`MainWindow::handleFKey()`, not the unused `NavigationModel::items()` list.

## Camera acceptance and fallback

A `CAMERA=OFF` executable cannot establish camera readiness. Record a build with
camera support and actual hardware trial separately. The two pages own separate
camera widgets. Stop one before starting the other to avoid device contention.

| ID | Steps | Expected result | Status |
| --- | --- | --- | --- |
| CAM-01 | Record Order: Start Cam or F9 | Live frames appear, or a clear error explains device/backend failure | Pending |
| CAM-02 | Hold a clear milk EAN-8 image flat, steady, with even light | Decoded `40001029` matches milk preview; no wrong product is accepted | Pending |
| CAM-03 | Auto-add off: detect milk | Preview changes without cart mutation | Pending |
| CAM-04 | Auto-add on: detect milk once, then move barcode away | Quantity rises by one after successful lookup/stock validation | Pending |
| CAM-05 | Keep same barcode visible over three seconds | Duplicate suppression can expire and emit it again; record actual repeated additions | Pending |
| CAM-06 | Stop Record Order camera; open Scanner and Start Cam/Start Scan | Scanner frames appear; detected code enters Recent Scans | Pending |
| CAM-07 | Scanner: Show description; Add last scan to order | Description matches; shared route previews/adds according to auto-add preference | Pending |
| CAM-08 | Stop camera; type milk barcode in Scanner and Lookup | Product and recent-scan display update without hardware capture | Pending |

If device opening fails, record the exact message and hardware/build context;
mark decoding cases Blocked rather than Passed. Manual lookup is the presentation
fallback. It tests product/cart flow but does not prove camera decoding works.

## Record actual results

Use `Pass`, `Fail`, `Blocked`, or `Not run`, keeping evidence separate from expected
behavior. If execution has started, replace Pending with the actual result only.

| Case ID | Actual result | Evidence/file or screenshot | Build/data directory | Note |
| --- | --- | --- | --- | --- |
| Fill after execution | Not run | None yet | Record both paths | Record observed value/message |

Known boundaries to mention during evaluation: whole-number UI quantities;
in-memory draft/customer retention; local card/wallet recording; no printer/PDF,
refund or hold/resume controls; no live file reload; non-atomic multi-file checkout;
08:00–20:00 chart window; archived orders counted as pending; and header/identity
fixture text that can differ from runtime behavior. A running GUI proves startup,
not every behavior listed above.

Source references: [Record Order](../../Frontend/src/ui/RecordOrderPage.cpp),
[pages and settings](../../Frontend/src/ui/OtherPages.cpp),
[navigation](../../Frontend/src/ui/MainWindow.cpp),
[payment and receipts](../../Frontend/src/ui/Dialogs.cpp),
[state and persistence](../../Frontend/src/ui/AppState.cpp),
[dashboard](../../Frontend/src/ui/DashboardPage.cpp),
[camera widget](../../Frontend/src/ui/Widgets.cpp), and
[fixture catalogue](../../data/Product.txt).
