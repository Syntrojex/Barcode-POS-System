#pragma once
// Front-end only state for the Increment 3 screens whose back-end is not built yet:
// product categories, promotional seasons, parked (on-hold) orders and the manager sign-off check.
// Nothing here touches the data files - when the back-end arrives (PCategory.txt, Seasons.txt, ...), replace the
// bodies of this class and keep its interface, so the pages do not change.
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
    QString id;   // "SEA-2026-01"
    QString name;
    QDate start, end;
    double percent = 0;
    bool active = false;       // isActive in Seasons.txt
    QStringList categories;    // empty = all categories
    // "active" | "scheduled" | "expired" for a given day (an inactive season is never "active")
    QString statusOn(const QDate &d) const;
};

struct ParkedOrder
{
    QString code; // "HLD-108"
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
    void setSeasonActive(const QString &id, bool on);
    QString nextSeasonId() const;
    const Season *activeSeason(const QDate &d = QDate::currentDate()) const; // first active season covering d
    QString validateSeason(const QString &name, const QDate &start, const QDate &end, double percent) const;

    // ---- on-hold orders ----
    const QVector<ParkedOrder> &parked() const { return m_parked; }
    QString park(const QString &customer, const QString &contact, const QVector<ReceiptLine> &lines);
    bool takeParked(const QString &code, ParkedOrder &out); // removes it from the list
    void discardParked(const QString &code);

    // ---- manager sign-off (needed to edit a paid order) ----
    // Demo check only: replace with real staff accounts when the back-end has them.
    static QString demoManagerId() { return QStringLiteral("MGR-004"); }
    static QString demoManagerName() { return QStringLiteral("Salman Tariq"); }
    static QString demoManagerPin() { return QStringLiteral("1234"); }
    // Approvals given on the Orders page for editing a paid order (kept for this session, consumed on use).
    struct Approval { QString manager, reason; QDateTime at; };
    void grantApproval(const QString &orderId, const QString &manager, const QString &reason);
    bool approvalFor(const QString &orderId, Approval *out = nullptr) const;
    void revokeApproval(const QString &orderId);
    bool verifyManager(const QString &staffId, const QString &pin, QString *managerName = nullptr) const;

signals:
    void categoriesChanged();
    void seasonsChanged();
    void parkedChanged();

private:
    UiStore();
    QVector<Category> m_categories;
    QVector<Season> m_seasons;
    QVector<ParkedOrder> m_parked;
    QMap<QString, Approval> m_approvals;
    int m_nextHold = 109;
};
