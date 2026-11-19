# Verification record - 19 November 2026

**Follow-up:** the original results below are historical. The separate header
repair at `883425c` resolves the reported compilation errors; a camera-disabled
Qt rebuild now passes. The backend rerun also passed 5/5 in 2.20 seconds.
See [follow-up results](presentation/review-results.md) for the current state.

Baseline: branch `ali`, commit `2d3c58d`. Review changes are Markdown documentation
only; executable source, tests, build configuration and data remain unchanged.

## Executed checks

| Check | Observed result |
| --- | --- |
| Fetch and fast-forward pull of `origin/ali` | Already up to date |
| Compare baseline with fetched `origin/main` | 17 commits ahead, 0 behind |
| Fresh backend Debug configure/build | Passed with GNU 13.1.0 and Ninja on Windows |
| Registered CTest suites | 5/5 passed; 4.22 seconds total |
| Fresh Qt 6.11.0 configure, camera disabled | Passed with Qt's matching MinGW 13.1 compiler |
| Qt frontend compilation | Failed: undeclared members in `RecordOrderPage.cpp` |
| Catalogue fixture inspection | 14 records, 14 valid EAN-8 checksums, 14 matching PNG files |

Exact configure/build/test commands are in [build.md](build.md).

## What the passing tests establish

| CTest name | Source and coverage |
| --- | --- |
| `product` | [product_tests.cpp](../../tests/product_tests.cpp): parsing, validation, lookup, stock, fixture loading and persistence checks |
| `order` | [OrderManagementSmokeTest.cpp](../../tests/OrderManagementSmokeTest.cpp): quantities, totals, IDs, record encoding, states and malformed records |
| `integration` | [integration_test.cpp](../../tests/integration_test.cpp): typed barcode lookup, product adapter, cart, order files and in-memory stock deduction |
| `checkout` | [checkout_test.cpp](../../tests/checkout_test.cpp): bill preview, insufficient cash, change, duplicate payment rejection and stored order/bill |
| `billing` | [billing_tests.cpp](../../tests/billing_tests.cpp): 19 cases for arithmetic, payment methods, codec and repository |

Integration and product tests depend on the shipped 14-item catalogue. Order,
integration, checkout and billing tests use temporary transaction directories.
No application checkout was executed against the repository's data files.

## Build failure evidence

The compiler reported at `Frontend/src/ui/RecordOrderPage.cpp:61`:

```text
error: 'm_feed' was not declared in this scope
```

Additional errors include `m_customer`, `m_contact`, `m_manual`, `m_sessionId`,
`m_checkout` and total-label members. The corresponding declarations in
[RecordOrderPage.h](../../Frontend/include/ui/RecordOrderPage.h) use stale names.
This failure exists before the documentation changes and was not repaired.

## Not verified

Live GUI interaction, hardware camera capture, image decoding quality, portable
deployment, receipt printing, power-loss recovery and concurrent application
instances were not tested. Backend CTest does not execute `AppState::addOrder`
or the Qt widgets. `tests/regression.cpp` and `CameraDemo` are not registered
CTest suites; their presence is not evidence of a passing current GUI test.
