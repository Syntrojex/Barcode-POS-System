# Changelog

All notable changes to **BarcodeHub** are documented in this file.

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

### Added — Product Management (Farzam Zeeshan)

- Pure C++17 product module (`Backend/*/product`, no Qt or OpenCV): `Product`, `ProductRecordCodec`
  (one-line parse/format), `ProductValidator` (pluggable rules), `StockValidator`,
  `ProductDescriber`, `ProductManager` (load/save `Product.txt`, search by barcode and ID,
  stock deduction).
- Unit tests (`product_tests`, run with `ctest`) and sample data in `tests/data/`.
- Dark mode: moon/sun button in the top bar or `Ctrl+D`; the choice is remembered.

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
