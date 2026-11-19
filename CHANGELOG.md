# Changelog

All notable changes to **BarcodeHub** are documented in this file.

## [Unreleased] — Frontend/backend integration (2026-11-18)

### Added
- Order/OrderDetail model definitions and validation reconstructed from existing OrderManager tests.
- Qt-to-backend checkout bridge; file-backed order and bill history; actual payment merchant entry.
- Order ID preview, unpaid-order rollback helper, data seeding script and checkout test.
- Camera barcode scanning via Qt Multimedia + a pure C++ EAN-8 decoder (`CAMERA_MODE=AUTO|ON|OFF`); OpenCV removed, works with Qt MinGW and MSVC.

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
- Qt desktop runtime and Windows package: not tested on this Linux machine (dependencies absent).

## [2.0.0] — Increment 2: Point of Sale System

### Completed — Billing & POS Subsystem (Ali Abdullah)
- Developed arithmetic calculation engine (`BillCalculator`) with subtotal, sales tax, discount, and net payable calculations conforming to 2-decimal monetary rules.
- Implemented Extensible Payment Strategy Pattern (`IPaymentMethod`, `CashPayment`, `CardPayment`, `WalletPayment`, `PaymentMethodRegistry`) adhering to the Open-Closed Principle.
- Built flat-file serialization and parsing codec (`BillRecordCodec`) conforming to the 12-field `Bill.txt` specification with terminal `#` delimiter.
- Built atomic file persistence repository (`BillRepository`) utilizing write-through-temp-file pattern and sequential 9-digit Bill ID generator (`YYMMDD###`).
- Designed checkout and payment modal dialogs (`Dialogs::payment`) supporting Cash change calculation and Card/Wallet merchant validation.
- Integrated Product Description dialog (`Dialogs::productDescription`) and receipt text generator (`Dialogs::receiptText`).
- Implemented comprehensive unit test suite in `tests/billing_tests.cpp` covering 19 distinct verification assertions across calculations, rounding, boundary conditions, payment strategies, codec parsing, and repository persistence.

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
