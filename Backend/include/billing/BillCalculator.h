#pragma once

#include "billing/Bill.h"

#include <string>
#include <vector>

namespace barcodehub::billing {

// Pure arithmetic, nothing else (no files, no GUI): easy to test and to reuse.
// payable = total without tax + total tax - total discount (same rule as the Order module).
class BillCalculator {
public:
    static bool calculate(const std::vector<BillLine>& lines, BillTotals& totals,
                          std::string* error = nullptr);
};

} // namespace barcodehub::billing
