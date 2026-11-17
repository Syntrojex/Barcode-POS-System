// Dependency-free unit tests for the Product module (no Qt, no OpenCV, no test framework).
#include "Product.h"
#include "ProductDescriber.h"
#include "ProductManager.h"
#include "ProductRecordCodec.h"
#include "ProductValidator.h"
#include "StockValidator.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

static int failures = 0, total = 0;
#define CHECK(cond)                                                                   \
    do                                                                                \
    {                                                                                 \
        ++total;                                                                      \
        if (!(cond))                                                                  \
        {                                                                             \
            ++failures;                                                               \
            std::cout << "FAIL " << __FILE__ << ":" << __LINE__ << "  " #cond "\n";   \
        }                                                                             \
    } while (0)

static const char *kGood = "1 40000019 MILK Whole_Milk_1L count 280.00 14.00 10.00 40 img/milk.png#";

static Product sample(int id = 1, const std::string &bc = "40000019")
{
    return Product(id, bc, "MILK", "Whole_Milk", "count", 280, 14, 10, 40, "img/milk.png");
}

static void codecTests()
{
    auto r = ProductRecordCodec::parse(kGood);
    CHECK(r.ok);
    CHECK(r.product.getProductID() == 1);
    CHECK(r.product.getBarcode() == "40000019");
    CHECK(r.product.getUnitPrice() == 280.0);
    CHECK(r.product.getAvailableQuantity() == 40);
    CHECK(r.product.getBarcodeFilePath() == "img/milk.png");

    CHECK(ProductRecordCodec::parse(std::string(kGood) + "\r").ok);               // CRLF file
    CHECK(ProductRecordCodec::parse(std::string("  ") + kGood + "  ").ok);        // stray spaces
    CHECK(!ProductRecordCodec::parse("1 40000019 MILK Milk count 1 1 1 4 p.png").ok); // no '#'
    CHECK(!ProductRecordCodec::parse("1 40000019 MILK Milk count 1 1 1 4#").ok);      // 9 fields
    CHECK(!ProductRecordCodec::parse("1 4 M N count 1 1 1 4 p.png extra#").ok);       // 11 fields
    CHECK(!ProductRecordCodec::parse("x 4 M N count 1 1 1 4 p.png#").ok);             // bad id
    CHECK(!ProductRecordCodec::parse("1 4 M N count 12abc 1 1 4 p.png#").ok);         // bad price
    CHECK(!ProductRecordCodec::parse("1 4 M N count 1,5 1 1 4 p.png#").ok);           // comma decimal
    CHECK(!ProductRecordCodec::parse("1 4 M N count 1 1 1 4.5 p.png#").ok);           // fractional qty
    CHECK(!ProductRecordCodec::parse("").ok);

    // format() is the inverse of parse()
    auto again = ProductRecordCodec::parse(ProductRecordCodec::format(r.product));
    CHECK(again.ok);
    CHECK(ProductRecordCodec::format(again.product) == ProductRecordCodec::format(r.product));
}

static void validatorTests()
{
    auto v = ProductValidator::withDefaultRules();
    CHECK(v.isValid(sample()));

    auto bad = [&](auto mutate)
    { Product p = sample(); mutate(p); return !v.isValid(p); };
    CHECK(bad([](Product &p) { p.setProductID(0); }));
    CHECK(bad([](Product &p) { p.setBarcode(""); }));
    CHECK(bad([](Product &p) { p.setAcronym(""); }));
    CHECK(bad([](Product &p) { p.setAcronym("TOOLONGAC"); })); // 9 chars
    CHECK(!bad([](Product &p) { p.setAcronym("EIGHTCHR"); })); // exactly 8 is allowed
    CHECK(bad([](Product &p) { p.setProductName(""); }));
    CHECK(bad([](Product &p) { p.setMeasurementUnit("litre"); }));
    CHECK(bad([](Product &p) { p.setUnitPrice(-1); }));
    CHECK(bad([](Product &p) { p.setTax(-1); }));
    CHECK(bad([](Product &p) { p.setDiscount(-1); }));
    CHECK(bad([](Product &p) { p.setDiscount(500); })); // discount > price
    CHECK(bad([](Product &p) { p.setAvailableQuantity(-1); }));
    CHECK(bad([](Product &p) { p.setBarcodeFilePath(""); }));
    CHECK(!bad([](Product &p) { p.setAvailableQuantity(0); })); // zero stock is valid data

    // every violation is reported, not just the first
    Product worst(0, "", "", "", "x", -1, -1, -1, -1, "");
    CHECK(v.validate(worst).size() >= 7);

    // open/closed: a new rule plugs in without touching ProductValidator
    CHECK(v.isValid(sample()));
    v.addRule(std::make_unique<BarcodeDigitsRule>(8));
    CHECK(v.isValid(sample()));
    CHECK(!v.isValid(sample(1, "1234")));
    CHECK(!v.isValid(sample(1, "4000001x")));
}

static void stockTests()
{
    Product p = sample(); // 40 available
    CHECK(StockValidator::check(p, 1) == StockStatus::Ok);
    CHECK(StockValidator::check(p, 40) == StockStatus::Ok);
    CHECK(StockValidator::check(p, 41) == StockStatus::Insufficient);
    CHECK(StockValidator::check(p, 0) == StockStatus::InvalidQuantity);
    CHECK(StockValidator::check(p, -3) == StockStatus::InvalidQuantity);
    p.setAvailableQuantity(0);
    CHECK(StockValidator::check(p, 1) == StockStatus::OutOfStock);
    CHECK(!StockValidator::canFulfil(p, 1));
    p.setAvailableQuantity(3);
    CHECK(StockValidator::message(p, 5) == "Only 3 available (requested 5)");
}

static void describerTests()
{
    const auto f = ProductDescriber::fields(sample());
    CHECK(f.size() == 11);
    CHECK(ProductDescriber::displayName(sample()) == "Whole Milk");
    const std::string t = ProductDescriber::text(sample());
    CHECK(t.find("Unit price: PKR 280.00") != std::string::npos);
    CHECK(t.find("Stock status: In stock") != std::string::npos);
    Product empty = sample();
    empty.setAvailableQuantity(0);
    CHECK(ProductDescriber::text(empty).find("Out of stock") != std::string::npos);
}

static void managerTests(const std::string &sampleFile)
{
    ProductManager m;
    CHECK(m.loadProducts(sampleFile));
    CHECK(m.getProductCount() == 4);
    CHECK(m.lastErrors().empty());

    const Product *byBc = m.searchByBarcode("40000026");
    CHECK(byBc && byBc->getAcronym() == "BREAD");
    const Product *byId = m.searchByID(3);
    CHECK(byId && byId->getMeasurementUnit() == "weight");
    CHECK(m.searchByBarcode("99999999") == nullptr);
    CHECK(m.searchByID(99) == nullptr);

    // stock
    CHECK(m.deductStock(1, 5) == StockStatus::Ok);
    CHECK(m.searchByID(1)->getAvailableQuantity() == 35);
    CHECK(m.deductStock(1, 36) == StockStatus::Insufficient);
    CHECK(m.searchByID(1)->getAvailableQuantity() == 35); // unchanged on refusal
    CHECK(m.deductStock(4, 1) == StockStatus::OutOfStock);
    CHECK(m.deductStock(77, 1) == StockStatus::UnknownProduct);

    // save -> load round trip keeps the updated stock
    const std::string out = "product_roundtrip.tmp.txt";
    CHECK(m.saveProducts(out));
    ProductManager m2;
    CHECK(m2.loadProducts(out));
    CHECK(m2.getProductCount() == 4);
    CHECK(m2.searchByID(1)->getAvailableQuantity() == 35);
    std::remove(out.c_str());

    // duplicate id / barcode and invalid data are refused, and ALL problems are reported
    std::istringstream badFile(
        "1 40000019 MILK Milk count 10 1 0 5 a.png#\n"
        "1 40000026 BREAD Bread count 10 1 0 5 b.png#\n"   // duplicate id
        "2 40000019 EGGS Eggs count 10 1 0 5 c.png#\n"     // duplicate barcode
        "3 40000040 TOOLONGAC Rice weight 10 1 0 5 d.png#\n" // acronym too long
        "4 40000057 OIL Oil count 10 1 0 5 e.png\n");      // missing '#'
    ProductManager m3;
    CHECK(!m3.loadProducts(badFile));
    CHECK(m3.lastErrors().size() == 4);
    CHECK(m3.getProductCount() == 0);

    // a failed reload leaves the previous catalogue intact
    std::istringstream bad2("garbage#\n");
    CHECK(!m.loadProducts(bad2));
    CHECK(m.getProductCount() == 4);
    CHECK(m.searchByBarcode("40000019") != nullptr);

    // missing file
    CHECK(!m.loadProducts("does/not/exist.txt"));
    CHECK(!m.lastErrors().empty());

    // blank lines and CRLF are tolerated; empty file is a valid empty catalogue
    std::istringstream crlf(std::string(kGood) + "\r\n\r\n");
    ProductManager m4;
    CHECK(m4.loadProducts(crlf));
    CHECK(m4.getProductCount() == 1);
    std::istringstream empty("");
    CHECK(m4.loadProducts(empty));
    CHECK(m4.getProductCount() == 0);

    // addProduct + clear
    ProductManager m5;
    CHECK(m5.addProduct(sample(1, "111")));
    CHECK(!m5.addProduct(sample(1, "222")));
    CHECK(!m5.addProduct(sample(2, "111")));
    m5.clearProducts();
    CHECK(m5.searchByID(1) == nullptr && m5.getProductCount() == 0);
}

int main(int argc, char **argv)
{
    const std::string sampleFile = argc > 1 ? argv[1] : "tests/data/Product.sample.txt";
    codecTests();
    validatorTests();
    stockTests();
    describerTests();
    managerTests(sampleFile);
    std::cout << (total - failures) << "/" << total << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
