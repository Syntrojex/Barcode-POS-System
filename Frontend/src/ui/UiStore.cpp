#include "UiStore.h"
#include "AppState.h"
#include "billing/SeasonRepository.h"
#include "product/CategoryStorage.h"
#include <QDir>
#include <QFile>
#include <QSet>
#include <QUrl>
#include <QSaveFile>
#include <QFileSystemWatcher>
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
    loadCategories(); // PCategory.txt (requirement 1)
    loadSeasons(); // real seasons from Seasons.txt (no demo seasons)
    reloadEditRequests();
}

// ---------------------------------------------------------------- categories (PCategory.txt)
// The categories are kept in m_categories (a list sorted by id) and saved to PCategory.txt after every change.

// Colour used to draw a category's badge on screen (not saved in the file).
static QString categoryColor(const QString &id)
{
    const char *colors[4] = {"#2554d8", "#535e7a", "#006e4c", "#b6c4ff"};
    return colors[id.toInt() % 4];
}

// Insertion sort: puts the categories in order of their id ("01", "10", "20", ...).
static void sortById(QVector<Category> &list)
{
    for (int i = 1; i < list.size(); i++)
    {
        Category key = list[i];
        int j = i - 1;
        while (j >= 0 && list[j].id > key.id)
        {
            list[j + 1] = list[j];
            j--;
        }
        list[j + 1] = key;
    }
}

// True if text is exactly two digits.
static bool isTwoDigits(const QString &text)
{
    return text.size() == 2 && text[0].isDigit() && text[1].isDigit();
}

QString UiStore::categoriesPath() const
{
    return QDir(AppState::instance().dataDir()).filePath("PCategory.txt");
}

void UiStore::loadCategories()
{
    CategoryRepository repo(categoriesPath().toStdString());
    m_categories.clear();

    if (!repo.exists())
    {
        // First run: there is no PCategory.txt yet, so create it with a starting set of categories.
        m_categories.push_back({"01", "General Store", "GEN", ""});
        m_categories.push_back({"10", "Dairy & Milk Products", "DAI", ""});
        m_categories.push_back({"20", "Bakery & Fresh Bread", "BAK", ""});
        m_categories.push_back({"30", "Beverages & Tea", "BEV", ""});
        m_categories.push_back({"40", "Snacks & Confectionery", "SNK", ""});
        m_categories.push_back({"50", "Grocery & Staples", "GRC", ""});
        m_categories.push_back({"60", "Personal Care & Hygiene", "PER", ""});
        m_categories.push_back({"70", "Household & Cleaning", "HSD", ""});
        QString error;
        if (!saveCategories(m_categories, &error))
            qWarning("Could not create PCategory.txt: %s", qPrintable(error));
    }
    else
    {
        // Read the file; lines that are not valid are skipped and reported.
        std::vector<std::string> errors;
        std::vector<ProductCategory> rows = repo.load(&errors);
        for (int i = 0; i < (int)rows.size(); i++)
        {
            Category c;
            c.id = QString::fromStdString(rows[i].id);
            c.name = QString::fromStdString(rows[i].name);
            c.acronym = QString::fromStdString(rows[i].acronym);
            m_categories.push_back(c);
        }
        for (int i = 0; i < (int)errors.size(); i++)
            qWarning("%s", errors[i].c_str());
    }

    // A product whose category is missing from the file still gets a row, so it can be seen and fixed.
    const QVector<ProductInfo> &products = AppState::instance().products();
    for (int i = 0; i < products.size(); i++)
    {
        const QString id = products[i].categoryId;
        if (id != "" && category(id) == nullptr)
            m_categories.push_back({id, "Category " + id, "C" + id, ""});
    }

    sortById(m_categories);
    for (int i = 0; i < m_categories.size(); i++)
        m_categories[i].color = categoryColor(m_categories[i].id);
}

bool UiStore::saveCategories(const QVector<Category> &categories, QString *error)
{
    // Copy the screen list into the back-end type, then let CategoryRepository write the file.
    std::vector<ProductCategory> rows;
    for (int i = 0; i < categories.size(); i++)
    {
        ProductCategory row;
        row.id = categories[i].id.toStdString();
        row.name = categories[i].name.trimmed().toStdString();
        row.acronym = categories[i].acronym.trimmed().toStdString();
        rows.push_back(row);
    }
    std::string why;
    CategoryRepository repo(categoriesPath().toStdString());
    if (!repo.save(rows, &why))
    {
        if (error)
            *error = "Could not save PCategory.txt: " + QString::fromStdString(why);
        return false;
    }
    return true;
}

// Linear search for a category by id; nullptr if there is none.
const Category *UiStore::category(const QString &id) const
{
    for (int i = 0; i < m_categories.size(); i++)
        if (m_categories[i].id == id)
            return &m_categories[i];
    return nullptr;
}

// How many products use this category.
int UiStore::productCount(const QString &id) const
{
    int count = 0;
    const QVector<ProductInfo> &products = AppState::instance().products();
    for (int i = 0; i < products.size(); i++)
    {
        QString productCategory = products[i].categoryId;
        if (productCategory == "")
            productCategory = products[i].ean.left(2); // older products: take it from the barcode
        if (productCategory == id)
            count++;
    }
    return count;
}

QString UiStore::rangeText(const QString &id) const
{
    return id + "000000 - " + id + "999999";
}

// Suggests up to n ids that are not used yet (10, then every 5th: 15, 20, ...).
QStringList UiStore::freeIds(int n) const
{
    QStringList result;
    int step = 1;
    for (int i = 10; i <= 99 && result.size() < n; i += step)
    {
        const QString id = QString::number(i);
        if (category(id) == nullptr)
        {
            result.push_back(id);
            step = 5;
        }
    }
    return result;
}

// Returns "" if the category may be saved, otherwise the reason. ignoreId = the id being edited (it may keep it).
QString UiStore::validateCategory(const QString &id, const QString &name, const QString &acronym,
                                  const QString &ignoreId) const
{
    const QString acr = acronym.trimmed();
    if (!isTwoDigits(id))
        return "Category ID must be exactly 2 digits.";
    if (id == "00")
        return "Category ID 00 is not allowed (barcodes cannot start with 00). Use 01 to 99.";
    const Category *existing = category(id);
    if (existing != nullptr && id != ignoreId)
    {
        QStringList free = freeIds(1);
        return QString("Category ID \"%1\" is already in use by \"%2\". Please pick an unused 2-digit code (e.g. %3).")
            .arg(id, existing->name, free.isEmpty() ? QString("80") : free[0]);
    }
    if (name.trimmed() == "")
        return "Category name is required.";
    if (name.contains('#'))
        return "Category name cannot contain '#'.";
    if (acr == "" || acr.size() > 3 || acr.contains(' '))
        return "Acronym must be 1 to 3 characters, without spaces.";
    for (int i = 0; i < m_categories.size(); i++)
        if (m_categories[i].id != ignoreId && m_categories[i].acronym.toUpper() == acr.toUpper())
            return QString("Acronym \"%1\" is already used by \"%2\".").arg(acr.toUpper(), m_categories[i].name);
    return "";
}

// Each change below is made on a copy of the list. The copy is saved first; only if the file was written
// does it replace m_categories - so the screen and PCategory.txt always show the same categories.

bool UiStore::addCategory(const Category &c, QString *error)
{
    const QString problem = validateCategory(c.id, c.name, c.acronym);
    if (problem != "")
    {
        if (error)
            *error = problem;
        return false;
    }
    Category added;
    added.id = c.id;
    added.name = c.name.trimmed();
    added.acronym = c.acronym.trimmed().toUpper();
    added.color = categoryColor(added.id);

    QVector<Category> copy = m_categories;
    copy.push_back(added);
    sortById(copy);
    if (!saveCategories(copy, error))
        return false;
    m_categories = copy;
    emit categoriesChanged();
    return true;
}

bool UiStore::updateCategory(const QString &oldId, const Category &c, QString *error)
{
    const QString problem = validateCategory(c.id, c.name, c.acronym, oldId);
    if (problem != "")
    {
        if (error)
            *error = problem;
        return false;
    }
    // The id is part of every product barcode, so it cannot change once products use it.
    if (oldId != c.id && productCount(oldId) > 0)
    {
        if (error)
            *error = "This category already has products; changing its id would break their barcodes.";
        return false;
    }

    QVector<Category> copy = m_categories;
    for (int i = 0; i < copy.size(); i++)
        if (copy[i].id == oldId)
        {
            copy[i].id = c.id;
            copy[i].name = c.name.trimmed();
            copy[i].acronym = c.acronym.trimmed().toUpper();
            copy[i].color = categoryColor(c.id);
        }
    sortById(copy);
    if (!saveCategories(copy, error))
        return false;
    m_categories = copy;
    emit categoriesChanged();
    return true;
}

bool UiStore::removeCategory(const QString &id, QString *error)
{
    // A category that products still use cannot be deleted.
    const int used = productCount(id);
    if (used > 0)
    {
        if (error)
            *error = QString("%1 product(s) still use this category.").arg(used);
        return false;
    }

    for (int i = 0; i < m_categories.size(); i++)
        if (m_categories[i].id == id)
        {
            QVector<Category> copy = m_categories;
            copy.removeAt(i);
            if (!saveCategories(copy, error))
                return false;
            m_categories = copy;
            emit categoriesChanged();
            return true;
        }
    return false; // no category with this id
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

// ---------------------------------------------------------------- edit requests (EditRequests.txt)
// One request per line, space separated, ending with '#':
//   orderId status requestedAt cashier reason decidedAt manager#
// Text fields are %-encoded (space = %20), so names and reasons may contain spaces; an empty field is "-".
namespace
{
    QString enc(const QString &t)
    {
        if (t.isEmpty())
            return "-";
        QString out = t;
        out.replace('%', "%25").replace(' ', "%20").replace('#', "%23").replace('\n', "%0A");
        return out;
    }
    QString dec(const QString &t)
    {
        return t == "-" ? QString() : QUrl::fromPercentEncoding(t.toUtf8());
    }
    QString when(const QDateTime &d) { return d.isValid() ? d.toString(Qt::ISODate) : "-"; }
}

QString UiStore::editRequestsPath() const
{
    return QDir(AppState::instance().dataDir()).filePath("EditRequests.txt");
}

void UiStore::reloadEditRequests()
{
    QVector<EditRequest> loaded;
    QFile f(editRequestsPath());
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        int number = 0;
        while (!f.atEnd())
        {
            ++number;
            QString line = QString::fromUtf8(f.readLine()).trimmed();
            if (line.isEmpty())
                continue;
            const QStringList t = line.endsWith('#') ? line.chopped(1).split(' ', Qt::SkipEmptyParts) : QStringList();
            static const QStringList kStatus = {"pending", "approved", "rejected", "used"};
            if (t.size() != 7 || !kStatus.contains(t[1]))
            {
                qWarning("EditRequests.txt line %d is not valid and was skipped", number);
                continue;
            }
            EditRequest r;
            r.orderId = t[0];
            r.status = t[1];
            r.requestedAt = QDateTime::fromString(t[2], Qt::ISODate);
            r.cashier = dec(t[3]);
            r.reason = dec(t[4]);
            r.decidedAt = t[5] == "-" ? QDateTime() : QDateTime::fromString(t[5], Qt::ISODate);
            r.manager = dec(t[6]);
            loaded << r;
        }
    }
    const bool changed = loaded.size() != m_requests.size() ||
                         !std::equal(loaded.begin(), loaded.end(), m_requests.begin(), [](const EditRequest &a, const EditRequest &b) {
                             return a.orderId == b.orderId && a.status == b.status && a.requestedAt == b.requestedAt;
                         });
    m_requests = loaded;
    if (!m_watcher)
    {
        m_watcher = new QFileSystemWatcher(this);
        connect(m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &) { reloadEditRequests(); });
        connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &) { reloadEditRequests(); });
        m_watcher->addPath(AppState::instance().dataDir());
    }
    if (QFile::exists(editRequestsPath()) && !m_watcher->files().contains(editRequestsPath()))
        m_watcher->addPath(editRequestsPath()); // re-added after every atomic replace
    if (changed)
        emit editRequestsChanged();
}

bool UiStore::saveEditRequests(const QVector<EditRequest> &requests, QString *error)
{
    QByteArray data;
    for (const auto &r : requests)
        data += QString("%1 %2 %3 %4 %5 %6 %7#\n")
                    .arg(r.orderId, r.status, when(r.requestedAt), enc(r.cashier), enc(r.reason), when(r.decidedAt), enc(r.manager))
                    .toUtf8();
    QSaveFile f(editRequestsPath());
    f.setDirectWriteFallback(true);
    if (f.open(QIODevice::WriteOnly) && f.write(data) == data.size() && f.commit())
        return true;
    // the safe replace was refused (on Windows another program can briefly hold the file): write it directly
    QFile direct(editRequestsPath());
    if (direct.open(QIODevice::WriteOnly | QIODevice::Truncate) && direct.write(data) == data.size())
        return true;
    if (error) *error = "Could not save EditRequests.txt: " + direct.errorString();
    return false;
}

void UiStore::setEditRequests(const QVector<EditRequest> &requests)
{
    m_requests = requests;
    if (m_watcher && QFile::exists(editRequestsPath()) && !m_watcher->files().contains(editRequestsPath()))
        m_watcher->addPath(editRequestsPath());
    emit editRequestsChanged();
}

QVector<UiStore::EditRequest> UiStore::editRequests() const
{
    QVector<EditRequest> out = m_requests;
    std::reverse(out.begin(), out.end());
    return out;
}

QVector<UiStore::EditRequest> UiStore::pendingEditRequests() const
{
    // only the latest record of each order counts; an older "pending" line that was later answered is history
    QVector<EditRequest> out;
    QSet<QString> seen;
    for (int i = m_requests.size() - 1; i >= 0; --i)
    {
        if (seen.contains(m_requests[i].orderId))
            continue;
        seen.insert(m_requests[i].orderId);
        if (m_requests[i].status == "pending")
            out.prepend(m_requests[i]);
    }
    return out;
}

const UiStore::EditRequest *UiStore::latestEditRequest(const QString &orderId) const
{
    for (int i = m_requests.size() - 1; i >= 0; --i)
        if (m_requests[i].orderId == orderId)
            return &m_requests[i];
    return nullptr;
}

int UiStore::approvedUnusedCount() const
{
    QSet<QString> seen;
    int n = 0;
    for (int i = m_requests.size() - 1; i >= 0; --i)
        if (!seen.contains(m_requests[i].orderId))
        {
            seen.insert(m_requests[i].orderId);
            n += m_requests[i].status == "approved";
        }
    return n;
}

QString UiStore::requestEdit(const QString &orderId, const QString &cashier, const QString &reason)
{
    reloadEditRequests();
    if (const EditRequest *r = latestEditRequest(orderId))
    {
        if (r->status == "pending")
            return "A request for order " + orderId + " is already waiting for a manager.";
        if (r->status == "approved")
            return "Order " + orderId + " is already approved - you can open it.";
    }
    EditRequest r;
    r.orderId = orderId;
    r.status = "pending";
    r.cashier = cashier.trimmed().isEmpty() ? QString("Cashier") : cashier.trimmed();
    r.reason = reason.trimmed().isEmpty() ? QString("Customer request") : reason.trimmed();
    r.requestedAt = QDateTime::currentDateTime();
    QVector<EditRequest> next = m_requests;
    next << r;
    QString error;
    if (!saveEditRequests(next, &error))
        return error;
    setEditRequests(next);
    return QString();
}

QString UiStore::decideEdit(const QString &orderId, const QString &manager, bool approve)
{
    reloadEditRequests();
    QVector<EditRequest> next = m_requests;
    for (int i = next.size() - 1; i >= 0; --i)
        if (next[i].orderId == orderId)
        {
            if (next[i].status != "pending")
                return "This request was already answered (" + next[i].status + ").";
            next[i].status = approve ? "approved" : "rejected";
            next[i].manager = manager.trimmed().isEmpty() ? QString("Manager") : manager.trimmed();
            next[i].decidedAt = QDateTime::currentDateTime();
            QString error;
            if (!saveEditRequests(next, &error))
                return error;
            setEditRequests(next);
            return QString();
        }
    return "No request was found for order " + orderId + ".";
}

void UiStore::grantApproval(const QString &orderId, const QString &manager, const QString &reason)
{
    // A signed-in manager editing an order approves it himself; it is recorded like any other approval.
    reloadEditRequests();
    QVector<EditRequest> next = m_requests;
    for (int i = next.size() - 1; i >= 0; --i)
        if (next[i].orderId == orderId)
        {
            if (next[i].status == "approved")
                return; // already approved and not used yet
            if (next[i].status == "pending")
            {
                // the manager opens an order a cashier asked about: that request is approved by him
                next[i].status = "approved";
                next[i].manager = manager;
                next[i].decidedAt = QDateTime::currentDateTime();
                if (saveEditRequests(next))
                    setEditRequests(next);
                return;
            }
            break;
        }
    EditRequest r;
    r.orderId = orderId;
    r.status = "approved";
    r.cashier = manager;
    r.reason = reason;
    r.requestedAt = r.decidedAt = QDateTime::currentDateTime();
    r.manager = manager;
    next << r;
    if (saveEditRequests(next))
        setEditRequests(next);
}

bool UiStore::approvalFor(const QString &orderId, Approval *out) const
{
    const EditRequest *r = latestEditRequest(orderId);
    if (!r || r->status != "approved")
        return false;
    if (out)
        *out = Approval{r->manager, r->reason, r->decidedAt};
    return true;
}

void UiStore::revokeApproval(const QString &orderId)
{
    QVector<EditRequest> next = m_requests;
    for (int i = next.size() - 1; i >= 0; --i)
        if (next[i].orderId == orderId)
        {
            if (next[i].status != "approved")
                return;
            next[i].status = "used";
            if (saveEditRequests(next))
                setEditRequests(next);
            return;
        }
}
