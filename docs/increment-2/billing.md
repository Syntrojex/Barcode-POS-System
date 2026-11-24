# Billing calculations and service

Billing separates domain data, arithmetic, payment rules, serialization, storage,
and checkout coordination. Its backend uses standard C++17 without Qt headers.

## Responsibilities

| Component | Responsibility |
| --- | --- |
| [`Bill`](../../Backend/include/billing/Bill.h) | Header, item lines, and aggregate totals. |
| [`BillCalculator`](../../Backend/src/billing/BillCalculator.cpp) | Arithmetic and input checks. |
| [`BillingService`](../../Backend/src/billing/BillingService.cpp) | Product lookup, payment, status, and save sequence. |
| [`BillRepository`](../../Backend/src/billing/BillRepository.cpp) | Load, group, append, and allocate bill IDs. |
| [`BillRecordCodec`](../../Backend/src/billing/BillRecordCodec.cpp) | Translate one item row to/from text. |

## Calculation rules

Tax and discount are PKR amounts per unit, not percentages.

```text
withoutTax = round(sum(unitPrice * quantity), 2)
tax        = round(sum(taxPerUnit * quantity), 2)
discount   = round(sum(discountPerUnit * quantity), 2)
payable    = round(withoutTax + tax - discount, 2)
```

Components are accumulated before rounding; they are not rounded per item first.
The implementation uses `double` and `std::round`, rather than integer minor units.
It rejects empty bills, nonfinite or negative price/tax/discount inputs,
nonpositive quantities, and a negative resulting payable amount.

For two units priced at PKR 250, with PKR 12.50 tax and PKR 10 discount per unit:

| Component | Calculation | PKR |
| --- | --- | ---: |
| Subtotal | 250 x 2 | 500.00 |
| Tax | 12.50 x 2 | 25.00 |
| Discount | 10 x 2 | 20.00 |
| Payable | 500 + 25 - 20 | 505.00 |

## Preview and saved bills

`preview(orderId)` loads order details and resolves current product metadata
through an injected `ProductLookup`; it does not save a bill or assign a bill ID.
Checkout adds payment validation and persistence, rejecting paid/archived orders.
Bill IDs use `YYMMDD` + sequence `001..999`, for example `261119001`.
The daily sequence is selected from existing bill IDs using the local date.

Saved rows retain item price/discount/quantity and aggregate tax; per-item tax is
not stored. Rebuilding a historical preview from current product data can therefore
differ from the saved bill. A zero-total bill passes the calculator but cannot be
marked paid because order validation requires a positive payment amount.

[`billing_tests.cpp`](../../tests/billing_tests.cpp) contains 19 cases, while
[`checkout_test.cpp`](../../tests/checkout_test.cpp) checks cash checkout end to end.
All five registered backend CTest targets passed during this inspection.
See [payments](payments.md), [records](records.md), and [checkout](checkout.md).
