#pragma once

/**
 * @file PaymentMethod.h
 * @brief Strategy pattern interfaces and implementations for processing customer payments.
 * @author Ali Abdullah (25L-3022)
 *
 * Implements extensible payment processing for BarcodeHub. Supports multiple payment
 * modes (Cash, Credit/Debit Card, Mobile Wallet) following the Open-Closed Principle (OCP):
 * new payment modes can be added without modifying existing processing workflows.
 */

#include "order/Order.h"   // PaymentMode (cash / card / wallet)

#include <memory>
#include <string>
#include <vector>

namespace barcodehub::billing {

/**
 * @struct PaymentRequest
 * @brief Input payload submitted by the POS cashier during the checkout workflow.
 */
struct PaymentRequest {
    order::PaymentMode mode{order::PaymentMode::Cash}; ///< Selected payment method (Cash, Card, Wallet).
    std::string merchant;        ///< For Card/Wallet: Bank or service provider name (e.g., "HBL", "JazzCash").
    double amountTendered{0.0};  ///< For Cash: Physical currency amount handed over by customer in PKR.
};

/**
 * @struct PaymentResult
 * @brief Outcome and financial reconciliation of an attempted payment transaction.
 */
struct PaymentResult {
    bool ok{false};              ///< True if the transaction succeeded; false if rejected.
    double change{0.0};          ///< Balance change to return to the customer in PKR (Cash payments).
    std::string error;           ///< Descriptive failure reason if the transaction was rejected.
};

/**
 * @class IPaymentMethod
 * @brief Abstract strategy interface defining standard behavior for all payment processors.
 */
class IPaymentMethod {
public:
    virtual ~IPaymentMethod() = default;

    /**
     * @brief Identifies the payment mode handled by this strategy implementation.
     * @return Corresponding order::PaymentMode enumeration.
     */
    virtual order::PaymentMode mode() const = 0;

    /**
     * @brief Validates and executes payment settlement against a payable amount.
     * @param request Payment parameters tendered by the cashier/customer.
     * @param payable Net bill total due in PKR.
     * @return PaymentResult indicating success status, change owed, or error detail.
     */
    virtual PaymentResult process(const PaymentRequest& request, double payable) const = 0;
};

/**
 * @class CashPayment
 * @brief Handles physical cash settlements, verifying sufficient funds and calculating change.
 */
class CashPayment : public IPaymentMethod {
public:
    order::PaymentMode mode() const override { return order::PaymentMode::Cash; }
    PaymentResult process(const PaymentRequest& request, double payable) const override;
};

/**
 * @class CardPayment
 * @brief Handles credit/debit card transactions, requiring valid acquiring bank identification.
 */
class CardPayment : public IPaymentMethod {
public:
    order::PaymentMode mode() const override { return order::PaymentMode::Card; }
    PaymentResult process(const PaymentRequest& request, double payable) const override;
};

/**
 * @class WalletPayment
 * @brief Handles digital mobile wallet transactions (e.g. JazzCash, Nayapay, SadaPay).
 */
class WalletPayment : public IPaymentMethod {
public:
    order::PaymentMode mode() const override { return order::PaymentMode::Wallet; }
    PaymentResult process(const PaymentRequest& request, double payable) const override;
};

/**
 * @class PaymentMethodRegistry
 * @brief Container and dispatcher managing registered payment processing strategies.
 *
 * Facilitates lookup and execution of the appropriate IPaymentMethod handler
 * based on the cashier's chosen payment mode.
 */
class PaymentMethodRegistry {
public:
    /**
     * @brief Factory creating a registry pre-populated with standard payment strategies (Cash, Card, Wallet).
     * @return Initialized PaymentMethodRegistry instance.
     */
    static PaymentMethodRegistry withDefaults();

    /**
     * @brief Registers an additional payment processor strategy into the registry.
     * @param method Owning pointer to an IPaymentMethod implementation.
     */
    void add(std::unique_ptr<IPaymentMethod> method);

    /**
     * @brief Retrieves the payment processor corresponding to the requested payment mode.
     * @param mode Target payment mode.
     * @return Non-owning pointer to the matching processor, or nullptr if unsupported.
     */
    const IPaymentMethod* find(order::PaymentMode mode) const;

private:
    std::vector<std::shared_ptr<const IPaymentMethod>> methods_; ///< Strategy storage.
};

} // namespace barcodehub::billing
