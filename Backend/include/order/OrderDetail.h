#pragma once

#include <string>

namespace barcodehub::order {

struct OrderDetail {
    std::string orderId;
    std::string productId;
    std::string productBarcode;
    double quantity{0.0};

    bool isValid(std::string* error = nullptr) const;
};

} // namespace barcodehub::order
