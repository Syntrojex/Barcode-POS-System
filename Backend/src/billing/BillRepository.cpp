#include "billing/BillRepository.h"

#include "billing/BillRecordCodec.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace barcodehub::billing {

namespace {

// Writes `lines` to `path` through a temporary file, then swaps it in (no half-written file on a crash).
bool writeAtomically(const std::filesystem::path& path, const std::vector<std::string>& lines, std::string* error) {
    std::error_code ec;
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path(), ec);
    const auto temp = path.string() + ".tmp";
    {
        std::ofstream out(temp, std::ios::trunc);
        if (!out) {
            if (error) *error = "Cannot write temporary file: " + temp;
            return false;
        }
        for (const auto& line : lines) out << line << '\n';
        out.flush();
        if (!out) {
            if (error) *error = "Failed while writing " + path.filename().string() + " (disk full or no permission?).";
            std::filesystem::remove(temp, ec);
            return false;
        }
    }
    std::filesystem::rename(temp, path, ec);
    if (ec) {   // some platforms refuse to rename over an existing file
        ec.clear();
        std::filesystem::copy_file(temp, path, std::filesystem::copy_options::overwrite_existing, ec);
        std::error_code ignored;
        std::filesystem::remove(temp, ignored);
        if (ec) {
            if (error) *error = "Cannot replace " + path.filename().string() + ": " + ec.message();
            return false;
        }
    }
    return true;
}

} // namespace

BillRepository::BillRepository(std::filesystem::path billFile, std::filesystem::path detailFile)
    : file_(std::move(billFile)),
      detailFile_(detailFile.empty() ? file_.parent_path() / "BillDetail.txt" : std::move(detailFile)) {}

bool BillRepository::loadAll(std::vector<Bill>& bills, std::string* error) const {
    bills.clear();
    std::error_code ec;
    auto findBill = [&](const std::string& id) {
        return std::find_if(bills.begin(), bills.end(), [&](const Bill& b) { return b.id == id; });
    };

    // 1. Bill.txt: one line per bill (old Increment 2 files have one line per product instead)
    if (std::filesystem::exists(file_, ec)) {
        std::ifstream in(file_);
        if (!in) {
            if (error) *error = "Cannot open bill file: " + file_.string();
            return false;
        }
        std::string text;
        int lineNo = 0;
        while (std::getline(in, text)) {
            ++lineNo;
            if (text.find_first_not_of(" \t\r\n") == std::string::npos) continue;   // blank line
            std::string why;
            const auto where = file_.filename().string() + " line " + std::to_string(lineNo) + ": ";
            if (BillRecordCodec::fieldCount(text) == 12) {   // old format: header + one product
                Bill header;
                BillLine line;
                if (!BillRecordCodec::parseLegacy(text, header, line, &why)) {
                    if (error) *error = where + why;
                    return false;
                }
                // the old format has no date/time: take the date from the bill id (YYMMDD###)
                header.date = "20" + header.id.substr(0, 2) + "-" + header.id.substr(2, 2) + "-" + header.id.substr(4, 2);
                header.time = "00:00:00";
                const auto same = findBill(header.id);
                if (same == bills.end()) {
                    header.lines.push_back(line);
                    bills.push_back(std::move(header));
                } else {
                    same->lines.push_back(line);
                }
                continue;
            }
            Bill bill;
            if (!BillRecordCodec::parse(text, bill, &why)) {
                if (error) *error = where + why;
                return false;
            }
            if (findBill(bill.id) != bills.end()) {
                if (error) *error = where + "Bill ID " + bill.id + " appears twice.";
                return false;
            }
            bills.push_back(std::move(bill));
        }
    }

    // 2. BillDetail.txt: the products of each bill
    if (std::filesystem::exists(detailFile_, ec)) {
        std::ifstream in(detailFile_);
        if (!in) {
            if (error) *error = "Cannot open bill detail file: " + detailFile_.string();
            return false;
        }
        std::string text;
        int lineNo = 0;
        while (std::getline(in, text)) {
            ++lineNo;
            if (text.find_first_not_of(" \t\r\n") == std::string::npos) continue;
            std::string billId, why;
            BillLine line;
            if (!BillRecordCodec::parseDetail(text, billId, line, &why)) {
                if (error) *error = detailFile_.filename().string() + " line " + std::to_string(lineNo) + ": " + why;
                return false;
            }
            const auto owner = findBill(billId);
            if (owner != bills.end()) owner->lines.push_back(line);   // lines of unknown bills are ignored
        }
    }
    return true;
}

bool BillRepository::append(const Bill& bill, std::string* error) {
    if (bill.lines.empty()) {
        if (error) *error = "Cannot save a bill without products.";
        return false;
    }
    std::vector<Bill> all;
    if (!loadAll(all, error)) return false;
    for (const auto& existing : all) {
        if (existing.id == bill.id) {
            if (error) *error = "Bill ID " + bill.id + " already exists.";
            return false;
        }
    }
    all.push_back(bill);

    // Both files are rewritten in the Increment 3 format (old bills are converted on the way).
    std::vector<std::string> billLines, detailLines;
    for (const auto& b : all) {
        billLines.push_back(BillRecordCodec::format(b));
        for (const auto& line : b.lines) detailLines.push_back(BillRecordCodec::formatDetail(b.id, line));
    }
    return writeAtomically(detailFile_, detailLines, error) && writeAtomically(file_, billLines, error);
}

std::string BillRepository::nextBillId(const std::string& date, const std::vector<Bill>& existing,
                                       std::string* error) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
        if (error) *error = "Cannot generate a Bill ID from an invalid date.";
        return {};
    }
    const std::string prefix = date.substr(2, 2) + date.substr(5, 2) + date.substr(8, 2);   // YYMMDD
    int highest = 0;
    for (const auto& bill : existing) {
        if (bill.id.size() == 9 && bill.id.compare(0, 6, prefix) == 0) {
            try {
                highest = std::max(highest, std::stoi(bill.id.substr(6, 3)));
            } catch (...) {
            }
        }
    }
    if (highest >= 999) {
        if (error) *error = "Daily Bill ID limit reached (999 bills for this date).";
        return {};
    }
    std::ostringstream id;
    id << prefix << std::setw(3) << std::setfill('0') << (highest + 1);
    return id.str();
}

} // namespace barcodehub::billing
