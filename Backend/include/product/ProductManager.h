#ifndef PRODUCT_MANAGER_H
#define PRODUCT_MANAGER_H

#include "Product.h"
#include "ProductValidator.h"
#include "StockValidator.h"

#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

// Owns the product catalogue: loads/saves Product.txt and answers lookups. It composes the
// single-purpose helpers (codec, validator, stock validator) instead of doing their jobs.
//
// Loading is all-or-nothing: if any line is bad the existing catalogue is left untouched and
// lastErrors() lists every problem (with line numbers) so the GUI can show them.
class ProductManager
{
public:
    ProductManager();
    explicit ProductManager(ProductValidator validator);

    // ---- loading / saving -------------------------------------------------------------
    bool loadProducts(const std::string &filename);
    bool loadProducts(std::istream &in);
    bool saveProducts(const std::string &filename) const;
    const std::vector<std::string> &lastErrors() const { return errors_; }

    // ---- lookup (the pointer stays valid until the next load/clear/add) --------------
    Product *searchByBarcode(const std::string &barcode);
    Product *searchByID(int id);
    const Product *searchByBarcode(const std::string &barcode) const;
    const Product *searchByID(int id) const;

    // ---- catalogue -------------------------------------------------------------------
    // Adds one product; false (and lastErrors()) when invalid or id/barcode already exist.
    bool addProduct(const Product &p);
    std::vector<Product> &getAllProducts() { return products_; }
    const std::vector<Product> &getAllProducts() const { return products_; }
    int getProductCount() const { return static_cast<int>(products_.size()); }
    void clearProducts();

    // ---- stock -----------------------------------------------------------------------
    // Checks the quantity (StockValidator) and, only if it can be sold, reduces the stock.
    StockStatus deductStock(int productId, int quantity);

private:
    void rebuildIndex();

    std::vector<Product> products_;
    std::unordered_map<std::string, std::size_t> byBarcode_;
    std::unordered_map<int, std::size_t> byId_;
    std::vector<std::string> errors_;
    ProductValidator validator_;
};

#endif
