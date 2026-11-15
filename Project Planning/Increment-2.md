# Increment 2 — Point of Sale (POS) System

## Overview

Increment 2 extends BarcodeHub from a barcode generation and reading application into a GUI-based Point of Sale (POS) system.

The system will allow cashiers to scan product barcodes, retrieve product information, manage orders, calculate bills, and record payment details.

## Objectives

- Implement a camera-based barcode reading workflow.
- Develop a GUI-based POS interface.
- Retrieve product specifications from `Product.txt`.
- Manage shopping carts and order records.
- Validate product quantities against available stock.
- Generate and save bills.
- Support different payment modes.
- Maintain modular, reusable, and maintainable code.

## Team Division & Responsibilities

### 1. Farzam Zeeshan — Product Management

**Responsibilities**
- Read and parse `Product.txt`.
- Create the `Product` class.
- Search products by barcode and product ID.
- Display product descriptions.
- Validate available product stock.

**Deliverable:** Product management module with `Product.txt` integration.

### 2. Shauzab Mukhtar — Camera & Scanner

**Responsibilities**
- Implement camera-based barcode scanning.
- Integrate the scanner with the POS workflow.
- Handle camera and barcode scanning errors.
- Reuse the barcode reading functionality developed in Increment 1.

**Deliverable:** Camera scanner module.

### 3. Muhammad Mustafa Amir — Order Management

**Responsibilities**
- Record orders and manage the shopping cart.
- Validate requested quantities against available stock.
- Generate unique 8-digit Order IDs.
- Save order records in `Order.txt`.
- Save product details and quantities in `OrderDetail.txt`.

**Deliverable:** Order management module.

### 4. Ali Abdullah — Billing & POS GUI

**Responsibilities**
- Design and implement the POS graphical interface.
- Generate bills and calculate bill totals.
- Add the Description button to the order-recording interface.
- Save generated bills in `Bill.txt`.
- Implement payment modes.

**Deliverable:** POS GUI and billing module.

## Integration Flow

The planned module integration sequence is:

1. **Product Management:** Retrieve product information and validate stock.
2. **Camera & Scanner:** Scan product barcodes and provide decoded barcode values to the POS workflow.
3. **Order Management:** Add products to the cart and manage order records.
4. **Billing:** Calculate totals, generate bills, and record payment details.

## Data File Specifications

### Product.txt

Each product occupies one line, ending with `#`. Fields are space-separated and must follow this order:

1. Product ID
2. Product barcode
3. Product acronym (maximum 8 characters)
4. Product name
5. Measurement unit (`count` or `weight`)
6. Product unit price
7. Product tax amount in PKR
8. Discount per unit
9. Available quantity
10. Barcode file name and path

### Order.txt

Each order occupies one line, ending with `#`. Fields are space-separated and follow this order:

1. 8-digit Order ID (last two digits indicate the order date)
2. Order date
3. Order time
4. Customer name
5. Customer contact (optional)
6. Order status (`in-progress`, `on-hold`, `paid`, `archived`)
7. Payment amount
8. Payment mode (`cash`, `card`, `wallet`)
9. Payment merchant name

### OrderDetail.txt

Each order detail record occupies one line and ends with `#`.

Fields:

1. Order ID
2. Product ID
3. Product barcode
4. Quantity

Each record associates a product and its quantity with an order.

### Bill.txt

Each bill occupies one line, ending with `#`. Fields are space-separated and follow this order:

1. 9-digit Bill ID
2. Customer name
3. Order ID
4. Product acronym
5. Product unit price without tax
6. Discount per unit (if applicable)
7. Product quantity
8. Total bill without tax
9. Total tax
10. Total discount
11. Total payable bill
12. Bill payment status (order status from `Order.txt`)

## Software Engineering Principles

The implementation should emphasize:

- **Single Responsibility Principle (SRP):** Each class and module should have one clear responsibility.
- **Open/Closed Principle (OCP):** Components should support extension without unnecessary modification to existing functionality.
- **Code Quality:** Keep code readable, consistent, and maintainable.
- **Reusability:** Reuse existing barcode functionality and design independent modules with clear interfaces.
