#pragma once

#include <string>
#include <vector>

namespace barcodehub::billing {

// One product on the bill.
struct BillLine {
    std::string productAcronym;
    double unitPrice{0.0};        // PKR, without tax
    double taxPerUnit{0.0};       // PKR; used for the totals, NOT stored in Bill.txt
    double discountPerUnit{0.0};  // PKR
    double quantity{0.0};         // count or weight
};

struct BillTotals {
    double withoutTax{0.0};
    double tax{0.0};
    double discount{0.0};
    double payable{0.0};
};

struct Bill {
    std::string id;              // 9 digits: YYMMDD + daily sequence (3 digits)
    std::string customerName;
    std::string orderId;
    std::string paymentStatus;   // the order's status from Order.txt, e.g. "paid"
    std::vector<BillLine> lines;
    BillTotals totals;           // totals of the WHOLE bill
};

} // namespace barcodehub::billing
