#pragma once
// Reads Product.txt: iterates lines, delegates parsing to ProductParser and data checks to
// ProductValidator, and rejects duplicate ids / barcodes. Invalid lines are skipped and reported;
// valid lines are still loaded.
#include "product/Product.h"
#include "product/ProductParser.h"
#include "product/ProductValidator.h"
#include <iosfwd>
#include <string>
#include <vector>

enum class IssueSeverity { Warning, Error };

struct ProductIssue
{
    int line = 0; // 1-based line in the file (0 = whole file)
    IssueSeverity severity = IssueSeverity::Error;
    std::string message;
};

struct ProductLoadResult
{
    bool fileOpened = false;
    std::vector<Product> products;
    std::vector<ProductIssue> issues;
    int rejectedLines = 0;

    int errorCount() const;
    int warningCount() const;
    bool hasErrors() const { return !fileOpened || errorCount() > 0; }
    std::string summary() const; // human readable, multi-line
};

class ProductFileLoader
{
public:
    ProductFileLoader() : validator_(ProductValidator::standard()) {}
    ProductFileLoader(ProductParser parser, ProductValidator validator)
        : parser_(parser), validator_(std::move(validator)) {}

    ProductLoadResult load(const std::string &path) const;
    ProductLoadResult loadFrom(std::istream &in) const; // used by tests, no file needed

private:
    ProductParser parser_;
    ProductValidator validator_;
};
