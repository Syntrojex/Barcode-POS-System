#include "billing/BillRecordCodec.h"

#include "common/RecordText.h"

#include <algorithm>
#include <cctype>
#include <vector>

namespace barcodehub::billing {
namespace {

bool fail(std::string* error, const std::string& why) {
    if (error) *error = why;
    return false;
}

bool isBillId(const std::string& s) {
    return s.size() == 9 &&
           std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
}

bool isDate(const std::string& s) {   // YYYY-MM-DD
    return s.size() == 10 && s[4] == '-' && s[7] == '-' &&
           std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c) || c == '-'; });
}

bool isTime(const std::string& s) {   // HH:MM:SS
    return s.size() == 8 && s[2] == ':' && s[5] == ':' &&
           std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c) || c == ':'; });
}

} // namespace

int BillRecordCodec::fieldCount(const std::string& record) {
    std::vector<std::string> t;
    if (!common::splitRecord(record, t)) return -1;
    return static_cast<int>(t.size());
}

// ------------------------------------------------------------------------------- Bill.txt
std::string BillRecordCodec::format(const Bill& b) {
    using common::encodeField;
    using common::formatMoney;
    return b.id + ' ' +
           encodeField(b.customerName) + ' ' +
           b.orderId + ' ' +
           formatMoney(b.totals.withoutTax) + ' ' +
           formatMoney(b.totals.tax) + ' ' +
           formatMoney(b.totals.discount) + ' ' +
           formatMoney(b.totals.payable) + ' ' +
           formatMoney(b.amountReceived) + ' ' +
           formatMoney(b.balanceReturned) + ' ' +
           encodeField(b.paymentStatus) + ' ' +
           formatMoney(b.seasonalDiscount) + ' ' +
           formatMoney(b.specialDiscount) + ' ' +
           b.date + ' ' +
           b.time + " #";
}

bool BillRecordCodec::parse(const std::string& record, Bill& out, std::string* error) {
    std::vector<std::string> t;
    if (!common::splitRecord(record, t)) return fail(error, "record must end with '#'.");
    if (t.size() != 14) return fail(error, "expected 14 fields, found " + std::to_string(t.size()) + ".");
    if (!isBillId(t[0])) return fail(error, "Bill ID must contain exactly 9 digits.");

    Bill b;
    b.id = t[0];
    b.orderId = t[2];
    if (!common::decodeField(t[1], b.customerName) || !common::decodeField(t[9], b.paymentStatus))
        return fail(error, "bad text encoding.");
    if (!common::parseNumber(t[3], b.totals.withoutTax) || !common::parseNumber(t[4], b.totals.tax) ||
        !common::parseNumber(t[5], b.totals.discount) || !common::parseNumber(t[6], b.totals.payable) ||
        !common::parseNumber(t[7], b.amountReceived) || !common::parseNumber(t[8], b.balanceReturned) ||
        !common::parseNumber(t[10], b.seasonalDiscount) || !common::parseNumber(t[11], b.specialDiscount))
        return fail(error, "a numeric field is not a number.");
    if (!isDate(t[12])) return fail(error, "bill date must be YYYY-MM-DD.");
    if (!isTime(t[13])) return fail(error, "bill time must be HH:MM:SS.");
    b.date = t[12];
    b.time = t[13];
    out = b;
    return true;
}

// ------------------------------------------------------------------------- BillDetail.txt
std::string BillRecordCodec::formatDetail(const std::string& billId, const BillLine& line) {
    using common::formatMoney;
    return billId + ' ' +
           common::encodeField(line.productAcronym) + ' ' +
           formatMoney(line.unitPrice) + ' ' +
           common::formatQuantity(line.quantity) + ' ' +
           formatMoney(common::roundMoney(line.discountPerUnit * line.quantity)) + " #";
}

bool BillRecordCodec::parseDetail(const std::string& record, std::string& billId, BillLine& line,
                                  std::string* error) {
    std::vector<std::string> t;
    if (!common::splitRecord(record, t)) return fail(error, "record must end with '#'.");
    if (t.size() != 5) return fail(error, "expected 5 fields, found " + std::to_string(t.size()) + ".");
    if (!isBillId(t[0])) return fail(error, "Bill ID must contain exactly 9 digits.");

    BillLine l;
    double lineDiscount = 0;
    if (!common::decodeField(t[1], l.productAcronym)) return fail(error, "bad text encoding.");
    if (!common::parseNumber(t[2], l.unitPrice) || !common::parseNumber(t[3], l.quantity) ||
        !common::parseNumber(t[4], lineDiscount))
        return fail(error, "a numeric field is not a number.");
    if (l.quantity <= 0) return fail(error, "quantity must be greater than zero.");
    l.discountPerUnit = lineDiscount / l.quantity;
    billId = t[0];
    line = l;
    return true;
}

// ------------------------------------------------------------- old Increment 2 Bill.txt
bool BillRecordCodec::parseLegacy(const std::string& record, Bill& header, BillLine& line,
                                  std::string* error) {
    std::vector<std::string> t;
    if (!common::splitRecord(record, t)) return fail(error, "record must end with '#'.");
    if (t.size() != 12) return fail(error, "expected 12 fields, found " + std::to_string(t.size()) + ".");
    if (!isBillId(t[0])) return fail(error, "Bill ID must contain exactly 9 digits.");

    Bill h;
    BillLine l;
    h.id = t[0];
    h.orderId = t[2];
    if (!common::decodeField(t[1], h.customerName) || !common::decodeField(t[3], l.productAcronym) ||
        !common::decodeField(t[11], h.paymentStatus))
        return fail(error, "bad text encoding.");
    if (!common::parseNumber(t[4], l.unitPrice) || !common::parseNumber(t[5], l.discountPerUnit) ||
        !common::parseNumber(t[6], l.quantity) || !common::parseNumber(t[7], h.totals.withoutTax) ||
        !common::parseNumber(t[8], h.totals.tax) || !common::parseNumber(t[9], h.totals.discount) ||
        !common::parseNumber(t[10], h.totals.payable))
        return fail(error, "a numeric field is not a number.");
    h.amountReceived = h.totals.payable; // not stored in the old format
    header = h;
    line = l;
    return true;
}

} // namespace barcodehub::billing
