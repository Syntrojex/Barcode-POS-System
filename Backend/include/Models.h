#pragma once

#include <string>

// File record model for the Billing module.
// Order / OrderDetail now live in order/Order.h and order/OrderDetail.h (namespace
// barcodehub::order), so they are not declared a second time here.
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
