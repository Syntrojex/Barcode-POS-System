#include "order/OrderDetail.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace barcodehub::order {
bool OrderDetail::isValid(std::string* error) const {
    if (orderId.size() != 8 || !std::all_of(orderId.begin(), orderId.end(),
            [](unsigned char c) { return std::isdigit(c); })) {
        if (error) *error = "Detail needs an eight-digit Order ID.";
        return false;
    }
    if (productId.empty() || productBarcode.empty() ||
        productId.find_first_of(" \t\r\n#") != std::string::npos ||
        productBarcode.find_first_of(" \t\r\n#") != std::string::npos) {
        if (error) *error = "Detail needs a valid product ID and barcode.";
        return false;
    }
    if (!std::isfinite(quantity) || quantity <= 0.0) {
        if (error) *error = "Detail quantity must be positive.";
        return false;
    }
    if (error) error->clear();
    return true;
}
} // namespace barcodehub::order
