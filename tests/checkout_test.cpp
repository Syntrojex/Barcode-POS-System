#include "integration/ProductAdapter.h"
#include "product/ProductStorage.h"
#include "order/OrderManager.h"
#include "billing/BillingService.h"
#include <cmath>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
#define CHECK(x) do { if (!(x)) { std::cerr << "FAILED " << __LINE__ << ": " #x "\n"; return 1; } } while(0)
int main() {
    namespace bh = barcodehub;
    const auto dir = fs::temp_directory_path() / "barcodehub_checkout_test";
    fs::remove_all(dir);
    fs::create_directories(dir);
    bh::order::OrderManager orders(dir / "Order.txt", dir / "OrderDetail.txt");
    bh::billing::BillRepository bills(dir / "Bill.txt");
    Product product(1, "01001020", "MILK", "Fresh Milk", MeasurementUnit::Count, 250, 12.5, 10, 5, "-");
    bh::billing::BillingService service(orders, bills, [&](const std::string& id, bh::order::ProductSnapshot& out) {
        if (id != "1") return false;
        out = bh::integration::toSnapshot(product);
        return true;
    });
    std::string error;
    CHECK(orders.addToCart(bh::integration::toSnapshot(product), 2, &error));
    bh::order::Order created;
    CHECK(orders.createOrder("Walk-in Customer", "", created, &error));
    bh::billing::Bill preview;
    CHECK(service.preview(created.id, preview, &error));
    CHECK(std::abs(preview.totals.payable - 505.0) < 0.001);
    bh::billing::PaymentRequest invalid; invalid.amountTendered = 500;
    bh::billing::Bill result;
    CHECK(!service.checkout(created.id, invalid, result, nullptr, &error));
    invalid.amountTendered = 510;
    double change = 0;
    CHECK(service.checkout(created.id, invalid, result, &change, &error));
    CHECK(std::abs(change - 5) < 0.001);
    CHECK(result.id.size() == 9);
    CHECK(!service.checkout(created.id, invalid, result, nullptr, &error));
    std::vector<bh::billing::Bill> stored;
    CHECK(bills.loadAll(stored, &error));
    CHECK(stored.size() == 1 && stored[0].orderId == created.id);
    CHECK(std::abs(stored[0].totals.payable - 505) < 0.001);
    bh::order::Order paid;
    CHECK(orders.findOrder(created.id, paid, &error));
    CHECK(paid.status == bh::order::OrderStatus::Paid);
    CHECK(std::abs(stored[0].amountReceived - 510) < 0.001 && std::abs(stored[0].balanceReturned - 5) < 0.001);
    CHECK(stored[0].date.size() == 10 && stored[0].time.size() == 8 && stored[0].lines.size() == 1);

    // Increment 3: seasonal + special discount are bill-level amounts saved in their own Bill.txt fields
    CHECK(orders.addToCart(bh::integration::toSnapshot(product), 2, &error));
    bh::order::Order second;
    CHECK(orders.createOrder("Sana", "", second, &error));
    bh::billing::BillAdjustments adj;
    adj.seasonalDiscount = 50;   // e.g. 10 percent season
    adj.specialDiscount = 20;
    bh::billing::PaymentRequest cash; cash.amountTendered = 500;
    bh::billing::Bill discounted;
    CHECK(service.checkout(second.id, cash, discounted, &change, &error, adj));
    CHECK(std::abs(discounted.totals.payable - 435) < 0.001);      // 505 - 50 - 20
    CHECK(std::abs(change - 65) < 0.001);
    bh::billing::BillAdjustments tooMuch; tooMuch.seasonalDiscount = 10000;
    CHECK(orders.addToCart(bh::integration::toSnapshot(product), 1, &error));
    bh::order::Order third;
    CHECK(orders.createOrder("X", "", third, &error));
    CHECK(!service.checkout(third.id, cash, discounted, nullptr, &error, tooMuch));
    CHECK(bills.loadAll(stored, &error));
    CHECK(stored.size() == 2 && std::abs(stored[1].seasonalDiscount - 50) < 0.001 && std::abs(stored[1].specialDiscount - 20) < 0.001);
    fs::remove_all(dir);
    std::cout << "checkout OK\n";
}
