#include "integration/ProductAdapter.h"

#include <string>

namespace barcodehub::integration {

order::ProductSnapshot toSnapshot(const Product& p) {
    order::ProductSnapshot s;
    s.productId = std::to_string(p.getProductID());   // Product keeps an int id, Order stores text
    s.barcode = p.getBarcode();
    s.acronym = p.getAcronym();
    s.name = p.getProductName();
    s.measurementUnit = p.getMeasurementUnitName();
    s.unitPrice = p.getUnitPrice();
    s.taxPerUnit = p.getTax();
    s.discountPerUnit = p.getDiscount();
    s.availableQuantity = p.getAvailableQuantity();   // int -> double
    return s;
}

} // namespace barcodehub::integration
