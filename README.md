# BarcodeHub

**Barcode-Based Point of Sale and Retail Management System**

BarcodeHub is a C++ desktop Point of Sale (POS) application designed to simplify product scanning, order processing, billing, and inventory management.

The application uses Qt Widgets for its graphical interface, OpenCV for camera-based barcode scanning, and plain-text files for persistent data storage.

Developed for the **Introduction to Software Engineering** course at **FAST-NUCES**.

## Project Status

| Increment | Description | Status |
|---|---|---|
| Increment 1 | Barcode generation and reading foundation | Completed |
| Increment 2 | POS workflow, orders, billing, and stock updates | Completed |
| Increment 3 | Extended retail management features | Planned / In Development |

> Increment 3 features listed below represent the planned scope and should not be considered implemented until completed and tested.

## Features

### v1.0.0 — Barcode Foundation

- Barcode generation.
- Barcode reading and decoding.
- Product barcode identification.

### v2.0.0 — Point of Sale System

- Camera-based barcode scanning.
- Product lookup from Product.txt.
- Product information display.
- Cart and order management.
- Quantity modification.
- Stock validation.
- Checkout and payment handling.
- Bill generation.
- Order and bill persistence.
- Automatic stock updates.
- Qt-based graphical interface.

### v3.0.0 — Retail Management Expansion

- Product category management.
- Category-based barcode generation.
- Manager-controlled product creation and editing.
- Inventory monitoring and stock adjustments.
- Existing order editing.
- Manager approval for paid-order editing.
- Improved hold and resume workflow.
- Expanded bill records.
- BillDetail.txt for bill line items.
- PDF bill printing.
- Cashier and manager interfaces.
- Seasonal discounts.
- Special discounts with an 8% maximum.
- Sales and inventory reports.

See [Increment 3.md](Increment%203.md) for detailed requirements, data formats, architecture, implementation plan, and testing checklist.

## Technology Stack

| Technology | Purpose |
|---|---|
| C++17 | Application and business logic |
| Qt 6 Widgets | Desktop graphical interface |
| OpenCV 4 | Camera input and image processing |
| CMake | Build configuration |
| CTest | Automated testing |
| Plain-text files | Local data persistence |

## Architecture

BarcodeHub separates backend business logic from the Qt frontend.

```text
BarcodeHub
│
├── Frontend
│   └── Qt Widgets
│       ├── Dashboard
│       ├── Record Order
│       ├── Orders
│       ├── Products
│       ├── Barcode Scanner
│       └── Settings
│
└── Backend
    ├── Scanner
    ├── Product Management
    ├── Order Management
    ├── Billing
    └── Integration
