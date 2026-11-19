# Cart and inventory flow

This reference follows the inspected Increment 2 source and sample data.
The GUI and camera were not runtime-verified for this document.
The following presentation sequence is a rehearsal guide, not a recorded test result.

## Adding products

The [order page](../../Frontend/src/ui/RecordOrderPage.cpp) previews a matched product.
Typed lookup and Browse select it; Add to order adds one unit by default.
A camera scan can add automatically when the corresponding setting is enabled.
The product card disables Add to order for a product whose stock is zero.
Existing cart lines are identified by acronym/SKU and their quantities increase.
The page supports quantity changes, line removal, clearing, and custom items.

Before increasing quantity, [AppState](../../Frontend/src/ui/AppState.cpp) asks
the [stock validator](../../Backend/src/product/ProductValidation.cpp) to check
the amount already in the cart plus the requested addition.
The default policy disallows a request above available inventory.
Zero/negative additions, out-of-stock products, and insufficient stock have distinct messages.
Reducing a line below one unit removes it.

## Price calculation

For each line, gross price, tax, and discount are multiplied by quantity.
The payable total is gross minus discount plus tax.
Tax and discount values in the catalogue are amounts per unit in PKR.
The UI recomputes its display and checkout compares it with backend totals.

Using unchanged [sample stock](../../data/Product.txt):

| Selection | Gross | Discount | Tax | Payable |
| --- | ---: | ---: | ---: | ---: |
| Two milk units, barcode `40001029` | 680.00 | 20.00 | 0.00 | 660.00 |
| One bread, barcode `40002149` | 260.00 | 0.00 | 18.00 | 278.00 |
| Combined order | 940.00 | 20.00 | 18.00 | 938.00 |

Cash of PKR 1,000 for that total gives PKR 62 change.
Eggs (`40007359`) demonstrate the four-unit stock boundary.
Cola (`40011721`) demonstrates an unavailable sample product.

## Checkout and persistence

Checkout rebuilds the backend cart from current catalogue product snapshots.
It rechecks accumulated stock demand and payment before creating an order.
Then it captures the original product file, deducts stock, and saves inventory.
Billing creates the bill and completes payment/status persistence.
If saving stock or billing fails, the UI attempts inventory restoration and order removal.
Rollback failure can require manual attention; the files are not one database transaction.

## Scope and limitations

Cart edits alone do not reserve stock or write inventory to disk.
Custom items have no catalogue stock entry and bypass inventory deduction.
The frontend uses integer quantities, including products labelled `weight`.
The [order backend](../../Backend/src/order/OrderManager.cpp) separately supports fractional weight quantities.
The loader rejects duplicate IDs/barcodes but allows duplicate acronyms;
because the UI merges by acronym, catalogue acronyms should remain unique.
The inspected checkout flow does not provide multi-process inventory locking.
Use a disposable data copy when rehearsing checkout, since a successful sale changes saved stock.
