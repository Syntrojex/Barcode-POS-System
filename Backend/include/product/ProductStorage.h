#pragma once
// ProductStorage: everything that moves products between the text file and memory —
// parsing lines, writing lines, loading a whole file with error reporting,
// and the ProductManager that holds the catalogue and searches it.
#include "ProductValidation.h"
#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// ---------- Parsing one line (multi-word names allowed) ----------
struct ParsedProduct
{
    std::optional<Product> product;
    std::string error;
    bool ok() const { return product.has_value() && error.empty(); }
};

class ProductParser
{
public:
    static constexpr char kEndMarker = '#';
    ParsedProduct parseLine(const std::string &rawLine) const;
};

// ---------- Older strict codec (exactly 10 fields, single-word name) ----------
class ProductRecordCodec
{
public:
    struct ParseResult
    {
        bool ok = false;
        std::string error;
        Product product;
    };
    static ParseResult parse(const std::string &rawLine);
    static std::string format(const Product &p);
};

// ---------- Writing the file ----------
class ProductFileWriter
{
public:
    std::string serialize(const Product &p) const;
    // Writes to "<path>.tmp" then swaps it in, so a crash never leaves a half-written file.
    bool save(const std::string &path, const std::vector<Product> &products, std::string *error = nullptr) const;
};

// ---------- Loading the file ----------
enum class IssueSeverity
{
    Error,
    Warning
};

struct LoadIssue
{
    int line;
    IssueSeverity severity;
    std::string message;
};

struct ProductLoadResult
{
    bool fileOpened = false;
    std::vector<Product> products;
    std::vector<LoadIssue> issues;
    int rejectedLines = 0;

    int errorCount() const;
    int warningCount() const;
    bool hasErrors() const { return errorCount() > 0; }
    std::string summary() const;
};

class ProductFileLoader
{
public:
    ProductLoadResult load(const std::string &path) const;
    ProductLoadResult loadFrom(std::istream &in) const;

private:
    ProductParser parser_;
    ProductValidator validator_ = ProductValidator::standard();
};

// ---------- In-memory catalogue ----------
class ProductManager
{
public:
    bool loadProducts(const std::string &filename);
    bool saveProducts(std::string *error = nullptr) const;
    void clearProducts();

    const Product *searchByBarcode(const std::string &barcode) const;
    const Product *searchByID(int id) const;
    const Product *searchByCode(const std::string &code) const; // barcode, then id, then acronym
    std::vector<const Product *> searchByText(const std::string &query) const;

    bool deductStock(int productId, int quantity);

    const std::vector<Product> &getProducts() const { return products_; }
    const ProductLoadResult &getLastLoad() const { return lastLoad_; }

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
