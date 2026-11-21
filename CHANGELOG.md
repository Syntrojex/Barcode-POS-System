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
