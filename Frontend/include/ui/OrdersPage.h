#pragma once
// Orders Management: search/filter every order, open one for editing, manager approval for paid orders.
#include "AppState.h"
#include "UserRole.h"
#include <QWidget>

class QLabel;
class QLineEdit;
class QTableWidget;
class QPushButton;

class OrdersPage : public QWidget
{
    Q_OBJECT
public:
    explicit OrdersPage(QWidget *parent = nullptr);
    void setSearch(const QString &text);
    void setRole(UserRole role); // Manager approves paid-order edits itself; Cashier must ask for approval
signals:
    void toast(const QString &message);
    void resumeOrder(const QString &orderId); // open an in-progress order from Order.txt in Record Order
    void resumeParked(const QString &holdCode); // resume an order parked with F2 ("HLD-108") in Record Order
    void editOrder(const QString &orderId);   // open a (paid, approved) order for editing in Record Order
    void showBill(const QString &orderId);    // jump to Bills for this order

private:
    void rebuild();
    void openPaid(const OrderRecord &order); // 7-day window + manager approval, then editOrder()
    QLineEdit *m_orderSearch = nullptr, *m_custSearch = nullptr;
    QTableWidget *m_table = nullptr;
    QString m_filter = "All";
    QVector<QPushButton *> m_tabs;
    QLabel *m_showing = nullptr;
    UserRole m_role = UserRole::Cashier;
};
