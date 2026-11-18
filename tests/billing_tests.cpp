/**
 * @file billing_tests.cpp
 * @brief Comprehensive unit test suite for the BarcodeHub Billing subsystem.
 * @author Ali Abdullah (25L-3022)
 *
 * Verifies calculation accuracy, monetary rounding, error validation,
 * payment strategy processing, record codec serialization, and repository persistence.
 */

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
    Bill bill;
    bill.id = "261118001";
    bill.customerName = "Ali Abdullah";
    bill.orderId = "26111818";
    bill.paymentStatus = "paid";
    bill.totals = {500.0, 50.0, 25.0, 525.0};

    BillLine line;
    line.productAcronym = "MILK";
    line.unitPrice = 250.0;
    line.discountPerUnit = 12.5;
    line.quantity = 2.0;

    std::string formatted = BillRecordCodec::format(bill, line);
    TEST_ASSERT(formatted.back() == '#', "Formatted record must end with '#'");

    Bill parsedHeader;
    BillLine parsedLine;
    std::string error;
    TEST_ASSERT(BillRecordCodec::parse(formatted, parsedHeader, parsedLine, &error),
                "Parse failed: " + error);

    TEST_ASSERT(parsedHeader.id == "261118001", "Parsed bill ID mismatch");
    TEST_ASSERT(parsedHeader.customerName == "Ali Abdullah", "Parsed customer mismatch");
    TEST_ASSERT(parsedHeader.orderId == "26111818", "Parsed order ID mismatch");
    TEST_ASSERT(parsedHeader.paymentStatus == "paid", "Parsed status mismatch");
    TEST_ASSERT(doubleNear(parsedHeader.totals.payable, 525.0), "Parsed payable mismatch");

    TEST_ASSERT(parsedLine.productAcronym == "MILK", "Parsed acronym mismatch");
    TEST_ASSERT(doubleNear(parsedLine.unitPrice, 250.0), "Parsed unit price mismatch");
    TEST_ASSERT(doubleNear(parsedLine.discountPerUnit, 12.5), "Parsed discount mismatch");
    TEST_ASSERT(doubleNear(parsedLine.quantity, 2.0), "Parsed quantity mismatch");
    return true;
}

bool testCodecParseErrors() {
    Bill header;
    BillLine line;
    std::string error;

    // Missing trailing '#'
    std::string noTerminator = "261118001 Ali 26111818 MILK 250 12.5 2 500 50 25 525 paid";
    TEST_ASSERT(!BillRecordCodec::parse(noTerminator, header, line, &error), "Missing '#' must fail");
    TEST_ASSERT(error.find("end with '#'") != std::string::npos, "Unexpected error: " + error);

    // Too few fields
    std::string shortRecord = "261118001 Ali 26111818 MILK #";
    TEST_ASSERT(!BillRecordCodec::parse(shortRecord, header, line, &error), "Short record must fail");
    TEST_ASSERT(error.find("expected 12 fields") != std::string::npos, "Unexpected error: " + error);

    // Invalid Bill ID length (8 digits instead of 9)
    std::string badId = "26111801 Ali 26111818 MILK 250 12.5 2 500 50 25 525 paid #";
    TEST_ASSERT(!BillRecordCodec::parse(badId, header, line, &error), "8-digit bill ID must fail");
    TEST_ASSERT(error.find("exactly 9 digits") != std::string::npos, "Unexpected error: " + error);

    // Non-numeric in number field
    std::string badNumber = "261118001 Ali 26111818 MILK NOT_A_NUM 12.5 2 500 50 25 525 paid #";
    TEST_ASSERT(!BillRecordCodec::parse(badNumber, header, line, &error), "Non-numeric price must fail");
    TEST_ASSERT(error.find("not a number") != std::string::npos, "Unexpected error: " + error);

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

    std::cout << "\n----------------------------------------\n";
    std::cout << "Test Summary: " << passedTests << "/" << totalTests << " passed.\n";
    std::cout << "----------------------------------------\n";

    return (failedTests == 0) ? 0 : 1;
}
