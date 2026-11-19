#pragma once

/**
 * @file BillCalculator.h
 * @brief Pure monetary calculation and validation engine for customer bills.
 * @author Ali Abdullah (25L-3022)
 *
 * Implements the financial accounting logic for the BarcodeHub POS system.
 * The BillCalculator is decoupled from file storage and user interfaces,
 * ensuring high modularity, high testability, and strict adherence to
 * business rules regarding subtotal, taxes, discounts, and rounding.
 */

#include "billing/Bill.h"

#include <string>
#include <vector>

namespace barcodehub::billing {

/**
 * @class BillCalculator
 * @brief Stateless arithmetic engine responsible for computing and validating bill totals.
 *
 * Enforces the standardized business calculation rule:
 * \f[
 *   \text{payable} = \text{totalWithoutTax} + \text{totalTax} - \text{totalDiscount}
 * \f]
 *
 * Validation checks executed during calculation:
 * - Empty bill check: At least one product line item must be present.
 * - Value sanity: Unit prices, taxes, and discounts must be finite and non-negative.
 * - Quantity validity: Purchased quantity must be finite and strictly greater than zero.
 * - Net total sanity: The net payable amount after discount cannot be negative.
 * - Precision: All computed totals are normalized to 2 decimal places using standard rounding.
 */
class BillCalculator {
public:
    /**
     * @brief Computes totals for a list of bill line items and validates financial boundaries.
     *
     * Iterates through all line items, accumulates subtotal, sales tax, and discounts,
     * rounds each component according to monetary precision rules, and derives final payable.
     *
     * @param[in] lines Collection of product line items to be totaled.
     * @param[out] totals Aggregated BillTotals structure populated upon success.
     * @param[out] error Optional pointer to receive a human-readable failure description on validation error.
     * @return True if all items are valid and totals were calculated successfully; false otherwise.
     */
    static bool calculate(const std::vector<BillLine>& lines, BillTotals& totals,
                          std::string* error = nullptr);
};

} // namespace barcodehub::billing
