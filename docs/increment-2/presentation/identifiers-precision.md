# Identifier and precision examples

Inspection began at `1b16a70`; final verification used revision `883425c`.
Backend CTest passed 5/5 and the Qt GUI built with `CAMERA=OFF`.
Interactive GUI/camera operation remains untested. These source-inspected examples
were not executed as additional tests.

## Order versus bill identifiers

| Identifier | Layout | Example for 19 November 2026 |
| --- | --- | --- |
| First order | `YYMM` + daily sequence `01` + `DD` | `26110119` |
| Second order | `YYMM` + daily sequence `02` + `DD` | `26110219` |
| Last permitted order sequence | `YYMM` + `99` + `DD` | `26119919` |
| First bill | `YYMMDD` + daily sequence `001` | `261119001` |
| Second bill | `YYMMDD` + daily sequence `002` | `261119002` |
| Last permitted bill sequence | `YYMMDD` + `999` | `261119999` |

The next sequence is one above the highest matching persisted ID, not the number
of records plus one. Gaps are not filled. Orders permit 99 sequences per day;
bills permit 999. Reaching either maximum produces an error.
`nextOrderId()` previews an ID without reserving it. Deleting the highest order
can allow its sequence to be reused because allocation only inspects current data.

[Order allocation](../../../Backend/src/order/OrderManager.cpp) and
[bill allocation](../../../Backend/src/billing/BillRepository.cpp) use local
dates. The format checks are not complete calendar validation, and there is no
lock preventing concurrent callers from proposing the same next ID.

## Monetary arithmetic

The [calculator](../../../Backend/src/billing/BillCalculator.cpp) accumulates
subtotal, tax and discount separately, rounds each to two decimals, then rounds
`subtotal + tax - discount`. Tax/discount inputs are PKR per unit.

| Example | Expected result/explanation |
| --- | --- |
| Two units: price 250, tax 12.50, discount 10 | Subtotal 500, tax 25, discount 20, payable 505 |
| Two lines each priced 0.004, quantity 1, no tax/discount | Aggregate subtotal 0.008 rounds to 0.01; no per-line rounding first |
| Ordinary stored money | Two decimal places in bill/order records |
| Fractional quantities | Three decimal places, trailing zeros removed |

Floating-point `double` and `std::round` are used. They do not provide integer
minor-unit accounting; binary representation affects exact decimal boundaries.
See [RecordText helpers](../../../Backend/src/common/RecordText.cpp).

## Source-inspected precision gaps

**Tiny weighted quantity:** `0.0001` is finite and positive, so a backend weight
cart can accept it. Order writing formats it to three decimals as `0`; reloading
then rejects the nonpositive detail quantity. No minimum precision is enforced.
Compare quantity validation and `formatQuantity` in
[OrderManager.cpp](../../../Backend/src/order/OrderManager.cpp).

**Large finite values:** a price of `1e308` and quantity 2 pass the calculator's
individual finite checks, but multiplication exceeds finite `double` range.
There is no post-calculation finite check, so the input checks alone do not
guarantee finite totals. This is an unexecuted boundary finding, not a test result.

**Historical reconstruction:** bill rows store aggregate tax but omit each
line's tax and the original full product name. Reloaded display data can combine
saved amounts with current catalogue metadata; see [record layouts](../records.md).
