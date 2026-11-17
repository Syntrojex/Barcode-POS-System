#include "AppState.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QtMath>
#include <algorithm>

#ifndef BARCODEHUB_SOURCE_DIR
#define BARCODEHUB_SOURCE_DIR ""
#endif

ProductInfo ProductInfo::from(const Product &p, const QString &rootDir)
{
    ProductInfo i;
    i.id = p.getProductID();
    i.sku = QString::fromStdString(p.getAcronym());
    i.ean = QString::fromStdString(p.getBarcode());
    i.name = QString::fromStdString(p.getProductName());
    i.unit = QString::fromStdString(p.getMeasurementUnitName());
    i.price = p.getUnitPrice();
    i.tax = p.getTax();
    i.discount = p.getDiscount();
    i.stock = p.getAvailableQuantity();
    const QString path = QString::fromStdString(p.getBarcodeFilePath());
    i.imagePath = QFileInfo(path).isAbsolute() ? path : QDir(rootDir).filePath(path);
    return i;
}

int OrderRecord::itemCount() const
{
    int n = 0;
    for (const auto &l : lines)
        n += l.qty;
    return n;
}

void OrderRecord::recompute()
{
    subtotal = discount = tax = 0;
    for (const auto &l : lines)
    {
        subtotal += l.price * l.qty;
        discount += l.discount * l.qty;
        tax += l.tax * l.qty;
    }
    total = subtotal - discount + tax;
}

QString OrderRecord::summary() const
{
    if (lines.isEmpty())
        return "-";
    QStringList names;
    for (const auto &l : lines)
        names << l.name;
    QString s = QString("%1 item%2  ").arg(itemCount()).arg(itemCount() == 1 ? "" : "s");
    s += names.mid(0, 2).join(", ");
    if (names.size() > 2)
        s += QString(", +%1").arg(names.size() - 2);
    return s;
}

AppState &AppState::instance()
{
    static AppState s;
    return s;
}

QString AppState::locateDataDir()
{
    QStringList candidates;
    const QByteArray env = qgetenv("BARCODEHUB_DATA");
    if (!env.isEmpty())
        candidates << QString::fromLocal8Bit(env);
    const QString app = QCoreApplication::applicationDirPath();
    candidates << app + "/data" << app + "/../data" << app + "/../../data" << app + "/../../../data"
               << QDir::currentPath() + "/data" << QString(BARCODEHUB_SOURCE_DIR) + "/data";
    for (const auto &c : candidates)
        if (QFileInfo::exists(c + "/Product.txt"))
            return QDir::cleanPath(c);
    return QString();
}

AppState::AppState()
{
    const QString dir = locateDataDir();
    if (!dir.isEmpty())
        loadProducts(dir + "/Product.txt");
    seedOrders();
}

bool AppState::loadProducts(const QString &path)
{
    m_root = QDir::cleanPath(QFileInfo(path).absolutePath() + "/..");
    const bool ok = m_manager.loadProducts(path.toStdString());
    refreshCache();
    emit changed();
    return ok;
}

void AppState::refreshCache()
{
    m_products.clear();
    for (const auto &p : m_manager.getAllProducts())
        m_products.push_back(ProductInfo::from(p, m_root));
}

QStringList AppState::productIssues() const
{
    QStringList out;
    const auto &r = m_manager.lastLoad();
    if (!r.fileOpened)
        out << QString::fromStdString(r.summary());
    for (const auto &i : r.issues)
        if (i.line > 0)
            out << QString("%1 line %2: %3")
                       .arg(i.severity == IssueSeverity::Error ? "Error  " : "Warning")
                       .arg(i.line)
                       .arg(QString::fromStdString(i.message));
    return out;
}

std::optional<ProductInfo> AppState::findByCode(const QString &code) const
{
    if (const Product *p = m_manager.searchByCode(code.trimmed().toStdString()))
        return ProductInfo::from(*p, m_root);
    return std::nullopt;
}

QVector<ProductInfo> AppState::searchText(const QString &query) const
{
    QVector<ProductInfo> out;
    for (const Product *p : m_manager.searchByText(query.trimmed().toStdString()))
        out.push_back(ProductInfo::from(*p, m_root));
    return out;
}

ProductDescriptionView AppState::describe(int productId) const
{
    ProductDescriptionView v;
    const Product *p = m_manager.searchByID(productId);
    if (!p)
        return v;
    const ProductDescription d = ProductDescriber(m_settings.lowStockThreshold).describe(*p);
    v.title = QString::fromStdString(d.title);
    v.level = QString::fromStdString(d.stockLevel);
    for (const auto &f : d.fields)
        v.fields.push_back({QString::fromStdString(f.first), QString::fromStdString(f.second)});
    v.imagePath = ProductInfo::from(*p, m_root).imagePath;
    return v;
}

QString AppState::stockProblem(int productId, int alreadyInOrder, int toAdd) const
{
    const Product *p = m_manager.searchByID(productId);
    if (!p)
        return QString(); // custom lines are not stock-controlled
    const StockCheck c = m_stock.checkAdd(*p, alreadyInOrder, toAdd);
    return c.ok() ? QString() : QString::fromStdString(c.message);
}

QVector<ProductInfo> AppState::lowStock() const
{
    QVector<ProductInfo> v;
    for (const auto &p : m_products)
        if (p.stock <= m_settings.lowStockThreshold)
            v << p;
    std::sort(v.begin(), v.end(), [](const ProductInfo &a, const ProductInfo &b) { return a.stock < b.stock; });
    return v;
}

void AppState::reorder(int id)
{
    m_reordered.insert(id);
    emit changed();
}

void AppState::reorderAllLow()
{
    for (const auto &p : lowStock())
        m_reordered.insert(p.id);
    emit changed();
}

QString AppState::nextOrderId()
{
    return QString("%1").arg(m_nextOrderNo, 8, 10, QChar('0'));
}

void AppState::addOrder(const OrderRecord &o)
{
    for (const auto &l : o.lines)
        if (const ProductInfo p = findByCode(l.acronym).value_or(ProductInfo()); p.id > 0)
            m_manager.deductStock(p.id, l.qty);
    std::string err;
    m_manager.saveProducts(&err); // keep Product.txt in step with the sale
    refreshCache();
    m_orders.prepend(o);
    ++m_nextOrderNo;
    ++m_sessionOrders;
    m_sessionSales += o.total;
    emit changed();
}

void AppState::seedOrders()
{
    auto line = [&](int productId, int qty)
    {
        for (const auto &p : m_products)
            if (p.id == productId)
                return ReceiptLine{p.name, p.sku, qty, p.price, p.tax, p.discount};
        return ReceiptLine{};
    };
    struct O
    {
        const char *customer, *contact, *tier, *payment, *merchant, *status;
        int minutesAgo;
        std::vector<std::pair<int, int>> items; // product id, quantity
    };
    const O os[] = {
        {"Walk-in Customer", "-", "Guest", "cash", "-", "Paid", 1, {{5, 2}, {1, 2}, {2, 1}}},
        {"Elena Rostova", "0300-3456712", "VIP", "card", "ABL", "In Progress", 4, {{8, 6}, {12, 3}}},
        {"Marco Diaz", "0321-4412890", "Standard", "wallet", "JazzCash", "On Hold", 10, {{4, 1}, {14, 2}}},
        {"Walk-in Customer", "-", "Guest", "wallet", "Easypaisa", "Paid", 17, {{6, 3}}},
        {"Sarah Jenkins", "0333-2348901", "VIP", "card", "HBL", "Paid", 31, {{3, 1}, {1, 1}, {2, 1}}},
        {"Marcus Vance", "0345-0192834", "Gold", "wallet", "JazzCash", "Paid", 44, {{10, 1}, {5, 2}}},
        {"David Kim", "0300-4412890", "Guest", "cash", "-", "Paid", 58, {{12, 4}}},
        {"Claire Dupont", "0311-8809921", "Rewards", "cash", "-", "On Hold", 71, {{9, 4}, {14, 1}}},
        {"Walk-in Customer", "-", "Guest", "card", "MCB", "Paid", 90, {{8, 3}, {7, 1}}},
        {"Aisha Khan", "0302-1204477", "Standard", "cash", "-", "Paid", 112, {{2, 2}, {3, 1}}},
        {"Tom Becker", "0309-9023310", "VIP", "wallet", "Easypaisa", "Paid", 135, {{13, 2}, {10, 1}}},
        {"Walk-in Customer", "-", "Guest", "cash", "-", "Paid", 160, {{8, 6}}},
    };
    int no = 89210;
    const QDateTime now = QDateTime::currentDateTime();
    for (const auto &o : os)
    {
        OrderRecord r;
        r.id = QString("%1").arg(no--, 8, 10, QChar('0'));
        r.time = now.addSecs(-60 * o.minutesAgo);
        r.customer = o.customer;
        r.contact = o.contact;
        r.tier = o.tier;
        r.payment = o.payment;
        r.merchant = o.merchant;
        r.status = o.status;
        r.shift = QString("POS #0%1 • Shift A").arg(1 + (no % 4));
        for (const auto &it : o.items)
            if (auto l = line(it.first, it.second); l.qty > 0)
                r.lines << l;
        r.recompute();
        if (r.status == "Paid" && r.payment == "cash")
            r.tendered = qCeil(r.total / 100.0) * 100.0;
        m_orders.push_back(r);
    }
}
