#pragma once
// Business Reports (manager): six tabular reports with summary tiles, totals, CSV export and PDF print.
//   1 Incomplete orders      2 Completed orders     3 Cash sales (daily / weekly / monthly / yearly / custom range)
//   4 All sales              5 Top 10 most sold     6 Top 10 by remaining stock
// Front end only: every report is computed from what AppState / UiStore already hold, so no new data file is needed.
#include <QDate>
#include <QStringList>
#include <QVector>
#include <QWidget>

class QDateEdit;
class QFrame;
class QLabel;
class QPushButton;
class QTableWidget;

class ReportsPage : public QWidget
{
    Q_OBJECT
public:
    explicit ReportsPage(QWidget *parent = nullptr);
    void refresh();
signals:
    void toast(const QString &message);

private:
    // One computed report, independent of widgets (so a new report only adds a function, see ReportsPage.cpp).
    struct Report
    {
        QString title, subtitle;
        QString captions[3], values[3], subs[3]; // summary tiles
        QStringList headers;
        QVector<QStringList> rows;
        QVector<int> rightCols;  // right-aligned numeric columns
        int statusCol = -1;      // column drawn as an order-status badge
        int barCol = -1;         // column drawn as a horizontal bar (Top 10 reports)
        QVector<double> bars;    // 0..1 per row
        QString footerLabel, footerValue, footerExtra;
        QString emptyText;
    };
    using Builder = Report (ReportsPage::*)() const;

    Report incomplete() const;
    Report completed() const;
    Report cashSales() const;
    Report allSales() const;
    Report topSold() const;
    Report topStock() const;

    void selectTab(int index);
    void selectPeriod(const QString &name);
    void applyCustomRange();
    void render();
    void exportCsv();
    void printPdf();

    QVector<QPushButton *> m_tabs, m_periods;
    QFrame *m_periodCard = nullptr;
    QDateEdit *m_from = nullptr, *m_to = nullptr;
    QLabel *m_cap[3] = {}, *m_val[3] = {}, *m_sub[3] = {};
    QLabel *m_tableTitle = nullptr, *m_tableSub = nullptr, *m_empty = nullptr;
    QLabel *m_footLabel = nullptr, *m_footValue = nullptr, *m_footExtra = nullptr;
    QTableWidget *m_table = nullptr;
    int m_tab = 0;
    QDate m_rangeFrom, m_rangeTo;
    Report m_current; // what the table shows now (used by the exports)
};
