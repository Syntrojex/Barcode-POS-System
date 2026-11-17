#include "billing/BillCalculator.h"

#include "common/RecordText.h"

#include <cmath>

namespace barcodehub::billing {

bool BillCalculator::calculate(const std::vector<BillLine>& lines, BillTotals& totals,
                               std::string* error) {
    totals = BillTotals{};
    if (lines.empty()) {
        if (error) *error = "A bill needs at least one product.";
        return false;
    }
    for (const auto& line : lines) {
        const bool bad = !std::isfinite(line.unitPrice) || line.unitPrice < 0.0 ||
                         !std::isfinite(line.taxPerUnit) || line.taxPerUnit < 0.0 ||
                         !std::isfinite(line.discountPerUnit) || line.discountPerUnit < 0.0 ||
                         !std::isfinite(line.quantity) || line.quantity <= 0.0;
        if (bad) {
            if (error) *error = "Invalid price, tax, discount or quantity for " + line.productAcronym + ".";
            return false;
        }
        totals.withoutTax += line.unitPrice * line.quantity;
        totals.tax += line.taxPerUnit * line.quantity;
        totals.discount += line.discountPerUnit * line.quantity;
    }
    totals.withoutTax = common::roundMoney(totals.withoutTax);
    totals.tax = common::roundMoney(totals.tax);
    totals.discount = common::roundMoney(totals.discount);
    totals.payable = common::roundMoney(totals.withoutTax + totals.tax - totals.discount);
    if (totals.payable < 0.0) {
        if (error) *error = "Discount is larger than the bill.";
        totals = BillTotals{};
        return false;
    }
    return true;
}

} // namespace barcodehub::billing
