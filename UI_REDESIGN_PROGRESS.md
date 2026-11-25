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
