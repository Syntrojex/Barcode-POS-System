#include "billing/BillingService.h"

#include "billing/BillCalculator.h"
#include "common/RecordText.h"

#include <algorithm>

namespace barcodehub::billing {
namespace {

void clearError(std::string* error) {
    if (error) error->clear();
}

bool fail(std::string* error, const std::string& why) {
    if (error) *error = why;
    return false;
}

} // namespace

BillingService::BillingService(order::OrderManager& orders, BillRepository& bills,
                               ProductLookup lookup, PaymentMethodRegistry methods)
    : orders_(orders), bills_(bills), lookup_(std::move(lookup)), methods_(std::move(methods)) {}

bool BillingService::preview(const std::string& orderId, Bill& bill, std::string* error) const {
    clearError(error);
    order::Order order;
    if (!orders_.findOrder(orderId, order, error)) return false;

    std::vector<order::OrderDetail> details;
    if (!orders_.loadOrderDetails(orderId, details, error)) return false;
    if (details.empty()) return fail(error, "Order " + orderId + " has no products.");

    Bill result;
    result.customerName = order.customerName;
    result.orderId = order.id;
    result.paymentStatus = order::toString(order.status);
    for (const auto& detail : details) {
        order::ProductSnapshot product;
        if (!lookup_ || !lookup_(detail.productId, product))
            return fail(error, "Product " + detail.productId + " was not found.");
        BillLine line;
        line.productAcronym = product.acronym;
        line.unitPrice = product.unitPrice;
        line.taxPerUnit = product.taxPerUnit;
        line.discountPerUnit = product.discountPerUnit;
        line.quantity = detail.quantity;
        result.lines.push_back(line);
    }
    if (!BillCalculator::calculate(result.lines, result.totals, error)) return false;

    bill = std::move(result);
    return true;
}

bool BillingService::checkout(const std::string& orderId, const PaymentRequest& payment,
                              Bill& bill, double* change, std::string* error,
                              const BillAdjustments& adjustments) {
    clearError(error);
    if (change) *change = 0.0;

    order::Order order;
    if (!orders_.findOrder(orderId, order, error)) return false;
    if (order.status == order::OrderStatus::Paid) return fail(error, "Order " + orderId + " is already paid.");
    if (order.status == order::OrderStatus::Archived) return fail(error, "Order " + orderId + " is archived.");

    Bill result;
    if (!preview(orderId, result, error)) return false;

    // 0. bill-level discounts (Increment 3): seasonal (Seasons.txt) and special (at most 8 percent)
    const double seasonal = common::roundMoney(adjustments.seasonalDiscount);
    const double special = common::roundMoney(adjustments.specialDiscount);
    if (seasonal < 0 || special < 0) return fail(error, "Seasonal and special discounts cannot be negative.");
    if (seasonal + special > result.totals.payable + 0.005)
        return fail(error, "Seasonal and special discounts are larger than the bill.");
    result.seasonalDiscount = seasonal;
    result.specialDiscount = special;
    result.totals.payable = common::roundMoney(result.totals.payable - seasonal - special);

    // 1. payment rules of the chosen mode
    const IPaymentMethod* method = methods_.find(payment.mode);
    if (!method) return fail(error, "This payment mode is not available.");
    const PaymentResult paid = method->process(payment, result.totals.payable);
    if (!paid.ok) return fail(error, paid.error);

    // 2. Bill ID
    std::vector<Bill> existing;
    if (!bills_.loadAll(existing, error)) return false;
    result.date = common::currentDate();
    result.time = common::currentTime();
    result.id = BillRepository::nextBillId(result.date, existing, error);
    if (result.id.empty()) return false;
    // amount received / balance returned: cash = what was handed over; card and wallet pay exactly the bill
    result.amountReceived = payment.mode == order::PaymentMode::Cash ? common::roundMoney(payment.amountTendered)
                                                                     : result.totals.payable;
    result.balanceReturned = common::roundMoney(paid.change);

    // 3. order becomes paid (Order.txt)
    order::Order paidOrder = order;
    paidOrder.status = order::OrderStatus::Paid;
    paidOrder.paymentAmount = result.totals.payable;
    paidOrder.paymentMode = payment.mode;
    paidOrder.paymentMerchant = payment.mode == order::PaymentMode::Cash ? std::string() : payment.merchant;
    if (!orders_.updateOrder(paidOrder, error)) return false;
    result.paymentStatus = order::toString(paidOrder.status);

    // 4. bill is saved (Bill.txt); if that fails the order goes back to how it was
    if (!bills_.append(result, error)) {
        std::string ignored;
        orders_.updateOrder(order, &ignored);
        return false;
    }

    bill = std::move(result);
    if (change) *change = paid.change;
    return true;
}

bool BillingService::rebill(const std::string& orderId, const PaymentRequest& payment,
                            Bill& bill, double* change, std::string* error,
                            const BillAdjustments& adjustments) {
    clearError(error);
    if (change) *change = 0.0;

    order::Order order;
    if (!orders_.findOrder(orderId, order, error)) return false;
    if (order.status != order::OrderStatus::Paid) return fail(error, "Order " + orderId + " is not paid.");

    std::vector<Bill> existing;
    if (!bills_.loadAll(existing, error)) return false;
    const auto old = std::find_if(existing.begin(), existing.end(), [&](const Bill& b) { return b.orderId == orderId; });
    if (old == existing.end()) return fail(error, "Order " + orderId + " has no bill to update.");

    Bill result;
    if (!preview(orderId, result, error)) return false;
    const double seasonal = common::roundMoney(adjustments.seasonalDiscount);
    const double special = common::roundMoney(adjustments.specialDiscount);
    if (seasonal < 0 || special < 0) return fail(error, "Seasonal and special discounts cannot be negative.");
    if (seasonal + special > result.totals.payable + 0.005)
        return fail(error, "Seasonal and special discounts are larger than the bill.");
    result.seasonalDiscount = seasonal;
    result.specialDiscount = special;
    result.totals.payable = common::roundMoney(result.totals.payable - seasonal - special);

    const IPaymentMethod* method = methods_.find(payment.mode);
    if (!method) return fail(error, "This payment mode is not available.");
    const PaymentResult paid = method->process(payment, result.totals.payable);
    if (!paid.ok) return fail(error, paid.error);

    result.id = old->id;
    result.date = old->date;
    result.time = old->time;
    result.amountReceived = payment.mode == order::PaymentMode::Cash ? common::roundMoney(payment.amountTendered)
                                                                     : result.totals.payable;
    result.balanceReturned = common::roundMoney(paid.change);

    order::Order paidOrder = order;
    paidOrder.paymentAmount = result.totals.payable;
    paidOrder.paymentMode = payment.mode;
    paidOrder.paymentMerchant = payment.mode == order::PaymentMode::Cash ? std::string() : payment.merchant;
    if (!orders_.updateOrder(paidOrder, error)) return false;
    result.paymentStatus = order::toString(paidOrder.status);

    if (!bills_.replace(result, error)) {
        std::string ignored;
        orders_.updateOrder(order, &ignored);
        return false;
    }
    bill = std::move(result);
    if (change) *change = paid.change;
    return true;
}

} // namespace barcodehub::billing
