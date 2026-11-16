#pragma once

#include "product/Product.h"
#include <string>
#include <vector>

// Owns the in-memory product catalogue and its business rules.
class ProductManager {
public:
    bool loadProducts(const std::string& filename, std::string* error = nullptr);
    bool saveProducts(const std::string& filename, std::string* error = nullptr) const;

    Product* searchByBarcode(const std::string& barcode);
    const Product* searchByBarcode(const std::string& barcode) const;
    Product* searchByID(int productId);
    const Product* searchByID(int productId) const;

    // Adds a product only when it is valid and its ID/barcode are unique.
    bool addProduct(const Product& product, std::string* error = nullptr);
    // Reduces stock only if enough stock is available.
    bool reserveStock(int productId, int quantity);

    const std::vector<Product>& getAllProducts() const noexcept;
    std::vector<Product>& getAllProducts() noexcept;
    std::size_t getProductCount() const noexcept;
    void clearProducts() noexcept;

private:
    std::vector<Product> products;
};
