# Order management

The order module owns cart rules, order identities, and order/detail persistence.
It is standard C++17 and does not depend on Qt or the product implementation.

## Cart contract

[`ProductSnapshot`](../../Backend/include/order/OrderManager.h) carries the product
ID, barcode, description, unit, pricing, and available quantity into this module.
[`ProductAdapter`](../../Backend/src/integration/ProductAdapter.cpp) builds that
snapshot from a catalog product.

- Adding the same product ID combines its quantity with the existing cart line.
- Quantities must be finite, positive, and within the snapshot's available stock.
- `count` requires whole quantities; `weight` permits fractions in this backend.
- Price, tax, and discount must be finite and nonnegative.
- Discount per unit cannot exceed price plus tax.
- Removing a line and clearing the cart are separate operations.

The active GUI uses integer quantities. Backend support for weighted fractions
does not imply fractional checkout support throughout the application.

## Saving an order

[`createOrder`](../../Backend/src/order/OrderManager.cpp) rejects an empty cart
or empty customer name, loads existing records, and allocates an ID at save time.
It writes `OrderDetail.txt` before `Order.txt`, attempts to restore the previous
details if the header write fails, and clears the cart only after success.
Creating an order alone neither charges a payment nor deducts product stock.

Order IDs have eight digits: `YYMM` + sequence `01..99` + `DD`.
For example, `26110119` is the first order on 19 November 2026.
The sequence is one above the highest existing ID for that date; 99 is the limit.
`nextOrderId()` is only a preview, not a reservation.

## Status and update rules

| Status | Behavior |
| --- | --- |
| `in-progress` | Initial persisted state; eligible for discard after failed checkout. |
| `on-hold` | Recognized persisted state; billing can settle it. |
| `paid` | Requires a positive payment amount; billing rejects another checkout. |
| `archived` | Cannot be reopened through `updateOrder`; billing rejects checkout. |

`updateOrder` preserves the original creation date and time. Beyond the archived
restriction and record validation, it is not a complete transition state machine.
Status support in the backend does not establish a working GUI action for each state.

## Verification and limits

[`OrderManagementSmokeTest`](../../tests/OrderManagementSmokeTest.cpp) covers cart
validation, totals, IDs, escaping, persistence, updates, and malformed records.
The inspection run passed all five registered backend CTest targets.
Order dates/times are shape-checked; calendar validity is not fully checked.
Multi-file recovery is best effort; see [checkout](checkout.md) and [records](records.md).
