#pragma once
// "Show product description". Two responsibilities, two classes:
//   ProductDescriber        - decides WHAT to show (an ordered list of label/value fields)
//   IProductFormatter       - decides HOW to show it. Add a new format (HTML, JSON, ...) by adding a
//                             formatter; nothing existing is modified (open/closed).
#include "product/Product.h"
#include <string>
#include <utility>
#include <vector>

struct ProductDescription
{
    std::string title; // product name
    std::vector<std::pair<std::string, std::string>> fields;
    std::string stockLevel; // "in-stock" | "low" | "out"  (for colouring in a GUI)
};

class ProductDescriber
{
public:
    explicit ProductDescriber(int lowStockThreshold = 10) : lowStockThreshold_(lowStockThreshold) {}
    ProductDescription describe(const Product &product) const;
    bool isLowStock(const Product &p) const { return p.getAvailableQuantity() <= lowStockThreshold_; }

private:
    int lowStockThreshold_;
};

class IProductFormatter
{
public:
    virtual ~IProductFormatter() = default;
    virtual std::string format(const ProductDescription &description) const = 0;
};

class TextProductFormatter : public IProductFormatter
{
public:
    std::string format(const ProductDescription &description) const override;
};
