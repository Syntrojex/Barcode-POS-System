#include "order/Order.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace barcodehub::order {
namespace {
bool digits(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}
bool fail(std::string* error, const char* message) {
    if (error) *error = message;
    return false;
}
}

std::string toString(OrderStatus status) {
    switch (status) {
    case OrderStatus::InProgress: return "in-progress";
    case OrderStatus::OnHold: return "on-hold";
    case OrderStatus::Paid: return "paid";
    case OrderStatus::Archived: return "archived";
    }
    return "unknown";
}

std::string toString(PaymentMode mode) {
    switch (mode) {
    case PaymentMode::Cash: return "cash";
    case PaymentMode::Card: return "card";
    case PaymentMode::Wallet: return "wallet";
    }
    return "unknown";
}

bool orderStatusFromString(const std::string& value, OrderStatus& status) {
    if (value == "in-progress") status = OrderStatus::InProgress;
    else if (value == "on-hold") status = OrderStatus::OnHold;
    else if (value == "paid") status = OrderStatus::Paid;
    else if (value == "archived") status = OrderStatus::Archived;
    else return false;
    return true;
}

bool paymentModeFromString(const std::string& value, PaymentMode& mode) {
    if (value == "cash") mode = PaymentMode::Cash;
    else if (value == "card") mode = PaymentMode::Card;
    else if (value == "wallet") mode = PaymentMode::Wallet;
    else return false;
    return true;
}

bool Order::isValid(std::string* error) const {
    if (id.size() != 8 || !digits(id)) return fail(error, "Order ID must contain eight digits.");
    if (date.size() != 10 || date[4] != '-' || date[7] != '-' ||
        !digits(date.substr(0, 4)) || !digits(date.substr(5, 2)) || !digits(date.substr(8, 2)))
        return fail(error, "Date must use YYYY-MM-DD.");
    if (time.size() != 8 || time[2] != ':' || time[5] != ':' ||
        !digits(time.substr(0, 2)) || !digits(time.substr(3, 2)) || !digits(time.substr(6, 2)))
        return fail(error, "Time must use HH:MM:SS.");
    if (customerName.empty()) return fail(error, "Customer name is required.");
    if (!std::isfinite(paymentAmount) || paymentAmount < 0.0)
        return fail(error, "Payment amount must be non-negative.");
    if (status == OrderStatus::Paid && paymentAmount <= 0.0)
        return fail(error, "Paid orders require a positive payment amount.");
    if (toString(status) == "unknown" || toString(paymentMode) == "unknown")
        return fail(error, "Unknown order status or payment method.");
    if (error) error->clear();
    return true;
}

} // namespace barcodehub::order
