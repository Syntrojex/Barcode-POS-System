#include "Product.h"

#include <utility>

Product::Product()
    : productID(0), unitPrice(0), tax(0), discount(0), availableQuantity(0)
{
}

Product::Product(int id, std::string barcode, std::string acronym,
                 std::string name, std::string unit,
                 double price, double tax,
                 double discount, int quantity,
                 std::string barcodePath)
    : productID(id),
      barcode(std::move(barcode)),
      acronym(std::move(acronym)),
      productName(std::move(name)),
      measurementUnit(std::move(unit)),
      unitPrice(price),
      tax(tax),
      discount(discount),
      availableQuantity(quantity),
      barcodeFilePath(std::move(barcodePath))
{
}

int Product::getProductID() const { return productID; }
const std::string &Product::getBarcode() const { return barcode; }
const std::string &Product::getAcronym() const { return acronym; }
const std::string &Product::getProductName() const { return productName; }
const std::string &Product::getMeasurementUnit() const { return measurementUnit; }
double Product::getUnitPrice() const { return unitPrice; }
double Product::getTax() const { return tax; }
double Product::getDiscount() const { return discount; }
int Product::getAvailableQuantity() const { return availableQuantity; }
const std::string &Product::getBarcodeFilePath() const { return barcodeFilePath; }

void Product::setProductID(int id) { productID = id; }
void Product::setBarcode(const std::string &b) { barcode = b; }
void Product::setAcronym(const std::string &a) { acronym = a; }
void Product::setProductName(const std::string &n) { productName = n; }
void Product::setMeasurementUnit(const std::string &u) { measurementUnit = u; }
void Product::setUnitPrice(double price) { unitPrice = price; }
void Product::setTax(double t) { tax = t; }
void Product::setDiscount(double d) { discount = d; }
void Product::setAvailableQuantity(int quantity) { availableQuantity = quantity; }
void Product::setBarcodeFilePath(const std::string &path) { barcodeFilePath = path; }
