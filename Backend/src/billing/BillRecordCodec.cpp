#include "billing/BillRecordCodec.h"

#include "common/RecordText.h"

#include <algorithm>
#include <cctype>
#include <vector>

namespace barcodehub::billing {

std::string BillRecordCodec::format(const Bill& bill, const BillLine& line) {
    using common::encodeField;
    using common::formatMoney;
    using common::formatQuantity;
    return bill.id + ' ' +
           encodeField(bill.customerName) + ' ' +
           bill.orderId + ' ' +
           encodeField(line.productAcronym) + ' ' +
           formatMoney(line.unitPrice) + ' ' +
           formatMoney(line.discountPerUnit) + ' ' +
           formatQuantity(line.quantity) + ' ' +
           formatMoney(bill.totals.withoutTax) + ' ' +
           formatMoney(bill.totals.tax) + ' ' +
           formatMoney(bill.totals.discount) + ' ' +
           formatMoney(bill.totals.payable) + ' ' +
           encodeField(bill.paymentStatus) + " #";
}

bool BillRecordCodec::parse(const std::string& record, Bill& header, BillLine& line,
                            std::string* error) {
    const auto fail = [&](const std::string& why) {
        if (error) *error = why;
        return false;
    };

    std::vector<std::string> t;
    if (!common::splitRecord(record, t)) return fail("record must end with '#'.");
    if (t.size() != 12) return fail("expected 12 fields, found " + std::to_string(t.size()) + ".");

    const bool nineDigits = t[0].size() == 9 &&
        std::all_of(t[0].begin(), t[0].end(), [](unsigned char c) { return std::isdigit(c) != 0; });
    if (!nineDigits) return fail("Bill ID must contain exactly 9 digits.");

    Bill h;
    BillLine l;
    h.id = t[0];
    h.orderId = t[2];
    if (!common::decodeField(t[1], h.customerName) ||
        !common::decodeField(t[3], l.productAcronym) ||
        !common::decodeField(t[11], h.paymentStatus)) return fail("bad text encoding.");

    if (!common::parseNumber(t[4], l.unitPrice) || !common::parseNumber(t[5], l.discountPerUnit) ||
        !common::parseNumber(t[6], l.quantity) || !common::parseNumber(t[7], h.totals.withoutTax) ||
        !common::parseNumber(t[8], h.totals.tax) || !common::parseNumber(t[9], h.totals.discount) ||
        !common::parseNumber(t[10], h.totals.payable)) return fail("a numeric field is not a number.");

    header = h;
    line = l;
    return true;
}

} // namespace barcodehub::billing
