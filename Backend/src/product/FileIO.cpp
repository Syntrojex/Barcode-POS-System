// backend/FileIO.cpp
#include "FileIO.h"
#include <QFile>
#include <QTextStream>
#include <QDir>

FileIO::FileIO(const QString& dataDir) : dirPath(dataDir) {
    QDir().mkpath(dirPath);
}

QVector<Product> FileIO::loadProducts() const {
    QVector<Product> products;
    QFile file(filePath("Product.txt"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return products;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        if (line.endsWith('#')) line.chop(1);
        Product p = Product::fromLine(line.toStdString());
        products.append(p);
    }
    return products;
}

bool FileIO::saveProducts(const QVector<Product>& products) const {
    QFile file(filePath("Product.txt"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&file);
    for (const auto& p : products) {
        out << QString::fromStdString(p.productId) << ' '
            << QString::fromStdString(p.barcode)   << ' '
            << QString::fromStdString(p.acronym)   << ' '
            << QString::fromStdString(p.name)      << ' '
            << QString::fromStdString(p.unit)      << ' '
            << p.unitPrice << ' '
            << p.tax        << ' '
            << p.discount   << ' '
            << p.quantity   << ' '
            << QString::fromStdString(p.barcodeImagePath) << "#\n";
    }
    return true;
}

bool FileIO::appendOrder(const Order& o) const {
    QFile file(filePath("Order.txt"));
    if (!file.open(QIODevice::Append | QIODevice::Text))
        return false;
    QTextStream out(&file);
    out << o.orderId << ' '
        << o.date    << ' '
        << o.time    << ' '
        << QString::fromStdString(o.customerName) << ' '
        << QString::fromStdString(o.contact)      << ' '
        << QString::fromStdString(o.status)       << ' '
        << o.paymentAmount << ' '
        << QString::fromStdString(o.paymentMode) << ' '
        << QString::fromStdString(o.merchant)     << "#\n";
    return true;
}

bool FileIO::appendOrderDetail(const OrderDetail& d) const {
    QFile file(filePath("OrderDetail.txt"));
    if (!file.open(QIODevice::Append | QIODevice::Text))
        return false;
    QTextStream out(&file);
    out << d.orderId << ' '
        << d.productId << ' '
        << d.barcode   << ' '
        << d.quantity   << "#\n";
    return true;
}

bool FileIO::appendBill(const Bill& b) const {
    QFile file(filePath("Bill.txt"));
    if (!file.open(QIODevice::Append | QIODevice::Text))
        return false;
    QTextStream out(&file);
    out << b.billId << ' '
        << QString::fromStdString(b.customerName) << ' '
        << b.orderId << ' '
        << QString::fromStdString(b.productAcronym) << ' '
        << b.unitPrice << ' '
        << b.discountPerUnit << ' '
        << b.quantity << ' '
        << b.totalWithoutTax << ' '
        << b.totalTax << ' '
        << b.totalDiscount << ' '
        << b.payable << ' '
        << QString::fromStdString(b.paymentStatus) << "#\n";
    return true;
}
