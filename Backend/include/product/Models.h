#pragma once

#include "product/Product.h"
#include <string>

// File record models for the other Increment-2 modules.
struct Order {
    std::string orderId;
    std::string date;
    std::string time;
    std::string customerName;
    std::string contact;
    std::string status;
    double paymentAmount{0.0};
    std::string paymentMode;
    std::string merchant;
};

struct OrderDetail {
    std::string orderId;
    int productId{0};
    std::string barcode;
    int quantity{0};
};

struct Bill {
    std::string billId;
    std::string customerName;
    std::string orderId;
    std::string productAcronym;
    double unitPrice{0.0};
    double discountPerUnit{0.0};
    int quantity{0};
    double totalWithoutTax{0.0};
    double totalTax{0.0};
    double totalDiscount{0.0};
    double payable{0.0};
    std::string paymentStatus;
};
