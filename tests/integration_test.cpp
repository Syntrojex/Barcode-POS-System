// End-to-end check of the backend flow: Product.txt -> find by barcode -> cart -> Order files.
// (The camera step is replaced by a typed barcode so this test needs no camera or OpenCV.)
#include "integration/ProductAdapter.h"
#include "order/OrderManager.h"
#include "product/ProductManager.h"

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
    std::istringstream products(
        "1 04829157 MLK Whole_Milk count 250.00 15.00 10.00 20 barcodes/04829157.png#\n"
        "2 01234565 RCE Basmati_Rice weight 400.00 20.00 0.00 5 barcodes/01234565.png#\n");
    ProductManager catalogue;
    CHECK(catalogue.loadProducts(products));
    CHECK(catalogue.getProductCount() == 2);

    // 2. A scanned barcode finds the product.
    const Product* found = catalogue.searchByBarcode("04829157");
    CHECK(found != nullptr);

    // 3. The adapter hands it to the Order module, which fills the cart.
    const auto dir = std::filesystem::temp_directory_path() / "barcodehub_integration";
    std::filesystem::remove_all(dir);
    order::OrderManager orders(dir / "Order.txt", dir / "OrderDetail.txt");
    std::string error;
    CHECK(orders.addToCart(integration::toSnapshot(*found), 3, &error));
    CHECK(!orders.addToCart(integration::toSnapshot(*found), 99, &error));   // more than in stock
    CHECK(orders.cart().size() == 1);
    // 3 x (250 + 15 - 10) = 765.00
    CHECK(orders.totalPayable() > 764.99 && orders.totalPayable() < 765.01);

    // 4. The order is saved and can be read back.
    order::Order created;
    CHECK(orders.createOrder("Ali Khan", "", created, &error));
    CHECK(created.id.size() == 8);
    std::vector<order::OrderDetail> lines;
    CHECK(orders.loadOrderDetails(created.id, lines, &error));
    CHECK(lines.size() == 1 && lines[0].productBarcode == "04829157");

    // 5. Stock is reduced through the Product module.
    CHECK(catalogue.deductStock(found->getProductID(), 3) == StockStatus::Ok);
    CHECK(catalogue.searchByBarcode("04829157")->getAvailableQuantity() == 17);

    std::cout << "integration OK, order " << created.id << "\n";
    return EXIT_SUCCESS;
}
