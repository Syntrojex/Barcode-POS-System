// Order Management tests. Plain checks (not assert) so they also run in Release builds.
#include "order/OrderManager.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace barcodehub::order;
namespace fs = std::filesystem;

static int failures = 0;
#define CHECK(cond)                                                                   \
    do {                                                                              \
        if (!(cond)) {                                                                \
            ++failures;                                                               \
            std::cout << "FAIL line " << __LINE__ << ": " #cond "\n";                 \
        }                                                                             \
    } while (0)

static std::string readAll(const fs::path& path) {
    std::ifstream in(path);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

static void writeAll(const fs::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::trunc);
    out << text;
}

static ProductSnapshot milk() {
    ProductSnapshot p;
    p.productId = "1";
    p.barcode = "12345678";
    p.acronym = "MILK";
    p.name = "Fresh Milk";
    p.measurementUnit = "count";
    p.unitPrice = 250.0;
    p.taxPerUnit = 12.5;
    p.discountPerUnit = 10.0;
    p.availableQuantity = 5.0;
    return p;
}

static ProductSnapshot rice() {
    ProductSnapshot p;
    p.productId = "2";
    p.barcode = "87654321";
    p.acronym = "RICE";
    p.name = "Basmati Rice";
    p.measurementUnit = "weight";
    p.unitPrice = 400.0;
    p.taxPerUnit = 0.0;
    p.discountPerUnit = 0.0;
    p.availableQuantity = 10.0;
    return p;
}

int main() {
    const auto dir = fs::temp_directory_path() / "barcodehub_order_test";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const auto orderFile = dir / "Order.txt";
    const auto detailFile = dir / "OrderDetail.txt";

    OrderManager m(orderFile, detailFile);
    std::string err;

    // ---- cart + stock validation ----
    CHECK(m.addToCart(milk(), 2, &err));
    CHECK(m.subtotalBeforeTax() == 500.0);
    CHECK(m.totalTax() == 25.0);
    CHECK(m.totalDiscount() == 20.0);
    CHECK(m.totalPayable() == 505.0);
    CHECK(!m.addToCart(milk(), 4, &err) && !err.empty());        // 2 + 4 > stock 5
    CHECK(m.cart().front().quantity == 2.0);                     // failed add changes nothing
    CHECK(!m.addToCart(milk(), 1.5, &err));                      // count needs whole number
    CHECK(!m.addToCart(milk(), 0, &err));                        // zero
    CHECK(!m.addToCart(milk(), -1, &err));                       // negative
    CHECK(m.addToCart(rice(), 1.5, &err));                       // weight allows decimals
    CHECK(!m.addToCart(rice(), 11, &err));                       // over stock
    CHECK(m.setCartQuantity("1", 5, &err));
    CHECK(!m.setCartQuantity("1", 6, &err));
    CHECK(!m.setCartQuantity("999", 1, &err));                   // not in cart
    CHECK(m.removeFromCart("2"));
    CHECK(!m.removeFromCart("2"));
    ProductSnapshot bad = milk();
    bad.discountPerUnit = 9999;
    CHECK(!m.addToCart(bad, 1, &err));                           // discount > price
    bad = milk();
    bad.barcode = "12 34";
    CHECK(!m.addToCart(bad, 1, &err));                           // space would break file format

    // ---- create order (names with spaces / special characters) ----
    Order o1;
    CHECK(m.createOrder("Ali Khan_Jr 100% #1", "", o1, &err));
    CHECK(o1.id.size() == 8);
    CHECK(o1.id.substr(6, 2) == o1.date.substr(8, 2));           // last two digits = day
    CHECK(o1.id.substr(4, 2) == "01");
    CHECK(m.cart().empty());

    const std::string orderText = readAll(orderFile);
    const std::string detailText = readAll(detailFile);
    CHECK(orderText.size() > 2 && orderText.substr(orderText.size() - 3) == " #\n");   // '#' ends the line
    CHECK(detailText == o1.id + " 1 12345678 5 #\n");
    CHECK(orderText.find("\"") == std::string::npos);            // no quotes in the file

    std::vector<Order> orders;
    CHECK(m.loadOrders(orders, &err));
    CHECK(orders.size() == 1);
    CHECK(orders[0].customerName == "Ali Khan_Jr 100% #1");     // lossless round trip
    CHECK(orders[0].customerContact.empty());
    CHECK(orders[0].status == OrderStatus::InProgress);

    CHECK(!m.createOrder("Someone", "", o1, &err));              // empty cart
    CHECK(m.addToCart(rice(), 2.25, &err));
    Order o2;
    CHECK(m.createOrder("Sara", "03001234567", o2, &err));
    CHECK(o2.id.substr(4, 2) == "02");                           // daily sequence increases
    CHECK(o2.id != o1.id);

    std::vector<OrderDetail> details;
    CHECK(m.loadOrderDetails(o2.id, details, &err));
    CHECK(details.size() == 1 && details[0].quantity == 2.25);

    // ---- update ----
    Order paid = o2;
    paid.status = OrderStatus::Paid;
    paid.paymentAmount = 900.0;
    paid.paymentMode = PaymentMode::Wallet;
    paid.paymentMerchant = "Jazz Cash";
    paid.date = "1999-01-01";                                    // must be ignored
    CHECK(m.updateOrder(paid, &err));
    Order reloaded;
    CHECK(m.findOrder(o2.id, reloaded, &err));
    CHECK(reloaded.paymentMerchant == "Jazz Cash");
    CHECK(reloaded.date == o2.date);
    Order zeroPaid = reloaded;
    zeroPaid.paymentAmount = 0;
    CHECK(!m.updateOrder(zeroPaid, &err));                       // paid but 0 amount
    Order ghost = reloaded;
    ghost.id = "99999999";
    CHECK(!m.updateOrder(ghost, &err));                          // unknown id
    Order archived = reloaded;
    archived.status = OrderStatus::Archived;
    CHECK(m.updateOrder(archived, &err));
    archived.status = OrderStatus::InProgress;
    CHECK(!m.updateOrder(archived, &err));                       // cannot reopen

    // ---- corrupted files are reported, not silently accepted ----
    writeAll(orderFile, "26110115 2026-11-15 14:25:10 Ali - in-progress 0.00 cash - \n");   // no '#'
    CHECK(!m.loadOrders(orders, &err) && err.find("'#'") != std::string::npos);
    writeAll(orderFile, "26110115 2026-11-15 14:25:10 Ali - shipped 0.00 cash - #\n");      // bad status
    CHECK(!m.loadOrders(orders, &err));
    writeAll(orderFile, "26110115 2026-11-15 14:25:10 Ali - paid 0.00 cash #\n");           // missing field
    CHECK(!m.loadOrders(orders, &err));
    writeAll(detailFile, "26110115 1 12345678 abc #\n");
    CHECK(!m.loadOrderDetails(details, &err));
    // hand-written valid file (e.g. from the instructor) with a blank line
    writeAll(orderFile, "26110115 2026-11-15 14:25:10 Ali_Khan 0300 paid 505.00 cash ABL #\n\n");
    CHECK(m.loadOrders(orders, &err) && orders.size() == 1 && orders[0].customerName == "Ali Khan");
    // empty files (as shipped in the repo) are fine
    writeAll(orderFile, "");
    writeAll(detailFile, "");
    CHECK(m.loadOrders(orders, &err) && orders.empty());
    CHECK(m.loadOrderDetails(details, &err) && details.empty());

    fs::remove_all(dir);
    if (failures == 0) std::cout << "Order Management tests: ALL PASSED\n";
    else std::cout << "Order Management tests: " << failures << " FAILED\n";
    return failures == 0 ? 0 : 1;
}
