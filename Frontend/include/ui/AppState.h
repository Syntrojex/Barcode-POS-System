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
#include <QMap>
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
    QDateTime time;   // order created (Order.txt)
    QDateTime paidAt; // payment moment (Bill.txt date/time); the 7-day edit window counts from here
    QString customer, contact, tier, payment, merchant, status, shift;
    QVector<ReceiptLine> lines;
    double subtotal = 0, discount = 0, tax = 0, total = 0; // total = subtotal - discount + tax; discount = ALL discounts
    double tendered = 0; // amount received (cash); 0 for card / wallet
    // Breakdown of discount stored on the bill. Legacy bills only store the combined discount.
    double seasonal = 0, special = 0;
    double seasonalPercent = 0, specialPercent = 0;
    QString seasonName;
    double itemDiscount() const { return discount - seasonal - special; }
    double change() const { return tendered > total ? tendered - total : 0.0; } // balance returned
    QDateTime paymentTime() const { return paidAt.isValid() ? paidAt : time; }
    int itemCount() const;
    void recompute();
    QString summary() const; // "3 items  Milk, Bread, +1"
};

struct Settings
{
    int lowStockThreshold = 10;
    bool autoAddScans = false;
    bool beepOnScan = true;
    barcodehub::order::PaymentMode defaultPayment = barcodehub::order::PaymentMode::Cash;
    bool receiptA4 = false;
    QString receiptFolder;

    static Settings load();
    bool save() const;
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
    QString dataDir() const; // folder that holds Product.txt, Order.txt, Bill.txt, barcodes/
    void reloadAll();        // re-read Product.txt, Order.txt, OrderDetail.txt and Bill.txt (after an external change)

    // ---- manager: catalogue and inventory (written to Product.txt with the backend's ProductFileWriter) ----
    int nextProductId() const;
    // 8-digit barcode "cc" + 5-digit serial + EAN-8 check digit for category `categoryId`; empty if that category is full.
    QString nextBarcode(const QString &categoryId) const;
    int nextSerial(const QString &categoryId) const; // the serial nextBarcode() will use
    // True when the Increment 2 backend would accept a barcode with this 2-digit prefix (it only allows 01-09 today:
    // "a product barcode starts with 0 and never with 00"). Used by the Categories page to refuse unusable ids.
    bool backendAcceptsPrefix(const QString &categoryId) const;
    // Empty when `p` may be saved; otherwise the first problem (backend ProductValidator + duplicate barcode/acronym).
    QString validateProduct(const ProductInfo &p, bool isNew) const;
    bool addProduct(ProductInfo &p, QString *error = nullptr);          // p.id 0 = assign the next id; writes the barcode PNG
    bool updateProduct(const ProductInfo &p, QString *error = nullptr); // matched by p.id
    // Inventory commit: productId -> new available quantity. All-or-nothing; saved to Product.txt.
    bool setStockLevels(const QMap<int, int> &levels, QString *error = nullptr);

    // ---- orders ----
    const QVector<OrderRecord> &orders() const { return m_orders; } // newest first
    QString nextOrderId();
    // Pays an order: creates the order (or, with `existingId`, pays that held / in-progress order under the SAME
    // Order ID), writes the bill and takes the stock. All-or-nothing: on failure every file is put back.
    bool addOrder(OrderRecord &o, QString *error = nullptr, const QString &existingId = QString());
    // Editing a paid order (requirement 3): allowed for 7 days after payment, only after manager approval.
    static constexpr int kEditWindowDays = 7;
    // -1 = not a paid order (no window), 0 = the 7 days are over, 1..7 = days left (rounded up).
    int editDaysLeft(const OrderRecord &o) const;
    // Edits a paid order IN PLACE: same Order ID and Bill ID, new lines/amounts, stock adjusted by the difference.
    bool replaceOrder(const QString &orderId, OrderRecord &edited, QString *error = nullptr);
    // Quantity of each product a paid order already took from stock (credited back while that order is edited).
    QMap<int, int> stockHeldBy(const QString &orderId) const;

    // ---- on-hold orders (requirement 10): real orders in Order.txt with status on-hold ----
    // Saves the cart as an on-hold order (a new one, or `existingId` updated). Returns the Order ID, empty on failure.
    QString holdOrder(const QString &customer, const QString &contact, const QVector<ReceiptLine> &lines,
                      const QString &existingId = QString(), QString *error = nullptr);
    bool discardHeldOrder(const QString &orderId, QString *error = nullptr);
    QVector<OrderRecord> heldOrders() const; // newest first
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
    bool settle(OrderRecord &o, const QString &existingId, bool editPaid, QString *error);
    bool cartFromLines(const QVector<ReceiptLine> &lines, QString *error); // fills the backend cart
    QStringList recordFiles() const; // Product/Order/OrderDetail/Bill/BillDetail.txt (for all-or-nothing saves)
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
