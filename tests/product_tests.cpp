// Unit tests for the product module. Plain C++17, no framework: run `product_tests` or `ctest`.
#include "product/ProductModel.h"
#include "product/ProductStorage.h"
#include "product/ProductValidation.h"
#include "product/CategoryStorage.h"
#include <cstdio>
#include <fstream>
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

static const char *kGood = "1 01001020 OLPMILK Olpers Full Cream Milk 1L count 340.00 0.00 10.00 42 data/barcodes/01001020.png#";

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
    CHECK(!parser.parseLine("1 01001020 OLP Milk count 340 0 10 42 a.png").ok());          // no '#'
    CHECK(!parser.parseLine("1 01001020 OLP Milk each 340 0 10 42 a.png#").ok());          // bad unit
    CHECK(!parser.parseLine("1 01001020 OLP Milk count abc 0 10 42 a.png#").ok());         // bad number
    CHECK(!parser.parseLine("1 01001020 OLP count 340 0 10 42 a.png#").ok());              // too few fields

    // ---- validation + loader
    CHECK(loadText(std::string(kGood) + "\n").products.size() == 1);
    CHECK(loadText("1 01001020 TOOLONGACRO Milk count 340 0 10 42 a.png#\n").products.empty());   // acronym > 8
    CHECK(loadText("1 40001029 OLP Milk count 100 0 0 5 a.png#\n").products.size() == 1);       // category 40 is fine
    CHECK(loadText("1 00001020 OLP Milk count 100 0 0 5 a.png#\n").products.empty());           // 00xxxxxx not allowed
    CHECK(loadText("1 01001020 OLP Milk count 100 0 0 5 a.png#\n").products.size() == 1);       // 01xxxxxx is fine
    CHECK(loadText("1 4000 OLP Milk count 340 0 10 42 a.png#\n").products.empty());              // barcode not 8 digits
    CHECK(loadText("1 01001020 OLP Milk count -5 0 0 42 a.png#\n").products.empty());            // negative price
    CHECK(loadText("1 01001020 OLP Milk count 100 0 150 42 a.png#\n").products.empty());         // discount > price
    CHECK(loadText("0 01001020 OLP Milk count 100 0 0 42 a.png#\n").products.empty());           // id 0
    auto dup = loadText(std::string(kGood) + "\n2 01001020 OTHER Other count 10 0 0 1 b.png#\n");
    CHECK(dup.products.size() == 1 && dup.errorCount() == 1);                                   // duplicate barcode
    auto mixed = loadText(std::string(kGood) + "\ngarbage line\n\n3 01002140 BRD Bread count 260 18 0 5 b.png#\n");
    CHECK(mixed.products.size() == 2 && mixed.rejectedLines == 1);                              // good lines survive
    CHECK(loadText("1 01001021 OLP Milk count 100 0 0 5 a.png#\n").warningCount() == 1);       // bad check digit = warning only
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
    CHECK(pm.getProducts().size() == 14);
    CHECK(pm.searchByBarcode("01001020") && pm.searchByBarcode("01001020")->getProductID() == 1);
    CHECK(pm.searchByBarcode("12345678") == nullptr);
    CHECK(pm.searchByID(5) && pm.searchByID(5)->getAcronym() == "BASMATI");
    CHECK(pm.searchByID(99) == nullptr);
    CHECK(pm.searchByCode("olpmilk") && pm.searchByCode("7") && pm.searchByCode("01001020"));
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
    struct BelowAvailable : IStockPolicy {
        bool allows(int requested, int available) const override { return requested < available; }
        std::string describe() const override { return "strictly below available"; }
    };
    StockValidator strict(std::make_shared<BelowAvailable>());
    CHECK(!strict.check(eggs, 4).ok() && strict.check(eggs, 3).ok());

    // ---- description (what) + formatter (how)
    ProductDescriber d(10);
    CHECK(d.describe(eggs).stockLevel == "low" && d.describe(*pm.searchByID(11)).stockLevel == "out");
    const std::string text = TextProductFormatter().format(d.describe(eggs));
    CHECK(text.find("Farm Fresh Eggs Dozen") != std::string::npos && text.find("PKR 390.00") != std::string::npos);
    CHECK(formatPKR(1234567.5) == "PKR 1234567.50" && formatPKR(0) == "PKR 0.00");

    // ---- writer round trip
    const std::string tmp = "product_roundtrip.txt";
    ProductManager a, b;
    a.loadProducts("data/Product.txt");
    a.deductStock(1, 2);
    ProductFileWriter w;
    CHECK(w.save(tmp, a.getProducts()));
    CHECK(b.loadProducts(tmp) && b.getProducts().size() == a.getProducts().size() &&
          b.searchByID(1)->getAvailableQuantity() == 40);
    std::remove(tmp.c_str());

    // ---- Increment 3, requirement 2: Category Id stored in Product.txt
    {
        const ProductLoadResult r = loadText("1 10000014 MILKPAK Nestle Milkpak 1L count 300 15 0 20 data/barcodes/10000014.png 10#\n");
        CHECK(r.products.size() == 1 && r.products[0].getCategoryId() == "10" &&
              r.products[0].getProductName() == "Nestle Milkpak 1L");
        CHECK(loadText(std::string(kGood) + "\n").products[0].getCategoryId() == "01");         // old line: from barcode
        CHECK(loadText("1 10000014 MILK Milk count 300 0 0 2 a.png 20#\n").products.empty());   // id != barcode prefix
        CHECK(loadText("1 10000014 MILK Milk count 300 0 0 2 a.png 00#\n").products.empty());   // category 00
        const Product p = r.products[0];
        const std::string line = ProductFileWriter().serialize(p);
        CHECK(line == "1 10000014 MILKPAK Nestle Milkpak 1L count 300.00 15.00 0.00 20 data/barcodes/10000014.png 10#");
        CHECK(ProductRecordCodec::format(p).size() > 0 && ProductRecordCodec::parse("1 10000014 MILK Milk count 1 0 0 2 a.png 10#").ok);
        CHECK(pm.loadProducts("data/Product.txt") && pm.searchByID(1)->getCategoryId() == "01");
    }

    // ---- Increment 3, requirement 1: PCategory.txt
    {
        ProductCategory c;
        CHECK(CategoryRepository::decode("10 Dairy & Milk Products DAI#", c) && c.id == "10" &&
              c.name == "Dairy & Milk Products" && c.acronym == "DAI");
        CHECK(CategoryRepository::encode(c) == "10 Dairy & Milk Products DAI#");
        CHECK(!CategoryRepository::decode("10 Dairy DAI", c));      // no '#'
        CHECK(!CategoryRepository::decode("1 Dairy DAI#", c));      // id not 2 digits
        CHECK(!CategoryRepository::decode("00 Nothing NON#", c));   // 00 is not a category
        CHECK(!CategoryRepository::decode("10 DAI#", c));           // name missing
        const std::string file = "pcategory_test.tmp";
        CategoryRepository repo(file);
        CHECK(repo.save({{"01", "General Store", "GEN"}, {"80", "Frozen Foods", "FRZ"}}));
        std::vector<std::string> errs;
        auto loaded = repo.load(&errs);
        CHECK(loaded.size() == 2 && loaded[1].name == "Frozen Foods" && errs.empty());
        CHECK(!repo.save({{"0A", "Bad", "BAD"}}));                   // invalid rows are refused
        { std::ofstream f(file, std::ios::app); f << "80 Duplicate DUP#\nbroken line\n"; }
        loaded = repo.load(&errs);
        CHECK(loaded.size() == 2 && errs.size() == 2);
        std::remove(file.c_str());
    }

    std::cout << (failures ? "FAILED" : "ALL OK") << " (" << failures << " failures)\n";
    return failures ? 1 : 0;
}
