#pragma once

#include "order/Order.h"   // PaymentMode (cash / card / wallet)

#include <memory>
#include <string>
#include <vector>

namespace barcodehub::billing {

struct PaymentRequest {
    order::PaymentMode mode{order::PaymentMode::Cash};
    std::string merchant;        // card / wallet: bank or wallet name, e.g. ABL, JazzCash
    double amountTendered{0.0};  // cash: what the customer hands over
};

struct PaymentResult {
    bool ok{false};
    double change{0.0};          // cash only
    std::string error;
};

// One way of paying. A new mode = a new class + one add() call; nothing existing changes (open/closed).
class IPaymentMethod {
public:
    virtual ~IPaymentMethod() = default;
    virtual order::PaymentMode mode() const = 0;
    virtual PaymentResult process(const PaymentRequest& request, double payable) const = 0;
};

class CashPayment : public IPaymentMethod {   // needs enough money, gives change
public:
    order::PaymentMode mode() const override { return order::PaymentMode::Cash; }
    PaymentResult process(const PaymentRequest& request, double payable) const override;
};

class CardPayment : public IPaymentMethod {   // needs the bank name
public:
    order::PaymentMode mode() const override { return order::PaymentMode::Card; }
    PaymentResult process(const PaymentRequest& request, double payable) const override;
};

class WalletPayment : public IPaymentMethod { // needs the wallet name
public:
    order::PaymentMode mode() const override { return order::PaymentMode::Wallet; }
    PaymentResult process(const PaymentRequest& request, double payable) const override;
};

class PaymentMethodRegistry {
public:
    static PaymentMethodRegistry withDefaults();   // cash, card, wallet
    void add(std::unique_ptr<IPaymentMethod> method);
    const IPaymentMethod* find(order::PaymentMode mode) const;

private:
    std::vector<std::shared_ptr<const IPaymentMethod>> methods_;   // shared: registry is cheap to copy
};

} // namespace barcodehub::billing
