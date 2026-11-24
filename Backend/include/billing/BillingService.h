#pragma once

/**
 * @file BillingService.h
 * @brief Application service orchestrating order billing, payment execution, and bill persistence.
 * @author Ali Abdullah (25L-3022)
 *
 * Coordinates the checkout lifecycle in the BarcodeHub POS system. Acts as the primary
 * bridge between the graphical interface and backend domain modules (OrderManager,
 * BillRepository, and PaymentMethodRegistry).
 */

#include "billing/Bill.h"
#include "billing/BillRepository.h"
#include "billing/PaymentMethod.h"
#include "order/OrderManager.h"

#include <functional>
#include <string>

namespace barcodehub::billing {

/**
 * @brief Callback function type used by BillingService to fetch product metadata by ID.
 *
 * Inverts the dependency on the Product subsystem: Billing never directly includes
 * ProductModel headers, fulfilling clean architectural layering and decoupling.
 */
using ProductLookup = std::function<bool(const std::string& productId, order::ProductSnapshot& out)>;

/**
 * @class BillingService
 * @brief High-level workflow coordinator managing checkout, payment settlement, and transaction atomicity.
 *
 * Responsibilities:
 * - Order verification: Ensures the target order exists, has line items, and is not already paid or archived.
 * - Product resolution: Obtains live product snapshots (acronym, pricing, tax, discount) via injected callback.
 * - Bill calculation: Invokes BillCalculator to determine monetary figures.
 * - Payment execution: Delegates settlement to the appropriate strategy from PaymentMethodRegistry.
 * - ID generation: Assigns standardized 9-digit Bill IDs based on current date and repository history.
 * - Status transition: Updates the order status to Paid in Order.txt.
 * - Persistence & Rollback: Appends the bill into Bill.txt; rolls back order status if file writing fails.
 */
class BillingService {
public:
    /**
     * @brief Constructs a BillingService with required domain collaborators.
     * @param orders Reference to the active OrderManager instance.
     * @param bills Reference to the BillRepository for disk persistence.
     * @param lookup Injected function for resolving product details by product ID.
     * @param methods Strategy registry of supported payment methods (defaults to Cash, Card, Wallet).
     */
    BillingService(order::OrderManager& orders, BillRepository& bills, ProductLookup lookup,
                   PaymentMethodRegistry methods = PaymentMethodRegistry::withDefaults());

    /**
     * @brief Generates an in-memory bill preview for an active order without committing payment.
     *
     * Used by the POS interface to display itemized line items, tax, discount, and total
     * payable on the screen before the customer renders payment.
     *
     * @param[in] orderId Unique identifier of the order to preview.
     * @param[out] bill Populated Bill object containing preview lines and computed totals (id remains unset).
     * @param[out] error Optional pointer to store error message upon failure.
     * @return True if preview generation succeeded; false if order was missing or invalid.
     */
    bool preview(const std::string& orderId, Bill& bill, std::string* error = nullptr) const;

    /**
     * @brief Executes payment processing, generates a permanent Bill ID, and persists bill records.
     *
     * If cash payment is selected, calculates change due. Performs atomic order status update
     * and file persistence into data/Bill.txt. If bill writing encounters an error, the order
     * status update is rolled back.
     *
     * @param[in] orderId Unique identifier of the order being settled.
     * @param[in] payment Payment request detailing chosen mode, tendered amount, and merchant details.
     * @param[out] bill Finalized Bill object populated with generated Bill ID and saved data.
     * @param[out] change Optional pointer to receive change amount in PKR for cash transactions.
     * @param[out] error Optional pointer to receive failure details.
     * @param[in] adjustments Seasonal and special discount amounts (PKR) taken off the whole bill (Increment 3).
     * @return True if checkout and persistence succeeded; false otherwise.
     */
    bool checkout(const std::string& orderId, const PaymentRequest& payment, Bill& bill,
                  double* change = nullptr, std::string* error = nullptr,
                  const BillAdjustments& adjustments = {});

private:
    order::OrderManager& orders_;           ///< Managed order subsystem.
    BillRepository& bills_;                 ///< Repository managing Bill.txt file I/O.
    ProductLookup lookup_;                  ///< Injected product metadata lookup callback.
    PaymentMethodRegistry methods_;         ///< Registered payment processors.
};

} // namespace barcodehub::billing
