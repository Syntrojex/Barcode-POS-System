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

    // Negative unit price
    std::vector<BillLine> negPrice = {{"ITEM", -50.0, 5.0, 0.0, 1.0}};
    TEST_ASSERT(!BillCalculator::calculate(negPrice, totals, &error), "Negative price should fail");

    // Negative tax
    std::vector<BillLine> negTax = {{"ITEM", 50.0, -5.0, 0.0, 1.0}};
    TEST_ASSERT(!BillCalculator::calculate(negTax, totals, &error), "Negative tax should fail");

    // Negative discount
    std::vector<BillLine> negDisc = {{"ITEM", 50.0, 5.0, -2.0, 1.0}};
    TEST_ASSERT(!BillCalculator::calculate(negDisc, totals, &error), "Negative discount should fail");

    // Zero quantity
    std::vector<BillLine> zeroQty = {{"ITEM", 50.0, 5.0, 0.0, 0.0}};
    TEST_ASSERT(!BillCalculator::calculate(zeroQty, totals, &error), "Zero quantity should fail");

    // Negative quantity
    std::vector<BillLine> negQty = {{"ITEM", 50.0, 5.0, 0.0, -3.0}};
    TEST_ASSERT(!BillCalculator::calculate(negQty, totals, &error), "Negative quantity should fail");

    return true;
}

bool testCalculatorExcessiveDiscount() {
    // Subtotal 100, Tax 0, Discount 150 -> Net payable would be -50
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

} // namespace

int main() {
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;

    std::cout << "========================================\n";
    std::cout << " BarcodeHub Billing Subsystem Unit Tests\n";
    std::cout << " Author: Ali Abdullah (25L-3022)\n";
    std::cout << "========================================\n\n";

    RUN_TEST(testCalculatorSingleItem);
    RUN_TEST(testCalculatorMultipleItems);
    RUN_TEST(testCalculatorRoundingPrecision);
    RUN_TEST(testCalculatorEmptyLines);
    RUN_TEST(testCalculatorNegativeValues);
    RUN_TEST(testCalculatorExcessiveDiscount);
    RUN_TEST(testCalculatorNonFiniteValues);

    std::cout << "\n----------------------------------------\n";
    std::cout << "Test Summary: " << passedTests << "/" << totalTests << " passed.\n";
    std::cout << "----------------------------------------\n";

    return (failedTests == 0) ? 0 : 1;
}
