# Per-screen acceptance rehearsal

All rows below are **pending interactive verification**. Initial review used
`1b16a70`; the GUI build with `CAMERA=OFF` now passes at `883425c` after its header
repair. Compilation does not execute the interactions below. Record a pass only
after observing the stated behavior in the built application; camera checks are unexecuted.

Use disposable copies of all four data files for checkout and restart checks.
The stock, order, detail, and bill files must belong to the same rehearsal set.

## Acceptance table

| Screen | Manual action to rehearse | Expected observation from source | Result |
| --- | --- | --- | --- |
| Startup | Launch with a known data copy | Record Order opens with an empty cart and sample customer fields | Pending |
| Products | Search `olpmilk`, then open Details | Milk row appears; description shows unit, stock and price fields | Pending |
| Products | Search zero-stock cola `40011721` | Add is disabled; stock badge says Out | Pending |
| Products | Click milk Add with auto-add off | Record Order opens with milk preview; cart remains unchanged | Pending |
| Record Order | Type `40001029`, Enter, Add to order | Preview appears, then one milk unit enters cart | Pending |
| Record Order | Increase milk to two; add bread `40002149` | Shipped fixture gives subtotal 940, discount 20, tax 18, total 938 PKR | Pending |
| Record Order | Press minus when a row quantity is one | Row is removed and totals update | Pending |
| Record Order | Add Custom item with a description and price | New manual line appears without a catalogue stock entry | Pending |
| Payment | Enter cash below total | Confirm Payment is disabled and shortfall is displayed | Pending |
| Payment | Cancel the payment dialog | Cart remains available; no checkout is requested | Pending |
| Payment | Select Card or Wallet without merchant text | Confirmation is disabled until merchant text is entered | Pending |
| Receipt | Complete cash checkout on copied data | Text receipt shows IDs, totals, tender and change | Pending |
| Orders | Find the completed order by ID or customer | Details and receipt actions show that saved transaction | Pending |
| Orders | Filter/search, then Export CSV | Export contains all loaded orders, not only visible matches | Pending |
| Bills | Search the completed bill ID | Paid order appears only when a saved bill ID exists | Pending |
| Dashboard | Revisit after checkout | Today's sales/orders and relevant hourly bucket update | Pending |
| Scanner | Type a known code and use Lookup | Last product and recent-scan list update | Pending |
| Settings | Change low-stock limit and Apply | Product badges/dashboard low-stock counts use the new threshold | Pending |
| Settings | Toggle dark mode; restart | Dark preference persists through QSettings | Pending |
| Restart | Reopen the same copied data | Paid order/bill persist; draft is empty; cash tender is not restored | Pending |

## Hardware checks

Stop capture on one camera page before starting the other. Start one camera,
present a clear fixture barcode, then confirm the product preview and emitted code.
Record the device and lighting if this passes; it is separate from manual lookup.
With auto-add enabled, holding a barcode in view can add it again after roughly
three seconds. Move it away after the intended detection and inspect quantity.

## Record useful evidence

For each passed row, record the source commit, executable build directory, data
directory, action, and observed result. For failed rows, keep the exact compiler
or UI message and the data copy used. Do not label pending rows as passed merely
because the five backend tests pass.

Sources: [page wiring](../../../Frontend/src/ui/MainWindow.cpp),
[cart](../../../Frontend/src/ui/RecordOrderPage.cpp),
[other pages](../../../Frontend/src/ui/OtherPages.cpp),
[dialogs](../../../Frontend/src/ui/Dialogs.cpp),
[dashboard](../../../Frontend/src/ui/DashboardPage.cpp),
and [camera widget](../../../Frontend/src/ui/Widgets.cpp).

## Check the copied data after the worked sale

These are pending acceptance checks for the shipped fixture, not observed GUI
results. Begin with milk stock 42 and bread stock 34 in disposable data, sell
two milk and one bread, and compare the records before and after:

| Evidence | Expected change for one successful checkout |
| --- | --- |
| Product.txt | Milk 42 to 40; bread 34 to 33 |
| Order.txt | One new paid header with paymentAmount 938 |
| OrderDetail.txt | Two matching rows: milk quantity 2, bread quantity 1 |
| Bill.txt | Two new rows sharing one bill ID and order ID; payable 938 |
| Second checkout attempt on that saved order | Backend rejects already-paid order |

Record the baseline ID set instead of assuming the data files were empty.
Closing the receipt should clear the draft; it should not remove saved records.
