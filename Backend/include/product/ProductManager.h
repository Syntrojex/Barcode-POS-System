#pragma once
// In-memory product catalogue: lookup by barcode, id and text, plus stock deduction.
// Loading/saving/validation are delegated (ProductFileLoader / ProductFileWriter / ProductValidator).
#include "product/Product.h"
#include "product/ProductFileLoader.h"
#include <string>
#include <unordered_map>
#include <vector>

class ProductManager
{
public:
    ProductManager() = default;
    explicit ProductManager(ProductFileLoader loader) : loader_(std::move(loader)) {}

    // true = file opened and every line was valid. Valid lines are kept either way (see lastLoad()).
    bool loadProducts(const std::string &filename);
    bool saveProducts(std::string *error = nullptr) const; // back to the file it was loaded from
    const ProductLoadResult &lastLoad() const { return lastLoad_; }
    const std::string &filePath() const { return path_; }

    // ---- search (O(1) for barcode / id) ----
    const Product *searchByBarcode(const std::string &barcode) const;
    const Product *searchByID(int id) const;
    // Accepts a barcode, a numeric product id or an acronym (case-insensitive) - what a scanner or a
    // cashier would type.
    const Product *searchByCode(const std::string &code) const;
    // Case-insensitive substring match on name, acronym or barcode.
    std::vector<const Product *> searchByText(const std::string &query) const;

    const std::vector<Product> &getAllProducts() const { return products_; }
    int getProductCount() const { return static_cast<int>(products_.size()); }
    void clearProducts();

    // Reduces stock after a sale. Returns false (and changes nothing) for unknown id or too much.
    bool deductStock(int productId, int quantity);

private:
    void rebuildIndex();
    Product *mutableById(int id);

    ProductFileLoader loader_;
    ProductLoadResult lastLoad_;
    std::string path_;
    std::vector<Product> products_;
    std::unordered_map<std::string, std::size_t> byBarcode_;
    std::unordered_map<int, std::size_t> byId_;
};
