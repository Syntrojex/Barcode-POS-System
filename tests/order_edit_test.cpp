// Increment 3, requirements 3 and 10: editing a paid order in place and keeping on-hold orders in Order.txt.
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
    const auto dir = fs::temp_directory_path() / "barcodehub_order_edit_test";
    fs::remove_all(dir);
    fs::create_directories(dir);
    bh::order::OrderManager orders(dir / "Order.txt", dir / "OrderDetail.txt");
    bh::billing::BillRepository bills(dir / "Bill.txt");
    Product milk(1, "01001020", "MILK", "Fresh Milk", MeasurementUnit::Count, 250, 12.5, 10, 50, "-");
    Product bread(2, "01002140", "BREAD", "Bread", MeasurementUnit::Count, 100, 0, 0, 50, "-");
    bh::billing::BillingService service(orders, bills, [&](const std::string& id, bh::order::ProductSnapshot& out) {
        if (id == "1") { out = bh::integration::toSnapshot(milk); return true; }
        if (id == "2") { out = bh::integration::toSnapshot(bread); return true; }
        return false;
    });
    std::string error;

    // ---- on-hold order: saved, changed under the same id, then paid under the same id
    CHECK(orders.addToCart(bh::integration::toSnapshot(milk), 1, &error));
    bh::order::Order held;
    CHECK(orders.createOrder("Hina", "0333", held, &error));
    held.status = bh::order::OrderStatus::OnHold;
    CHECK(orders.updateOrder(held, &error));
    CHECK(orders.addToCart(bh::integration::toSnapshot(milk), 2, &error));
    CHECK(orders.addToCart(bh::integration::toSnapshot(bread), 1, &error));
    CHECK(orders.saveCartToOrder(held.id, "Hina K", "0333", &error));
    CHECK(orders.cart().empty());
    std::vector<bh::order::OrderDetail> lines;
    CHECK(orders.loadOrderDetails(held.id, lines, &error));
    CHECK(lines.size() == 2 && lines[0].quantity == 2);
    bh::order::Order reread;
    CHECK(orders.findOrder(held.id, reread, &error));
    CHECK(reread.customerName == "Hina K" && reread.status == bh::order::OrderStatus::OnHold);
    CHECK(reread.date == held.date && reread.time == held.time);   // creation moment is kept
    bh::billing::PaymentRequest cash; cash.amountTendered = 1000;
    bh::billing::Bill bill;
    CHECK(service.checkout(held.id, cash, bill, nullptr, &error));      // an on-hold order can be paid
    CHECK(bill.orderId == held.id);

    // ---- paid order edited in place: same Order ID and Bill ID, new amounts, no second bill
    const std::string billId = bill.id, billDate = bill.date, billTime = bill.time;
    CHECK(std::abs(bill.totals.payable - 605) < 0.001);                // 2 x (250 + 12.5 - 10) + 100
    CHECK(orders.addToCart(bh::integration::toSnapshot(milk), 1, &error));
    CHECK(orders.saveCartToOrder(held.id, "Hina K", "0333", &error));
    CHECK(orders.findOrder(held.id, reread, &error));
    CHECK(reread.status == bh::order::OrderStatus::Paid);                // status untouched
    bh::billing::Bill edited;
    double change = 0;
    CHECK(!service.checkout(held.id, cash, edited, nullptr, &error));    // checkout still refuses a paid order
    CHECK(service.rebill(held.id, cash, edited, &change, &error));
    CHECK(edited.id == billId && edited.date == billDate && edited.time == billTime);
    CHECK(std::abs(edited.totals.payable - 252.5) < 0.001);
    CHECK(std::abs(change - 747.5) < 0.001);
    std::vector<bh::billing::Bill> stored;
    CHECK(bills.loadAll(stored, &error));
    CHECK(stored.size() == 1 && stored[0].id == billId && stored[0].lines.size() == 1);
    CHECK(std::abs(stored[0].totals.payable - 252.5) < 0.001);
    CHECK(orders.findOrder(held.id, reread, &error));
    CHECK(std::abs(reread.paymentAmount - 252.5) < 0.001);
    bh::billing::PaymentRequest tooLittle; tooLittle.amountTendered = 1;
    CHECK(!service.rebill(held.id, tooLittle, edited, nullptr, &error));
    CHECK(bills.loadAll(stored, &error) && std::abs(stored[0].totals.payable - 252.5) < 0.001);   // unchanged

    // ---- archived orders cannot be changed; on-hold orders can be discarded, paid ones cannot
    reread.status = bh::order::OrderStatus::Archived;
    CHECK(orders.updateOrder(reread, &error));
    CHECK(orders.addToCart(bh::integration::toSnapshot(milk), 1, &error));
    CHECK(!orders.saveCartToOrder(held.id, "Hina", "", &error));
    orders.clearCart();
    CHECK(orders.addToCart(bh::integration::toSnapshot(bread), 1, &error));
    bh::order::Order second;
    CHECK(orders.createOrder("Sana", "", second, &error));
    second.status = bh::order::OrderStatus::OnHold;
    CHECK(orders.updateOrder(second, &error));
    CHECK(orders.discardOrder(second.id, &error));
    CHECK(!orders.findOrder(second.id, reread, &error));
    CHECK(!orders.discardOrder(held.id, &error));

    // ---- requirement 8: the special discount is not more than 8 percent
    CHECK(orders.addToCart(bh::integration::toSnapshot(bread), 2, &error));            // 2 x 100, no discount
    bh::order::Order capped;
    CHECK(orders.createOrder("Cap", "", capped, &error));
    bh::billing::BillAdjustments tooMuch; tooMuch.specialDiscount = 17.0;                  // 8.5 %
    CHECK(!service.checkout(capped.id, cash, bill, nullptr, &error, tooMuch));
    CHECK(error.find("8 percent") != std::string::npos);
    bh::billing::BillAdjustments eight; eight.specialDiscount = 16.0;                      // exactly 8 %
    CHECK(service.checkout(capped.id, cash, bill, nullptr, &error, eight));
    CHECK(std::abs(bill.specialDiscount - 16.0) < 0.001);

    fs::remove_all(dir);
    std::cout << "order edit OK\n";
}
