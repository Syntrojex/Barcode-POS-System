#pragma once
#include <string>
#include <sstream>
#include <vector>

struct Product {
    std::string productId;
    std::string barcode;
    std::string acronym; // max 8 chars
    std::string name;
    std::string unit; // count or weight
    double unitPrice;
    double tax; // PKR amount per unit
    double discount; // per unit
    int quantity;
    std::string barcodeImagePath;

    static Product fromLine(const std::string& line) {
        Product p;
        std::istringstream ss(line);
        ss >> p.productId >> p.barcode >> p.acronym >> p.name >> p.unit >> p.unitPrice >> p.tax >> p.discount >> p.quantity >> p.barcodeImagePath;
        // Remove trailing '#', if present
        if (!p.barcodeImagePath.empty() && p.barcodeImagePath.back() == '#') {
            p.barcodeImagePath.pop_back();
        }
        return p;
    }
};

struct Order {
    std::string orderId; // 8 digits
    std::string date; // YYYY-MM-DD
    std::string time; // HH:MM:SS
    std::string customerName;
    std::string contact; // optional, may be empty
    std::string status; // in-progress, on-hold, paid, archived
    double paymentAmount;
    std::string paymentMode; // cash, card, wallet
    std::string merchant;
};

struct OrderDetail {
    std::string orderId;
    std::string productId;
    std::string barcode;
    int quantity;
};

struct Bill {
    std::string billId; // 9 digits
    std::string customerName;
    std::string orderId;
    std::string productAcronym;
    double unitPrice; // without tax
    double discountPerUnit;
    int quantity;
    double totalWithoutTax;
    double totalTax;
    double totalDiscount;
    double payable;
    std::string paymentStatus; // mirrors order status
};

using ProductList = std::vector<Product>;
