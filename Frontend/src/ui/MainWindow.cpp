#include "MainWindow.h"
#include "AppState.h"
#include "BillsPage.h"
#include "CategoriesPage.h"
#include "DarkModeManager.h"
#include "DashboardPage.h"
#include "DiscountsPage.h"
#include "InventoryPage.h"
#include "OrdersPage.h"
#include "ProductsPage.h"
#include "RecordOrderPage.h"
#include "ReportsPage.h"
#include "ScannerPage.h"
#include "SettingsPage.h"
#include "Theme.h"
#include "Widgets.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QToolButton>
#include <QWidgetAction>
#include <QShortcut>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
    struct NavDef
    {
        const char *text, *icon, *subtitle;
    };
    // Same order as Pages::Id / the sidebar of the design.
    const NavDef kNav[Pages::Count] = {
        {"Dashboard", "dashboard", "Track sales and recent activity"},
        {"Record Order", "cart", "Build and check out a sale"},
        {"Orders", "orders", "Find saved orders and resume drafts"},
        {"Inventory", "inventory", "Review stock levels and low-stock items"},
        {"Products", "products", "Search the product catalog"},
        {"Categories", "categories", "Organize products into categories"},
        {"Barcode Scanner", "barcode", "Scan or enter a barcode to find an item"},
        {"Bills", "bills", "Find bills and print receipts"},
        {"Reports", "reports", "Review sales and inventory reports"},
        {"Discounts", "percent", "Manage discount rules"},
        {"Settings", "settings", "Adjust terminal preferences"}};

    constexpr int sidebarWidth = 232;
    const QColor navIconColor("#b9c6da");
}

MainWindow::MainWindow(UserRole role, const QString &userName, QWidget *parent)
    : QMainWindow(parent), m_role(role), m_userName(userName.isEmpty() ? RolePolicy::name(role) : userName)
{
    setWindowTitle("Syntro-Check");
    setMinimumSize(1100, 700);
    resize(1440, 900);

    auto *root = new QWidget;
    root->setObjectName("root");
    setCentralWidget(root);
    auto *h = new QHBoxLayout(root);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(0);
    h->addWidget(buildSidebar());

    auto *right = new QWidget;
    auto *rv = new QVBoxLayout(right);
    rv->setContentsMargins(0, 0, 0, 0);
    rv->setSpacing(0);
    rv->addWidget(buildTopBar());

    m_stack = new QStackedWidget;
    m_dashboard = new DashboardPage;
    m_record = new RecordOrderPage;
    m_orders = new OrdersPage;
    m_inventory = new InventoryPage;
    m_products = new ProductsPage;
    m_categories = new CategoriesPage;
    m_scanner = new ScannerPage;
    m_bills = new BillsPage;
    m_reports = new ReportsPage;
    m_discounts = new DiscountsPage;
    m_settings = new SettingsPage;
    const QList<QWidget *> pages = {m_dashboard, m_record,  m_orders,  m_inventory, m_products, m_categories,
                                    m_scanner,   m_bills,   m_reports, m_discounts, m_settings};
    Q_ASSERT(pages.size() == PageCount);
    for (QWidget *w : pages)
        m_stack->addWidget(w);
    rv->addWidget(m_stack, 1);
    h->addWidget(right, 1);

    // Cashier: products and scanner are look-up only. Manager: full catalogue tools.
    m_products->setRole(m_role);
    m_products->setViewOnly(!RolePolicy::canOpen(m_role, RecordOrder));
    m_scanner->setViewOnly(!RolePolicy::canOpen(m_role, RecordOrder));
    m_orders->setRole(m_role);
    m_record->setRole(m_role);

    m_toast = new QLabel(root);
    m_toast->setObjectName("toast");
    m_toast->hide();
    m_toastTimer = new QTimer(this);
    m_toastTimer->setSingleShot(true);
    connect(m_toastTimer, &QTimer::timeout, m_toast, &QWidget::hide);

    // ---- page wiring ----
    for (QObject *o : QList<QObject *>(pages.begin(), pages.end()))
        connect(o, SIGNAL(toast(QString)), this, SLOT(showToast(QString)));

    connect(m_dashboard, &DashboardPage::navigate, this, &MainWindow::goTo);
    connect(m_dashboard, &DashboardPage::lookup, this, [this](const QString &text)
            {
        if (text.trimmed().isEmpty()) return;
        if (AppState::instance().findByCode(text)) { m_products->setSearch(text); goTo(Products); }
        else { m_orders->setSearch(text); goTo(Orders); } });
    connect(m_dashboard, &DashboardPage::resumeOrder, this, &MainWindow::openOrderForEdit);

    connect(m_products, &ProductsPage::addToOrder, this, [this](const QString &ean)
            { if (goTo(RecordOrder)) m_record->showCode(ean, true); });
    connect(m_scanner, &ScannerPage::sendToOrder, this, [this](const QString &ean)
            { if (goTo(RecordOrder)) m_record->showCode(ean, true); });
    connect(m_scanner, &ScannerPage::sendBatchToOrder, this, [this](const QVector<QPair<QString, int>> &items)
            { if (goTo(RecordOrder)) m_record->addScannedBatch(items); });

    connect(m_orders, &OrdersPage::resumeOrder, this, &MainWindow::openOrderForEdit);
    connect(m_orders, &OrdersPage::editOrder, this, &MainWindow::openOrderForEdit);
    connect(m_orders, &OrdersPage::showBill, this, &MainWindow::showBillFor);

    connect(m_record, &RecordOrderPage::billReady, this, &MainWindow::showBillFor);
    connect(m_bills, &BillsPage::newOrderRequested, this, [this]
            { if (goTo(RecordOrder)) m_record->newDraft(); });
    connect(m_inventory, &InventoryPage::newProduct, this, [this]
            { if (goTo(Products)) m_products->addNew(); });

    // ---- shell shortcuts (pages register their own) ----
    auto *f1 = new QShortcut(QKeySequence(Qt::Key_F1), this);
    f1->setContext(Qt::ApplicationShortcut);
    connect(f1, &QShortcut::activated, this, [this] { goTo(Dashboard); });
    auto *k = new QShortcut(QKeySequence("Ctrl+K"), this);
    connect(k, &QShortcut::activated, this, [this]
            { if (goTo(Dashboard)) m_dashboard->focusLookup(); });
    auto *nd = new QShortcut(QKeySequence("Ctrl+N"), this);
    connect(nd, &QShortcut::activated, this, [this]
            { if (goTo(RecordOrder)) m_record->newDraft(); });
    // Ctrl+1 .. Ctrl+0: the pages in sidebar order (only the ones this role can see)
    QVector<int> visible;
    for (int i = 0; i < PageCount; ++i)
        if (RolePolicy::canOpen(m_role, i))
            visible << i;
    for (int n = 0; n < visible.size() && n < 10; ++n)
    {
        auto *sc = new QShortcut(QKeySequence(QString("Ctrl+%1").arg((n + 1) % 10)), this);
        const int page = visible[n];
        connect(sc, &QShortcut::activated, this, [this, page] { goTo(page); });
    }

    goTo(RolePolicy::startPage(m_role));
}

QWidget *MainWindow::buildSidebar()
{
    auto *sb = new QFrame;
    sb->setObjectName("sidebar");
    sb->setFixedWidth(sidebarWidth);
    auto *v = new QVBoxLayout(sb);
    v->setContentsMargins(12, 16, 12, 16);
    v->setSpacing(4);

    auto *brand = new QHBoxLayout;
    brand->setContentsMargins(4, 0, 4, 0);
    brand->setSpacing(11);
    auto *logo = new QFrame;
    logo->setObjectName("logoBox");
    logo->setFixedSize(40, 40);
    auto *ll = new QVBoxLayout(logo);
    ll->setContentsMargins(0, 0, 0, 0);
    auto *ls = new QLabel("S");
    ls->setAlignment(Qt::AlignCenter);
    ll->addWidget(ls);
    brand->addWidget(logo);
    auto *bt = new QVBoxLayout;
    bt->setSpacing(3);
    bt->addWidget(W::label("Syntro-Check", "brand"));
    auto *brandDetails = new QHBoxLayout;
    brandDetails->setSpacing(5);
    brandDetails->addWidget(W::label("Point of sale", "tagBlue"));
    brandDetails->addWidget(W::label(QStringLiteral("· ") + RolePolicy::name(m_role), "tagGrey"));
    brandDetails->addStretch();
    bt->addLayout(brandDetails);
    brand->addLayout(bt, 1);
    v->addLayout(brand);

    const auto section = [v](const QString &text)
    {
        v->addSpacing(10);
        v->addWidget(W::label(text, "navSection"));
    };
    for (int i = 0; i < PageCount; ++i)
    {
        switch (i)
        {
        case Dashboard: section("Sales"); break;
        case Inventory: section("Catalog"); break;
        case Scanner: section("Operations"); break;
        default: break;
        }
        const auto title = QString::fromLatin1(kNav[i].text);
        auto *b = new QPushButton;
        b->setObjectName("nav");
        b->setProperty("active", false);
        b->setAccessibleName(title);
        b->setCursor(Qt::PointingHandCursor);
        b->setFocusPolicy(Qt::StrongFocus);
        b->setFixedHeight(36);
        b->setToolTip(title);
        auto *bh = new QHBoxLayout(b);
        bh->setContentsMargins(12, 0, 12, 0);
        bh->setSpacing(11);
        auto *ic = new QLabel;
        ic->setObjectName("navIcon");
        ic->setFixedSize(18, 18);
        ic->setPixmap(Theme::iconPixmap(kNav[i].icon, navIconColor, ic->width()));
        auto *tx = new QLabel(title);
        for (QWidget *l : {static_cast<QWidget *>(ic), static_cast<QWidget *>(tx)})
            l->setAttribute(Qt::WA_TransparentForMouseEvents);
        bh->addWidget(ic);
        bh->addWidget(tx, 1);
        v->addWidget(b);
        m_nav << b;
        b->setVisible(RolePolicy::canOpen(m_role, i)); // hide pages this role cannot use
        connect(b, &QPushButton::clicked, this, [this, i] { goTo(i); });
    }

    v->addStretch();
    return sb;
}

QWidget *MainWindow::buildTopBar()
{
    auto *bar = new QFrame;
    bar->setObjectName("topbar");
    bar->setMinimumHeight(70);
    auto *h = new QHBoxLayout(bar);
    h->setContentsMargins(20, 10, 16, 10);
    h->setSpacing(12);
    auto *t = new QVBoxLayout;
    t->setSpacing(2);
    t->addStretch();
    m_pageTitle = W::label("", "pageTitle");
    m_pageSubtitle = W::label("", "pageSub");
    m_pageSubtitle->setWordWrap(true);
    t->addWidget(m_pageTitle);
    t->addWidget(m_pageSubtitle);
    t->addStretch();
    h->addLayout(t, 1);

    auto *ready = new QFrame;
    ready->setObjectName("readyPill");
    ready->setToolTip("Type a barcode to look up an item. The scanning page shows camera status.");
    auto *rh = new QHBoxLayout(ready);
    rh->setContentsMargins(10, 6, 10, 6);
    rh->addWidget(W::label("Barcode input available", "readyText"));
    h->addWidget(ready, 0, Qt::AlignVCenter);

    auto *chip = new QFrame;
    chip->setObjectName("chip");
    auto *ch = new QHBoxLayout(chip);
    ch->setContentsMargins(12, 5, 12, 5);
    m_clockTime = W::label("", "clockTime");
    m_clockDate = W::label("", "clockDate");
    m_clockTime->setAlignment(Qt::AlignCenter);
    m_clockDate->setAlignment(Qt::AlignCenter);
    ch->addWidget(m_clockTime);
    ch->addWidget(m_clockDate);
    h->addWidget(chip, 0, Qt::AlignVCenter);

    // Dark Mode toggle (DarkModeManager)
    auto *darkToggle = barcodehub::ui::DarkModeManager::instance().createTopBarToggle(bar);
    h->addWidget(darkToggle, 0, Qt::AlignVCenter);
    connect(&barcodehub::ui::DarkModeManager::instance(), &barcodehub::ui::DarkModeManager::themeChanged,
            this, [this](bool isDark)
            {
                showToast(isDark ? "Switched to Dark Mode" : "Switched to Light Mode");
                AppState::instance().emitChanged(); // rebuild tables so their text colours match the theme
            });

    auto *avatar = new QToolButton;
    avatar->setObjectName("topAvatar");
    avatar->setFixedSize(36, 36);
    const auto names = m_userName.simplified().split(' ', Qt::SkipEmptyParts);
    const auto initials = names.size() > 1 ? names.first().left(1) + names.last().left(1)
                                           : m_userName.trimmed().left(2);
    avatar->setText(initials.toUpper());
    avatar->setAccessibleName("User menu");
    avatar->setCursor(Qt::PointingHandCursor);
    avatar->setFocusPolicy(Qt::StrongFocus);
    avatar->setToolTip(m_userName + " (" + RolePolicy::name(m_role) + ")");
    avatar->setPopupMode(QToolButton::InstantPopup);
    auto *menu = new QMenu(avatar);
    menu->setObjectName("accountMenu");
    menu->setMinimumWidth(208);
    auto *who = new QWidget;
    auto *wv = new QVBoxLayout(who);
    wv->setContentsMargins(12, 8, 12, 8);
    wv->setSpacing(2);
    auto *nm = W::label("", "h2", who);
    nm->setTextFormat(Qt::PlainText);
    nm->setAccessibleName(m_userName);
    nm->ensurePolished();
    nm->setText(nm->fontMetrics().elidedText(m_userName.simplified(), Qt::ElideRight, 220));
    wv->addWidget(nm);
    wv->addWidget(W::label(RolePolicy::title(m_role), "muted"));
    auto *header = new QWidgetAction(menu);
    header->setDefaultWidget(who);
    menu->addAction(header);
    menu->addSeparator();
    QAction *logout = menu->addAction("Log out");
    connect(logout, &QAction::triggered, this, [this]
            { m_loggedOut = true; if (!close()) m_loggedOut = false; });
    avatar->setMenu(menu);
    h->addWidget(avatar, 0, Qt::AlignVCenter);

    auto *timer = new QTimer(this);
    auto tick = [this]
    {
        const auto now = QDateTime::currentDateTime();
        m_clockTime->setText(now.toString("HH:mm:ss"));
        m_clockDate->setText(now.toString("d MMM yyyy"));
    };
    connect(timer, &QTimer::timeout, this, tick);
    timer->start(1000);
    tick();
    return bar;
}

bool MainWindow::goTo(int page)
{
    if (page < 0 || page >= PageCount)
        return false;
    if (!RolePolicy::canOpen(m_role, page))
    {
        showToast(QString("%1 cannot open %2.").arg(RolePolicy::name(m_role), QString::fromLatin1(kNav[page].text)));
        return false;
    }
    m_stack->setCurrentIndex(page);
    m_pageTitle->setText(QString::fromLatin1(kNav[page].text));
    m_pageSubtitle->setText(QString::fromLatin1(kNav[page].subtitle));
    for (int i = 0; i < m_nav.size(); ++i)
    {
        auto *button = m_nav[i];
        const bool active = i == page;
        if (button->property("active").toBool() == active)
            continue;
        button->setProperty("active", active);
        if (auto *ic = button->findChild<QLabel *>("navIcon"))
            ic->setPixmap(Theme::iconPixmap(kNav[i].icon, active ? Theme::white : navIconColor, ic->width()));
        Theme::repolish(button);
        for (auto *c : button->findChildren<QLabel *>())
            Theme::repolish(c);
    }
    switch (page)
    {
    case Dashboard: m_dashboard->refresh(); break;
    case RecordOrder: m_record->focusInput(); break;
    case Inventory: m_inventory->refresh(); break;
    case Categories: m_categories->refresh(); break;
    case Reports: m_reports->refresh(); break;
    case Discounts: m_discounts->refresh(); break;
    default: break;
    }
    return true;
}

QPixmap MainWindow::grabPage(int page)
{
    goTo(page);
    QApplication::processEvents();
    return grab();
}

void MainWindow::openOrderForEdit(const QString &orderId)
{
    if (goTo(RecordOrder))
        m_record->loadOrderForEdit(orderId);
}

void MainWindow::showBillFor(const QString &orderId)
{
    if (goTo(Bills))
        m_bills->showOrder(orderId);
}

void MainWindow::showToast(const QString &text)
{
    m_toast->setText(text);
    m_toast->adjustSize();
    positionToast();
    m_toast->raise();
    m_toast->show();
    m_toastTimer->start(2600);
}

void MainWindow::positionToast()
{
    if (!m_toast)
        return;
    const QWidget *root = centralWidget();
    int bottom = root->height() - 36;
    if (m_stack->currentIndex() == RecordOrder)
        if (const auto *summary = m_record->findChild<QWidget *>("checkoutSummary"))
            bottom = summary->mapTo(root, QPoint()).y();
    m_toast->move((root->width() + sidebarWidth - m_toast->width()) / 2, bottom - m_toast->height());
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    positionToast();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    const auto r = m_loggedOut
        ? QMessageBox::question(this, "Log out", "Log out and return to the sign-in screen? The current draft will be lost.")
        : QMessageBox::question(this, "Exit Terminal", "Close Syntro-Check? The current draft will be lost.");
    if (r == QMessageBox::Yes)
        e->accept();
    else
        e->ignore();
}
