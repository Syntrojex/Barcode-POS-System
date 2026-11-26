#pragma once
// State for the Increment 3 manager screens: product categories (PCategory.txt), promotional seasons
// (Seasons.txt), on-hold orders (Order.txt through AppState) and the manager sign-off check.
#include "AppState.h"
#include <QDate>
#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

struct Category
{
    QString id;      // 2 digits, becomes the first two digits of every barcode in the category
    QString name;
    QString acronym; // up to 3 letters
    QString color;   // theme tag, e.g. "#2554d8"
};

struct Season
{
    // Seasons.txt (requirement 13) stores exactly: start date, end date, discount percentage, isActive.
    QDate start, end;
    double percent = 0;
    bool active = false;       // isActive in Seasons.txt
    // Not stored - derived for the screen only:
    QString id;   // row handle "SEA-2026-01" (position in Seasons.txt)
    QString name; // label made from the dates, e.g. "23 Nov - 30 Nov 2026"
    // "active" | "scheduled" | "inactive" | "expired" for a given day
    QString statusOn(const QDate &d) const;
};

struct ParkedOrder
{
    QString code; // the Order ID of the on-hold order (Order.txt)
    QString customer, contact;
    QDateTime parkedAt;
    QVector<ReceiptLine> lines;
    int itemCount() const;
    double total() const; // subtotal - discount + tax
};

class UiStore : public QObject
{
    Q_OBJECT
public:
    static UiStore &instance();
    static constexpr double kSpecialDiscountCapPercent = 8.0; // requirement 8: special discount is at most 8 percent

    // ---- categories ----
    const QVector<Category> &categories() const { return m_categories; }
    const Category *category(const QString &id) const;
    int productCount(const QString &id) const; // products whose barcode starts with this id (from Product.txt)
    QString rangeText(const QString &id) const; // "10000000 - 10999999"
    QStringList freeIds(int n = 3) const;       // unused ids from 10 to 99, e.g. {"80","85","90"}
    // Empty string when valid; otherwise the message to show (e.g. the "already in use" validation error).
    QString validateCategory(const QString &id, const QString &name, const QString &acronym,
                             const QString &ignoreId = QString()) const;
    bool addCategory(const Category &c, QString *error = nullptr);
    bool updateCategory(const QString &oldId, const Category &c, QString *error = nullptr);
    bool removeCategory(const QString &id, QString *error = nullptr); // refused while products use it

    // ---- seasons (Seasons.txt: start date, end date, percentage, isActive) ----
    const QVector<Season> &seasons() const { return m_seasons; }
    void addSeason(const Season &s);
    void updateSeason(const QString &id, const Season &s);
    void removeSeason(const QString &id);
    // Empty on success; otherwise why it was refused (an active season may not overlap another active one).
    QString setSeasonActive(const QString &id, bool on);
    QString nextSeasonId() const;
    const Season *activeSeason(const QDate &d = QDate::currentDate()) const; // first active season covering d
    // Requirement 13: a season is only start date, end date, discount percentage and isActive (Seasons.txt).
    // Two ACTIVE seasons may not share a day, so a bill date always matches at most one discount (requirement 15).
    QString validateSeason(const QDate &start, const QDate &end, double percent, bool active,
                           const QString &ignoreId = QString()) const;
    static QString seasonLabel(const QDate &start, const QDate &end); // "23 Nov - 30 Nov 2026"

    // ---- on-hold orders: saved in Order.txt / OrderDetail.txt with status on-hold (see AppState::holdOrder) ----
    QVector<ParkedOrder> parked() const; // newest first
    // Saves the cart as an on-hold order (`existingId` = update that held order). Returns the Order ID, empty on failure.
    QString park(const QString &customer, const QString &contact, const QVector<ReceiptLine> &lines,
                 const QString &existingId = QString(), QString *error = nullptr);
    bool takeParked(const QString &code, ParkedOrder &out); // reads it (it stays on hold until it is paid)
    bool discardParked(const QString &code, QString *error = nullptr);

    // ---- edit requests (requirement 3): a cashier asks, a manager approves or rejects ----
    // Saved in data/EditRequests.txt, so a request survives a restart and is seen by every terminal on the same data
    // folder. An approval is good for one edit: it is used up when the edit is saved or cancelled.
    struct EditRequest
    {
        QString orderId;
        QString status; // "pending" | "approved" | "rejected" | "used"
        QString cashier, reason;
        QDateTime requestedAt;
        QString manager; // who decided
        QDateTime decidedAt;
    };
    QVector<EditRequest> editRequests() const;            // newest first
    QVector<EditRequest> pendingEditRequests() const;     // waiting for a manager, oldest first
    const EditRequest *latestEditRequest(const QString &orderId) const;
    QString requestEdit(const QString &orderId, const QString &cashier, const QString &reason); // empty = sent
    QString decideEdit(const QString &orderId, const QString &manager, bool approve);          // empty = done
    int approvedUnusedCount() const;                      // approved, not yet used (cashier's to-do)
    void reloadEditRequests();                            // re-read the file (another terminal may have changed it)

    // Used by Record Order: an approved request for this order (a signed-in manager approves his own edit).
    struct Approval { QString manager, reason; QDateTime at; };
    void grantApproval(const QString &orderId, const QString &manager, const QString &reason);
    bool approvalFor(const QString &orderId, Approval *out = nullptr) const;
    void revokeApproval(const QString &orderId); // the approval is used up

signals:
    void categoriesChanged();
    void seasonsChanged();
    void parkedChanged();
    void editRequestsChanged();

private:
    UiStore();
    // Seasons.txt (start end percent isActive#) via the backend SeasonRepository.
    QString categoriesPath() const; // PCategory.txt
    void loadCategories();
    bool saveCategories(const QVector<Category> &categories, QString *error);
    QString seasonsPath() const;
    void loadSeasons();
    void saveSeasons();
    QVector<Category> m_categories;
    QVector<Season> m_seasons;
    QString editRequestsPath() const;
    bool saveEditRequests(const QVector<EditRequest> &requests, QString *error = nullptr);
    void setEditRequests(const QVector<EditRequest> &requests); // keeps memory + file + watcher in step
    QVector<EditRequest> m_requests;
    class QFileSystemWatcher *m_watcher = nullptr;
};
