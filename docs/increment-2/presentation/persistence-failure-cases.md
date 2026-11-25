# Persistence failure walkthrough

Inspection began at `1b16a70`; final verification used revision `883425c`.
Backend CTest passed 5/5 and the Qt GUI built with `CAMERA=OFF`.
Interactive GUI/camera operation remains untested; failures were not injected.
Use these source-inspected cases to explain recovery, not to claim tested crash
or disk-failure guarantees.

## Which component owns each stage?

| Stage/failure | Attempted behavior | Source |
| --- | --- | --- |
| Validate cart/tender before writes | Return error before creating an order | [AppState::addOrder](../../../Frontend/src/ui/AppState.cpp) |
| Write new order details fails | Return failure; order header is not written | [OrderManager::createOrder](../../../Backend/src/order/OrderManager.cpp) |
| Details saved, order header fails | Attempt to restore old detail rows | Same order manager |
| Original product file cannot be opened for backup | Attempt to discard new in-progress order | AppState |
| Deducted stock cannot be saved | Restore original product bytes; discard new order | AppState |
| Bill history cannot load / ID allocation fails | Return billing failure before marking paid | [BillingService](../../../Backend/src/billing/BillingService.cpp) |
| Marking order paid fails | Do not append bill | BillingService |
| Bill append fails after order marked paid | Attempt to restore previous order state | BillingService |
| Any billing failure reaches AppState | Restore product bytes; attempt to discard new order | AppState |

Creating an order and settling it are separate backend operations. Stock rollback
belongs to AppState, not BillingService. A backend-only caller must supply its
own inventory orchestration if it needs equivalent behavior.

## Replacement and rollback limits

- Order/detail and bill writers stage `<file>.tmp`, then rename it into place.
- When rename-over-existing fails, they fall back to `copy_file(...overwrite_existing)`.
- Product saving uses a similar temporary-file replacement strategy.
- Qt stock restoration uses `QSaveFile` to replace the backed-up product bytes.
- Some rollback return values are ignored; recovery itself may fail.
- Checked Qt restoration/discard failures add `rollback needs manual attention`
  to the reported checkout error.

Evidence: [order writers](../../../Backend/src/order/OrderManager.cpp),
[bill writer](../../../Backend/src/billing/BillRepository.cpp),
[product writer](../../../Backend/src/product/ProductStorage.cpp) and
[Qt restoration](../../../Frontend/src/ui/AppState.cpp).

## Questions to answer during a presentation

**Can a power loss leave stock/order/bill inconsistent?** Yes. Four files are
updated separately. There is no transaction journal, commit marker or process
lock coordinating all writes; successful temporary-file writes do not make the
whole checkout crash-safe.

**Can two running copies safely sell the final item?** The code has no shared
lock or reservation. Both can read old stock; concurrent checkout is unverified.

**What should be checked after a failed demo checkout?** In a disposable data
copy, compare all four files with the pre-demo backup. Check order/detail IDs,
paid status, corresponding bill rows and catalogue stock together. Preserve the
error and backup before any manual recovery; no automated repair tool is supplied.

**What has actually been tested?** Existing backend tests cover successful file
round trips and normal rejection paths. They do not simulate power loss, full
disks or rollback failures. See [verification](../verification.md).

## Preserve the relevant evidence when a read fails

| Reported stage | Files to compare with the same backup set | Reason |
| --- | --- | --- |
| Product lookup or stock save | Product.txt and referenced barcode path | Product metadata and image resolution are separate |
| Order lookup | Order.txt | Header identity, state and payment are stored here |
| Empty/missing order products | OrderDetail.txt together with Order.txt | Detail rows require the correct order identity |
| Bill parsing or history display | Bill.txt together with both order files | History combines all three record sources |
| Inconsistent inventory after failure | All four files | A checkout spans multiple independent replacements |

A single file from an older backup can disagree with the other three even if
every row parses. Match order IDs, bill order references and item quantities
before drawing conclusions from screen totals. No repair action was executed
as part of this review.
