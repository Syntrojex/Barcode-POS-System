// End-to-end check of the backend flow: Product.txt -> find by barcode -> cart -> Order files.
// (The camera step is replaced by a typed barcode so this test needs no camera or OpenCV.)
#include "integration/ProductAdapter.h"
#include "order/OrderManager.h"
#include "product/ProductStorage.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::cerr << "FAILED line " << __LINE__ << ": " #cond "\n";          \
            return EXIT_FAILURE;                                                 \
        }                                                                        \
    } while (0)

int main() {
    using namespace barcodehub;

    // 1. Product module loads a catalogue (same '#'-terminated layout as Product.txt).
    const auto productPath = std::filesystem::path("data/Product.txt");
    ProductManager catalogue;
    CHECK(catalogue.loadProducts(productPath.string()));
    CHECK(catalogue.getProducts().size() == 14);

    // 2. A scanned barcode finds the product.
    const Product* found = catalogue.searchByBarcode("40001029");
    CHECK(found != nullptr);

    // 3. The adapter hands it to the Order module, which fills the cart.
    const auto dir = std::filesystem::temp_directory_path() / "barcodehub_integration";
    std::filesystem::remove_all(dir);
    order::OrderManager orders(dir / "Order.txt", dir / "OrderDetail.txt");
    std::string error;
    CHECK(orders.addToCart(integration::toSnapshot(*found), 3, &error));
    CHECK(!orders.addToCart(integration::toSnapshot(*found), 99, &error));   // more than in stock
    CHECK(orders.cart().size() == 1);
    // 3 units at product price plus tax minus discount
    CHECK(orders.totalPayable() > 989.99 && orders.totalPayable() < 990.01);

    // 4. The order is saved and can be read back.
    order::Order created;
    CHECK(orders.createOrder("Ali Khan", "", created, &error));
    CHECK(created.id.size() == 8);
    std::vector<order::OrderDetail> lines;
    CHECK(orders.loadOrderDetails(created.id, lines, &error));
    CHECK(lines.size() == 1 && lines[0].productBarcode == "40001029");

    // 5. Stock is reduced through the Product module.
    CHECK(catalogue.deductStock(found->getProductID(), 3));
    CHECK(catalogue.searchByBarcode("40001029")->getAvailableQuantity() == 39);

    std::filesystem::remove_all(dir);
    std::cout << "integration OK, order " << created.id << "\n";
    return EXIT_SUCCESS;
}
