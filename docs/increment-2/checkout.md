# Checkout coordination and recovery

Checkout joins the product, order, and billing modules through
[`AppState::addOrder`](../../Frontend/src/ui/AppState.cpp).
The sequence below describes the implemented source path, not a live GUI test.

## Before persistence

1. Reject missing data services, an empty order, or an unsupported payment mode.
2. Rebuild the backend cart from current catalog snapshots and manual item data.
3. Validate quantities and aggregate stock deductions for catalog products.
4. Compare GUI totals with backend totals and apply the selected payment rules.

Manual lines receive `CUSTOM-<acronym>` IDs and exist only in the checkout lookup.
Their bill price/discount/quantity survive restart; their full description does not.
The current GUI uses integer quantities, even though backend orders support weight
fractions. See [orders](orders.md) for the cart contract.

## Write sequence

1. Create an in-progress order: persist details, then its header.
2. Read the original `Product.txt` bytes for a possible stock restoration.
3. Deduct catalog stock in memory and save `Product.txt`.
4. Call [`BillingService::checkout`](../../Backend/src/billing/BillingService.cpp).
5. Revalidate order/payment, calculate the bill, and allocate a nine-digit bill ID.
6. Update the order to paid, then append the bill through `BillRepository`.
7. Refresh cached stock/history and return the saved IDs/totals to the GUI.

[`RecordOrderPage`](../../Frontend/src/ui/RecordOrderPage.cpp) displays a receipt
and clears its visible cart only after `addOrder` succeeds. A failed checkout
displays its error and retains the visible cart for correction or retry.

## Recovery paths

| Failure | Recovery attempted |
| --- | --- |
| Order header save after details | Restore the previous detail rows. |
| Product backup cannot be read | Discard the new in-progress order. |
| Product save or billing fails | Restore original product bytes and discard the new order. |
| Bill append fails after marking paid | Billing first attempts to restore the previous order state. |

Stock restoration uses `QSaveFile`. Several backend writers use a temporary file,
rename it into place, and fall back to overwriting by copy if rename fails.
Some rollback errors are ignored; the GUI reports "rollback needs manual attention"
when its checked restoration/discard operations fail.
These are best-effort recovery steps, not a crash-safe transaction across files.
No transaction journal or multi-process lock coordinates the whole sequence.

## Verification and demonstration

All five registered backend CTest targets passed during this inspection.
[`checkout_test.cpp`](../../tests/checkout_test.cpp) checks cash settlement, change,
insufficient tender, repeat-payment rejection, and saved order/bill reload.
It does not exercise the Qt stock orchestration or simulated disk/rollback failures.

For a later GUI rehearsal, use a disposable copy of the starter data. Two units of
barcode `40001029` should total PKR 660; tender PKR 700 returns PKR 40 change and
successful checkout reduces that product's stock from 42 to 40.
Inspect all four files afterward; preserve the original fixture for repeatable demos.
