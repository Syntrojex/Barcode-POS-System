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
    Product product(1, "40001029", "MILK", "Fresh Milk", MeasurementUnit::Count, 250, 12.5, 10, 5, "-");
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
    fs::remove_all(dir);
    std::cout << "checkout OK\n";
}
