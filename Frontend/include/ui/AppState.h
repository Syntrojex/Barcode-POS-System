#pragma once
// GUI-side application state. Products come from the pure-C++ product module (Product.txt);
// orders are kept in memory (the Order/Bill modules belong to teammates and plug in here later).
// ProductInfo is a Qt-friendly view of the backend Product - the GUI never touches std::string.
#include "product/ProductDescriber.h"
#include "product/ProductManager.h"
#include "product/StockValidator.h"
#include <QDateTime>
#include <QObject>
#include <QPair>
#include <QSet>
#include <QString>
#include <QVector>
#include <optional>

struct ProductInfo
{
    int id = 0; // 0 = custom (manual) line, not in the catalogue
    QString sku;   // acronym (max 8 chars)
    QString ean;   // barcode
    QString name;
    QString unit;  // "count" | "weight"
    double price = 0;    // unit price, no tax
    double tax = 0;      // tax per unit (PKR)
    double discount = 0; // discount per unit (PKR)
    int stock = 0;
    QString imagePath;   // resolved barcode image (may not exist)
    static ProductInfo from(const Product &p, const QString &rootDir);
};

struct ProductDescriptionView
{
    QString title, level; // level: in-stock | low | out
    QVector<QPair<QString, QString>> fields;
    QString imagePath;
};

struct ReceiptLine
{
    QString name, acronym;
    int qty = 1;
    double price = 0;    // unit price, no tax
    double tax = 0;      // per unit
    double discount = 0; // per unit
};

struct OrderRecord
{
    QString id;
    QDateTime time;
    QString customer, contact, tier, payment, merchant, status, shift;
    QVector<ReceiptLine> lines;
    double subtotal = 0, discount = 0, tax = 0, total = 0; // total = subtotal - discount + tax
    double tendered = 0;
    int itemCount() const;
    void recompute();
    QString summary() const; // "3 items  Milk, Bread, +1"
};

struct Settings
{
    int lowStockThreshold = 10;
    bool autoAddScans = false;
    bool beepOnScan = true;
};

class AppState : public QObject
{
    Q_OBJECT
public:
    static AppState &instance();

    // ---- products (backed by Product.txt) ----
    bool loadProducts(const QString &path);
    QString productFile() const { return QString::fromStdString(m_manager.filePath()); }
    QStringList productIssues() const; // "line 4: acronym ... " (errors and warnings)
    int productErrorCount() const { return m_manager.lastLoad().errorCount(); }
    const QVector<ProductInfo> &products() const { return m_products; }
    std::optional<ProductInfo> findByCode(const QString &code) const; // barcode, id or acronym
    QVector<ProductInfo> searchText(const QString &query) const;
    ProductDescriptionView describe(int productId) const;
    // Empty when `toAdd` more units may be added; otherwise the reason (from the StockValidator).
    QString stockProblem(int productId, int alreadyInOrder, int toAdd) const;
    QVector<ProductInfo> lowStock() const;
    bool isReordered(int productId) const { return m_reordered.contains(productId); }
    void reorder(int productId);
    void reorderAllLow();
    static QString locateDataDir();

    // ---- orders ----
    const QVector<OrderRecord> &orders() const { return m_orders; } // newest first
    QString nextOrderId();
    void addOrder(const OrderRecord &o); // deducts stock and saves Product.txt
    double salesToday() const { return m_baseSales + m_sessionSales; }
    int ordersToday() const { return m_baseOrders + m_sessionOrders; }

    Settings &settings() { return m_settings; }
    void emitChanged() { emit changed(); }
    void emitScan(const QString &code) { emit scanned(code); }

signals:
    void changed();
    void scanned(const QString &code);

private:
    AppState();
    void refreshCache();
    void seedOrders();
    ProductManager m_manager;
    StockValidator m_stock;
    QVector<ProductInfo> m_products;
    QString m_root;
    QVector<OrderRecord> m_orders;
    QSet<int> m_reordered;
    Settings m_settings;
    int m_nextOrderNo = 89211;
    double m_baseSales = 184250.0, m_sessionSales = 0;
    int m_baseOrders = 148, m_sessionOrders = 0;
};
