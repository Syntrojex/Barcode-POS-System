# Changelog

All notable changes to **BarcodeHub** are documented in this file.

## [3.0.0] — Increment 3: Inventory, Discounts, Reporting & Advanced Billing

### Planned

- Add product category management through the GUI, stored in `PCategory.txt`.
- Use the first two digits of every barcode as the product category.
- Store the category ID of each product in `Product.txt`.
- Allow managers to generate barcodes and to add or edit products.
- Add an inventory view and inventory updates for managers.
- Update available product quantities with every order.
- Allow existing orders to be opened and edited; paid orders only within 7 days of payment and after manager approval.
- Reduce the clicks needed to record an order, put an order on hold, and resume an on-hold order.
- Extend `Bill.txt` with amount received, balance returned, seasonal discount, special discount, bill date, and bill time.
- Introduce `BillDetail.txt` to store the product lines of each bill.
- Introduce `Seasons.txt` for seasonal discounts, and allow managers to add discounts and set them active or inactive.
- Apply the seasonal discount automatically when the bill date falls within an active season.
- Support special discounts of up to 8 percent.
- Print bills as PDF for cashiers and managers.
- Add management reports: incomplete orders, completed orders, cash sales, all sales, top ten most sold items, and top ten items by remaining stock.
- Provide separate cashier and manager features in the user interface.

### Fixed — editing orders (requirements 3 and 10)

- Editing a paid order now changes **that** order: it keeps its Order ID and Bill ID (and its payment date, so the
  7-day window does not restart). `OrderDetail.txt`, `Bill.txt` and `BillDetail.txt` are rewritten and stock moves by
  the difference only. Previously a new order was created and the original was archived.
- While a paid order is edited, the units it already took count as available, so a low-stock line can be kept or
  restored.
- On-hold orders are real orders now: saved in `Order.txt` with status `on-hold`, listed on the Orders page
  ("On Hold" tab, one-click Resume), kept after a restart, and paid under the same Order ID. The two hard-coded demo
  held orders were removed.
- Every payment or edit backs up the five record files first and puts them all back if any step fails.
- The 7-day edit window counts from the bill (payment) date and time.
- Removed the stray duplicate `Frontend/src/ui/RecordOrderPage .cpp`, which the build compiled alongside
  `RecordOrderPage.cpp`.
- New backend calls: `OrderManager::saveCartToOrder`, `BillRepository::replace`, `BillingService::rebill`;
  `discardOrder` also accepts on-hold orders. Covered by `tests/order_edit_test.cpp`.

### Changed — Discounts page follows requirements 13-15

- The Discounts page asks only for what `Seasons.txt` stores: start date, end date, discount percentage and isActive.
  The season name field and column were removed (the requirements have no name, and the file cannot keep one); a
  season is shown by its dates, e.g. "23 Nov - 30 Nov 2026".
- Two active seasons may not share a day, so a bill date always matches at most one seasonal discount (requirement 15).
  An overlapping season can still be saved as inactive.
- A season with isActive = false shows as INACTIVE (it used to show as SCHEDULED) and has its own filter tab.
- The seasonal discount applies to the whole bill; an unused per-category filter was removed (amounts are unchanged).
- Layout: the form scrolls instead of squashing its labels onto the inputs; table columns no longer clip.

### Development Notes

- Increment 3 builds on the Point of Sale system from Increment 2.
- Requirements and open design decisions are documented in `Project-Planning/Increment-3.md`.
- Increment 3 is currently in the planning stage. None of the items listed above are implemented yet.

## [2.0.0] — Increment 2: Point of Sale System

### Planned

- Introduce a GUI-based Point of Sale (POS) workflow.
- Implement product management using `Product.txt`.
- Add product lookup by barcode and product ID.
- Display product specifications and validate available stock.
- Integrate camera-based barcode scanning with the POS.
- Implement cart and order management.
- Generate unique 8-digit Order IDs.
- Store order records in `Order.txt`.
- Store product details and quantities in `OrderDetail.txt`.
- Implement bill generation and payment calculations.
- Save generated bills in `Bill.txt`.
- Support payment modes: cash, card, and wallet.

### Development Notes

- Increment 2 extends the existing barcode generator and reader from Increment 1.
- The system is being developed using modular components to promote separation of responsibilities, reusability, and maintainability.
- Increment 2 is currently in development.

## [1.0.0] — Increment 1: Initial Project Setup

### Added

- Established the initial project structure and directory organization.
- Configured CMake as the project build system.
- Implemented the Qt Widgets-based desktop application foundation.
- Created the initial application window and navigation.
- Implemented barcode generation for six-digit numeric input.
- Implemented barcode reading functionality.
- Added the Decode Barcode interface with camera and image-upload sections.
- Introduced the initial Scan Logs interface.
- Organized backend modules for barcode generation, reading, and scanning.
- Added project documentation and contribution guidelines.

### Notes

- Established the foundational barcode generation and reading functionality.
- Increment 1 serves as the base for the POS capabilities introduced in Increment 2.
