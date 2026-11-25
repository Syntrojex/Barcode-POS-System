/**
 * @file billing_tests.cpp
 * @brief Comprehensive unit test suite for the BarcodeHub Billing subsystem.
 * @author Ali Abdullah (25L-3022)
 *
 * Verifies calculation accuracy, monetary rounding, error validation,
 * payment strategy processing, record codec serialization, and repository persistence.
 */

#include <fstream>
#include "billing/Bill.h"
#include "billing/BillCalculator.h"
#include "billing/BillRecordCodec.h"
#include "billing/BillRepository.h"
#include "billing/PaymentMethod.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace barcodehub::billing;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[-] ASSERTION FAILED: " << msg \
                      << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            return false; \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        std::cout << "[RUN] " #fn "... "; \
        if (fn()) { \
            std::cout << "PASSED\n"; \
            ++passedTests; \
        } else { \
            std::cout << "FAILED\n"; \
            ++failedTests; \
        } \
        ++totalTests; \
    } while (0)

namespace {

bool doubleNear(double a, double b, double epsilon = 0.001) {
    return std::abs(a - b) <= epsilon;
}

// ============================================================
// BILL CALCULATOR TESTS
// ============================================================

bool testCalculatorSingleItem() {
    std::vector<BillLine> lines = {
        {"MILK", 100.0, 17.0, 5.0, 2.0} // 2 units: subtotal 200, tax 34, discount 10 -> payable 224
    };

    BillTotals totals;
    std::string error;
    TEST_ASSERT(BillCalculator::calculate(lines, totals, &error), "Single item calculation failed: " + error);
    TEST_ASSERT(doubleNear(totals.withoutTax, 200.0), "Subtotal mismatch");
    TEST_ASSERT(doubleNear(totals.tax, 34.0), "Tax mismatch");
    TEST_ASSERT(doubleNear(totals.discount, 10.0), "Discount mismatch");
    TEST_ASSERT(doubleNear(totals.payable, 224.0), "Payable mismatch");
    return true;
}

bool testCalculatorMultipleItems() {
    std::vector<BillLine> lines = {
        {"BREAD", 150.0, 0.0, 10.0, 1.0},   // subtotal 150, tax 0, disc 10
        {"EGG", 25.0, 2.5, 0.0, 12.0},      // subtotal 300, tax 30, disc 0
        {"APPLE", 200.0, 10.0, 5.0, 1.5}    // subtotal 300, tax 15, disc 7.5
    };

    BillTotals totals;
    std::string error;
    TEST_ASSERT(BillCalculator::calculate(lines, totals, &error), "Multi-item calculation failed: " + error);
    TEST_ASSERT(doubleNear(totals.withoutTax, 750.0), "Multi-item subtotal mismatch");
    TEST_ASSERT(doubleNear(totals.tax, 45.0), "Multi-item tax mismatch");
    TEST_ASSERT(doubleNear(totals.discount, 17.5), "Multi-item discount mismatch");
    TEST_ASSERT(doubleNear(totals.payable, 777.5), "Multi-item payable mismatch");
    return true;
}

bool testCalculatorRoundingPrecision() {
    std::vector<BillLine> lines = {
        {"RICE", 33.3333, 1.6666, 0.5555, 3.0}
    };

    BillTotals totals;
    std::string error;
    TEST_ASSERT(BillCalculator::calculate(lines, totals, &error), "Rounding test failed: " + error);
    TEST_ASSERT(doubleNear(totals.withoutTax, 100.0, 0.01), "Rounded withoutTax mismatch");
    TEST_ASSERT(doubleNear(totals.tax, 5.0, 0.01), "Rounded tax mismatch");
    TEST_ASSERT(doubleNear(totals.discount, 1.67, 0.01), "Rounded discount mismatch");
    TEST_ASSERT(doubleNear(totals.payable, 103.33, 0.01), "Rounded payable mismatch");
    return true;
}

bool testCalculatorEmptyLines() {
    std::vector<BillLine> lines;
    BillTotals totals;
    std::string error;
    TEST_ASSERT(!BillCalculator::calculate(lines, totals, &error), "Empty bill should fail");
    TEST_ASSERT(error.find("at least one product") != std::string::npos, "Unexpected error: " + error);
    return true;
}

bool testCalculatorNegativeValues() {
    BillTotals totals;
    std::string error;

    std::vector<BillLine> negPrice = {{"ITEM", -50.0, 5.0, 0.0, 1.0}};
    TEST_ASSERT(!BillCalculator::calculate(negPrice, totals, &error), "Negative price should fail");

    std::vector<BillLine> negTax = {{"ITEM", 50.0, -5.0, 0.0, 1.0}};
    TEST_ASSERT(!BillCalculator::calculate(negTax, totals, &error), "Negative tax should fail");

    std::vector<BillLine> negDisc = {{"ITEM", 50.0, 5.0, -2.0, 1.0}};
    TEST_ASSERT(!BillCalculator::calculate(negDisc, totals, &error), "Negative discount should fail");

    std::vector<BillLine> zeroQty = {{"ITEM", 50.0, 5.0, 0.0, 0.0}};
    TEST_ASSERT(!BillCalculator::calculate(zeroQty, totals, &error), "Zero quantity should fail");

    std::vector<BillLine> negQty = {{"ITEM", 50.0, 5.0, 0.0, -3.0}};
    TEST_ASSERT(!BillCalculator::calculate(negQty, totals, &error), "Negative quantity should fail");

    return true;
}

bool testCalculatorExcessiveDiscount() {
    std::vector<BillLine> lines = {
        {"COFFEE", 100.0, 0.0, 150.0, 1.0}
    };

    BillTotals totals;
    std::string error;
    TEST_ASSERT(!BillCalculator::calculate(lines, totals, &error), "Excessive discount should be rejected");
    TEST_ASSERT(error.find("Discount is larger") != std::string::npos, "Unexpected error: " + error);
    return true;
}

bool testCalculatorNonFiniteValues() {
    BillTotals totals;
    std::string error;
    double nanVal = std::numeric_limits<double>::quiet_NaN();
    double infVal = std::numeric_limits<double>::infinity();

    std::vector<BillLine> nanLine = {{"NAN_ITEM", nanVal, 0.0, 0.0, 1.0}};
    TEST_ASSERT(!BillCalculator::calculate(nanLine, totals, &error), "NaN price should be rejected");

    std::vector<BillLine> infLine = {{"INF_ITEM", 10.0, infVal, 0.0, 1.0}};
    TEST_ASSERT(!BillCalculator::calculate(infLine, totals, &error), "Infinity tax should be rejected");

    return true;
}

// ============================================================
// PAYMENT STRATEGY TESTS
// ============================================================

bool testCashPaymentExact() {
    CashPayment cash;
    PaymentRequest req;
    req.mode = barcodehub::order::PaymentMode::Cash;
    req.amountTendered = 500.0;

    PaymentResult res = cash.process(req, 500.0);
    TEST_ASSERT(res.ok, "Exact cash should succeed");
    TEST_ASSERT(doubleNear(res.change, 0.0), "Change should be 0 for exact payment");
    return true;
}

bool testCashPaymentWithChange() {
    CashPayment cash;
    PaymentRequest req;
    req.mode = barcodehub::order::PaymentMode::Cash;
    req.amountTendered = 1000.0;

    PaymentResult res = cash.process(req, 777.50);
    TEST_ASSERT(res.ok, "Overpayment should succeed");
    TEST_ASSERT(doubleNear(res.change, 222.50), "Change should be 222.50");
    return true;
}

bool testCashPaymentInsufficient() {
    CashPayment cash;
    PaymentRequest req;
    req.mode = barcodehub::order::PaymentMode::Cash;
    req.amountTendered = 450.0;

    PaymentResult res = cash.process(req, 500.0);
    TEST_ASSERT(!res.ok, "Insufficient cash should be rejected");
    TEST_ASSERT(res.error.find("less than the amount payable") != std::string::npos,
                "Unexpected error message: " + res.error);

    // Negative cash tendered
    req.amountTendered = -50.0;
    PaymentResult resNeg = cash.process(req, 500.0);
    TEST_ASSERT(!resNeg.ok, "Negative cash tendered must be rejected");
    TEST_ASSERT(resNeg.error.find("Invalid cash amount") != std::string::npos,
                "Unexpected error message: " + resNeg.error);
    return true;
}

bool testCardPaymentValid() {
    CardPayment card;
    PaymentRequest req;
    req.mode = barcodehub::order::PaymentMode::Card;
    req.merchant = "HBL";

    PaymentResult res = card.process(req, 1250.0);
    TEST_ASSERT(res.ok, "Card payment with valid bank should succeed");
    TEST_ASSERT(doubleNear(res.change, 0.0), "Card payment change should be zero");
    return true;
}

bool testCardPaymentMissingBank() {
    CardPayment card;
    PaymentRequest req;
    req.mode = barcodehub::order::PaymentMode::Card;
    req.merchant = "";

    PaymentResult res = card.process(req, 1250.0);
    TEST_ASSERT(!res.ok, "Card payment without bank name should fail");
    TEST_ASSERT(res.error.find("bank") != std::string::npos, "Unexpected error: " + res.error);

    // Whitespace-only name should also fail
    req.merchant = "   \t  ";
    PaymentResult resWs = card.process(req, 1250.0);
    TEST_ASSERT(!resWs.ok, "Card payment with whitespace bank name should fail");
    return true;
}

bool testCardPaymentInvalidCharacter() {
    CardPayment card;
    PaymentRequest req;
    req.mode = barcodehub::order::PaymentMode::Card;
    req.merchant = "Bank#Branch"; // '#' is forbidden delimiter

    PaymentResult res = card.process(req, 1250.0);
    TEST_ASSERT(!res.ok, "Merchant with '#' delimiter should be rejected");
    return true;
}

bool testWalletPaymentValid() {
    WalletPayment wallet;
    PaymentRequest req;
    req.mode = barcodehub::order::PaymentMode::Wallet;
    req.merchant = "JazzCash";

    PaymentResult res = wallet.process(req, 350.0);
    TEST_ASSERT(res.ok, "Wallet payment with valid provider should succeed");
    return true;
}

bool testPaymentRegistryDefaults() {
    PaymentMethodRegistry reg = PaymentMethodRegistry::withDefaults();
    TEST_ASSERT(reg.find(barcodehub::order::PaymentMode::Cash) != nullptr, "Cash method must be registered");
    TEST_ASSERT(reg.find(barcodehub::order::PaymentMode::Card) != nullptr, "Card method must be registered");
    TEST_ASSERT(reg.find(barcodehub::order::PaymentMode::Wallet) != nullptr, "Wallet method must be registered");
    return true;
}

// ============================================================
// BILL RECORD CODEC TESTS
// ============================================================

bool testCodecFormatAndParse() {
    // Bill.txt: one line per bill, 14 fields (requirement 8)
    Bill bill;
    bill.id = "261118001";
    bill.customerName = "Ali Abdullah";
    bill.orderId = "26111818";
    bill.paymentStatus = "paid";
    bill.totals = {500.0, 50.0, 25.0, 475.0};
    bill.amountReceived = 500.0;
    bill.balanceReturned = 25.0;
    bill.seasonalDiscount = 40.0;
    bill.specialDiscount = 10.0;
    bill.date = "2026-11-18";
    bill.time = "14:05:09";

    const std::string formatted = BillRecordCodec::format(bill);
    TEST_ASSERT(formatted == "261118001 Ali_Abdullah 26111818 500.00 50.00 25.00 475.00 500.00 25.00 paid 40.00 10.00 2026-11-18 14:05:09 #",
                "Unexpected Bill.txt line: " + formatted);

    Bill parsed;
    std::string error;
    TEST_ASSERT(BillRecordCodec::parse(formatted, parsed, &error), "Parse failed: " + error);
    TEST_ASSERT(parsed.id == "261118001" && parsed.customerName == "Ali Abdullah", "Parsed id/customer mismatch");
    TEST_ASSERT(parsed.orderId == "26111818" && parsed.paymentStatus == "paid", "Parsed order/status mismatch");
    TEST_ASSERT(doubleNear(parsed.totals.payable, 475.0) && doubleNear(parsed.totals.discount, 25.0), "Parsed totals mismatch");
    TEST_ASSERT(doubleNear(parsed.amountReceived, 500.0) && doubleNear(parsed.balanceReturned, 25.0), "Parsed received/balance mismatch");
    TEST_ASSERT(doubleNear(parsed.seasonalDiscount, 40.0) && doubleNear(parsed.specialDiscount, 10.0), "Parsed seasonal/special mismatch");
    TEST_ASSERT(parsed.date == "2026-11-18" && parsed.time == "14:05:09", "Parsed date/time mismatch");

    // BillDetail.txt: line discount = per-unit discount x quantity (requirement 9: 10 x 5 = 50)
    BillLine line;
    line.productAcronym = "MILK";
    line.unitPrice = 250.0;
    line.discountPerUnit = 5.0;
    line.quantity = 10.0;
    const std::string detail = BillRecordCodec::formatDetail("261118001", line);
    TEST_ASSERT(detail == "261118001 MILK 250.00 10 50.00 #", "Unexpected BillDetail.txt line: " + detail);
    std::string billId;
    BillLine back;
    TEST_ASSERT(BillRecordCodec::parseDetail(detail, billId, back, &error), "Detail parse failed: " + error);
    TEST_ASSERT(billId == "261118001" && back.productAcronym == "MILK", "Detail id/acronym mismatch");
    TEST_ASSERT(doubleNear(back.quantity, 10.0) && doubleNear(back.discountPerUnit, 5.0), "Detail qty/discount mismatch");

    // old Increment 2 line (12 fields) can still be read
    Bill oldHeader;
    BillLine oldLine;
    TEST_ASSERT(BillRecordCodec::parseLegacy("261118001 Ali 26111818 MILK 250 12.5 2 500 50 25 525 paid #",
                                             oldHeader, oldLine, &error), "Legacy parse failed: " + error);
    TEST_ASSERT(doubleNear(oldHeader.totals.payable, 525.0) && doubleNear(oldLine.quantity, 2.0), "Legacy values mismatch");
    return true;
}

bool testCodecParseErrors() {
    Bill header;
    std::string error;
    const std::string good = "261118001 Ali 26111818 500 50 25 475 500 25 paid 40 10 2026-11-18 14:05:09";

    TEST_ASSERT(!BillRecordCodec::parse(good, header, &error), "Missing '#' must fail");
    TEST_ASSERT(error.find("end with '#'") != std::string::npos, "Unexpected error: " + error);

    TEST_ASSERT(!BillRecordCodec::parse("261118001 Ali 26111818 500 #", header, &error), "Short record must fail");
    TEST_ASSERT(error.find("expected 14 fields") != std::string::npos, "Unexpected error: " + error);

    TEST_ASSERT(!BillRecordCodec::parse("26111801" + good.substr(9) + " #", header, &error), "8-digit bill ID must fail");
    TEST_ASSERT(error.find("exactly 9 digits") != std::string::npos, "Unexpected error: " + error);

    TEST_ASSERT(!BillRecordCodec::parse("261118001 Ali 26111818 X 50 25 475 500 25 paid 40 10 2026-11-18 14:05:09 #", header, &error),
                "Non-numeric total must fail");
    TEST_ASSERT(error.find("not a number") != std::string::npos, "Unexpected error: " + error);

    TEST_ASSERT(!BillRecordCodec::parse("261118001 Ali 26111818 500 50 25 475 500 25 paid 40 10 18-11-2026 14:05:09 #", header, &error),
                "Bad date must fail");
    TEST_ASSERT(!BillRecordCodec::parse("261118001 Ali 26111818 500 50 25 475 500 25 paid 40 10 2026-11-18 2pm #", header, &error),
                "Bad time must fail");

    std::string billId;
    BillLine line;
    TEST_ASSERT(!BillRecordCodec::parseDetail("261118001 MILK 250 0 0 #", billId, line, &error), "Zero quantity detail must fail");
    TEST_ASSERT(!BillRecordCodec::parseDetail("261118001 MILK 250 2 #", billId, line, &error), "Short detail must fail");
    return true;
}

// ============================================================
// BILL REPOSITORY TESTS
// ============================================================

bool testRepositoryNextBillId() {
    std::vector<Bill> existing;
    std::string error;

    // 1. First bill of the day
    std::string id1 = BillRepository::nextBillId("2026-11-18", existing, &error);
    TEST_ASSERT(id1 == "261118001", "Expected 261118001, got: " + id1);

    // 2. Sequential increment
    Bill b1;
    b1.id = id1;
    existing.push_back(b1);
    std::string id2 = BillRepository::nextBillId("2026-11-18", existing, &error);
    TEST_ASSERT(id2 == "261118002", "Expected 261118002, got: " + id2);

    // 3. Different date does not interfere
    Bill bPrevDate;
    bPrevDate.id = "261117099"; // previous day had 99 bills
    existing.push_back(bPrevDate);
    std::string idNewDate = BillRepository::nextBillId("2026-11-18", existing, &error);
    TEST_ASSERT(idNewDate == "261118002", "Previous date should not increment current date sequence");

    // 4. Invalid date format
    std::string badDateId = BillRepository::nextBillId("18-11-2026", existing, &error);
    TEST_ASSERT(badDateId.empty(), "Invalid date format should fail");

    return true;
}

bool testRepositoryAppendAndLoad() {
    const fs::path testDir = fs::temp_directory_path() / "barcodehub_billing_unit_test";
    std::error_code ec;
    fs::remove_all(testDir, ec);
    fs::create_directories(testDir, ec);

    const fs::path billFile = testDir / "Bill.txt";
    BillRepository repo(billFile);

    std::vector<Bill> initial;
    std::string error;
    TEST_ASSERT(repo.loadAll(initial, &error), "Load from non-existent file should return empty vector");
    TEST_ASSERT(initial.empty(), "Initial bills must be empty");

    // Create a 2-line bill
    Bill bill;
    bill.id = "261118001";
    bill.customerName = "Ali Abdullah";
    bill.orderId = "26111818";
    bill.paymentStatus = "paid";
    bill.totals = {400.0, 40.0, 20.0, 420.0};
    bill.date = "2026-11-18";
    bill.time = "10:00:00";
    bill.lines = {
        {"MILK", 200.0, 20.0, 10.0, 1.0},
        {"BREAD", 200.0, 20.0, 10.0, 1.0}
    };

    TEST_ASSERT(repo.append(bill, &error), "Append bill failed: " + error);

    // Verify loaded content
    std::vector<Bill> loaded;
    TEST_ASSERT(repo.loadAll(loaded, &error), "Load bills failed: " + error);
    TEST_ASSERT(loaded.size() == 1, "Expected 1 aggregated bill");
    TEST_ASSERT(loaded[0].id == "261118001", "Bill ID mismatch");
    TEST_ASSERT(loaded[0].lines.size() == 2, "Expected 2 lines in aggregated bill");
    TEST_ASSERT(doubleNear(loaded[0].totals.payable, 420.0), "Payable mismatch");

    // Bill.txt has ONE line for the bill; BillDetail.txt has one line per product
    {
        std::ifstream billIn(billFile), detailIn(testDir / "BillDetail.txt");
        int billLines = 0, detailLines = 0;
        std::string t;
        while (std::getline(billIn, t)) ++billLines;
        while (std::getline(detailIn, t)) ++detailLines;
        TEST_ASSERT(billLines == 1, "Bill.txt should have 1 line, has " + std::to_string(billLines));
        TEST_ASSERT(detailLines == 2, "BillDetail.txt should have 2 lines, has " + std::to_string(detailLines));
    }

    // Attempting to append duplicate bill ID should fail
    TEST_ASSERT(!repo.append(bill, &error), "Duplicate bill ID append must be rejected");
    TEST_ASSERT(error.find("already exists") != std::string::npos, "Unexpected error: " + error);

    // Clean up
    fs::remove_all(testDir, ec);
    return true;
}

} // namespace

int main() {
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;

    std::cout << "========================================\n";
    std::cout << " BarcodeHub Billing Subsystem Unit Tests\n";
    std::cout << " Author: Ali Abdullah (25L-3022)\n";
    std::cout << "========================================\n\n";

    // Bill Calculator Tests
    RUN_TEST(testCalculatorSingleItem);
    RUN_TEST(testCalculatorMultipleItems);
    RUN_TEST(testCalculatorRoundingPrecision);
    RUN_TEST(testCalculatorEmptyLines);
    RUN_TEST(testCalculatorNegativeValues);
    RUN_TEST(testCalculatorExcessiveDiscount);
    RUN_TEST(testCalculatorNonFiniteValues);

    // Payment Strategy Tests
    RUN_TEST(testCashPaymentExact);
    RUN_TEST(testCashPaymentWithChange);
    RUN_TEST(testCashPaymentInsufficient);
    RUN_TEST(testCardPaymentValid);
    RUN_TEST(testCardPaymentMissingBank);
    RUN_TEST(testCardPaymentInvalidCharacter);
    RUN_TEST(testWalletPaymentValid);
    RUN_TEST(testPaymentRegistryDefaults);

    // Bill Record Codec Tests
    RUN_TEST(testCodecFormatAndParse);
    RUN_TEST(testCodecParseErrors);

    // Bill Repository Tests
    RUN_TEST(testRepositoryNextBillId);
    RUN_TEST(testRepositoryAppendAndLoad);

    std::cout << "\n----------------------------------------\n";
    std::cout << "Test Summary: " << passedTests << "/" << totalTests << " passed.\n";
    std::cout << "----------------------------------------\n";

    return (failedTests == 0) ? 0 : 1;
}
