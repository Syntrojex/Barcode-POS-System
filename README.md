<h1 align="center">🏷️ BarcodeHub</h1>

<p align="center">
  <b>A C++17-based Point of Sale system with barcode scanning, product management, order processing, and billing.</b>
  <br />
  Developed as an academic project for Introduction to Software Engineering at FAST-NUCES.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=cplusplus&logoColor=white" />
  <img src="https://img.shields.io/badge/Qt-6-41CD52?style=for-the-badge&logo=qt&logoColor=white" />
  <img src="https://img.shields.io/badge/OpenCV-5C3EE8?style=for-the-badge&logo=opencv&logoColor=white" />
  <img src="https://img.shields.io/badge/CMake-Build%20System-064F8C?style=for-the-badge&logo=cmake&logoColor=white" />
  <img src="https://img.shields.io/badge/Platform-Windows%20x64-0078D4?style=for-the-badge&logo=windows&logoColor=white" />
</p>

---

## 📌 Project Overview

**BarcodeHub** is a C++ desktop application being developed as a barcode-based Point of Sale (POS) system.

The project extends its initial barcode generation and reading capabilities into a GUI-based retail workflow, allowing cashiers to scan products, retrieve product information, manage orders, and generate bills. Later increments add category-based barcodes, inventory management, discounts, and management reports.

Developed incrementally, BarcodeHub emphasizes modularity, code quality, and reusability.

---

## ✨ Key Features

### v1.0.0

- 🏷️ Barcode generation for supported six-digit numeric product codes.
- 📖 Barcode reading functionality.
- 🖥️ Qt Widgets-based desktop application foundation.

### v2.0.0

- 📷 Camera-based barcode scanning.
- 📦 Product lookup and description from `Product.txt`.
- 🛒 Cart and order management.
- 📊 Product stock validation.
- 🧾 Automated bill generation.
- 💳 Cash, card, and wallet payment modes.
- 📁 File-based product, order, and billing records.
- 🎨 GUI-based Point of Sale workflow.

### v3.0.0

- 🗂️ Product category management, with the first two barcode digits representing the category.
- ➕ Manager tools to generate barcodes and to add or edit products.
- 📦 Inventory view and inventory updates, with stock updated on every order.
- ✏️ Opening and editing existing orders (paid orders only within 7 days of payment and after manager approval).
- ⚡ Fast order recording, with easy on-hold and resume.
- 🧾 Extended bills with bill details, amount received, and balance returned.
- 🌦️ Seasonal discounts from `Seasons.txt`, plus special discounts of up to 8 percent.
- 🖨️ Bill printing as PDF.
- 📈 Management reports: incomplete orders, completed orders, cash sales, all sales, top ten most sold items, and top ten items by remaining stock.
- 👥 Separate cashier and manager features.

---

## 🔄 POS Workflow

The POS workflow consists of four main modules:

1. **Product Management** — Retrieve product information and validate stock.
2. **Camera & Scanner** — Scan product barcodes using a connected camera.
3. **Order Management** — Record orders and manage product quantities.
4. **Billing** — Calculate totals, generate bills, and record payment information.

---

## 📄 Data Management

BarcodeHub uses text files to store its records:

| File | Purpose | Increment |
|---|---|---|
| `Product.txt` | Product specifications and available stock | 2 (category ID added in 3) |
| `Order.txt` | Customer and order information | 2 |
| `OrderDetail.txt` | Products and quantities associated with orders | 2 |
| `Bill.txt` | Generated bill information and payment status | 2 (extended in 3) |
| `PCategory.txt` | Product categories (ID, name, acronym) | 3 (planned) |
| `BillDetail.txt` | Product lines belonging to each bill | 3 (planned) |
| `Seasons.txt` | Seasonal discount periods and their status | 3 (planned) |

Each record follows the specified field order and uses `#` as its end-of-record marker.

---

## 🛠️ Technology Stack

| Technology | Purpose |
|---|---|
| **C++17** | Core application logic |
| **Qt 6 Widgets** | Desktop graphical interface |
| **OpenCV** | Camera capture and image processing |
| **CMake** | Build configuration |
| **MinGW-w64** | Windows C++ compilation |
| **PowerShell** | Build automation |
| **Git & GitHub** | Version control and collaboration |
| **GitHub Actions** | Automated branch synchronization |

---

## 📂 Project Structure

```text
Bar-Code-System-CPP/
│
├── Backend/
│   ├── include/
│   └── src/
│
├── Frontend/
│   ├── assets/
│   ├── include/
│   └── src/
│
├── data/
│   ├── Product.txt
│   ├── Order.txt
│   ├── OrderDetail.txt
│   ├── Bill.txt
│   ├── PCategory.txt
│   ├── BillDetail.txt
│   └── Seasons.txt
│
├── Project-Planning/
├── tests/
├── .github/
│
├── CMakeLists.txt
├── CMakePresets.json
├── project.json
├── CHANGELOG.md
├── CONTRIBUTORS.md
├── LICENSE
└── README.md
```

---

## 📚 Project Documentation

| Document | Description |
|---|---|
| [Increment 1](Project-Planning/Increment-1.md) | Barcode generator and reader |
| [Increment 2](Project-Planning/Increment-2.md) | Point of Sale system |
| [Increment 3](Project-Planning/Increment-3.md) | Inventory, discounts, reporting, and advanced billing |
| [Changelog](CHANGELOG.md) | History of notable changes |

---

## 🎓 Academic Information

| Detail | Information |
|---|---|
| Project | BarcodeHub |
| Repository | Bar-Code-System-CPP |
| Course | Introduction to Software Engineering (ISE) |
| Institution | FAST-NUCES |
| Semester | 3rd Semester |
| Instructor | Professor Rana Zeeshan |
| Language | C++ |
| Methodology | Incremental Development |
| Submission Platform | GitHub |

---

## 👥 Project Team & Credits

| Team Member | Roll Number | GitHub Profile |
|---|---|---|
| **Muhammad Mustafa Amir** | 25L-3107 | [@Syntrojex](https://github.com/Syntrojex) |
| **Ali Abdullah** | 25L-3022 | [@aliabdullah-se](https://github.com/aliabdullah-se) |
| **Farzam Zeeshan** | 25L-3049 | [@farzamzeeshan1](https://github.com/farzamzeeshan1) |
| **Shauzab Mukhtar** | 25L-3084 | [@shauzab-72](https://github.com/shauzab-72) |

---

## 🤝 Contributing

Contributions, suggestions, and improvements are welcome.

Please read the [Contributing Guidelines](CONTRIBUTING.md) before making changes to the project.

---

<p align="center">
  <b>BarcodeHub</b>
  <br />
  Building a practical barcode solution, one increment at a time.
  <br /><br />
  <sub>Developed with ❤️ by the BarcodeHub team.</sub>
</p>
