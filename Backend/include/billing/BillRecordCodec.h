#pragma once

#include "billing/Bill.h"

#include <string>

namespace barcodehub::billing {

// Converts ONE Bill.txt line <-> data. It knows the file layout and nothing else
// (no file access, no rules). One line = one product of the bill:
//   billId customer orderId acronym unitPrice discountPerUnit quantity
//   totalWithoutTax totalTax totalDiscount totalPayable status #
// The five totals and the status describe the whole bill, so every line of a bill repeats them.
class BillRecordCodec {
public:
    static std::string format(const Bill& bill, const BillLine& line);

    // Fills the bill header (id, customer, order, totals, status) and one line.
    static bool parse(const std::string& record, Bill& header, BillLine& line,
                      std::string* error = nullptr);
};

} // namespace barcodehub::billing
