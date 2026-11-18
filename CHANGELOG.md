# Changelog

All notable changes to **BarcodeHub** are documented in this file.

## [Unreleased] — Frontend/backend integration (2026-11-18)

### Added
- Order/OrderDetail model definitions and validation reconstructed from existing OrderManager tests.
- Qt-to-backend checkout bridge; file-backed order and bill history; actual payment merchant entry.
- Order ID preview, unpaid-order rollback helper, data seeding script and checkout test.
- Optional camera-backed barcode scanning via OpenCV (`CAMERA=ON`).

### Fixed
- CMake referred to missing product headers and quietly skipped order/billing modules.
- Product adapter API and frontend product lookups used obsolete names.
- Stock changes and bills were not persisted from the Qt checkout flow.
- Portable rebuilds risked overwriting previous records; bill display fabricated numbers.
- Scanner returned six digits while catalogue barcodes were eight digits.

### Removed from the running UI
- Mock camera/automatic fake sample scans, fabricated order history/sales chart,
  nonfunctional Resume action and misleading "purchase order drafted" messages.
- No nonempty teammate source files were deleted.

### Verification
- Backend: 4/4 CTest tests passed.
- Qt/OpenCV desktop runtime and Windows package: not tested on this Linux machine (dependencies absent).

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
