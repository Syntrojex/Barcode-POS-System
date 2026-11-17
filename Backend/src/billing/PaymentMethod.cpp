#include "billing/PaymentMethod.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace barcodehub::billing {
namespace {

bool validMerchant(const std::string& name) {
    return !name.empty() && name.find('#') == std::string::npos;
}

PaymentResult electronic(const PaymentRequest& request, const char* what) {
    PaymentResult result;
    if (!validMerchant(request.merchant)) {
        result.error = std::string("Enter the ") + what + " name (for example ABL or JazzCash).";
        return result;
    }
    result.ok = true;
    return result;
}

} // namespace

PaymentResult CashPayment::process(const PaymentRequest& request, double payable) const {
    PaymentResult result;
    if (!std::isfinite(request.amountTendered) || request.amountTendered + 1e-9 < payable) {
        result.error = "Cash received is less than the amount payable.";
        return result;
    }
    result.ok = true;
    result.change = std::round((request.amountTendered - payable) * 100.0) / 100.0;
    return result;
}

PaymentResult CardPayment::process(const PaymentRequest& request, double) const {
    return electronic(request, "bank");
}

PaymentResult WalletPayment::process(const PaymentRequest& request, double) const {
    return electronic(request, "wallet");
}

PaymentMethodRegistry PaymentMethodRegistry::withDefaults() {
    PaymentMethodRegistry registry;
    registry.add(std::make_unique<CashPayment>());
    registry.add(std::make_unique<CardPayment>());
    registry.add(std::make_unique<WalletPayment>());
    return registry;
}

void PaymentMethodRegistry::add(std::unique_ptr<IPaymentMethod> method) {
    methods_.push_back(std::shared_ptr<const IPaymentMethod>(std::move(method)));
}

const IPaymentMethod* PaymentMethodRegistry::find(order::PaymentMode mode) const {
    const auto it = std::find_if(methods_.begin(), methods_.end(),
                                 [&](const auto& m) { return m->mode() == mode; });
    return it == methods_.end() ? nullptr : it->get();
}

} // namespace barcodehub::billing
