# Diagnosing empty or unexpected history

Initial review used `1b16a70`; final source check is `883425c`.
The GUI build with `CAMERA=OFF` passes; GUI and camera interactions remain unexecuted.
Use copied data when reproducing malformed records or reload differences.

## Empty Orders or Bills does not establish that files are empty

`AppState::refreshOrders()` clears the cached order list and today's counters,
then loads `Order.txt`, `OrderDetail.txt`, and `Bill.txt`. If any load fails, it
returns before rebuilding history and writes a `qWarning` message:

```text
Could not load persisted orders/bills: <loader error>
```

There is no connected GUI history-error banner in this path. A malformed bill
can therefore hide otherwise valid orders in the current cached view. Consult
the application's captured warning output and validate all three record files.
Do not delete transaction files to make the screen appear populated.

## Check the selected directory first

`BARCODEHUB_DATA` is a preferred candidate only when it contains `Product.txt`.
Otherwise the application searches executable-relative, working-directory,
and source-directory candidates. A valid but unintended runtime data copy can
explain why the history differs from the repository's `data` folder.
See [build and run](../build.md) for candidate ordering.

## Distinguish Orders from Bills filtering

Orders applies the selected status tab and its text search. Clear the search
and choose All when checking whether a loaded order exists. Bills requires both
`Paid` status and a nonempty ID from an actual saved bill. A paid order without
a corresponding loaded bill will not appear there.

## Keep detail order aligned with saved bill lines

History finds a bill by order ID, then uses the matching order-detail row index
to select `bill.lines[detailIndex]`. It does not join those lines by product ID
or verify the saved acronym against each detail. Manually reordering details
can attach the wrong historical unit price/discount to a displayed product.
Keep the original files together when comparing a restored transaction.

Aggregate subtotal, discount, tax and payable are overwritten from the saved
bill when present. A correct aggregate total alone does not prove every
displayed product line was reconstructed correctly.

## Explain differences between views

Order Details shows each product's `unit price × quantity` as SUBTOTAL.
Its bottom total also reflects tax and discounts, so those numbers need not sum
directly without the adjustments. Receipt text exposes the aggregate adjustments.
Historical catalogue names come from the current catalogue; custom descriptions
fall back to saved acronyms after restart. Cash tender/change are session data.

Sources: [history loader](../../../Frontend/src/ui/AppState.cpp),
[Orders/Bills filtering](../../../Frontend/src/ui/OtherPages.cpp),
[detail and receipt views](../../../Frontend/src/ui/Dialogs.cpp),
and [record format reference](../records.md).
