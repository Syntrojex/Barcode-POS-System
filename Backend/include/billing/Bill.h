#pragma once

/**
 * @file Bill.h
 * @brief Core data structures representing customer bills, line items, and monetary totals.
 * @author Ali Abdullah (25L-3022)
 *
 * This header defines the foundational domain models for Increment 2's Billing subsystem
 * in BarcodeHub. It encapsulates individual line items, aggregated bill totals, and
 * the full bill entity matching the Bill.txt specification.
 */

#include <string>
#include <vector>

namespace barcodehub::billing {

/**
 * @struct BillLine
 * @brief Represents a single product entry within a customer bill.
 *
 * Each line item retains the product acronym, pricing breakdown (in PKR),
 * applicable tax and discount amounts, and purchased quantity.
 */
struct BillLine {
    std::string productAcronym;   ///< Up to 8-character product acronym (e.g. "MILK", "BREAD").
    double unitPrice{0.0};        ///< Base unit price in PKR (exclusive of tax).
    double taxPerUnit{0.0};       ///< Tax amount in PKR per unit (used for calculation, transient).
    double discountPerUnit{0.0};  ///< Applicable discount amount in PKR per unit.
    double quantity{0.0};         ///< Purchased quantity (supports integer counts or fractional weights).
};

/**
 * @struct BillTotals
 * @brief Aggregated financial summary of an entire bill.
 *
 * Maintains the subtotal before tax, cumulative tax, total discount,
 * and the final net payable amount calculated as:
 *   payable = withoutTax + tax - discount
 */
struct BillTotals {
    double withoutTax{0.0};  ///< Net subtotal of all items before taxes in PKR.
    double tax{0.0};         ///< Cumulative tax amount across all line items in PKR.
    double discount{0.0};    ///< Cumulative discount applied across all items in PKR.
    double payable{0.0};     ///< Final total payable by the customer in PKR.
};

/**
 * @struct Bill
 * @brief Complete representation of a finalized or previewed customer bill.
 *
 * Corresponds to records stored in data/Bill.txt. A bill contains a unique 9-digit
 * identifier (YYMMDD + 3-digit daily sequence), customer details, reference to the
 * originating order ID, payment status, associated line items, and total figures.
 */
struct Bill {
    std::string id;              ///< Unique 9-digit Bill ID formatted as YYMMDD### (e.g., "261118001").
    std::string customerName;    ///< Name of the purchasing customer.
    std::string orderId;         ///< 8-digit unique identifier of the source order from Order.txt.
    std::string paymentStatus;   ///< Current payment status (e.g., "paid", "in-progress", "cancelled").
    std::vector<BillLine> lines; ///< Collection of purchased products and quantities in this bill.
    BillTotals totals;           ///< Calculated monetary summary for the entire bill.
};

} // namespace barcodehub::billing
