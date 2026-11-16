#include "product/ProductManager.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace {
void setError(std::string* error, const std::string& message) {
    if (error) *error = message;
}

bool parseProductLine(const std::string& line, Product& product, std::string& error) {
    if (line.empty() || line.back() != '#') {
        error = "record must end with #";
        return false;
    }

    std::istringstream input(line.substr(0, line.size() - 1));
    Product parsed;
    std::string extra;
    if (!(input >> parsed.productId >> parsed.barcode >> parsed.acronym >> parsed.name
                 >> parsed.measurementUnit >> parsed.unitPrice >> parsed.tax
                 >> parsed.discount >> parsed.availableQuantity >> parsed.barcodeFilePath)
        || (input >> extra)) {
        error = "expected exactly 10 space-separated fields";
        return false;
    }

    if (!parsed.isValid(&error)) return false;
    product = std::move(parsed);
    return true;
}
}

bool ProductManager::loadProducts(const std::string& filename, std::string* error) {
    std::ifstream file(filename);
    if (!file) {
        setError(error, "cannot open product file: " + filename);
        return false;
    }

    std::vector<Product> loaded;
    std::unordered_set<int> ids;
    std::unordered_set<std::string> barcodes;
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        Product product;
        std::string reason;
        if (!parseProductLine(line, product, reason)) {
            setError(error, "line " + std::to_string(lineNumber) + ": " + reason);
            return false;
        }
        if (!ids.insert(product.productId).second || !barcodes.insert(product.barcode).second) {
            setError(error, "line " + std::to_string(lineNumber) + ": duplicate product ID or barcode");
            return false;
        }
        loaded.push_back(std::move(product));
    }
    if (file.bad()) {
        setError(error, "error while reading product file: " + filename);
        return false;
    }

    products.swap(loaded); // Keep the old catalogue if parsing fails.
    setError(error, {});
    return true;
}

bool ProductManager::saveProducts(const std::string& filename, std::string* error) const {
    std::ofstream file(filename, std::ios::trunc);
    if (!file) {
        setError(error, "cannot write product file: " + filename);
        return false;
    }

    file << std::fixed << std::setprecision(2);
    for (const Product& p : products) {
        std::string reason;
        if (!p.isValid(&reason)) {
            setError(error, "invalid product " + std::to_string(p.productId) + ": " + reason);
            return false;
        }
        file << p.productId << ' ' << p.barcode << ' ' << p.acronym << ' ' << p.name << ' '
             << p.measurementUnit << ' ' << p.unitPrice << ' ' << p.tax << ' ' << p.discount << ' '
             << p.availableQuantity << ' ' << p.barcodeFilePath << "#\n";
    }
    if (!file) {
        setError(error, "error while writing product file: " + filename);
        return false;
    }
    setError(error, {});
    return true;
}

Product* ProductManager::searchByBarcode(const std::string& barcode) {
    for (auto& product : products) if (product.barcode == barcode) return &product;
    return nullptr;
}
const Product* ProductManager::searchByBarcode(const std::string& barcode) const {
    for (const auto& product : products) if (product.barcode == barcode) return &product;
    return nullptr;
}
Product* ProductManager::searchByID(int id) {
    for (auto& product : products) if (product.productId == id) return &product;
    return nullptr;
}
const Product* ProductManager::searchByID(int id) const {
    for (const auto& product : products) if (product.productId == id) return &product;
    return nullptr;
}

bool ProductManager::addProduct(const Product& product, std::string* error) {
    std::string reason;
    if (!product.isValid(&reason)) { setError(error, reason); return false; }
    if (searchByID(product.productId) || searchByBarcode(product.barcode)) {
        setError(error, "duplicate product ID or barcode");
        return false;
    }
    products.push_back(product);
    setError(error, {});
    return true;
}

bool ProductManager::reserveStock(int id, int quantity) {
    Product* product = searchByID(id);
    if (!product || quantity <= 0 || quantity > product->availableQuantity) return false;
    product->availableQuantity -= quantity;
    return true;
}

const std::vector<Product>& ProductManager::getAllProducts() const noexcept { return products; }
std::vector<Product>& ProductManager::getAllProducts() noexcept { return products; }
std::size_t ProductManager::getProductCount() const noexcept { return products.size(); }
void ProductManager::clearProducts() noexcept { products.clear(); }
