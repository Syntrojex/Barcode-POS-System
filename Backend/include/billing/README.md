# Billing Module (Standard C++17)

**Author:** Ali Abdullah (25L-3022)  
**Deliverable:** Increment 2 — Billing & POS Subsystem

This module implements the business logic, financial calculations, payment processing strategies, and flat-file persistence for customer bills in **BarcodeHub**. It is written in pure standard C++17, decoupled from Qt GUI headers, enabling high testability, modularity, and rapid unit testing.

---

## 1. Architecture & Component Responsibilities

The subsystem follows the **Single Responsibility Principle (SRP)** and **Open-Closed Principle (OCP)** across six core components:

| Component | Header | Responsibility |
|---|---|---|
| **Domain Models** | `Bill.h` | Defines `BillLine`, `BillTotals`, and `Bill` structures matching project specifications. |
| **Calculation Engine** | `BillCalculator.h` | Pure arithmetic engine computing subtotals, taxes, discounts, and net payable with 2-decimal rounding. |
| **Payment Strategies** | `PaymentMethod.h` | Strategy pattern interface (`IPaymentMethod`) and implementations (`CashPayment`, `CardPayment`, `WalletPayment`, `PaymentMethodRegistry`). |
| **Record Codec** | `BillRecordCodec.h` | Pure string-level encoder/decoder translating between `Bill` domain models and `Bill.txt` flat-file format. |
| **Data Repository** | `BillRepository.h` | Data Access Object (DAO) providing atomic write-through-temp-file persistence and sequential 9-digit Bill ID generation. |
| **Application Service** | `BillingService.h` | Orchestrates order settlement, product metadata lookup, payment execution, and atomic order status synchronization. |

---

## 2. Bill.txt Format Specification

Each line in `data/Bill.txt` represents one purchased line item of a customer bill, ending with `#`. All monetary totals and status fields are repeated across lines sharing the same Bill ID:

```text
billId customer orderId acronym unitPrice discountPerUnit quantity totalWithoutTax totalTax totalDiscount totalPayable status #
```

### Field Breakdown:
1. **billId**: Exactly 9 numeric digits formatted as `YYMMDD###` (e.g., `261118001`).
2. **customer**: Customer name (encoded via `RecordText` to escape spaces).
3. **orderId**: 8-digit originating order identifier from `Order.txt`.
4. **acronym**: Product acronym (up to 8 characters, e.g., `MILK`, `BREAD`).
5. **unitPrice**: Base unit price in PKR.
6. **discountPerUnit**: Discount amount per unit in PKR.
7. **quantity**: Purchased quantity (count or weight).
8. **totalWithoutTax**: Bill subtotal before tax in PKR.
9. **totalTax**: Cumulative sales tax in PKR.
10. **totalDiscount**: Cumulative discounts in PKR.
11. **totalPayable**: Net amount payable (`withoutTax + tax - discount`) in PKR.
12. **status**: Payment status (e.g., `paid`).
13. **#**: Mandatory terminal character.

---

## 3. Financial Calculation Rules

`BillCalculator` enforces standard business accounting rules:
- $\text{subtotal} = \sum (\text{unitPrice} \times \text{quantity})$
- $\text{totalTax} = \sum (\text{taxPerUnit} \times \text{quantity})$
- $\text{totalDiscount} = \sum (\text{discountPerUnit} \times \text{quantity})$
- $\text{payable} = \text{roundMoney}(\text{subtotal} + \text{totalTax} - \text{totalDiscount})$

All numbers are verified for finite validity ($\text{non-NaN}$, $\text{non-Inf}$), strictly non-negative prices/taxes/discounts, and positive quantities.

---

## 4. Payment Strategy Pattern

Payment processing is architected with the Strategy Pattern:
- **Cash (`CashPayment`)**: Verifies tendered cash is finite, non-negative, and covers the payable amount. Calculates change returned to the customer.
- **Card (`CardPayment`)**: Validates acquiring bank name (must be non-empty and free of `#` delimiters).
- **Wallet (`WalletPayment`)**: Validates digital wallet provider (e.g., JazzCash, EasyPaisa, SadaPay).
- **Extensibility**: To add a new payment method (e.g., Crypto, Voucher), implement `IPaymentMethod` and register it with `PaymentMethodRegistry::add()` without modifying any existing service logic.

---

## 5. Verification & Testing

Unit tests are implemented in `tests/billing_tests.cpp`. To execute the billing test suite:

```sh
# Using CMake / CTest
ctest --test-dir build/test-build -R billing --output-on-failure
```
