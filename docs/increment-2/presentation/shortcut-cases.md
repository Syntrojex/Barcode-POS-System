# Keyboard cases for presentation rehearsal

Initial review used `1b16a70`; the GUI build with `CAMERA=OFF` passes at `883425c`.
All cases below are source-derived expectations; GUI and camera trials are unexecuted.
Test with no modal dialog open, as focus can affect delivery. Use a disposable data copy.

## Context changes the action

| Starting context | Key | Expected action | Rehearsal detail |
| --- | --- | --- | --- |
| Any main page | F1 | Open Dashboard | Existing cart is retained |
| Any main page | F2 | Open Record Order and focus input | Does not create a new draft |
| Any main page | F3 | Open Orders | Existing search/filter can remain |
| Record Order | F4 | Open Browse products chooser | Choosing an item previews it; no auto-add through this manual path |
| Any other page | F4 | Open Products | Sidebar label describes this general destination |
| Any main page | F5 | Open Barcode Scanner | Does not itself start capture |
| Any main page | F6 | Open Bills | Only paid orders with real bill IDs are listed |
| Record Order | F9 | Start its camera if stopped | Despite method name simulateScan, this starts capture |
| Barcode Scanner | F9 | Navigate to Record Order and start that camera | Does not invoke the Scanner screen's Start Scan button |
| Other main page | F9 | Navigate to Record Order and start its camera | Stop any other capture first |
| Record Order, empty cart | F10 | Call checkout and show empty-order toast | It does not open Settings here |
| Record Order, populated cart | F10 | Open payment dialog | Cancelling preserves the draft |
| Any other page | F10 | Open Settings | No checkout occurs through this key in that context |
| Any main page | Ctrl+K | Open Dashboard and focus lookup | Lookup resolves a product code first, otherwise searches Orders |
| Any main page | Ctrl+N | Open Record Order and clear draft | Customer/contact fields are retained |

## Suggested three-minute keyboard rehearsal

1. Put one milk unit in a draft using manual entry and Add to order.
2. Press F1, then F2; verify the draft survived and input has focus.
3. Press F4; select bread; verify preview changes before explicitly adding.
4. Press F10; cancel; verify neither draft contents nor history changed.
5. Press F3, then F10; verify Settings opens rather than payment.
6. Return with F2; check the customer fields; use Ctrl+N on the disposable draft.
7. Test F9 separately after confirming only one camera page is active.

## Which source defines these shortcuts?

`MainWindow` creates QShortcut objects and routes them through `handleFKey()`.
Its local navigation table builds the sidebar. This is the active authority for
the context-dependent F4/F10/F9 behavior described above.

`NavigationModel::items()` also lists page shortcuts, but no consumer is present
in the active frontend sources. Updating that list alone would not change the
MainWindow routing. The RecordOrderPage header's F4 Description comment also
differs from `quickLookup()`, which opens Browse products in the implementation.
Use visible Details/Description buttons when demonstrating product descriptions.

Sources: [MainWindow routing](../../../Frontend/src/ui/MainWindow.cpp),
[unused navigation list](../../../Frontend/src/ui/NavigationModel.cpp),
[Record Order handlers](../../../Frontend/src/ui/RecordOrderPage.cpp),
and [existing settings guide](../settings.md).
