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
class ProductsPage;
class BillsPage;
class ScannerPage;
class SettingsPage;

// Application shell: sidebar navigation, top bar, page stack, F-key shortcuts and toasts.
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    enum Page { Dashboard, RecordOrder, Orders, Products, Scanner, Bills, Settings, PageCount };
    explicit MainWindow(UserRole role = UserRole::Cashier, const QString &userName = QString(), QWidget *parent = nullptr);
    bool loggedOut() const { return m_loggedOut; } // true if closed with "Log out"
    bool goTo(int page); // false if the signed-in role may not open this page
    QPixmap grabPage(int page); // used by the screenshot/self-test helper

protected:
    void resizeEvent(QResizeEvent *e) override;
    void closeEvent(QCloseEvent *e) override;

private slots:
    void showToast(const QString &text);

private:
    QWidget *buildSidebar();
    QWidget *buildTopBar();
    void handleFKey(int n);
    void positionToast();

    UserRole m_role;
    QString m_userName;
    bool m_loggedOut = false;
    QStackedWidget *m_stack;
    QVector<QPushButton *> m_nav;
    QLabel *m_title, *m_subtitle, *m_clockTime, *m_clockDate, *m_toast;
    QTimer *m_toastTimer;

    DashboardPage *m_dashboard;
    RecordOrderPage *m_record;
    OrdersPage *m_orders;
    ProductsPage *m_products;
    ScannerPage *m_scanner;
    BillsPage *m_bills;
    SettingsPage *m_settings;
};
