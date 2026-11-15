#pragma once
#include <QWidget>

class QLabel;
class QLineEdit;
class QTableWidget;
class RevenueChart;
class QPushButton;
class QVBoxLayout;

class DashboardPage : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardPage(QWidget *parent = nullptr);
public slots:
    void focusLookup();
    void refresh();

signals:
    void navigate(int page);
    void lookup(const QString &text);
    void resumeOrder(const QString &orderId);
    void toast(const QString &message);

private:
    void build();
    void setDay(bool today);

    QLabel *m_sales, *m_salesDelta, *m_orders, *m_ticket, *m_lowCount;
    RevenueChart *m_chart;
    QPushButton *m_todayBtn, *m_yestBtn, *m_genPO;
    QLabel *m_peak, *m_quota, *m_ingest;
    QTableWidget *m_low, *m_recent;
    QLabel *m_showing;
    QLineEdit *m_search;
};
