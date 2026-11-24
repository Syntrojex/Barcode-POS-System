#include "UiStore.h"
#include <QRegularExpression>
#include <algorithm>

QString Season::statusOn(const QDate &d) const
{
    if (end < d)
        return "expired";
    if (start > d)
        return "scheduled";
    return active ? "active" : "scheduled";
}

int ParkedOrder::itemCount() const
{
    int n = 0;
    for (const auto &l : lines)
        n += l.qty;
    return n;
}

double ParkedOrder::total() const
{
    double t = 0;
    for (const auto &l : lines)
        t += (l.price - l.discount + l.tax) * l.qty;
    return t;
}

UiStore &UiStore::instance()
{
    static UiStore s;
    return s;
}

UiStore::UiStore()
{
    // Categories: the ones from the design plus "01" for the products already in Product.txt (Increment 2 barcodes).
    m_categories = {
        {"01", "General Store", "GEN", "#2554d8"},
        {"10", "Dairy & Milk Products", "DAI", "#2554d8"},
        {"20", "Bakery & Fresh Bread", "BAK", "#535e7a"},
        {"30", "Beverages & Tea", "BEV", "#006e4c"},
        {"40", "Snacks & Confectionery", "SNK", "#2554d8"},
        {"50", "Grocery & Staples", "GRC", "#b6c4ff"},
        {"60", "Personal Care & Hygiene", "PER", "#b6c4ff"},
        {"70", "Household & Cleaning", "HSD", "#b6c4ff"},
    };

    const QDate today = QDate::currentDate();
    auto mk = [](const QString &id, const QString &name, QDate a, QDate b, double pct, bool on) {
        Season s;
        s.id = id; s.name = name; s.start = a; s.end = b; s.percent = pct; s.active = on;
        return s;
    };
    const int y = today.year();
    m_seasons = {
        mk(QString("SEA-%1-01").arg(y), "Winter Warmth Sale", today.addDays(-3), today.addDays(4), 10, true),
        mk(QString("SEA-%1-02").arg(y + 1), "Eid-ul-Fitr Grand Fest", QDate(y + 1, 3, 20), QDate(y + 1, 3, 27), 15, false),
        mk(QString("SEA-%1-03").arg(y + 1), "Pakistan Day Mega Deal", QDate(y + 1, 3, 23), QDate(y + 1, 3, 24), 8, false),
        mk(QString("SEA-%1-08").arg(y), "Summer Clearance", QDate(y, 6, 15), QDate(y, 6, 30), 5, false),
        mk(QString("SEA-%1-12").arg(y - 1), "Winter Promo", QDate(y - 1, 12, 1), QDate(y - 1, 12, 31), 12, false),
    };

    // A couple of parked orders so the "Parked" strip is not empty on first launch.
    auto line = [](const char *name, const char *ac, int q, double price, double tax, double disc, int id) {
        ReceiptLine l;
        l.name = name; l.acronym = ac; l.qty = q; l.price = price; l.tax = tax; l.discount = disc; l.productId = id;
        return l;
    };
    ParkedOrder a;
    a.code = "HLD-108"; a.customer = "Asad Malik"; a.contact = "0301-5551212";
    a.parkedAt = QDateTime::currentDateTime().addSecs(-4 * 60);
    a.lines = {line("Olpers Full Cream Milk 1L", "OLPMILK", 2, 340, 0, 10, 1),
               line("National Whole Wheat Bread", "NBREAD", 1, 260, 18, 0, 2)};
    ParkedOrder b;
    b.code = "HLD-107"; b.customer = "Sana Malik"; b.contact = "0321-9876543";
    b.parkedAt = QDateTime::currentDateTime().addSecs(-17 * 60);
    b.lines = {line("Tapal Danedar Tea 190g", "TAPALTEA", 3, 210, 15, 5, 3)};
    m_parked = {a, b};
}

// ---------------------------------------------------------------- categories
const Category *UiStore::category(const QString &id) const
{
    for (const auto &c : m_categories)
        if (c.id == id)
            return &c;
    return nullptr;
}

int UiStore::productCount(const QString &id) const
{
    int n = 0;
    for (const auto &p : AppState::instance().products())
        if (p.ean.left(2) == id)
            ++n;
    return n;
}

QString UiStore::rangeText(const QString &id) const
{
    return QString("%100000 - %199999").arg(id);
}

QStringList UiStore::freeIds(int n) const
{
    QStringList out;
    for (int i = 10; i <= 99 && out.size() < n; i += (out.isEmpty() ? 1 : 5))
    {
        const QString id = QString::number(i);
        if (!category(id))
            out << id;
    }
    return out;
}

QString UiStore::validateCategory(const QString &id, const QString &name, const QString &acronym,
                                  const QString &ignoreId) const
{
    static const QRegularExpression twoDigits("^\\d{2}$");
    if (!twoDigits.match(id).hasMatch())
        return "Category ID must be exactly 2 digits.";
    if (const Category *c = category(id); c && id != ignoreId)
        return QString("Category ID \"%1\" is already in use by \"%2\". Please pick an unused 2-digit code (e.g. %3).")
            .arg(id, c->name, freeIds(1).value(0, "80"));
    if (name.trimmed().isEmpty())
        return "Category name is required.";
    if (acronym.trimmed().isEmpty() || acronym.trimmed().size() > 3)
        return "Acronym must be 1 to 3 characters.";
    for (const auto &c : m_categories)
        if (c.id != ignoreId && c.acronym.compare(acronym.trimmed(), Qt::CaseInsensitive) == 0)
            return QString("Acronym \"%1\" is already used by \"%2\".").arg(acronym.trimmed().toUpper(), c.name);
    return QString();
}

bool UiStore::addCategory(const Category &c, QString *error)
{
    const QString why = validateCategory(c.id, c.name, c.acronym);
    if (!why.isEmpty())
    {
        if (error) *error = why;
        return false;
    }
    Category n = c;
    n.name = c.name.trimmed();
    n.acronym = c.acronym.trimmed().toUpper();
    m_categories.push_back(n);
    std::sort(m_categories.begin(), m_categories.end(), [](const Category &a, const Category &b) { return a.id < b.id; });
    emit categoriesChanged();
    return true;
}

bool UiStore::updateCategory(const QString &oldId, const Category &c, QString *error)
{
    const QString why = validateCategory(c.id, c.name, c.acronym, oldId);
    if (!why.isEmpty())
    {
        if (error) *error = why;
        return false;
    }
    if (oldId != c.id && productCount(oldId) > 0)
    {
        if (error) *error = "This category already has products; changing its id would break their barcodes.";
        return false;
    }
    for (auto &x : m_categories)
        if (x.id == oldId)
        {
            x = c;
            x.name = c.name.trimmed();
            x.acronym = c.acronym.trimmed().toUpper();
        }
    std::sort(m_categories.begin(), m_categories.end(), [](const Category &a, const Category &b) { return a.id < b.id; });
    emit categoriesChanged();
    return true;
}

bool UiStore::removeCategory(const QString &id, QString *error)
{
    if (productCount(id) > 0)
    {
        if (error) *error = QString("%1 product(s) still use this category.").arg(productCount(id));
        return false;
    }
    for (int i = 0; i < m_categories.size(); ++i)
        if (m_categories[i].id == id)
        {
            m_categories.removeAt(i);
            emit categoriesChanged();
            return true;
        }
    return false;
}

// ---------------------------------------------------------------- seasons
void UiStore::addSeason(const Season &s)
{
    m_seasons.push_back(s);
    emit seasonsChanged();
}

void UiStore::updateSeason(const QString &id, const Season &s)
{
    for (auto &x : m_seasons)
        if (x.id == id)
            x = s;
    emit seasonsChanged();
}

void UiStore::removeSeason(const QString &id)
{
    for (int i = 0; i < m_seasons.size(); ++i)
        if (m_seasons[i].id == id)
        {
            m_seasons.removeAt(i);
            break;
        }
    emit seasonsChanged();
}

void UiStore::setSeasonActive(const QString &id, bool on)
{
    for (auto &x : m_seasons)
        if (x.id == id)
            x.active = on;
    emit seasonsChanged();
}

QString UiStore::nextSeasonId() const
{
    int maxN = 0;
    for (const auto &s : m_seasons)
        maxN = qMax(maxN, s.id.section('-', 2).toInt());
    return QString("SEA-%1-%2").arg(QDate::currentDate().year()).arg(maxN + 1, 2, 10, QLatin1Char('0'));
}

const Season *UiStore::activeSeason(const QDate &d) const
{
    for (const auto &s : m_seasons)
        if (s.active && s.start <= d && d <= s.end)
            return &s;
    return nullptr;
}

QString UiStore::validateSeason(const QString &name, const QDate &start, const QDate &end, double percent) const
{
    if (name.trimmed().isEmpty())
        return "Give the season a name.";
    if (!start.isValid() || !end.isValid())
        return "Enter valid start and end dates.";
    if (end < start)
        return "The end date must not be before the start date.";
    if (percent <= 0 || percent > 100)
        return "Discount must be between 0 and 100 percent.";
    return QString();
}

// ---------------------------------------------------------------- parked orders
QString UiStore::park(const QString &customer, const QString &contact, const QVector<ReceiptLine> &lines)
{
    ParkedOrder p;
    p.code = QString("HLD-%1").arg(m_nextHold++);
    p.customer = customer.trimmed().isEmpty() ? "Walk-in Customer" : customer;
    p.contact = contact;
    p.parkedAt = QDateTime::currentDateTime();
    p.lines = lines;
    m_parked.prepend(p);
    emit parkedChanged();
    return p.code;
}

bool UiStore::takeParked(const QString &code, ParkedOrder &out)
{
    for (int i = 0; i < m_parked.size(); ++i)
        if (m_parked[i].code == code)
        {
            out = m_parked[i];
            m_parked.removeAt(i);
            emit parkedChanged();
            return true;
        }
    return false;
}

void UiStore::discardParked(const QString &code)
{
    ParkedOrder ignored;
    takeParked(code, ignored);
}

// ---------------------------------------------------------------- manager sign-off
bool UiStore::verifyManager(const QString &staffId, const QString &pin, QString *managerName) const
{
    const bool ok = staffId.trimmed().compare(demoManagerId(), Qt::CaseInsensitive) == 0 && pin == demoManagerPin();
    if (ok && managerName)
        *managerName = demoManagerName();
    return ok;
}

// ---------------------------------------------------------------- approvals for editing paid orders
void UiStore::grantApproval(const QString &orderId, const QString &manager, const QString &reason)
{
    m_approvals[orderId] = Approval{manager, reason, QDateTime::currentDateTime()};
}

bool UiStore::approvalFor(const QString &orderId, Approval *out) const
{
    const auto it = m_approvals.constFind(orderId);
    if (it == m_approvals.constEnd())
        return false;
    if (out)
        *out = it.value();
    return true;
}

void UiStore::revokeApproval(const QString &orderId) { m_approvals.remove(orderId); }
