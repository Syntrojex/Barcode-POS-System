#pragma once

#include "order/OrderManager.h"   // ProductSnapshot
#include "product/ProductModel.h"      // Farzam's Product

namespace barcodehub::integration {

// Glue between the Product module and the Order module.
// Order only knows ProductSnapshot, Product only knows Product; this one function is the
// single place where one is turned into the other (SRP, and neither module depends on the other).
order::ProductSnapshot toSnapshot(const Product& product);

} // namespace barcodehub::integration
