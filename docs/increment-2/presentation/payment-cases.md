# Worked payment acceptance cases

Inspection began at `1b16a70`; final verification used revision `883425c`.
Backend CTest passed 5/5 and the Qt GUI built with `CAMERA=OFF`.
Interactive GUI/camera operation remains untested. Cases below describe source
behavior, not newly executed payment transactions.

## Cash example from the actual catalogue

[Milk `40001029`](../../../data/Product.txt) costs PKR 340 with zero tax and
PKR 10 discount per unit. Two units total `680 + 0 - 20 = PKR 660`.
This differs from the synthetic PKR 505 example in the backend checkout test.

| Tender/request | Expected local result for payable 660 | Reason |
| --- | --- | --- |
| Cash 660 | Accept; change 0 | Exact amount |
| Cash 700 | Accept; change 40 | Tender exceeds payable |
| Cash 659 | Reject | Insufficient tender |
| Cash -1 | Reject | Negative tender |
| Cash infinity/NaN | Reject | Tender must be finite |
| Cash 0 | Reject for this bill | Zero is below payable, although not intrinsically invalid tender |
| Card, merchant `HBL` | Accept local payment rule | Nonblank name without `#` |
| Wallet, merchant `JazzCash` | Accept local payment rule | Same merchant validation |
| Card/wallet, blank or spaces | Reject | Merchant name required |
| Card/wallet, merchant `Bank#1` | Reject | Delimiter prohibited |

[PaymentMethod.cpp](../../../Backend/src/billing/PaymentMethod.cpp) supplies
these rules. Card/wallet ignore cash tender and do not authorize or settle with
an external provider. A locally accepted rule can still be followed by a file error.

## Zero-total boundary: rule success versus checkout failure

Consider one item with price 100, tax 0, discount 100, quantity 1:

| Stage | Expected source behavior |
| --- | --- |
| Bill calculation | Accepts payable 0; only a negative total is rejected |
| Cash rule with tender 0 | Accepts and returns change 0 |
| Card/wallet with valid merchant | Accepts merchant text |
| Mark order paid with paymentAmount 0 | Rejects: paid orders require a positive amount |
| Full checkout | Fails before saving a finalized bill |

This boundary is source-inspected, not a newly run zero-total test.
Evidence: [calculator](../../../Backend/src/billing/BillCalculator.cpp),
[payment rules](../../../Backend/src/billing/PaymentMethod.cpp),
[paid-order validation](../../../Backend/src/order/Order.cpp), and
[service sequence](../../../Backend/src/billing/BillingService.cpp).

## State and persistence boundaries

`BillingService::checkout` rejects paid and archived orders before payment;
an on-hold order is eligible. A bill save failure triggers an attempt to restore
the previous order state. [Qt checkout](../../../Frontend/src/ui/AppState.cpp)
additionally orchestrates stock restoration and removal of the new unpaid order.

[checkout_test.cpp](../../../tests/checkout_test.cpp) executes a synthetic
two-unit bill totaling 505: tender 500 fails, tender 510 returns 5, and repeat
checkout fails. [billing_tests.cpp](../../../tests/billing_tests.cpp) covers
cash, merchant validation and default strategies. Neither proves bank settlement.

## Explain the three cash amounts separately

For the milk-and-bread rehearsal, payable is PKR 938. If the customer hands over
PKR 1,000, change is PKR 62. The paid order records paymentAmount as 938, not
1,000. The bill repeats the payable total; neither file stores tender or change.

| Value | Current-session receipt | Restored transaction |
| --- | --- | --- |
| Amount payable | 938 | Saved as 938 |
| Cash received | 1,000 | Not persisted |
| Change | 62 | Cannot be recovered uniquely from saved totals |

This explains why a historical receipt can show the sale total but cannot
prove how much cash was handed over. See the assignment to
paidOrder.paymentAmount in [BillingService](../../../Backend/src/billing/BillingService.cpp).
