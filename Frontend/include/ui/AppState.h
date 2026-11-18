#pragma once
// Qt-facing bridge to the Product, Order, and Billing backend modules.
// Data is loaded from and persisted to Product.txt, Order.txt, OrderDetail.txt and Bill.txt.
#include "product/ProductModel.h"
#include "product/ProductStorage.h"
#include "product/ProductValidation.h"
#include "order/OrderManager.h"
#include "billing/BillRepository.h"
#include <memory>
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
    int productId = 0;   // 0 = manually entered product
};

struct OrderRecord
{
    QString id;
    QString billId; // only nonempty when an actual Bill.txt record exists
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
    QString productFile() const { return m_productPath; }
    QStringList productIssues() const; // "line 4: acronym ... " (errors and warnings)
    int productErrorCount() const { return m_manager.getLastLoad().errorCount(); }
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
    bool addOrder(OrderRecord &o, QString *error = nullptr); // creates order, bill, updates stock; reports failures
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
    void refreshOrders();
    ProductManager m_manager;
    StockValidator m_stock;
    QVector<ProductInfo> m_products;
    QString m_root;
    QString m_productPath;
    std::unique_ptr<barcodehub::order::OrderManager> m_orderManager;
    std::unique_ptr<barcodehub::billing::BillRepository> m_billRepository;
    QVector<OrderRecord> m_orders;
    QSet<int> m_reordered;
    Settings m_settings;
    double m_baseSales = 0, m_sessionSales = 0;
    int m_baseOrders = 0, m_sessionOrders = 0;
};
