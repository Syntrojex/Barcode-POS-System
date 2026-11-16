#include "order/OrderDetail.h"

#include <cmath>

namespace barcodehub::order {

bool OrderDetail::isValid(std::string* error) const {
    if (orderId.empty() || productId.empty() || productBarcode.empty()) {
        if (error) *error = "Order ID, product ID, and barcode are required.";
        return false;
    }
    const auto hasSpace = [](const std::string& text) {
        return text.find_first_of(" \t\r\n") != std::string::npos;
    };
    if (hasSpace(orderId) || hasSpace(productId) || hasSpace(productBarcode)) {
        if (error) *error = "Order ID, product ID, and barcode must not contain spaces.";
        return false;
    }
    if (!std::isfinite(quantity) || quantity <= 0.0) {
        if (error) *error = "Quantity must be a finite value greater than zero.";
        return false;
    }
    return true;
}

} // namespace barcodehub::order
