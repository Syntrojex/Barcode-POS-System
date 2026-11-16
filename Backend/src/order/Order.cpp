#include "order/Order.h"

#include <cmath>
#include <regex>
#include <string>

namespace barcodehub::order {

std::string toString(OrderStatus status) {
    switch (status) {
        case OrderStatus::InProgress: return "in-progress";
        case OrderStatus::OnHold: return "on-hold";
        case OrderStatus::Paid: return "paid";
        case OrderStatus::Archived: return "archived";
    }
    return "in-progress";
}

std::string toString(PaymentMode mode) {
    switch (mode) {
        case PaymentMode::Cash: return "cash";
        case PaymentMode::Card: return "card";
        case PaymentMode::Wallet: return "wallet";
    }
    return "cash";
}

bool orderStatusFromString(const std::string& value, OrderStatus& result) {
    if (value == "in-progress") result = OrderStatus::InProgress;
    else if (value == "on-hold") result = OrderStatus::OnHold;
    else if (value == "paid") result = OrderStatus::Paid;
    else if (value == "archived") result = OrderStatus::Archived;
    else return false;
    return true;
}

bool paymentModeFromString(const std::string& value, PaymentMode& result) {
    if (value == "cash") result = PaymentMode::Cash;
    else if (value == "card") result = PaymentMode::Card;
    else if (value == "wallet") result = PaymentMode::Wallet;
    else return false;
    return true;
}

bool Order::isValid(std::string* error) const {
    static const std::regex idPattern(R"(^[0-9]{8}$)");
    static const std::regex datePattern(R"(^[0-9]{4}-[0-9]{2}-[0-9]{2}$)");
    static const std::regex timePattern(R"(^[0-9]{2}:[0-9]{2}:[0-9]{2}$)");

    if (!std::regex_match(id, idPattern)) {
        if (error) *error = "Order ID must contain exactly 8 digits.";
        return false;
    }
    if (!std::regex_match(date, datePattern) || !std::regex_match(time, timePattern)) {
        if (error) *error = "Date must be YYYY-MM-DD and time must be HH:MM:SS.";
        return false;
    }
    if (customerName.empty()) {
        if (error) *error = "Customer name is required.";
        return false;
    }
    if (!std::isfinite(paymentAmount) || paymentAmount < 0.0) {
        if (error) *error = "Payment amount cannot be negative or non-finite.";
        return false;
    }
    if (status == OrderStatus::Paid && paymentAmount <= 0.0) {
        if (error) *error = "A paid order must have a payment amount greater than zero.";
        return false;
    }
    return true;
}

} // namespace barcodehub::order
