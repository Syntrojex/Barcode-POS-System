#pragma once

#include <string>

namespace barcodehub::order {

enum class OrderStatus { InProgress, OnHold, Paid, Archived };
enum class PaymentMode { Cash, Card, Wallet };

std::string toString(OrderStatus status);
std::string toString(PaymentMode mode);
bool orderStatusFromString(const std::string& value, OrderStatus& status);
bool paymentModeFromString(const std::string& value, PaymentMode& mode);

// Persistent record in Order.txt. One order header per line.
struct Order {
    std::string id;              // YYMM + daily sequence (01..99) + DD
    std::string date;            // YYYY-MM-DD
    std::string time;            // HH:MM:SS
    std::string customerName;
    std::string customerContact;
    OrderStatus status{OrderStatus::InProgress};
    double paymentAmount{0.0};
    PaymentMode paymentMode{PaymentMode::Cash};
    std::string paymentMerchant;

    bool isValid(std::string* error = nullptr) const;
};

} // namespace barcodehub::order
