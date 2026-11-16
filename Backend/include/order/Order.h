#pragma once

#include <string>

namespace barcodehub::order {

enum class OrderStatus {
    InProgress,
    OnHold,
    Paid,
    Archived
};

enum class PaymentMode {
    Cash,
    Card,
    Wallet
};

std::string toString(OrderStatus status);
std::string toString(PaymentMode mode);
bool orderStatusFromString(const std::string& value, OrderStatus& result);
bool paymentModeFromString(const std::string& value, PaymentMode& result);

struct Order {
    std::string id;              // 8 digits: YYMM + daily sequence (2 digits) + day (2 digits)
    std::string date;            // YYYY-MM-DD
    std::string time;            // HH:MM:SS
    std::string customerName;
    std::string customerContact; // Optional
    OrderStatus status{OrderStatus::InProgress};
    double paymentAmount{0.0};
    PaymentMode paymentMode{PaymentMode::Cash};
    std::string paymentMerchant; // Optional; e.g. ABL, JazzCash

    bool isValid(std::string* error = nullptr) const;
};

} // namespace barcodehub::order
