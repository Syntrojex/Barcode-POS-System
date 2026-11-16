#pragma once
#include <QString>
#include <QVector>
#include "product\Models.h"

class FileIO {
public:
    explicit FileIO(const QString& dataDir);

    // Product handling
    QVector<Product> loadProducts() const;
    bool saveProducts(const QVector<Product>& products) const;

    // Order handling
    bool appendOrder(const Order& order) const;
    bool appendOrderDetail(const OrderDetail& detail) const;

    // Bill handling
    bool appendBill(const Bill& bill) const;

private:
    QString dirPath;
    QString filePath(const QString& name) const { return dirPath + QDir::separator() + name; }
};
