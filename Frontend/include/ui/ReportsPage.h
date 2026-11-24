#pragma once
// Business Reports (manager): six tabular reports with totals, CSV export and PDF print.
//   1 incomplete orders (not paid / not archived) + total      4 all sales with total amount and discounts
//   2 completed orders + total                                 5 top ten most sold items with remaining stock
//   3 cash sales: daily / weekly / monthly / yearly / period   6 top ten items by remaining stock (do not re-order)
// Frontend only: everything is computed from AppState::orders() and AppState::products() (the Increment 2 files).
#include <QDate>
#include <QStringList>
#include <QVector>
#include <QWidget>

class QDateEdit;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QTableWidget;
class QVBoxLayout;

class ReportsPage : public QWidget
{
    Q_OBJECT
public:
    explicit ReportsPage(QWidget *parent = nullptr);
    void refresh();
signals:
    void toast(const QString &message);

private:
    enum Tab { Incomplete, Completed, CashSales, AllSales, TopSold, TopRemaining, TabCount };
    enum Period { All, Daily, Weekly, Monthly, Yearly, Custom };

    struct Tile { QString caption, icon, value, sub, tint; };
    struct TopItem { QString name, acronym; int units = 0; double revenue = 0; int stock = 0; };
    struct Report
    {
        QString title, subtitle;
        QStringList headers;
        QVector<bool> right;        // right-aligned columns
        QVector<int> accent;        // blue + bold columns
        QVector<QStringList> rows;
        QVector<Tile> tiles;        // three summary tiles
        QString footLabel, footValue, footNote;
        bool showTopSold = false;   // "Top sold staples" side panel
    };

    Report build(int tab) const;
    QVector<TopItem> topSold(int limit) const;
    QString periodText() const;
    void setPeriod(int period);
    void setTab(int tab);
    void render();
    void exportCsv();
    void exportPdf();

    int m_tab = CashSales;
    int m_period = All;
    QDate m_from, m_to; // inclusive; invalid = no limit
    Report m_report;

    QVector<QPushButton *> m_tabButtons, m_periodButtons;
    QDateEdit *m_fromEdit = nullptr, *m_toEdit = nullptr;
    QWidget *m_periodBar = nullptr;
    QHBoxLayout *m_tiles = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_ledgerTitle = nullptr, *m_ledgerSub = nullptr;
    QWidget *m_side = nullptr;
    QVBoxLayout *m_sideList = nullptr;
    QLabel *m_footLabel = nullptr, *m_footValue = nullptr, *m_footNote = nullptr;
};
