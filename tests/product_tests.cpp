// Unit tests for the product module. Plain C++17, no framework: run `product_tests` or `ctest`.
#include "product/Money.h"
#include "product/ProductDescriber.h"
#include "product/ProductFileLoader.h"
#include "product/ProductFileWriter.h"
#include "product/ProductManager.h"
#include "product/StockValidator.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <sstream>

static int failures = 0;
#define CHECK(cond)                                                                  \
    do                                                                               \
    {                                                                                \
        if (cond) std::cout << "PASS " << #cond << "\n";                             \
        else { std::cout << "FAIL " << #cond << "  (line " << __LINE__ << ")\n"; ++failures; } \
    } while (0)

static const char *kGood = "1 40001029 OLPMILK Olpers Full Cream Milk 1L count 340.00 0.00 10.00 42 data/barcodes/40001029.png#";

static ProductLoadResult loadText(const std::string &text)
{
    std::istringstream in(text);
    return ProductFileLoader().loadFrom(in);
}

int main()
{
    // ---- parser
    ProductParser parser;
    auto p = parser.parseLine(kGood);
    CHECK(p.ok());
    CHECK(p.product->getProductName() == "Olpers Full Cream Milk 1L");   // multi-word name
    CHECK(p.product->getAvailableQuantity() == 42 && p.product->getMeasurementUnit() == MeasurementUnit::Count);
    CHECK(!parser.parseLine("1 40001029 OLP Milk count 340 0 10 42 a.png").ok());          // no '#'
    CHECK(!parser.parseLine("1 40001029 OLP Milk each 340 0 10 42 a.png#").ok());          // bad unit
    CHECK(!parser.parseLine("1 40001029 OLP Milk count abc 0 10 42 a.png#").ok());         // bad number
    CHECK(!parser.parseLine("1 40001029 OLP count 340 0 10 42 a.png#").ok());              // too few fields

    // ---- validation + loader
    CHECK(loadText(std::string(kGood) + "\n").products.size() == 1);
    CHECK(loadText("1 40001029 TOOLONGACRO Milk count 340 0 10 42 a.png#\n").products.empty());   // acronym > 8
    CHECK(loadText("1 4000 OLP Milk count 340 0 10 42 a.png#\n").products.empty());              // barcode not 8 digits
    CHECK(loadText("1 40001029 OLP Milk count -5 0 0 42 a.png#\n").products.empty());            // negative price
    CHECK(loadText("1 40001029 OLP Milk count 100 0 150 42 a.png#\n").products.empty());         // discount > price
    CHECK(loadText("0 40001029 OLP Milk count 100 0 0 42 a.png#\n").products.empty());           // id 0
    auto dup = loadText(std::string(kGood) + "\n2 40001029 OTHER Other count 10 0 0 1 b.png#\n");
    CHECK(dup.products.size() == 1 && dup.errorCount() == 1);                                   // duplicate barcode
    auto mixed = loadText(std::string(kGood) + "\ngarbage line\n\n3 40002149 BRD Bread count 260 18 0 5 b.png#\n");
    CHECK(mixed.products.size() == 2 && mixed.rejectedLines == 1);                              // good lines survive
    CHECK(loadText("1 40001020 OLP Milk count 100 0 0 5 a.png#\n").warningCount() == 1);       // bad check digit = warning only
    CHECK(!ProductFileLoader().load("no/such/file.txt").fileOpened);

    // ---- open/closed: a new rule is added without editing the validator
    struct NoMilkRule : IProductRule
    {
        void check(const Product &pr, ValidationReport &r) const override
        {
            if (pr.getProductName().find("Milk") != std::string::npos) r.errors.push_back("milk not allowed");
        }
    };
    auto v = ProductValidator::standard();
    v.addRule(std::make_unique<NoMilkRule>());
    CHECK(!v.validate(*p.product).ok());

    // ---- manager: search + stock
    ProductManager pm;
    CHECK(pm.loadProducts("data/Product.txt"));
    CHECK(pm.getProductCount() == 14);
    CHECK(pm.searchByBarcode("40001029") && pm.searchByBarcode("40001029")->getProductID() == 1);
    CHECK(pm.searchByBarcode("12345678") == nullptr);
    CHECK(pm.searchByID(5) && pm.searchByID(5)->getAcronym() == "BASMATI");
    CHECK(pm.searchByID(99) == nullptr);
    CHECK(pm.searchByCode("olpmilk") && pm.searchByCode("7") && pm.searchByCode("40001029"));
    CHECK(pm.searchByText("tea").size() == 1);
    CHECK(pm.deductStock(1, 2) && pm.searchByID(1)->getAvailableQuantity() == 40);
    CHECK(!pm.deductStock(1, 9999) && pm.searchByID(1)->getAvailableQuantity() == 40);

    // ---- stock validator + swappable policy
    const Product eggs = *pm.searchByID(7); // 4 in stock
    StockValidator sv;
    CHECK(sv.check(eggs, 4).ok());
    CHECK(sv.check(eggs, 5).status == StockStatus::InsufficientStock);
    CHECK(sv.checkAdd(eggs, 3, 1).ok() && !sv.checkAdd(eggs, 4, 1).ok());
    CHECK(sv.check(eggs, 0).status == StockStatus::InvalidQuantity);
    CHECK(sv.check(*pm.searchByID(11), 1).status == StockStatus::OutOfStock);
    StockValidator strict(std::make_shared<StrictlyBelowAvailablePolicy>());
    CHECK(!strict.check(eggs, 4).ok() && strict.check(eggs, 3).ok());

    // ---- description (what) + formatter (how)
    ProductDescriber d(10);
    CHECK(d.describe(eggs).stockLevel == "low" && d.describe(*pm.searchByID(11)).stockLevel == "out");
    const std::string text = TextProductFormatter().format(d.describe(eggs));
    CHECK(text.find("Farm Fresh Eggs Dozen") != std::string::npos && text.find("PKR 390.00") != std::string::npos);
    CHECK(formatPKR(1234567.5) == "PKR 1,234,567.50" && formatPKR(0) == "PKR 0.00");

    // ---- writer round trip
    const std::string tmp = "product_roundtrip.txt";
    ProductManager a, b;
    a.loadProducts("data/Product.txt");
    a.deductStock(1, 2);
    ProductFileWriter w;
    CHECK(w.save(tmp, a.getAllProducts()));
    CHECK(b.loadProducts(tmp) && b.getProductCount() == a.getProductCount() &&
          b.searchByID(1)->getAvailableQuantity() == 40);
    std::remove(tmp.c_str());

    std::cout << (failures ? "FAILED" : "ALL OK") << " (" << failures << " failures)\n";
    return failures ? 1 : 0;
}
