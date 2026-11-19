# GUI viva: questions and evidence

Initial source review: `ali` at `1b16a70`, 19 November 2026; rechecked at `883425c`.
The GUI build with `CAMERA=OFF` now passes after the header repair in `883425c`.
These answers describe implementation; interactive GUI and camera checks are unexecuted.

## What does the cashier interface actually do?

It provides product lookup, a cart, quantity controls, customer fields, payment
selection, receipt preview, and views of saved orders and bills. `MainWindow`
connects pages; `AppState` translates between Qt data and the C++ backend.
Start with [MainWindow](../../../Frontend/src/ui/MainWindow.cpp) and
[AppState](../../../Frontend/src/ui/AppState.cpp) when explaining integration.

## Is the displayed cashier a logged-in user?

No authentication flow is present in the active application. The sidebar text
`Alex Rivera`, `Cashier`, and `Terminal 04` is hard-coded display content.
The initial draft also contains `Marcus Vance` and a sample telephone number.
Replace customer fields before a real demonstration transaction.
See [MainWindow](../../../Frontend/src/ui/MainWindow.cpp) and
[RecordOrderPage](../../../Frontend/src/ui/RecordOrderPage.cpp).

## Why does an order's shift text change after restart?

Checkout sets the display shift to `POS #04 • Shift A`; history reconstruction
sets `POS #01` and the customer tier to `Guest`. Neither shift nor tier is stored
in `Order.txt`. These labels are UI metadata, not audited cashier identities.
The persisted fields are explained in [records](../records.md).

## Does clicking Products Add always increase the quantity?

The button sends the barcode to Record Order through the scan-style entry path.
With auto-add disabled, the product is previewed and the cashier must use
`Add to order`. With auto-add enabled, one unit is added after lookup succeeds.
This differs from typing a barcode, which previews it regardless of auto-add.
See `showCode()` in [RecordOrderPage](../../../Frontend/src/ui/RecordOrderPage.cpp).

## Are card and wallet payments connected to a bank?

They validate merchant text locally and record the selected payment mode.
The application has no gateway authorization, settlement, or refund operation.
Cash shows tender/change; all three modes still pass backend payment validation.
See [payment dialog](../../../Frontend/src/ui/Dialogs.cpp) and [payments](../payments.md).

## Can the receipt be printed or downloaded as PDF?

The current receipt is formatted text in a modal viewer with a Close button.
There is no print/PDF action in that viewer. Orders offers a separate CSV export;
CSV is a transaction table rather than a receipt rendering.

## What can I claim about testing?

Five backend CTest suites passed in the current inspection. They do not execute
the Qt widgets or `AppState::addOrder()`. The GUI build at `883425c` passes with
`CAMERA=OFF`; interactive checkout and a camera trial remain separate acceptance steps.
Use [verification](../verification.md) for the earlier recorded test coverage
and [screen rehearsal](screen-rehearsal.md) for pending GUI checks.

## How is a custom item different from a catalogue product?

The custom-item dialog supplies a description and positive unit price. The
draft gives it a generated MAN-prefixed acronym, count unit and zero tax/
discount. It has no catalogue product ID and does not reduce catalogue stock.
AppState builds a temporary CUSTOM-prefixed snapshot for this checkout so
billing can still resolve the line and persist its amount.

The full custom description is session metadata. After restart, a historical
custom line can fall back to the saved acronym because the bill schema does
not store a complete product name. A manual line therefore demonstrates billing
flexibility, rather than catalogue creation or inventory replenishment.
Evidence: [custom dialog](../../../Frontend/src/ui/Dialogs.cpp),
[draft conversion](../../../Frontend/src/ui/RecordOrderPage.cpp), and
[AppState snapshots](../../../Frontend/src/ui/AppState.cpp).
