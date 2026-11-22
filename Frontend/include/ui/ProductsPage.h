#pragma once
// Products catalogue: search, description, add-to-order (cashier) and add/edit/barcode (manager).
#include "AppState.h"
#include "UserRole.h"
#include <QWidget>

class QLabel;
class QLineEdit;
class QTableWidget;
class QPushButton;

class ProductsPage : public QWidget
{
    Q_OBJECT
public:
    explicit ProductsPage(QWidget *parent = nullptr);
    void setSearch(const QString &text);
    void setViewOnly(bool on);       // hide the "Add" (to order) buttons
    void setRole(UserRole role);     // Manager gets "New Product" / edit actions
public slots:
    void addNew();                   // open "Add New Product to Catalog" (Manager)
public:
signals:
    void toast(const QString &message);
    void addToOrder(const QString &ean);

private:
    void rebuild();
    bool m_viewOnly = false;
    UserRole m_role = UserRole::Cashier;
    QLineEdit *m_search = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_count = nullptr;
    QPushButton *m_issues = nullptr;
    QPushButton *m_newBtn = nullptr; // "New Product" (manager only)
};
