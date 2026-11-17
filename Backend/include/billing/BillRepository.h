#pragma once

#include "billing/Bill.h"

#include <filesystem>
#include <string>
#include <vector>

namespace barcodehub::billing {

// Reads and writes Bill.txt (file access only; the layout of one line lives in BillRecordCodec).
class BillRepository {
public:
    explicit BillRepository(std::filesystem::path billFile);

    // Lines that share a Bill ID are joined into one Bill. A missing file means "no bills yet".
    bool loadAll(std::vector<Bill>& bills, std::string* error = nullptr) const;

    // Adds one bill. The file is rewritten through a temporary file, so a crash cannot damage it.
    bool append(const Bill& bill, std::string* error = nullptr);

    // 9-digit ID: YYMMDD of `date` (YYYY-MM-DD) + daily sequence 001-999.
    static std::string nextBillId(const std::string& date, const std::vector<Bill>& existing,
                                  std::string* error = nullptr);

private:
    std::filesystem::path file_;
};

} // namespace barcodehub::billing
