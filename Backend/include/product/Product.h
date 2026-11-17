#ifndef PRODUCT_H
#define PRODUCT_H

#include <string>

// Plain product record (one row of Product.txt). Holds data only: parsing lives in
// ProductRecordCodec, validation in ProductValidator, stock rules in StockValidator and
// presentation in ProductDescriber, so this class has a single reason to change.
class Product
{
private:
    int productID;
    std::string barcode;
    std::string acronym;
    std::string productName;
    std::string measurementUnit; // "count" or "weight"

    double unitPrice;  // PKR, without tax
    double tax;        // PKR, per unit
    double discount;   // PKR, per unit

    int availableQuantity;
    std::string barcodeFilePath;

public:
    Product();

    Product(int id, std::string barcode, std::string acronym,
            std::string name, std::string unit,
            double price, double tax,
            double discount, int quantity,
            std::string barcodePath);

    int getProductID() const;
    const std::string &getBarcode() const;
    const std::string &getAcronym() const;
    const std::string &getProductName() const;
    const std::string &getMeasurementUnit() const;

    double getUnitPrice() const;
    double getTax() const;
    double getDiscount() const;

    int getAvailableQuantity() const;
    const std::string &getBarcodeFilePath() const;

    void setProductID(int id);
    void setBarcode(const std::string &barcode);
    void setAcronym(const std::string &acronym);
    void setProductName(const std::string &name);
    void setMeasurementUnit(const std::string &unit);

    void setUnitPrice(double price);
    void setTax(double tax);
    void setDiscount(double discount);

    void setAvailableQuantity(int quantity);
    void setBarcodeFilePath(const std::string &path);
};

#endif
