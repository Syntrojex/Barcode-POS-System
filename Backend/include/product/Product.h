#pragma once
// Product domain object. Pure C++17 (no Qt, no I/O): holds product data and its own invariants only.
// Parsing, validation, persistence, searching, stock policy and presentation live in separate classes.
#include <string>

enum class MeasurementUnit { Count, Weight };

std::string toString(MeasurementUnit unit);                              // "count" | "weight"
bool parseMeasurementUnit(const std::string &text, MeasurementUnit &out); // exact, lower-case

class Product
{
public:
    Product() = default;
    Product(int id, std::string barcode, std::string acronym, std::string name, MeasurementUnit unit,
            double unitPrice, double tax, double discount, int availableQuantity,
            std::string barcodeFilePath);

    // ---- getters (names kept compatible with the rest of the team's code) ----
    int getProductID() const { return id_; }
    const std::string &getBarcode() const { return barcode_; }
    const std::string &getAcronym() const { return acronym_; }
    const std::string &getProductName() const { return name_; }
    MeasurementUnit getMeasurementUnit() const { return unit_; }
    std::string getMeasurementUnitName() const { return toString(unit_); }
    double getUnitPrice() const { return unitPrice_; }   // price without tax, PKR
    double getTax() const { return tax_; }               // tax per unit, PKR
    double getDiscount() const { return discount_; }     // discount per unit, PKR
    int getAvailableQuantity() const { return availableQuantity_; }
    const std::string &getBarcodeFilePath() const { return barcodeFilePath_; }

    // ---- setters ----
    void setProductID(int v) { id_ = v; }
    void setBarcode(std::string v) { barcode_ = std::move(v); }
    void setAcronym(std::string v) { acronym_ = std::move(v); }
    void setProductName(std::string v) { name_ = std::move(v); }
    void setMeasurementUnit(MeasurementUnit v) { unit_ = v; }
    void setUnitPrice(double v) { unitPrice_ = v; }
    void setTax(double v) { tax_ = v; }
    void setDiscount(double v) { discount_ = v; }
    void setAvailableQuantity(int v) { availableQuantity_ = v; }
    void setBarcodeFilePath(std::string v) { barcodeFilePath_ = std::move(v); }

    // ---- behaviour that belongs to the product itself ----
    double netUnitPrice() const { return unitPrice_ - discount_; } // after discount, before tax
    bool isInStock() const { return availableQuantity_ > 0; }
    // Removes sold stock. Throws std::invalid_argument if qty < 0 or qty > available.
    void deductStock(int qty);

private:
    int id_ = 0;
    std::string barcode_, acronym_, name_;
    MeasurementUnit unit_ = MeasurementUnit::Count;
    double unitPrice_ = 0, tax_ = 0, discount_ = 0;
    int availableQuantity_ = 0;
    std::string barcodeFilePath_;
};
