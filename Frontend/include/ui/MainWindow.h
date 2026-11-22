#pragma once
#include <QMainWindow>
#include <QVector>
#include "UserRole.h"

class QLabel;
class QPushButton;
class QStackedWidget;
class QTimer;
class DashboardPage;
class RecordOrderPage;
class OrdersPage;
class InventoryPage;
class ProductsPage;
class CategoriesPage;
class ScannerPage;
class BillsPage;
class ReportsPage;
class DiscountsPage;
class SettingsPage;

// Application shell: dark sidebar (pages depend on the signed-in role), top bar, page stack and toasts.
// Pages own their own shortcuts (F2 Hold, F10 Checkout, Ctrl+S, Ctrl+P ... only fire while the page is visible);
// the shell only registers F1 (Dashboard), Ctrl+K (lookup), Ctrl+N (new order) and Ctrl+1..0 (sidebar pages).
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    // Same values as Pages::Id (UserRole.h)
    enum Page
    {
        Dashboard = Pages::Dashboard,
        RecordOrder = Pages::RecordOrder,
        Orders = Pages::Orders,
        Inventory = Pages::Inventory,
        Products = Pages::Products,
        Categories = Pages::Categories,
        Scanner = Pages::Scanner,
        Bills = Pages::Bills,
        Reports = Pages::Reports,
        Discounts = Pages::Discounts,
        Settings = Pages::Settings,
        PageCount = Pages::Count
    };
    explicit MainWindow(UserRole role = UserRole::Cashier, const QString &userName = QString(), QWidget *parent = nullptr);
    bool loggedOut() const { return m_loggedOut; } // true if closed with "Log out"
    bool goTo(int page);                           // false if the signed-in role may not open this page
    QPixmap grabPage(int page);                    // used by the screenshot/self-test helper
    UserRole role() const { return m_role; }

    // Pages, for the screenshot harness and tests
    RecordOrderPage *recordPage() const { return m_record; }
    OrdersPage *ordersPage() const { return m_orders; }
    BillsPage *billsPage() const { return m_bills; }
    ProductsPage *productsPage() const { return m_products; }

protected:
    void resizeEvent(QResizeEvent *e) override;
    void closeEvent(QCloseEvent *e) override;

private slots:
    void showToast(const QString &text);

private:
    QWidget *buildSidebar();
    QWidget *buildTopBar();
    void positionToast();
    void openOrderForEdit(const QString &orderId);
    void showBillFor(const QString &orderId);

    UserRole m_role;
    QString m_userName;
    bool m_loggedOut = false;
    QStackedWidget *m_stack;
    QVector<QPushButton *> m_nav;
    QLabel *m_clockTime, *m_clockDate, *m_toast;
    QTimer *m_toastTimer;

    DashboardPage *m_dashboard;
    RecordOrderPage *m_record;
    OrdersPage *m_orders;
    InventoryPage *m_inventory;
    ProductsPage *m_products;
    CategoriesPage *m_categories;
    ScannerPage *m_scanner;
    BillsPage *m_bills;
    ReportsPage *m_reports;
    DiscountsPage *m_discounts;
    SettingsPage *m_settings;
};
