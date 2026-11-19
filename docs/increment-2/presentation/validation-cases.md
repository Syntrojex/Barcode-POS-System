# Validation cases across layers

Inspection began at `1b16a70`; final verification used revision `883425c`.
Backend suites passed 5/5 and the Qt GUI built with `CAMERA=OFF`.
Interactive GUI/camera operation remains untested. Rows below are source-inspected
examples, not newly executed test results; consult linked assertions for tested cases.

## Catalogue loading versus ordering

| Input/example | Catalogue rule | Order/cart rule | Practical consequence |
| --- | --- | --- | --- |
| Barcode `40001029` | Eight digits and valid checksum | Snapshot needs nonempty barcode without whitespace | Known fixture can be ordered |
| Eight digits with wrong checksum | Warning; row retained | No checksum verification in cart | A warning does not prohibit a sale |
| Seven-digit barcode | Row rejected | Backend snapshot alone does not enforce EAN-8 length | Use the catalogue loader before ordering |
| Duplicate product ID or barcode | Later row rejected | Cart merges by product ID | ID is the backend cart identity |
| Duplicate acronym | Not rejected | Backend IDs remain distinct | GUI acronym-based merging is a separate risk |
| Acronym length 9 | Default rule allows up to 10 | No acronym-length check in cart | Increment plan's limit of 8 differs |
| Price `nan` | `strtod` and sign checks can allow it | Nonfinite price rejected | Later validation adds protection |
| Price 100, tax 10, discount 105 | Catalogue rejects discount above price | Snapshot permits discount up to price + tax | Layers have different contracts |
| Empty barcode image path | Rejected | Not represented in snapshot | Cart cannot validate an image path |

Evidence: [product rules](../../../Backend/src/product/ProductValidation.cpp),
[loader](../../../Backend/src/product/ProductStorage.cpp),
[cart validation](../../../Backend/src/order/OrderManager.cpp), and
[GUI cart](../../../Frontend/src/ui/RecordOrderPage.cpp).

## Quantity examples

| Example | Expected source behavior | Existing test reference |
| --- | --- | --- |
| Count product, quantity 1.5 | Reject: whole number required | [order smoke test](../../../tests/OrderManagementSmokeTest.cpp) |
| Weight product, quantity 1.5 | Backend cart accepts within stock | Same order smoke test |
| Stock 5, add 2 then 4 | Reject combined quantity 6; retain previous line | Same order smoke test |
| Quantity zero/negative | Reject before cart update | Same order smoke test |
| Product stock zero, request 1 | Reject for insufficient stock | [product tests](../../../tests/product_tests.cpp) |
| Weight quantity 0.0001 | Positive input passes backend validation; stored quantity rounds to zero | Source inspection only; precision gap |

Quantity records use three decimals; see [precision examples](identifiers-precision.md).
Successful backend acceptance of a weight fraction does not imply UI support.

## File validation and error scope

| File/load | Example problem | Scope of failure |
| --- | --- | --- |
| Product catalogue | One malformed row among valid rows | Reject that row; keep accepted rows and issues |
| `ProductManager::loadProducts` | Mixed valid/rejected rows | Return false but replace memory with accepted rows |
| Order file | Duplicate order ID, bad enum or missing `#` | Fail the load with line-specific error |
| Detail file | Nonpositive quantity or malformed identity | Fail the load |
| Bill file | Bad nine-digit bill ID or nonfinite number | Fail the load |
| Bill file | Negative finite quantity or inconsistent repeated totals | Parser/grouping lacks full business validation |

See [order record validation](../../../Backend/src/order/Order.cpp),
[detail validation](../../../Backend/src/order/OrderDetail.cpp),
[bill parsing](../../../Backend/src/billing/BillRecordCodec.cpp) and
[bill grouping](../../../Backend/src/billing/BillRepository.cpp).
Do not equate syntactically valid stored text with a valid business transaction.

## Status validation is not a complete workflow state machine

The following are source expectations, not new executed tests. An updated
record must first pass Order::isValid; updateOrder then adds the restriction
that an archived order cannot reopen.

| Starting state | Requested operation | Current backend boundary |
| --- | --- | --- |
| In-progress | Update to on-hold | Allowed for an otherwise valid record |
| Paid | Update to in-progress | Not prohibited by updateOrder; no refund is generated |
| Archived | Update to in-progress | Rejected |
| Archived | Update while staying archived | Allowed for an otherwise valid record |
| On-hold | Billing checkout | Eligible; normal payment/persistence rules still apply |
| Paid | Billing checkout again | Rejected as already paid |

The GUI does not expose all these mutations. An enum listing four states does
not establish a persisted hold/resume/refund interface.
