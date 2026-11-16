#pragma once
#include <QMainWindow>
#include <QVector>

class QFrame;
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
    explicit MainWindow(QWidget *parent = nullptr);
    void goTo(int page);
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
    void setSidebarCollapsed(bool on); // icon-only rail on narrow windows

    QStackedWidget *m_stack;
    QVector<QPushButton *> m_nav;
    QLabel *m_title, *m_subtitle, *m_clockTime, *m_clockDate, *m_toast;
    QTimer *m_toastTimer;
    QFrame *m_sidebar = nullptr;
    QPushButton *m_exit = nullptr;
    bool m_collapsed = false;

    DashboardPage *m_dashboard;
    RecordOrderPage *m_record;
    OrdersPage *m_orders;
    ProductsPage *m_products;
    ScannerPage *m_scanner;
    BillsPage *m_bills;
    SettingsPage *m_settings;
};
