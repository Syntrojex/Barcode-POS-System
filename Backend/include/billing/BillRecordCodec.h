#pragma once

/**
 * @file BillRecordCodec.h
 * @brief Serialization and deserialization engine for Bill.txt flat-file records.
 * @author Ali Abdullah (25L-3022)
 *
 * Implements bidirectional translation between structured Bill / BillLine objects
 * and raw delimited text lines stored in data/Bill.txt.
 *
 * Line layout specification (12 whitespace-delimited fields ending with '#'):
 *   [0]  billId           (9 digits: YYMMDD###)
 *   [1]  customerName     (encoded string via RecordText)
 *   [2]  orderId          (8-digit order identifier)
 *   [3]  productAcronym   (encoded string, max 8 chars)
 *   [4]  unitPrice        (formatted floating point, PKR)
 *   [5]  discountPerUnit  (formatted floating point, PKR)
 *   [6]  quantity         (formatted floating point)
 *   [7]  totalWithoutTax  (bill subtotal before tax, PKR)
 *   [8]  totalTax         (cumulative bill tax, PKR)
 *   [9]  totalDiscount    (cumulative bill discount, PKR)
 *   [10] totalPayable     (net bill payable, PKR)
 *   [11] paymentStatus    (encoded order status string, e.g. "paid")
 *   [12] "#"              (record terminator delimiter)
 */

#include "billing/Bill.h"

#include <string>

namespace barcodehub::billing {

/**
 * @class BillRecordCodec
 * @brief Stateless encoder and decoder for flat-file bill records.
 *
 * Operates purely on strings and data models without performing disk I/O,
 * facilitating isolated unit testing of file formatting and parsing logic.
 */
class BillRecordCodec {
public:
    /**
     * @brief Serializes a Bill header and a specific BillLine into a compliant Bill.txt line.
     *
     * Encodes text fields safely using common::encodeField, formats currency figures,
     * and appends the mandatory '#' terminal symbol.
     *
     * @param bill The overarching bill containing ID, customer name, totals, and payment status.
     * @param line The specific product line item to serialize.
     * @return Formatted record string ready for writing to disk.
     */
    static std::string format(const Bill& bill, const BillLine& line);

    /**
     * @brief Deserializes a raw record line into a Bill header and BillLine instance.
     *
     * Validates that the line ends with '#', splits tokens, checks for the 12-field contract,
     * verifies that the Bill ID comprises exactly 9 numeric digits, and decodes numeric and text fields.
     *
     * @param[in] record Raw line read from Bill.txt.
     * @param[out] header Output Bill instance populated with global bill metadata and totals.
     * @param[out] line Output BillLine instance populated with the item-specific values.
     * @param[out] error Optional pointer to store error message upon parse failure.
     * @return True if parsing succeeded and fields conform to specifications; false otherwise.
     */
    static bool parse(const std::string& record, Bill& header, BillLine& line,
                      std::string* error = nullptr);
};

} // namespace barcodehub::billing
