#include "billing/BillRepository.h"

#include "billing/BillRecordCodec.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace barcodehub::billing {

BillRepository::BillRepository(std::filesystem::path billFile) : file_(std::move(billFile)) {}

bool BillRepository::loadAll(std::vector<Bill>& bills, std::string* error) const {
    bills.clear();
    std::error_code ec;
    if (!std::filesystem::exists(file_, ec)) return true;

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

        Bill header;
        BillLine line;
        std::string why;
        if (!BillRecordCodec::parse(text, header, line, &why)) {
            if (error) *error = file_.filename().string() + " line " + std::to_string(lineNo) + ": " + why;
            return false;
        }
        const auto same = std::find_if(bills.begin(), bills.end(),
                                       [&](const Bill& b) { return b.id == header.id; });
        if (same == bills.end()) {
            header.lines.push_back(line);
            bills.push_back(std::move(header));
        } else {
            same->lines.push_back(line);
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

    std::error_code ec;
    if (!file_.parent_path().empty()) std::filesystem::create_directories(file_.parent_path(), ec);
    const auto temp = file_.string() + ".tmp";
    {
        std::ofstream out(temp, std::ios::trunc);
        if (!out) {
            if (error) *error = "Cannot write temporary bill file: " + temp;
            return false;
        }
        for (const auto& b : all)
            for (const auto& line : b.lines) out << BillRecordCodec::format(b, line) << '\n';
        out.flush();
        if (!out) {
            if (error) *error = "Failed while writing bills (disk full or no permission?).";
            std::filesystem::remove(temp, ec);
            return false;
        }
    }
    std::filesystem::rename(temp, file_, ec);
    if (ec) {   // some platforms refuse to rename over an existing file
        ec.clear();
        std::filesystem::copy_file(temp, file_, std::filesystem::copy_options::overwrite_existing, ec);
        std::error_code ignored;
        std::filesystem::remove(temp, ignored);
        if (ec) {
            if (error) *error = "Cannot replace bill file: " + ec.message();
            return false;
        }
    }
    return true;
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
