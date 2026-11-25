#include "AppState.h"
#include "BarcodeArt.h"
#include "integration/ProductAdapter.h"
#include "billing/BillingService.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QSaveFile>
#include <QSettings>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <map>

#ifndef BARCODEHUB_SOURCE_DIR
#define BARCODEHUB_SOURCE_DIR ""
#endif

ProductInfo ProductInfo::from(const Product& p, const QString& rootDir)
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
    for (const auto& l : lines)
        n += l.qty;
    return n;
}

void OrderRecord::recompute()
{
    subtotal = discount = tax = 0;
    for (const auto& l : lines)
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
    for (const auto& l : lines)
        names << l.name;
    QString s = QString("%1 item%2  ").arg(itemCount()).arg(itemCount() == 1 ? "" : "s");
    s += names.mid(0, 2).join(", ");
    if (names.size() > 2)
        s += QString(", +%1").arg(names.size() - 2);
    return s;
}

AppState& AppState::instance()
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
    for (const auto& c : candidates)
        if (QFileInfo::exists(c + "/Product.txt"))
            return QDir::cleanPath(c);
    return QString();
}

Settings Settings::load()
{
    QSettings store(QSettings::defaultFormat(), QSettings::UserScope, "BarcodeHub", "SmartPOS");
    store.beginGroup("terminal");
    Settings settings;
    settings.lowStockThreshold = qBound(0, store.value("lowStockThreshold", settings.lowStockThreshold).toInt(), 1000);
    settings.autoAddScans = store.value("autoAddScans", settings.autoAddScans).toBool();
    settings.beepOnScan = store.value("beepOnScan", settings.beepOnScan).toBool();
    using Payment = barcodehub::order::PaymentMode;
    const auto payment = static_cast<Payment>(store.value("defaultPayment", int(Payment::Cash)).toInt());
    if (payment == Payment::Card || payment == Payment::Wallet)
        settings.defaultPayment = payment;
    settings.receiptA4 = store.value("receiptA4", settings.receiptA4).toBool();
    settings.receiptFolder = store.value("receiptFolder").toString();
    return settings;
}

bool Settings::save() const
{
    QSettings store(QSettings::defaultFormat(), QSettings::UserScope, "BarcodeHub", "SmartPOS");
    store.beginGroup("terminal");
    store.setValue("lowStockThreshold", lowStockThreshold);
    store.setValue("autoAddScans", autoAddScans);
    store.setValue("beepOnScan", beepOnScan);
    store.setValue("defaultPayment", int(defaultPayment));
    store.setValue("receiptA4", receiptA4);
    store.setValue("receiptFolder", receiptFolder);
    store.sync();
    return store.status() == QSettings::NoError;
}

AppState::AppState() : m_settings(Settings::load())
{
    const QString dir = locateDataDir();
    if (!dir.isEmpty())
        loadProducts(dir + "/Product.txt");
    if (!dir.isEmpty())
    {
        const std::string root = dir.toStdString();
        m_orderManager = std::make_unique<barcodehub::order::OrderManager>(root + "/Order.txt", root + "/OrderDetail.txt");
        m_billRepository = std::make_unique<barcodehub::billing::BillRepository>(root + "/Bill.txt");
        refreshOrders();
    }
}

bool AppState::loadProducts(const QString& path)
{
    m_root = QDir::cleanPath(QFileInfo(path).absolutePath() + "/..");
    m_productPath = path;
    const bool ok = m_manager.loadProducts(path.toStdString());
    refreshCache();
    emit changed();
    return ok;
}

void AppState::refreshCache()
{
    m_products.clear();
    for (const auto& p : m_manager.getProducts())
        m_products.push_back(ProductInfo::from(p, m_root));
}

QStringList AppState::productIssues() const
{
    QStringList out;
    const auto& r = m_manager.getLastLoad();
    if (!r.fileOpened)
        out << QString::fromStdString(r.summary());
    for (const auto& i : r.issues)
        if (i.line > 0)
            out << QString("%1 line %2: %3")
            .arg(i.severity == IssueSeverity::Error ? "Error  " : "Warning")
            .arg(i.line)
            .arg(QString::fromStdString(i.message));
    return out;
}

std::optional<ProductInfo> AppState::findByCode(const QString& code) const
{
    if (const Product* p = m_manager.searchByCode(code.trimmed().toStdString()))
        return ProductInfo::from(*p, m_root);
    return std::nullopt;
}

QVector<ProductInfo> AppState::searchText(const QString& query) const
{
    QVector<ProductInfo> out;
    for (const Product* p : m_manager.searchByText(query.trimmed().toStdString()))
        out.push_back(ProductInfo::from(*p, m_root));
    return out;
}

ProductDescriptionView AppState::describe(int productId) const
{
    ProductDescriptionView v;
    const Product* p = m_manager.searchByID(productId);
    if (!p)
        return v;
    const ProductDescription d = ProductDescriber(m_settings.lowStockThreshold).describe(*p);
    v.title = QString::fromStdString(d.title);
    v.level = QString::fromStdString(d.stockLevel);
    for (const auto& f : d.fields)
        v.fields.push_back({ QString::fromStdString(f.first), QString::fromStdString(f.second) });
    v.imagePath = ProductInfo::from(*p, m_root).imagePath;
    return v;
}

QString AppState::stockProblem(int productId, int alreadyInOrder, int toAdd) const
{
    const Product* p = m_manager.searchByID(productId);
    if (!p)
        return QString(); // custom lines are not stock-controlled
    const StockCheck c = m_stock.checkAdd(*p, alreadyInOrder, toAdd);
    return c.ok() ? QString() : QString::fromStdString(c.message);
}

QVector<ProductInfo> AppState::lowStock() const
{
    QVector<ProductInfo> v;
    for (const auto& p : m_products)
        if (p.stock <= m_settings.lowStockThreshold)
            v << p;
    std::sort(v.begin(), v.end(), [](const ProductInfo& a, const ProductInfo& b) { return a.stock < b.stock; });
    return v;
}

void AppState::reorder(int id)
{
    m_reordered.insert(id);
    emit changed();
}

void AppState::reorderAllLow()
{
    for (const auto& p : lowStock())
        m_reordered.insert(p.id);
    emit changed();
}

QString AppState::nextOrderId()
{
    if (!m_orderManager)
        return "--------";
    std::string error;
    const std::string id = m_orderManager->nextOrderId(&error);
    return id.empty() ? QString("--------") : QString::fromStdString(id);
}

// ======================================================================== orders: paying, editing, holding
QStringList AppState::recordFiles() const
{
    const QString d = dataDir();
    return { m_productPath, d + "/Order.txt", d + "/OrderDetail.txt", d + "/Bill.txt", d + "/BillDetail.txt" };
}

bool AppState::cartFromLines(const QVector<ReceiptLine>& lines, QString* error)
{
    namespace bh = barcodehub;
    m_orderManager->clearCart();
    std::string why;
    for (const auto& line : lines)
    {
        bh::order::ProductSnapshot snapshot;
        if (line.productId > 0)
        {
            const Product* p = m_manager.searchByID(line.productId);
            if (!p || QString::fromStdString(p->getAcronym()) != line.acronym)
            {
                if (error) *error = "A product in the order no longer exists. Refresh and retry.";
                m_orderManager->clearCart();
                return false;
            }
            snapshot = bh::integration::toSnapshot(*p);
            snapshot.discountPerUnit = line.discount; // product discount (seasonal/special are bill-level)
            snapshot.availableQuantity = 1000000;     // stock is checked separately (holds take no stock)
        }
        else
        {
            snapshot.productId = "CUSTOM-" + line.acronym.toStdString();
            snapshot.barcode = snapshot.productId;
            snapshot.acronym = line.acronym.toStdString();
            snapshot.name = line.name.toStdString();
            snapshot.measurementUnit = "count";
            snapshot.unitPrice = line.price;
            snapshot.taxPerUnit = line.tax;
            snapshot.discountPerUnit = line.discount;
            snapshot.availableQuantity = 1000000;
        }
        if (!m_orderManager->addToCart(snapshot, line.qty, &why))
        {
            if (error) *error = QString::fromStdString(why);
            m_orderManager->clearCart();
            return false;
        }
    }
    return true;
}

bool AppState::addOrder(OrderRecord& o, QString* error, const QString& existingId)
{
    return settle(o, existingId, false, error);
}

bool AppState::replaceOrder(const QString& orderId, OrderRecord& edited, QString* error)
{
    return settle(edited, orderId, true, error);
}

QMap<int, int> AppState::stockHeldBy(const QString& orderId) const
{
    QMap<int, int> held;
    for (const auto& o : m_orders)
        if (o.id == orderId && o.status == "Paid")
            for (const auto& l : o.lines)
                if (l.productId > 0)
                    held[l.productId] += l.qty;
    return held;
}

// One routine for every payment, so a new order, a resumed held order and an edited paid order follow the
// same rules. Everything it writes (Product, Order, OrderDetail, Bill, BillDetail) is backed up first and put
// back if any step fails, so a failed checkout never leaves half-saved records or wrong stock.
bool AppState::settle(OrderRecord& o, const QString& existingId, bool editPaid, QString* error)
{
    namespace bh = barcodehub;
    if (error) error->clear();
    if (!m_orderManager || !m_billRepository)
    {
        if (error) *error = "Data directory is unavailable. Check BARCODEHUB_DATA and Product.txt.";
        return false;
    }
    if (o.lines.isEmpty())
    {
        if (error) *error = "The order is empty.";
        return false;
    }

    std::string why;
    bh::order::Order existing;
    if (!existingId.isEmpty())
    {
        if (!m_orderManager->findOrder(existingId.toStdString(), existing, &why))
        {
            if (error) *error = "Order " + existingId + " was not found.";
            return false;
        }
        const auto st = existing.status;
        if (st == bh::order::OrderStatus::Archived)
        {
            if (error) *error = "Order " + existingId + " is archived and cannot be changed.";
            return false;
        }
        if (editPaid && st != bh::order::OrderStatus::Paid)
        {
            if (error) *error = "Order " + existingId + " is not a paid order.";
            return false;
        }
        if (!editPaid && st == bh::order::OrderStatus::Paid)
        {
            if (error) *error = "Order " + existingId + " is already paid.";
            return false;
        }
        if (editPaid)
            for (const auto& rec : m_orders)
                if (rec.id == existingId && editDaysLeft(rec) == 0)
                {
                    if (error) *error = "The 7-day edit window of order " + existingId + " is over.";
                    return false;
                }
    }

    bh::order::PaymentMode mode = bh::order::PaymentMode::Cash;
    if (!bh::order::paymentModeFromString(o.payment.toLower().toStdString(), mode))
    {
        if (error) *error = "Unsupported payment method.";
        return false;
    }
    bh::billing::PaymentRequest payment;
    payment.mode = mode;
    payment.merchant = o.merchant.toStdString();
    payment.amountTendered = o.tendered;
    const auto methods = bh::billing::PaymentMethodRegistry::withDefaults();
    if (!methods.find(mode))
    {
        if (error) *error = "Payment method unavailable.";
        return false;
    }

    // ---- back up every record file, so any failure below can put them all back
    QMap<QString, QByteArray> backup;
    QSet<QString> absent;
    for (const QString& f : recordFiles())
    {
        QFile file(f);
        if (!file.exists()) { absent.insert(f); continue; }
        if (!file.open(QIODevice::ReadOnly))
        {
            if (error) *error = "Cannot back up " + QFileInfo(f).fileName() + " before saving.";
            return false;
        }
        backup[f] = file.readAll();
    }
    auto fail = [&](const QString& message) {
        bool clean = true;
        for (auto it = backup.constBegin(); it != backup.constEnd(); ++it)
        {
            QSaveFile out(it.key());
            clean = out.open(QIODevice::WriteOnly) && out.write(it.value()) == it.value().size() && out.commit() && clean;
        }
        for (const QString& f : absent)
            QFile::remove(f);
        m_orderManager->clearCart();
        m_manager.loadProducts(m_productPath.toStdString());
        refreshCache();
        refreshOrders();
        emit changed();
        if (error) *error = message + (clean ? QString() : QString(" (restoring the files failed - check the data folder)"));
        return false;
    };

    // ---- an edited paid order first gives back the stock it took, then takes what the edited lines need
    if (editPaid)
    {
        QMap<int, int> levels;
        const auto held = stockHeldBy(existingId);
        for (auto it = held.constBegin(); it != held.constEnd(); ++it)
            if (const Product* p = m_manager.searchByID(it.key()))
                levels[it.key()] = p->getAvailableQuantity() + it.value();
        QString stockError;
        if (!levels.isEmpty() && !setStockLevels(levels, &stockError))
            return fail(stockError);
    }

    // ---- the backend cart (validates every line)
    std::map<std::string, bh::order::ProductSnapshot> manualProducts;
    std::map<std::string, double> lineDiscount; // product id -> the product's own per-unit discount
    std::map<int, int> stockToDeduct;
    QString cartError;
    if (!cartFromLines(o.lines, &cartError))
        return fail(cartError);
    for (const auto& item : m_orderManager->cart())
    {
        if (item.product.productId.rfind("CUSTOM-", 0) == 0)
            manualProducts[item.product.productId] = item.product;
        else
        {
            lineDiscount[item.product.productId] = item.product.discountPerUnit;
            stockToDeduct[std::stoi(item.product.productId)] += static_cast<int>(item.quantity);
        }
    }
    for (const auto& [id, qty] : stockToDeduct)
        if (const auto* p = m_manager.searchByID(id); !p || qty > p->getAvailableQuantity())
            return fail(QString("Not enough stock of %1 (%2 available).")
                            .arg(p ? QString::fromStdString(p->getProductName()) : QString::number(id))
                            .arg(p ? p->getAvailableQuantity() : 0));

    // ---- totals must match the backend (seasonal and special discounts are bill-level amounts)
    const double seasonal = std::max(0.0, o.seasonal), special = std::max(0.0, o.special);
    o.recompute(); // line totals only
    o.seasonal = seasonal;
    o.special = special;
    o.total = std::round((o.total - seasonal - special) * 100.0) / 100.0;
    const double payable = std::round((m_orderManager->totalPayable() - seasonal - special) * 100.0) / 100.0;
    if (std::fabs(o.total - payable) > 0.011)
        return fail("Cart totals differ from backend totals. Please refresh the order.");
    const auto accepted = methods.find(mode)->process(payment, payable);
    if (!accepted.ok)
        return fail(QString::fromStdString(accepted.error));

    // ---- Order.txt / OrderDetail.txt: a new order, or the existing one keeps its Order ID
    const std::string name = o.customer.trimmed().isEmpty() ? "Walk-in Customer" : o.customer.trimmed().toStdString();
    const std::string contact = (o.contact == "-" ? QString() : o.contact.trimmed()).toStdString();
    std::string orderId;
    if (existingId.isEmpty())
    {
        bh::order::Order created;
        if (!m_orderManager->createOrder(name, contact, created, &why))
            return fail(QString::fromStdString(why));
        orderId = created.id;
    }
    else
    {
        if (!m_orderManager->saveCartToOrder(existingId.toStdString(), name, contact, &why))
            return fail(QString::fromStdString(why));
        orderId = existingId.toStdString();
    }

    // ---- stock (Product.txt)
    for (const auto& [id, qty] : stockToDeduct)
        m_manager.deductStock(id, qty);
    if (!m_manager.saveProducts(&why))
        return fail("Could not save stock: " + QString::fromStdString(why));

    // ---- bill (Bill.txt / BillDetail.txt); an edited paid order keeps its Bill ID and payment date
    bh::billing::BillingService service(*m_orderManager, *m_billRepository,
        [this, &manualProducts, &lineDiscount](const std::string& id, bh::order::ProductSnapshot& snapshot) {
            const auto custom = manualProducts.find(id);
            if (custom != manualProducts.end()) { snapshot = custom->second; return true; }
            try {
                const Product* p = m_manager.searchByID(std::stoi(id));
                if (p) {
                    snapshot = bh::integration::toSnapshot(*p);
                    const auto d = lineDiscount.find(id);
                    if (d != lineDiscount.end()) snapshot.discountPerUnit = d->second;
                    return true;
                }
            }
            catch (...) {}
            return false;
        });
    bh::billing::Bill bill;
    double change = 0;
    bh::billing::BillAdjustments adjustments;
    adjustments.seasonalDiscount = seasonal;
    adjustments.specialDiscount = special;
    const bool billed = editPaid ? service.rebill(orderId, payment, bill, &change, &why, adjustments)
                                 : service.checkout(orderId, payment, bill, &change, &why, adjustments);
    if (!billed)
        return fail("Checkout failed: " + QString::fromStdString(why));

    bh::order::Order saved;
    m_orderManager->findOrder(orderId, saved, &why);
    o.id = QString::fromStdString(orderId);
    o.billId = QString::fromStdString(bill.id);
    o.time = QDateTime::fromString(QString::fromStdString(saved.date + " " + saved.time), "yyyy-MM-dd HH:mm:ss");
    o.paidAt = QDateTime::fromString(QString::fromStdString(bill.date + " " + bill.time), "yyyy-MM-dd HH:mm:ss");
    o.status = "Paid";
    o.total = bill.totals.payable;
    o.subtotal = bill.totals.withoutTax;
    o.discount = bill.totals.discount + bill.seasonalDiscount + bill.specialDiscount; // OrderRecord: all discounts
    o.seasonal = bill.seasonalDiscount;
    o.special = bill.specialDiscount;
    o.tax = bill.totals.tax;
    o.tendered = bill.amountReceived;
    m_manager.loadProducts(m_productPath.toStdString());
    refreshCache();
    refreshOrders();
    // The current session knows the manual item descriptions even if the catalogue doesn't.
    for (auto& history : m_orders)
        if (history.id == o.id) { history = o; break; }
    emit changed();
    return true;
}

int AppState::editDaysLeft(const OrderRecord& o) const
{
    if (o.status != "Paid")
        return -1;
    const QDateTime paid = o.paymentTime();
    const qint64 window = qint64(kEditWindowDays) * 86400;
    const qint64 secs = paid.secsTo(QDateTime::currentDateTime());
    if (secs > window)
        return 0;
    return qBound(1, int((window - secs + 86399) / 86400), kEditWindowDays);
}

// ---------------------------------------------------------------- on-hold orders
QString AppState::holdOrder(const QString& customer, const QString& contact, const QVector<ReceiptLine>& lines,
                            const QString& existingId, QString* error)
{
    namespace bh = barcodehub;
    if (error) error->clear();
    if (!m_orderManager)
    {
        if (error) *error = "Data directory is unavailable.";
        return {};
    }
    if (lines.isEmpty())
    {
        if (error) *error = "Nothing to hold - the order is empty.";
        return {};
    }
    QString cartError;
    if (!cartFromLines(lines, &cartError))
    {
        if (error) *error = cartError;
        return {};
    }
    const std::string name = customer.trimmed().isEmpty() ? "Walk-in Customer" : customer.trimmed().toStdString();
    const std::string phone = (contact.trimmed() == "-" ? QString() : contact.trimmed()).toStdString();
    std::string why;
    bh::order::Order order;
    bool ok = false;
    if (!existingId.isEmpty() && m_orderManager->findOrder(existingId.toStdString(), order, &why) &&
        (order.status == bh::order::OrderStatus::OnHold || order.status == bh::order::OrderStatus::InProgress))
    {
        ok = m_orderManager->saveCartToOrder(order.id, name, phone, &why) &&
             m_orderManager->findOrder(order.id, order, &why);
    }
    else
        ok = m_orderManager->createOrder(name, phone, order, &why);
    if (ok)
    {
        order.status = bh::order::OrderStatus::OnHold;
        ok = m_orderManager->updateOrder(order, &why);
    }
    m_orderManager->clearCart();
    refreshOrders();
    emit changed();
    if (!ok)
    {
        if (error) *error = "Could not put the order on hold: " + QString::fromStdString(why);
        return {};
    }
    return QString::fromStdString(order.id);
}

bool AppState::discardHeldOrder(const QString& orderId, QString* error)
{
    if (!m_orderManager)
        return false;
    std::string why;
    const bool ok = m_orderManager->discardOrder(orderId.toStdString(), &why);
    if (!ok && error)
        *error = QString::fromStdString(why);
    refreshOrders();
    emit changed();
    return ok;
}

QVector<OrderRecord> AppState::heldOrders() const
{
    QVector<OrderRecord> held;
    for (const auto& o : m_orders)
        if (o.status == "On Hold" || o.status == "In Progress")
            held << o;
    return held;
}

void AppState::refreshOrders()
{
    m_orders.clear();
    m_baseSales = 0;
    m_baseOrders = 0;
    if (!m_orderManager || !m_billRepository) return;
    std::vector<barcodehub::order::Order> orders;
    std::vector<barcodehub::order::OrderDetail> details;
    std::vector<barcodehub::billing::Bill> bills;
    std::string error;
    if (!m_orderManager->loadOrders(orders, &error) ||
        !m_orderManager->loadOrderDetails(details, &error) ||
        !m_billRepository->loadAll(bills, &error))
    {
        qWarning("Could not load persisted orders/bills: %s", error.c_str());
        return;
    }
    const QDate today = QDate::currentDate();
    for (const auto& entry : orders)
    {
        OrderRecord rec;
        rec.id = QString::fromStdString(entry.id);
        rec.time = QDateTime::fromString(QString::fromStdString(entry.date + " " + entry.time), "yyyy-MM-dd HH:mm:ss");
        rec.customer = QString::fromStdString(entry.customerName);
        rec.contact = entry.customerContact.empty() ? "-" : QString::fromStdString(entry.customerContact);
        rec.tier = "Guest";
        rec.shift = "POS #01";
        rec.status = QString::fromStdString(barcodehub::order::toString(entry.status));
        if (entry.status == barcodehub::order::OrderStatus::InProgress) rec.status = "In Progress";
        if (entry.status == barcodehub::order::OrderStatus::OnHold) rec.status = "On Hold";
        if (entry.status == barcodehub::order::OrderStatus::Paid) rec.status = "Paid";
        if (entry.status == barcodehub::order::OrderStatus::Archived) rec.status = "Archived";
        rec.payment = QString::fromStdString(barcodehub::order::toString(entry.paymentMode));
        rec.merchant = QString::fromStdString(entry.paymentMerchant);
        const auto bill = std::find_if(bills.begin(), bills.end(), [&](const auto& b) { return b.orderId == entry.id; });
        if (bill != bills.end())
        {
            rec.billId = QString::fromStdString(bill->id);
            rec.paidAt = QDateTime::fromString(QString::fromStdString(bill->date + " " + bill->time), "yyyy-MM-dd HH:mm:ss");
        }
        int detailIndex = 0;
        for (const auto& detail : details)
        {
            if (detail.orderId != entry.id) continue;
            const Product* p = nullptr;
            try { p = m_manager.searchByID(std::stoi(detail.productId)); }
            catch (...) {}
            ReceiptLine line;
            line.name = p ? QString::fromStdString(p->getProductName()) : QString::fromStdString(detail.productId);
            line.acronym = p ? QString::fromStdString(p->getAcronym()) : QString::fromStdString(detail.productId);
            line.productId = p ? p->getProductID() : 0;
            line.qty = static_cast<int>(detail.quantity);
            line.price = p ? p->getUnitPrice() : 0;
            line.tax = p ? p->getTax() : 0;
            line.discount = p ? p->getDiscount() : 0;
            if (bill != bills.end() && detailIndex < static_cast<int>(bill->lines.size()))
            {
                const auto& saved = bill->lines[detailIndex];
                line.acronym = QString::fromStdString(saved.productAcronym);
                if (!p) line.name = line.acronym;
                line.price = saved.unitPrice;
                line.discount = saved.discountPerUnit;
            }
            rec.lines.push_back(line);
            ++detailIndex;
        }
        rec.recompute();
        if (bill != bills.end())
        {
            rec.subtotal = bill->totals.withoutTax;
            rec.discount = bill->totals.discount + bill->seasonalDiscount + bill->specialDiscount; // all discounts
            rec.seasonal = bill->seasonalDiscount;
            rec.special = bill->specialDiscount;
            rec.tax = bill->totals.tax;
            rec.total = bill->totals.payable;
            rec.tendered = bill->amountReceived;
        }
        if (rec.time.date() == today)
            ++m_baseOrders;
        if (rec.status == "Paid" && rec.paymentTime().date() == today)
            m_baseSales += rec.total;
        m_orders.push_back(std::move(rec));
    }
    std::sort(m_orders.begin(), m_orders.end(), [](const OrderRecord& a, const OrderRecord& b) { return a.time > b.time; });
}

// ======================================================================== data directory / reload
QString AppState::dataDir() const
{
    return m_productPath.isEmpty() ? QString() : QFileInfo(m_productPath).absolutePath();
}

void AppState::reloadAll()
{
    if (!m_productPath.isEmpty())
        m_manager.loadProducts(m_productPath.toStdString());
    refreshCache();
    refreshOrders();
    emit changed();
}

// ======================================================================== catalogue (manager)
namespace
{
    Product toProduct(const ProductInfo& p, const QString& barcodeFile)
    {
        MeasurementUnit unit = MeasurementUnit::Count;
        parseMeasurementUnit(p.unit.toStdString(), unit);
        return Product(p.id, p.ean.toStdString(), p.sku.toStdString(), p.name.toStdString(), unit, p.price, p.tax,
                       p.discount, p.stock, barcodeFile.toStdString());
    }
}

int AppState::nextProductId() const
{
    int maxId = 0;
    for (const auto& p : m_products)
        maxId = qMax(maxId, p.id);
    return maxId + 1;
}

int AppState::nextSerial(const QString& categoryId) const
{
    int maxSerial = 0;
    for (const auto& p : m_products)
        if (p.ean.size() == 8 && p.ean.left(2) == categoryId)
            maxSerial = qMax(maxSerial, p.ean.mid(2, 5).toInt());
    return maxSerial + 1;
}

QString AppState::nextBarcode(const QString& categoryId) const
{
    if (categoryId.size() != 2)
        return QString();
    int serial = nextSerial(categoryId);
    while (serial <= 99999)
    {
        const QString code = BarcodeArt::makeCode(categoryId, serial);
        bool taken = false;
        for (const auto& p : m_products)
            if (p.ean == code) { taken = true; break; }
        if (!taken)
            return code;
        ++serial;
    }
    return QString();
}

bool AppState::backendAcceptsPrefix(const QString& categoryId) const
{
    const QString code = BarcodeArt::makeCode(categoryId, 1);
    if (code.isEmpty())
        return false;
    const Product probe(1, code.toStdString(), "PROBE", "Probe", MeasurementUnit::Count, 1, 0, 0, 0, "data/barcodes/probe.png");
    return ProductValidator::standard().validate(probe).ok();
}

QString AppState::validateProduct(const ProductInfo& p, bool isNew) const
{
    if (p.name.trimmed().isEmpty())
        return "Product name is required.";
    if (p.sku.trimmed().isEmpty())
        return "Acronym is required.";
    if (p.ean.isEmpty())
        return "Generate a barcode first.";
    const Product candidate = toProduct(p, "data/barcodes/" + p.ean + ".png");
    const ValidationReport report = ProductValidator::standard().validate(candidate);
    if (!report.ok())
        return QString::fromStdString(report.errors.front());
    for (const auto& o : m_products)
    {
        if (!isNew && o.id == p.id)
            continue;
        if (o.ean == p.ean)
            return QString("Barcode %1 already belongs to \"%2\".").arg(p.ean, o.name);
        if (o.sku.compare(p.sku, Qt::CaseInsensitive) == 0)
            return QString("Acronym %1 is already used by \"%2\".").arg(p.sku.toUpper(), o.name);
    }
    return QString();
}

namespace
{
    // Saves `products` to Product.txt; on failure puts the previous file content back.
    bool saveCatalogue(const std::string& path, const std::vector<Product>& products, QString* error)
    {
        QFile f(QString::fromStdString(path));
        QByteArray backup;
        if (f.open(QIODevice::ReadOnly)) { backup = f.readAll(); f.close(); }
        std::string why;
        if (ProductFileWriter().save(path, products, &why))
            return true;
        if (!backup.isEmpty())
        {
            QSaveFile restore(QString::fromStdString(path));
            if (restore.open(QIODevice::WriteOnly)) { restore.write(backup); restore.commit(); }
        }
        if (error) *error = "Could not save Product.txt: " + QString::fromStdString(why);
        return false;
    }
}

bool AppState::addProduct(ProductInfo& p, QString* error)
{
    if (m_productPath.isEmpty())
    {
        if (error) *error = "Product.txt was not found.";
        return false;
    }
    if (p.id <= 0)
        p.id = nextProductId();
    p.sku = p.sku.trimmed().toUpper();
    p.name = p.name.trimmed();
    const QString problem = validateProduct(p, true);
    if (!problem.isEmpty())
    {
        if (error) *error = problem;
        return false;
    }
    const QString rel = "data/barcodes/" + p.ean + ".png";
    QString imgError;
    if (!BarcodeArt::savePng(p.ean, QDir(m_root).filePath(rel), &imgError))
    {
        if (error) *error = imgError;
        return false;
    }
    std::vector<Product> all = m_manager.getProducts();
    all.push_back(toProduct(p, rel));
    if (!saveCatalogue(m_productPath.toStdString(), all, error))
        return false;
    m_manager.loadProducts(m_productPath.toStdString());
    refreshCache();
    refreshOrders();
    emit changed();
    return true;
}

bool AppState::updateProduct(const ProductInfo& p, QString* error)
{
    ProductInfo q = p;
    q.sku = q.sku.trimmed().toUpper();
    q.name = q.name.trimmed();
    const QString problem = validateProduct(q, false);
    if (!problem.isEmpty())
    {
        if (error) *error = problem;
        return false;
    }
    std::vector<Product> all = m_manager.getProducts();
    bool found = false;
    for (auto& e : all)
        if (e.getProductID() == q.id)
        {
            e = toProduct(q, QString::fromStdString(e.getBarcodeFilePath()));
            found = true;
        }
    if (!found)
    {
        if (error) *error = "That product no longer exists.";
        return false;
    }
    if (!saveCatalogue(m_productPath.toStdString(), all, error))
        return false;
    m_manager.loadProducts(m_productPath.toStdString());
    refreshCache();
    refreshOrders();
    emit changed();
    return true;
}

bool AppState::setStockLevels(const QMap<int, int>& levels, QString* error)
{
    std::vector<Product> all = m_manager.getProducts();
    for (auto& e : all)
    {
        const auto it = levels.constFind(e.getProductID());
        if (it == levels.constEnd())
            continue;
        if (it.value() < 0)
        {
            if (error) *error = "Quantity cannot be negative.";
            return false;
        }
        ProductInfo info = ProductInfo::from(e, m_root);
        info.stock = it.value();
        e = toProduct(info, QString::fromStdString(e.getBarcodeFilePath()));
    }
    if (!saveCatalogue(m_productPath.toStdString(), all, error))
        return false;
    m_manager.loadProducts(m_productPath.toStdString());
    refreshCache();
    emit changed();
    return true;
}
