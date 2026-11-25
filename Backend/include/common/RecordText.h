#pragma once

#include <string>
#include <vector>

// Small helpers for the '#'-terminated text records used by Order.txt, OrderDetail.txt and Bill.txt.
// One place for the file rules, so every module reads and writes them the same way (reusability).
//   record  = field ' ' field ' ' ... ' ' '#'
//   a text field has ' ' written as '_', and '_' '%' '#' escaped; an empty field is written as '-'
// (the same encoding the Order module uses, so names look identical in every file).
namespace barcodehub::common {

std::string encodeField(const std::string& text);
bool decodeField(const std::string& token, std::string& out);

// Splits one record line into its fields. False when the line does not end with '#'.
bool splitRecord(const std::string& rawLine, std::vector<std::string>& tokens);

bool parseNumber(const std::string& token, double& value);   // whole token, finite numbers only
std::string formatMoney(double value);                        // 2 decimals, '.' always
std::string formatQuantity(double value);                     // 3 decimals, trailing zeros removed
double roundMoney(double value);                              // to 2 decimals

std::string currentDate();                                    // YYYY-MM-DD, local time
std::string currentTime();                                    // HH:MM:SS, local time

} // namespace barcodehub::common
