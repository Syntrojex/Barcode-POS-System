#include "product/ProductDescriber.h"
#include "product/Money.h"
#include <algorithm>

ProductDescription ProductDescriber::describe(const Product &p) const
{
    ProductDescription d;
    d.title = p.getProductName();
    const int qty = p.getAvailableQuantity();
    d.stockLevel = qty <= 0 ? "out" : qty <= lowStockThreshold_ ? "low" : "in-stock";
    const std::string unit = p.getMeasurementUnitName();
    d.fields = {
        {"Product ID", std::to_string(p.getProductID())},
        {"Barcode", p.getBarcode()},
        {"Acronym", p.getAcronym()},
        {"Name", p.getProductName()},
        {"Measured by", unit},
        {"Unit price", formatPKR(p.getUnitPrice())},
        {"Tax per unit", formatPKR(p.getTax())},
        {"Discount per unit", formatPKR(p.getDiscount())},
        {"You pay per unit", formatPKR(p.getUnitPrice() - p.getDiscount() + p.getTax())},
        {"Available", std::to_string(qty) + (d.stockLevel == "out" ? " (out of stock)" : d.stockLevel == "low" ? " (low stock)" : "")},
        {"Barcode file", p.getBarcodeFilePath()},
    };
    return d;
}

std::string TextProductFormatter::format(const ProductDescription &d) const
{
    std::size_t w = 0;
    for (const auto &f : d.fields)
        w = std::max(w, f.first.size());
    std::string out = d.title + "\n" + std::string(d.title.size(), '-') + "\n";
    for (const auto &f : d.fields)
        out += f.first + std::string(w - f.first.size() + 2, ' ') + f.second + "\n";
    return out;
}
