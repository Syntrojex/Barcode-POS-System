#pragma once

/**
 * @file BillRecordCodec.h
 * @brief Converts bills to and from the text lines of Bill.txt and BillDetail.txt.
 * @author Ali Abdullah (25L-3022); Increment 3 format by Shauzab Mukhtar
 *
 * Bill.txt - one bill per line, 14 space separated fields, ending with '#' (requirement 8):
 *   [0]  billId            9 digits YYMMDD### (unique in the file)
 *   [1]  customerName      text (spaces written as '_')
 *   [2]  orderId           from Order.txt
 *   [3]  totalWithoutTax   PKR
 *   [4]  totalTax          PKR
 *   [5]  totalDiscount     PKR, sum of the discounts on all bought products
 *   [6]  totalPayable      PKR
 *   [7]  amountReceived    PKR
 *   [8]  balanceReturned   PKR
 *   [9]  paymentStatus     order status from Order.txt (e.g. paid)
 *   [10] seasonalDiscount  PKR
 *   [11] specialDiscount   PKR (special discount is at most 8 percent)
 *   [12] billDate          YYYY-MM-DD
 *   [13] billTime          HH:MM:SS
 *
 * BillDetail.txt - one bought product per line, 5 fields, ending with '#' (requirement 9):
 *   billId productAcronym unitPriceWithoutTax quantity lineDiscount
 *   (lineDiscount = discount per unit x quantity: 10 products with 5 off each -> 50)
 *
 * Old Increment 2 Bill.txt lines (12 fields, one line per product) can still be read with parseLegacy(),
 * so bills saved before the new format do not break the app.
 */

#include "billing/Bill.h"

#include <string>

namespace barcodehub::billing {

/**
 * @class BillRecordCodec
 * @brief Stateless encoder / decoder for bill records (no file I/O, so it is easy to test).
 */
class BillRecordCodec {
public:
    /** @brief One Bill.txt line (14 fields + '#') for the bill header and totals. */
    static std::string format(const Bill& bill);

    /** @brief Parses one 14-field Bill.txt line into @p bill (its lines are left empty). */
    static bool parse(const std::string& record, Bill& bill, std::string* error = nullptr);

    /** @brief One BillDetail.txt line for one product of the bill. */
    static std::string formatDetail(const std::string& billId, const BillLine& line);

    /** @brief Parses one BillDetail.txt line; discountPerUnit is lineDiscount / quantity. */
    static bool parseDetail(const std::string& record, std::string& billId, BillLine& line,
                            std::string* error = nullptr);

    /** @brief Reads an old Increment 2 Bill.txt line (12 fields: bill header + one product). */
    static bool parseLegacy(const std::string& record, Bill& header, BillLine& line,
                            std::string* error = nullptr);

    /** @brief Number of fields in the line (ignoring the '#'); -1 if it does not end with '#'. */
    static int fieldCount(const std::string& record);
};

} // namespace barcodehub::billing
