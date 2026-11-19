# Payment modes

Payment handling records the cashier's selected mode and applies local validation.
Card and wallet support does not connect to a bank, terminal, or payment gateway.

## Strategy design

[`IPaymentMethod`](../../Backend/include/billing/PaymentMethod.h) defines `mode()`
and `process(request, payable)`. `PaymentMethodRegistry::withDefaults()` installs
cash, card, and wallet strategies; the service finds one using `PaymentMode`.

`PaymentRequest` contains a mode, merchant name, and cash amount tendered.
`PaymentResult` returns success, change, and an error message.
This separates payment rules from bill calculation and storage.
Adding a new mode also needs the enum/string mappings and any relevant UI changes;
implementing and registering a strategy alone does not add a new UI option.

## Current rules

| Mode | Required information | Behavior |
| --- | --- | --- |
| Cash | Finite, nonnegative tender covering payable | Returns change rounded to two decimals. |
| Card | Nonblank bank name without `#` | Records local acceptance; no remote authorization. |
| Wallet | Nonblank provider name without `#` | Records local acceptance; no remote authorization. |

For payable PKR 505 and tender PKR 510, cash returns PKR 5 change.
Tendering PKR 500 is rejected; negative or nonfinite tender is also rejected.
Electronic strategies ignore the tendered amount and return zero change.
Merchant validation rejects names made only of spaces, tabs, CR, or LF.

[`PaymentMethod.cpp`](../../Backend/src/billing/PaymentMethod.cpp) implements these
rules. Strategies assume the payable amount came from the billing calculation;
they do not each independently validate every possible payable input.

## GUI and persistence path

The payment dialog offers Cash, Card, and Wallet, showing either tender/change or
a bank/provider field. Cancel returns to the cart without committing checkout.
The dialog trims electronic merchant names before handing them to the backend.
These behaviors were inspected in [`Dialogs.cpp`](../../Frontend/src/ui/Dialogs.cpp);
they are not a claim of a live GUI test.

Successful billing writes the payable amount, mode, and merchant to `Order.txt`.
Cash saves an empty merchant. Tendered cash and change are not durable fields in
the order/bill schemas, so those values cannot be reconstructed from them alone.
`Bill.txt` stores the resulting payment status, not card or wallet credentials.

## Verification

[`billing_tests.cpp`](../../tests/billing_tests.cpp) checks exact cash, change,
insufficient/negative cash, valid and invalid card merchants, wallet acceptance,
and the default registry. The checkout test also verifies insufficient cash does
not complete payment and an already-paid order cannot be checked out again.
All five registered backend CTest targets passed during this inspection.
See [checkout](checkout.md) for write ordering and recovery limitations.
