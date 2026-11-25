# Syntro-Check UI redesign checkpoint

Task 01 is complete on branch `ali`. The shared Qt Widgets theme now uses primary blue `#2554D8`, navy `#172443`, and a light canvas `#F5F7FB`. `Theme.h` and `Theme.cpp` define flat surfaces, neutral borders, consistent control states, and semantic light and dark colors. Existing widget object names and dynamic properties still drive the stylesheet.

## Verification

- Baseline and updated builds passed with `cmake --preset qt-mingw` and `cmake --build --preset qt-mingw-debug --parallel 4` using Qt 6.11.0 with MinGW.
- All six checks passed with `ctest --test-dir build/qt-mingw -C Debug --output-on-failure --no-tests=error` using the Qt and MinGW runtime directories on `PATH`.
- `BarcodeHub.exe` opened its Windows sign-in dialog and exited normally. A temporary harness rendered Dashboard, Record Order, Inventory, and Reports in both themes and checked the theme toggle, input focus, and disabled button styling.
- CMake reported missing optional Vulkan headers and unused Qt Creator preset variables during the baseline configure.

## Follow-up context

- Custom-painted cards and charts, icon tiles, and inline widget styles still contain legacy visuals. The hard-coded dark sign-in text and some chart labels still have low contrast in dark mode.
- Continue from the existing `UiKit`, `Widgets`, and 11 pages, including `ReportsPage`, when a later task authorizes wider edits.
- The pre-existing untracked `Barcode-POS-System.txt` remains in the working tree.

## Task 02 · Application shell and navigation

Completed on `ali`. The navy sidebar groups the existing pages into Sales, Catalog, and Operations. The top bar follows the active page and includes a neutral barcode-input status, clock, date, theme control, and keyboard-accessible account menu. Role restrictions, page IDs, and shortcut mappings were preserved.

- Changed `MainWindow.h`, `MainWindow.cpp`, `Theme.cpp` (shell selectors), and this checkpoint.
- `cmake --preset qt-mingw` and `cmake --build --preset qt-mingw-debug --parallel 4` passed. The link succeeded on retry after a transient permission error on `BarcodeHub.exe`. All six CTest checks passed. The Windows executable opened and closed its sign-in window normally.
- A temporary Windows Qt Test harness checked cashier and manager navigation, permissions, active state, and title updates at 1100×700 and 1440×900 in light and dark mode. It exercised F1, Ctrl+K, Ctrl+N, the mapped Ctrl+1…0 shortcuts, checkout F10, Alt+D, Tab and Space navigation, the theme button, and the account menu and logout cancellation.
- The checkout barcode placeholder and detected-product copy crowd at minimum width. Custom-painted cards and charts, as well as the dark sign-in contrast issue from Task 01, remain.
- Run `git log -1 --format=%H --grep='^feat(ui): redesign application shell and navigation$'` for the full Task 02 commit hash.

**Next task** · Checkout UI redesign.

## Increment 2 acceptance

- [x] The Qt MinGW Debug build passed. The final Windows executable opened and closed the sign-in dialog successfully.
- [x] The sidebar showed seven permitted pages for Cashier and eleven for Manager. The UI check confirmed that direct attempts to open restricted pages were rejected.
- [x] Each permitted page opened from its sidebar button and showed the matching title and active state.
- [x] F1, Ctrl+K, Ctrl+N, and every mapped Ctrl+1 through Ctrl+0 shortcut worked. The check also exercised checkout F10, Alt+D, Tab and Space navigation, and logout cancellation.
- [x] Light and dark themes were checked for both roles at 1100×700 and 1440×900. Theme checks included sign-in, the account menu, date popups, checkout payment, and receipts. The theme preference persisted.
- [x] At 1100×700, populated checkout controls, category filters, the discounts form, and report date filters fit or could be scrolled fully into view. Wide tables expose their action columns through horizontal scrolling.
- [x] The local commit `fix(ui): meet increment 2 acceptance checklist` records the acceptance fixes, the preceding dashboard and theme upgrade, and this progress log.

All six CTest checks passed. Cash checkout in both themes used a temporary copy of the catalog. Typed barcode lookup found the product, cash payment created the paid order, order detail, and bill records, and stock decreased by one. Scanner decoding was covered by CTest.
