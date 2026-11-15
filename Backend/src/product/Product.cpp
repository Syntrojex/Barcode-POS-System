
#include "Product.h"

Product::Product()
{
    productID = 0;
    unitPrice = 0;
    tax = 0;
    discount = 0;
    availableQuantity = 0;
}

Product::Product(int id, string barcode, string acronym,
                 string name, string unit,
                 double price, double tax,
                 double discount, int quantity,
                 string barcodePath)
{
    productID = id;
    this->barcode = barcode;
    this->acronym = acronym;
    productName = name;
    measurementUnit = unit;

    unitPrice = price;
    this->tax = tax;
    this->discount = discount;

    availableQuantity = quantity;
    barcodeFilePath = barcodePath;
}

int Product::getProductID() const
{
    return productID;
}

string Product::getBarcode() const
{
    return barcode;
}

string Product::getAcronym() const
{
    return acronym;
}

string Product::getProductName() const
{
    return productName;
}

string Product::getMeasurementUnit() const
{
    return measurementUnit;
}

double Product::getUnitPrice() const
{
    return unitPrice;
}

double Product::getTax() const
{
    return tax;
}

double Product::getDiscount() const
{
    return discount;
}

int Product::getAvailableQuantity() const
{
    return availableQuantity;
}

string Product::getBarcodeFilePath() const
{
    return barcodeFilePath;
}

void Product::setProductID(int id)
{
    productID = id;
}

void Product::setBarcode(string barcode)
{
    this->barcode = barcode;
}

void Product::setAcronym(string acronym)
{
    this->acronym = acronym;
}

void Product::setProductName(string name)
{
    productName = name;
}

void Product::setMeasurementUnit(string unit)
{
    measurementUnit = unit;
}

void Product::setUnitPrice(double price)
{
    unitPrice = price;
}

void Product::setTax(double tax)
{
    this->tax = tax;
}

void Product::setDiscount(double discount)
{
    this->discount = discount;
}

void Product::setAvailableQuantity(int quantity)
{
    availableQuantity = quantity;
}

void Product::setBarcodeFilePath(string path)
{
    barcodeFilePath = path;
}
