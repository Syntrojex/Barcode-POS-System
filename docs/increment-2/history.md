# Orders, bills, and dashboard

This guide describes the inspected Increment 2 implementation.
GUI behavior still needs a successful frontend build and interactive verification.
See the [presentation walkthrough](demo.md) for the build limitation and demo order.

## Orders and bills

- Orders loads persisted records through `AppState::refreshOrders()`.
- Rows are sorted newest first, with customer, contact, items, payment, and status.
- Search matches order ID, customer name, or contact; a leading `#` is ignored.
- Tabs filter All, Paid, In Progress, and On Hold.
- View details opens an item table; Receipt opens the text receipt viewer.
- There is no connected hold, resume, archive, or refund action in these screens.
- Bills lists paid orders only when an actual saved bill ID is available.
- Bill search accepts bill ID, order ID, or customer name.

The active cart is an in-memory draft. Clearing it does not delete saved history.
Closing the application loses that draft after the exit confirmation.
New Draft and successful checkout retain the customer and contact fields.
Check those fields before starting the next customer's order.

## Receipt persistence

Receipt totals come from the saved bill when one exists.
Saved bill lines retain unit prices and discounts for historical receipts.
The viewer shows item quantities, subtotal, discount, tax, total, and payment mode.
An immediate cash receipt also shows tendered cash and change.

The receipt dialog is a text preview with a Close button; it has no print or PDF action.
Cash tendered/change are not restored when history is reloaded from disk.
Custom item descriptions are available during checkout, but after reload their saved
acronyms are used as names. Catalogue names are obtained from the current catalogue.

## Dashboard

Sales Today sums paid orders dated today; Orders counts all saved orders dated today.
The average divides today's paid revenue by today's number of paid orders.
Recent Orders shows the latest five saved orders across all dates.
Low Stock counts products at or below the configured threshold, including zero stock.
Hourly revenue groups today's paid orders from 08:00 through 20:00 inclusive.
Revenue outside those hours remains in Sales Today but is absent from that chart.
Pending currently counts every non-Paid status, so Archived is included too.
Daily totals are cached on load/checkout; there is no midnight refresh timer.

## CSV export

Orders exports all loaded orders, regardless of the current search or status tab.
Columns are order ID, time, customer, contact, items, payment, amount, and status.
Embedded quotation marks in text fields are not escaped, so such names can break CSV.
The export helper reports open failure silently and does not verify final write status.

## Source map

- [Orders and Bills screens](../../Frontend/src/ui/OtherPages.cpp)
- [History loading and totals](../../Frontend/src/ui/AppState.cpp)
- [Dashboard aggregation](../../Frontend/src/ui/DashboardPage.cpp)
- [Receipt text and viewer](../../Frontend/src/ui/Dialogs.cpp)
