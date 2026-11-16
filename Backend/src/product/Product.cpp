#include "product/Product.h"

#include <cmath>

bool Product::isValid(std::string* reason) const {
    const auto fail = [reason](const char* message) {
        if (reason) *reason = message;
        return false;
    };

    if (productId <= 0) return fail("product id must be positive");
    if (barcode.empty()) return fail("barcode is required");
    if (acronym.empty() || acronym.size() > 8) return fail("acronym must contain 1 to 8 characters");
    if (name.empty()) return fail("product name is required");
    if (measurementUnit != "count" && measurementUnit != "weight")
        return fail("measurement unit must be count or weight");
    if (!std::isfinite(unitPrice) || unitPrice < 0.0) return fail("unit price must be non-negative");
    if (!std::isfinite(tax) || tax < 0.0) return fail("tax must be non-negative");
    if (!std::isfinite(discount) || discount < 0.0) return fail("discount must be non-negative");
    if (availableQuantity < 0) return fail("available quantity must be non-negative");
    if (barcodeFilePath.empty()) return fail("barcode file path is required");
    if (reason) reason->clear();
    return true;
}
