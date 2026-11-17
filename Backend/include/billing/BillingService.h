#pragma once

#include "billing/Bill.h"
#include "billing/BillRepository.h"
#include "billing/PaymentMethod.h"
#include "order/OrderManager.h"

#include <functional>
#include <string>

namespace barcodehub::billing {

// Finds a product by the Product ID stored in OrderDetail.txt. The POS supplies it from the
// Product module (see integration/ProductAdapter.h), so Billing never depends on Product.
using ProductLookup = std::function<bool(const std::string& productId, order::ProductSnapshot& out)>;

// The cashier's last step: order -> bill -> payment -> Bill.txt. It only coordinates;
// the arithmetic, the file layout and the payment rules each live in their own class.
class BillingService {
public:
    BillingService(order::OrderManager& orders, BillRepository& bills, ProductLookup lookup,
                   PaymentMethodRegistry methods = PaymentMethodRegistry::withDefaults());

    // Bill for an order WITHOUT paying (what the screen shows before the customer pays).
    // The returned bill has no ID yet.
    bool preview(const std::string& orderId, Bill& bill, std::string* error = nullptr) const;

    // Takes the payment, marks the order as paid in Order.txt and saves the bill in Bill.txt.
    // `change` (optional) receives the money to hand back for cash.
    bool checkout(const std::string& orderId, const PaymentRequest& payment, Bill& bill,
                  double* change = nullptr, std::string* error = nullptr);

private:
    order::OrderManager& orders_;
    BillRepository& bills_;
    ProductLookup lookup_;
    PaymentMethodRegistry methods_;
};

} // namespace barcodehub::billing
