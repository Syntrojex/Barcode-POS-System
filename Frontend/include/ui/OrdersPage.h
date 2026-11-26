#pragma once
// Orders Management: search/filter every order, open one for editing.
// Paid orders (requirement 3): a cashier sends an edit request; a manager approves or rejects it here.
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
    void setRole(UserRole role); // Manager approves requests and edits directly; Cashier sends a request first
    void setUser(const QString &name) { m_user = name; }
signals:
    void toast(const QString &message);
    void resumeOrder(const QString &orderId); // open an on-hold / in-progress order in Record Order
    void editOrder(const QString &orderId);   // open a (paid, approved) order for editing in Record Order
    void showBill(const QString &orderId);    // jump to Bills for this order

private:
    void rebuild();
    void openPaid(const OrderRecord &order); // manager: edit now; cashier: open if approved, else send a request
    void rebuildRequests();                  // manager's "waiting for approval" panel
    void showEvent(QShowEvent *e) override;
    QLineEdit *m_orderSearch = nullptr, *m_custSearch = nullptr;
    QTableWidget *m_table = nullptr;
    QString m_filter = "All";
    QVector<QPushButton *> m_tabs;
    QLabel *m_showing = nullptr;
    QLabel *m_empty = nullptr;
    UserRole m_role = UserRole::Cashier;
    QString m_user;
    QWidget *m_requestsCard = nullptr;
    QLabel *m_requestsTitle = nullptr;
    class QVBoxLayout *m_requestsList = nullptr;
};
