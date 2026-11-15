#pragma once
// In-memory, frontend-only data used by every screen.
// There is NO backend here: products, orders and bills live in RAM with seeded sample data.
// When a real backend exists, replace the bodies of this class and keep the signals.
#include <QDateTime>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>
#include <optional>

struct Product
{
    QString sku;      // e.g. PRD-8840
    QString ean;      // 8-digit EAN-8 (valid check digit)
    QString name;
    QString category; // e.g. Dairy
    QString location; // e.g. Fridge 02
    double price = 0;
    int stock = 0;
    int reorderLimit = 0;
};

struct ReceiptLine
{
    QString name;
    int qty = 1;
    double price = 0;
};

struct OrderRecord
{
    QString id;
    QDateTime time;
    QString customer, contact, tier, payment, status, shift;
    QVector<ReceiptLine> lines;
    double discount = 0;
    double total = 0;    // amount payable (tax included)
    double tendered = 0; // cash handed over (cash payments)
    int itemCount() const
    {
        int n = 0;
        for (const auto &l : lines)
            n += l.qty;
        return n;
    }
    QString summary() const; // "3 items  Milk, Bread, +1"
};

struct Settings
{
    double taxPercent = 8.0;
    double vipDiscountPercent = 5.0;
    bool autoAddScans = false;
    bool beepOnScan = true;
};

class AppState : public QObject
{
    Q_OBJECT
public:
    static AppState &instance();

    // ---- products ----
    const QVector<Product> &products() const { return m_products; }
    std::optional<Product> findByCode(const QString &code) const; // EAN-8, 6-digit prefix or SKU
    QVector<Product> lowStock() const;                            // stock below reorder limit
    bool isReordered(const QString &sku) const { return m_reordered.contains(sku); }
    void reorder(const QString &sku);
    void reorderAllLow();
    static QString ean8(const QString &seven); // appends the check digit

    // ---- orders ----
    const QVector<OrderRecord> &orders() const { return m_orders; } // newest first
    QString nextOrderId();
    void addOrder(const OrderRecord &o); // also decrements stock
    double salesToday() const { return m_baseSales + m_sessionSales; }
    int ordersToday() const { return m_baseOrders + m_sessionOrders; }

    // ---- settings ----
    Settings &settings() { return m_settings; }

    // ---- scanner hand-off (Barcode Scanner page -> Record Order page) ----
    void emitScan(const QString &ean) { emit scanned(ean); }

signals:
    void changed();
    void scanned(const QString &ean);

private:
    AppState();
    void seed();
    QVector<Product> m_products;
    QVector<OrderRecord> m_orders;
    QSet<QString> m_reordered;
    Settings m_settings;
    int m_nextOrderNo = 89211;
    double m_baseSales = 3842.50, m_sessionSales = 0;
    int m_baseOrders = 148, m_sessionOrders = 0;
};
