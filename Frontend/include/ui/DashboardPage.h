#pragma once
#include <QWidget>

class QLabel;
class QLineEdit;
class QTableWidget;
class RevenueChart;
class QPushButton;

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

    QLabel *m_sales, *m_salesSub, *m_orders, *m_ordersSub, *m_pending, *m_pendingSub, *m_lowCount, *m_lowSub;
    RevenueChart *m_chart;
    QTableWidget *m_recent;
    QLabel *m_showing;
    QLineEdit *m_search;
};
