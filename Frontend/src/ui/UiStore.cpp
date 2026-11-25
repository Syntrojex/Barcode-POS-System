#include "UiStore.h"
#include "AppState.h"
#include "billing/SeasonRepository.h"
#include <QDir>
#include <QRegularExpression>
#include <algorithm>

QString Season::statusOn(const QDate &d) const
{
    if (end < d)
        return "expired";
    if (!active)
        return "inactive"; // isActive = false: never applied, whatever the dates
    return start > d ? "scheduled" : "active";
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

    loadSeasons(); // real seasons from Seasons.txt (no demo seasons)
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
QString UiStore::seasonLabel(const QDate &start, const QDate &end)
{
    if (start.year() != end.year())
        return start.toString("d MMM yyyy") + " - " + end.toString("d MMM yyyy");
    return start.toString("d MMM") + " - " + end.toString("d MMM yyyy");
}

QString UiStore::seasonsPath() const
{
    return QDir(AppState::instance().dataDir()).filePath("Seasons.txt");
}

void UiStore::loadSeasons()
{
    namespace bb = barcodehub::billing;
    std::vector<std::string> errors;
    const auto records = bb::SeasonRepository(seasonsPath().toStdString()).load(&errors);
    m_seasons.clear();
    int n = 0;
    for (const auto &r : records)
    {
        Season s;
        s.start = QDate::fromString(QString::fromStdString(r.startDate), Qt::ISODate);
        s.end = QDate::fromString(QString::fromStdString(r.endDate), Qt::ISODate);
        s.percent = r.percent;
        s.active = r.active;
        s.id = QString("SEA-%1-%2").arg(s.start.year()).arg(++n, 2, 10, QLatin1Char('0'));
        s.name = seasonLabel(s.start, s.end);
        m_seasons.push_back(s);
    }
    for (const auto &e : errors)
        qWarning("%s", e.c_str()); // bad lines are skipped, not fatal
}

void UiStore::saveSeasons()
{
    namespace bb = barcodehub::billing;
    std::vector<bb::SeasonRecord> records;
    for (const auto &s : m_seasons)
        records.push_back({s.start.toString(Qt::ISODate).toStdString(), s.end.toString(Qt::ISODate).toStdString(),
                           s.percent, s.active});
    std::string error;
    if (!bb::SeasonRepository(seasonsPath().toStdString()).save(records, &error))
        qWarning("Could not save Seasons.txt: %s", error.c_str());
}

void UiStore::addSeason(const Season &s)
{
    m_seasons.push_back(s);
    m_seasons.last().name = seasonLabel(s.start, s.end);
    saveSeasons();
    emit seasonsChanged();
}

void UiStore::updateSeason(const QString &id, const Season &s)
{
    for (auto &x : m_seasons)
        if (x.id == id)
        {
            x = s;
            x.id = id;
            x.name = seasonLabel(s.start, s.end);
        }
    saveSeasons();
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
    saveSeasons();
    emit seasonsChanged();
}

QString UiStore::setSeasonActive(const QString &id, bool on)
{
    for (auto &x : m_seasons)
        if (x.id == id)
        {
            if (on)
                if (const QString why = validateSeason(x.start, x.end, x.percent, true, id); !why.isEmpty())
                {
                    emit seasonsChanged(); // redraw the toggle as it was
                    return why;
                }
            x.active = on;
        }
    saveSeasons();
    emit seasonsChanged();
    return QString();
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

QString UiStore::validateSeason(const QDate &start, const QDate &end, double percent, bool active,
                                const QString &ignoreId) const
{
    if (!start.isValid() || !end.isValid())
        return "Enter valid start and end dates.";
    if (end < start)
        return "The end date must not be before the start date.";
    if (percent <= 0 || percent > 100)
        return "Discount percentage must be more than 0 and at most 100.";
    if (active)
        for (const auto &s : m_seasons)
            if (s.id != ignoreId && s.active && s.start <= end && start <= s.end)
                return QString("These dates overlap the active season %1 (%2%). Change the dates, save it as "
                               "inactive, or switch that season off first.")
                    .arg(s.name).arg(s.percent, 0, 'g', 4);
    return QString();
}

// ---------------------------------------------------------------- parked orders
QVector<ParkedOrder> UiStore::parked() const
{
    QVector<ParkedOrder> out;
    for (const auto &o : AppState::instance().heldOrders())
    {
        if (o.status != "On Hold")
            continue;
        ParkedOrder p;
        p.code = o.id;
        p.customer = o.customer;
        p.contact = o.contact;
        p.parkedAt = o.time;
        p.lines = o.lines;
        out << p;
    }
    return out;
}

QString UiStore::park(const QString &customer, const QString &contact, const QVector<ReceiptLine> &lines,
                      const QString &existingId, QString *error)
{
    const QString id = AppState::instance().holdOrder(customer, contact, lines, existingId, error);
    emit parkedChanged();
    return id;
}

bool UiStore::takeParked(const QString &code, ParkedOrder &out)
{
    for (const auto &o : AppState::instance().heldOrders())
        if (o.id == code)
        {
            out.code = o.id;
            out.customer = o.customer;
            out.contact = o.contact;
            out.parkedAt = o.time;
            out.lines = o.lines;
            return true;
        }
    return false;
}

bool UiStore::discardParked(const QString &code, QString *error)
{
    const bool ok = AppState::instance().discardHeldOrder(code, error);
    emit parkedChanged();
    return ok;
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
