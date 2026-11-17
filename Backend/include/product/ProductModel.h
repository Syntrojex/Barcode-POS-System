#pragma once
// ProductModel: the core data type (Product) plus small helpers it needs
// (measurement unit, PKR money formatting, EAN-8 barcode checks) and the
// classes that turn a Product into human-readable text.
#include <string>
#include <utility>
#include <vector>

// ---------- Measurement unit ----------
enum class MeasurementUnit
{
    Count,
    Weight
};
std::string toString(MeasurementUnit unit);
bool parseMeasurementUnit(const std::string &text, MeasurementUnit &out);

// ---------- Money ----------
std::string formatPKR(double amount);

// ---------- EAN-8 barcode helpers ----------
namespace ean8
{
    bool isEightDigits(const std::string &code);
    bool hasValidCheckDigit(const std::string &code);
}

// ---------- Product ----------
class Product
{
public:
    Product() = default;
    Product(int id, std::string barcode, std::string acronym, std::string name, MeasurementUnit unit,
            double unitPrice, double tax, double discount, int availableQuantity,
            std::string barcodeFilePath);

    int getProductID() const { return id_; }
    const std::string &getBarcode() const { return barcode_; }
    const std::string &getAcronym() const { return acronym_; }
    const std::string &getProductName() const { return name_; }
    MeasurementUnit getMeasurementUnit() const { return unit_; }
    std::string getMeasurementUnitName() const { return toString(unit_); }
    double getUnitPrice() const { return unitPrice_; }
    double getTax() const { return tax_; }
    double getDiscount() const { return discount_; }
    int getAvailableQuantity() const { return availableQuantity_; }
    const std::string &getBarcodeFilePath() const { return barcodeFilePath_; }

    void deductStock(int qty); // throws std::invalid_argument if qty is invalid

private:
    int id_ = 0;
    std::string barcode_;
    std::string acronym_;
    std::string name_;
    MeasurementUnit unit_ = MeasurementUnit::Count;
    double unitPrice_ = 0;
    double tax_ = 0;
    double discount_ = 0;
    int availableQuantity_ = 0;
    std::string barcodeFilePath_;
};

// ---------- Describing a product ----------
struct ProductDescription
{
    std::string title;
    std::string stockLevel; // "out", "low" or "in-stock"
    std::vector<std::pair<std::string, std::string>> fields; // label -> value
};

class ProductDescriber
{
public:
    explicit ProductDescriber(int lowStockThreshold = 5) : lowStockThreshold_(lowStockThreshold) {}
    ProductDescription describe(const Product &p) const;

private:
    int lowStockThreshold_;
};

class IProductFormatter
{
public:
    virtual ~IProductFormatter() = default;
    virtual std::string format(const ProductDescription &d) const = 0;
};

class TextProductFormatter : public IProductFormatter
{
public:
    std::string format(const ProductDescription &d) const override;
};
