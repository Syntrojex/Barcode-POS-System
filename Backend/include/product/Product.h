
#ifndef PRODUCT_H
#define PRODUCT_H

#include <QString>

class Product
{
private:
    int productID;
    QString barcode;
    QString acronym;
    QString productName;
    QString measurementUnit;

    double unitPrice;
    double tax;
    double discount;

    int availableQuantity;
    QString barcodeFilePath;

public:
    Product();

    Product(int id, QString barcode, QString acronym,
        QString name, QString unit,
        double price, double tax,
        double discount, int quantity,
        QString barcodePath);

    int getProductID() const;
    QString getBarcode() const;
    QString getAcronym() const;
    QString getProductName() const;
    QString getMeasurementUnit() const;

    double getUnitPrice() const;
    double getTax() const;
    double getDiscount() const;

    int getAvailableQuantity() const;
    QString getBarcodeFilePath() const;

    void setProductID(int id);
    void setBarcode(QString barcode);
    void setAcronym(QString acronym);
    void setProductName(QString name);
    void setMeasurementUnit(QString unit);

    void setUnitPrice(double price);
    void setTax(double tax);
    void setDiscount(double discount);

    void setAvailableQuantity(int quantity);
    void setBarcodeFilePath(QString path);
};

#endif
