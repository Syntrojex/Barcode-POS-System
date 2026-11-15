#include "AppState.h"
#include <QtMath>
#include <algorithm>

AppState &AppState::instance()
{
    static AppState s;
    return s;
}

AppState::AppState() { seed(); }

QString AppState::ean8(const QString &seven)
{
    int sum = 0;
    for (int i = 0; i < 7; ++i)
        sum += seven[i].digitValue() * ((i % 2 == 0) ? 3 : 1);
    return seven + QString::number((10 - sum % 10) % 10);
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

std::optional<Product> AppState::findByCode(const QString &codeIn) const
{
    const QString code = codeIn.trimmed();
    if (code.isEmpty())
        return std::nullopt;
    for (const auto &p : m_products)
        if (p.ean == code || p.sku.compare(code, Qt::CaseInsensitive) == 0 ||
            (code.size() == 6 && p.ean.left(6) == code))
            return p;
    return std::nullopt;
}

QVector<Product> AppState::lowStock() const
{
    QVector<Product> v;
    for (const auto &p : m_products)
        if (p.stock < p.reorderLimit)
            v << p;
    std::sort(v.begin(), v.end(), [](const Product &a, const Product &b)
              { return double(a.stock) / a.reorderLimit < double(b.stock) / b.reorderLimit; });
    return v;
}

void AppState::reorder(const QString &sku)
{
    m_reordered.insert(sku);
    emit changed();
}

void AppState::reorderAllLow()
{
    for (const auto &p : lowStock())
        m_reordered.insert(p.sku);
    emit changed();
}

QString AppState::nextOrderId()
{
    return QString("ORD-%1").arg(m_nextOrderNo, 8, 10, QChar('0'));
}

void AppState::addOrder(const OrderRecord &o)
{
    for (const auto &l : o.lines)
        for (auto &p : m_products)
            if (p.name == l.name)
                p.stock = qMax(0, p.stock - l.qty);
    m_orders.prepend(o);
    ++m_nextOrderNo;
    ++m_sessionOrders;
    m_sessionSales += o.total;
    emit changed();
}

void AppState::seed()
{
    struct P
    {
        const char *sku, *base7, *name, *cat, *loc;
        double price;
        int stock, limit;
    };
    const P ps[] = {
        {"PRD-102", "4000102", "Organic Whole Milk 1L", "Dairy", "Fridge 02", 3.80, 2, 15},
        {"PRD-214", "4000214", "Whole Wheat Bread 600g", "Bakery", "Batch #441", 4.20, 34, 10},
        {"PRD-8840", "4008840", "Artisan Roast Coffee Beans 500g", "Aisle 4", "Single Origin", 14.50, 48, 12},
        {"PRD-512", "4000512", "Dark Chocolate Bar 85% 100g", "Confectionery", "Shelf B-3", 5.60, 120, 25},
        {"PRD-551", "4000551", "Fresh Eggs 12pk", "Dairy", "Fridge 01", 5.10, 4, 20},
        {"PRD-303", "4000303", "Avocado Hass ea", "Produce", "Bin 7", 1.80, 5, 30},
        {"PRD-497", "4000497", "Ground Coffee 250g", "Aisle 4", "Shelf A-2", 7.90, 6, 25},
        {"PRD-611", "4000611", "Olive Oil Extra Virgin 750ml", "Pantry", "Aisle 2", 9.40, 22, 8},
        {"PRD-612", "4000612", "Aged Balsamic Vinegar 250ml", "Pantry", "Aisle 2", 6.75, 3, 10},
        {"PRD-620", "4000620", "Sea Salt 500g", "Pantry", "Aisle 2", 2.30, 60, 15},
        {"PRD-701", "4000701", "Almond Milk 1L", "Dairy", "Fridge 02", 3.60, 28, 12},
        {"PRD-702", "4000702", "Gluten Free Granola 400g", "Cereal", "Aisle 5", 6.40, 9, 12},
        {"PRD-715", "4000715", "Sourdough Loaf", "Bakery", "Batch #442", 5.20, 14, 8},
        {"PRD-733", "4000733", "French Butter Croissants 4pk", "Bakery", "Batch #443", 6.10, 7, 10},
        {"PRD-741", "4000741", "Raspberry Preserves 340g", "Pantry", "Aisle 3", 4.45, 18, 8},
        {"PRD-760", "4000760", "Single Origin Espresso Beans 250g", "Aisle 4", "Shelf A-1", 9.80, 25, 10},
    };
    for (const auto &p : ps)
        m_products.push_back({p.sku, ean8(p.base7), p.name, p.cat, p.loc, p.price, p.stock, p.limit});

    auto line = [&](const char *sku, int qty)
    {
        for (const auto &p : m_products)
            if (p.sku == sku)
                return ReceiptLine{p.name, qty, p.price};
        return ReceiptLine{};
    };
    struct O
    {
        const char *customer, *contact, *tier, *payment, *status;
        int minutesAgo;
        std::initializer_list<std::pair<const char *, int>> items;
    };
    const O os[] = {
        {"Walk-in Customer", "-", "Guest", "Cash", "Paid", 1, {{"PRD-8840", 1}, {"PRD-102", 2}, {"PRD-715", 1}}},
        {"Elena Rostova", "+1 (555) 345-6712", "VIP", "Visa •• 4912", "In Progress", 4, {{"PRD-701", 2}, {"PRD-702", 3}}},
        {"Marco Diaz", "+1 (555) 441-2890", "Standard", "Pending Tender", "On Hold", 10, {{"PRD-760", 1}, {"PRD-733", 2}}},
        {"Walk-in Customer", "-", "Guest", "NFC Pay", "Paid", 17, {{"PRD-620", 2}}},
        {"Sarah Jenkins", "+1 (555) 234-8901", "VIP", "Visa •• 4210", "Paid", 31, {{"PRD-8840", 1}, {"PRD-102", 1}, {"PRD-715", 1}}},
        {"Marcus Vance", "+1 (555) 019-2834", "Gold", "Wallet", "Paid", 44, {{"PRD-611", 2}, {"PRD-612", 1}, {"PRD-620", 1}}},
        {"David Kim", "+1 (555) 441-2890", "Guest", "Cash", "Paid", 58, {{"PRD-760", 1}}},
        {"Claire Dupont", "+1 (555) 880-9921", "Rewards", "Pending Tender", "On Hold", 71, {{"PRD-733", 4}, {"PRD-741", 1}}},
        {"Walk-in Customer", "-", "Guest", "Visa •• 7781", "Paid", 90, {{"PRD-512", 3}, {"PRD-551", 1}}},
        {"Aisha Khan", "+1 (555) 120-4477", "Standard", "Cash", "Paid", 112, {{"PRD-214", 2}, {"PRD-701", 1}}},
        {"Tom Becker", "+1 (555) 902-3310", "VIP", "Wallet", "Paid", 135, {{"PRD-497", 2}, {"PRD-512", 1}}},
        {"Walk-in Customer", "-", "Guest", "Cash", "Paid", 160, {{"PRD-303", 6}}},
    };
    int no = 89210;
    const QDateTime now = QDateTime::currentDateTime();
    for (const auto &o : os)
    {
        OrderRecord r;
        r.id = QString("ORD-%1").arg(no--, 8, 10, QChar('0'));
        r.time = now.addSecs(-60 * o.minutesAgo);
        r.customer = o.customer;
        r.contact = o.contact;
        r.tier = o.tier;
        r.payment = o.payment;
        r.status = o.status;
        r.shift = QString("POS #0%1 • Shift A").arg(1 + (no % 4));
        double gross = 0;
        for (const auto &it : o.items)
        {
            r.lines << line(it.first, it.second);
            gross += r.lines.last().price * it.second;
        }
        r.total = gross;
        if (r.status == "Paid" && r.payment.startsWith("Cash"))
            r.tendered = qCeil(gross / 5.0) * 5.0;
        m_orders.push_back(r);
    }
}
