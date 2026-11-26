#include "ProductModel.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <stdexcept>

// ======================= Measurement unit =======================
std::string toString(MeasurementUnit unit)
{
    return unit == MeasurementUnit::Count ? "count" : "weight";
}

bool parseMeasurementUnit(const std::string &text, MeasurementUnit &out)
{
    if (text == "count")
        out = MeasurementUnit::Count;
    else if (text == "weight")
        out = MeasurementUnit::Weight;
    else
        return false;
    return true;
}

// ======================= Money =======================
std::string formatPKR(double amount)
{
    char buf[64];
    std::snprintf(buf, sizeof buf, "PKR %.2f", amount);
    return buf;
}

// ======================= EAN-8 =======================
namespace ean8
{
    bool isEightDigits(const std::string &code)
    {
        return code.size() == 8 &&
               std::all_of(code.begin(), code.end(), [](unsigned char c) { return std::isdigit(c); });
    }

    // EAN-8: first 7 digits weighted 3,1,3,1,3,1,3; check digit makes the sum a multiple of 10.
    bool hasValidCheckDigit(const std::string &code)
    {
        if (!isEightDigits(code))
            return false;
        int sum = 0;
        for (int i = 0; i < 7; ++i)
            sum += (code[i] - '0') * (i % 2 == 0 ? 3 : 1);
        return (10 - sum % 10) % 10 == code[7] - '0';
    }
}

// ======================= Product =======================
Product::Product(int id, std::string barcode, std::string acronym, std::string name, MeasurementUnit unit,
                 double unitPrice, double tax, double discount, int availableQuantity,
                 std::string barcodeFilePath, std::string categoryId)
    : id_(id), barcode_(std::move(barcode)), acronym_(std::move(acronym)), name_(std::move(name)),
      unit_(unit), unitPrice_(unitPrice), tax_(tax), discount_(discount),
      availableQuantity_(availableQuantity), barcodeFilePath_(std::move(barcodeFilePath)),
      categoryId_(std::move(categoryId))
{
    if (categoryId_.empty() && barcode_.size() >= 2)
        categoryId_ = barcode_.substr(0, 2); // older Product.txt lines have no Category Id field
}

void Product::deductStock(int qty)
{
    if (qty < 0 || qty > availableQuantity_)
        throw std::invalid_argument("cannot deduct " + std::to_string(qty) + " from stock of " +
                                    std::to_string(availableQuantity_));
    availableQuantity_ -= qty;
}

// ======================= ProductDescriber =======================
ProductDescription ProductDescriber::describe(const Product &p) const
{
    ProductDescription d;
    d.title = p.getProductName();
    const int qty = p.getAvailableQuantity();
    d.stockLevel = qty <= 0 ? "out" : qty <= lowStockThreshold_ ? "low" : "in-stock";
    const std::string unit = p.getMeasurementUnitName();
    d.fields = {
        {"Product ID", std::to_string(p.getProductID())},
        {"Category ID", p.getCategoryId()},
        {"Barcode", p.getBarcode()},
        {"Acronym", p.getAcronym()},
        {"Name", p.getProductName()},
        {"Measured by", unit},
        {"Unit price", formatPKR(p.getUnitPrice())},
        {"Tax per unit", formatPKR(p.getTax())},
        {"Discount per unit", formatPKR(p.getDiscount())},
        {"You pay per unit", formatPKR(p.getUnitPrice() - p.getDiscount() + p.getTax())},
        {"Available", std::to_string(qty) + (d.stockLevel == "out"   ? " (out of stock)"
                                             : d.stockLevel == "low" ? " (low stock)"
                                                                     : "")},
        {"Barcode file", p.getBarcodeFilePath()},
    };
    return d;
}

// ======================= TextProductFormatter =======================
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
