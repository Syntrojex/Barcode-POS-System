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
#include <QHBoxLayout>
#include <QMessageBox>
#include <QShortcut>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
    struct NavDef
    {
        const char *text, *icon;
    };
    // Same order as Pages::Id / the sidebar of the design.
    const NavDef kNav[Pages::Count] = {
        {"Dashboard", "dashboard"},    {"Record Order", "cart"},   {"Orders", "orders"},
        {"Inventory", "inventory"},    {"Products", "products"},   {"Categories", "categories"},
        {"Barcode Scanner", "barcode"}, {"Bills", "bills"},        {"Reports", "reports"},
        {"Discounts", "percent"},      {"Settings", "settings"}};

    QLabel *chipLabel(const QString &text, const char *objectName)
    {
        auto *l = W::label(text, objectName);
        l->setAlignment(Qt::AlignCenter);
        return l;
    }
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
    sb->setFixedWidth(250);
    auto *v = new QVBoxLayout(sb);
    v->setContentsMargins(14, 18, 14, 16);
    v->setSpacing(4);

    // brand: [S] Syntro-Check / (POS V2.4 PRO) (MANAGER)
    auto *brand = new QHBoxLayout;
    brand->setContentsMargins(4, 0, 4, 0);
    brand->setSpacing(11);
    auto *logo = new QFrame;
    logo->setObjectName("logoBox");
    logo->setFixedSize(44, 44);
    auto *ll = new QVBoxLayout(logo);
    ll->setContentsMargins(0, 0, 0, 0);
    auto *ls = new QLabel("S");
    ls->setAlignment(Qt::AlignCenter);
    ll->addWidget(ls);
    brand->addWidget(logo);
    auto *bt = new QVBoxLayout;
    bt->setSpacing(3);
    bt->addWidget(W::label("Syntro-Check", "brand"));
    auto *tags = new QHBoxLayout;
    tags->setSpacing(4);
    tags->addWidget(chipLabel("POS V2.4 PRO", "tagBlue"));
    tags->addWidget(chipLabel(RolePolicy::name(m_role).toUpper(), "tagGrey"));
    tags->addStretch();
    bt->addLayout(tags);
    brand->addLayout(bt, 1);
    v->addLayout(brand);
    v->addSpacing(16);

    for (int i = 0; i < PageCount; ++i)
    {
        auto *b = new QPushButton;
        b->setObjectName("nav");
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedHeight(40);
        b->setToolTip(QString::fromLatin1(kNav[i].text));
        auto *bh = new QHBoxLayout(b);
        bh->setContentsMargins(14, 0, 14, 0);
        bh->setSpacing(13);
        auto *ic = new QLabel;
        ic->setObjectName("navIcon");
        ic->setFixedSize(20, 20);
        ic->setPixmap(Theme::iconPixmap(kNav[i].icon, Theme::themed("#94a3b8"), 20));
        auto *tx = new QLabel(QString::fromLatin1(kNav[i].text));
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
    auto *user = W::card(sb, "userCard");
    auto *uv = new QVBoxLayout(user);
    uv->setContentsMargins(10, 9, 10, 10);
    uv->setSpacing(8);
    auto *uh = new QHBoxLayout;
    uh->setSpacing(10);
    auto *av = new QLabel(m_userName.left(2).toUpper());
    av->setObjectName("avatar");
    av->setFixedSize(34, 34);
    av->setAlignment(Qt::AlignCenter);
    uh->addWidget(av);
    auto *un = new QVBoxLayout;
    un->setSpacing(0);
    un->addWidget(W::label(m_userName, "userName"));
    un->addWidget(W::label(RolePolicy::title(m_role), "userRole"));
    uh->addLayout(un, 1);
    uv->addLayout(uh);
    auto *mode = chipLabel(RolePolicy::name(m_role).toUpper() + " MODE", "modeChip");
    uv->addWidget(mode, 0, Qt::AlignLeft);
    auto *logoutB = new QPushButton("  Log out");
    logoutB->setObjectName("exitBtn");
    logoutB->setIcon(Theme::icon("logout", QColor("#c9d3ee"), 16));
    logoutB->setCursor(Qt::PointingHandCursor);
    uv->addWidget(logoutB);
    v->addWidget(user);
    connect(logoutB, &QPushButton::clicked, this, [this]
            { m_loggedOut = true; if (!close()) m_loggedOut = false; });
    return sb;
}

QWidget *MainWindow::buildTopBar()
{
    auto *bar = new QFrame;
    bar->setObjectName("topbar");
    bar->setFixedHeight(66);
    auto *h = new QHBoxLayout(bar);
    h->setContentsMargins(24, 0, 20, 0);
    h->setSpacing(12);
    auto *t = new QVBoxLayout;
    t->setSpacing(0);
    t->addStretch();
    t->addWidget(W::label("Syntro Terminal Operations", "pageTitle"));
    t->addWidget(W::label("Station ST-01  •  Main Floor Cash Register", "pageSub"));
    t->addStretch();
    h->addLayout(t);

    h->addStretch();

    auto *cur = new QFrame;
    cur->setObjectName("chip");
    auto *curH = new QHBoxLayout(cur);
    curH->setContentsMargins(14, 7, 14, 9);
    curH->setSpacing(6);
    curH->addWidget(W::label("CURRENCY", "caption"));
    curH->addWidget(W::label("PKR", "clockTime"));
    h->addWidget(cur, 0, Qt::AlignVCenter);

    auto *chip = new QFrame;
    chip->setObjectName("chip");
    auto *ch = new QHBoxLayout(chip);
    ch->setContentsMargins(14, 5, 14, 7);
    ch->setSpacing(8);
    ch->addWidget(W::iconLabel("clock", Theme::primary, 18));
    m_clockTime = W::label("", "clockTime");
    m_clockDate = W::label("", "clockDate");
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

    auto *avatar = new QLabel;
    avatar->setObjectName("topAvatar");
    avatar->setFixedSize(36, 36);
<<<<<<< HEAD
    avatar->setText(m_userName.left(2).toUpper());
    avatar->setCursor(Qt::PointingHandCursor);
    avatar->setToolTip(m_userName + " (" + RolePolicy::name(m_role) + ")");
    avatar->setPopupMode(QToolButton::InstantPopup);
    avatar->setStyleSheet("QToolButton{color:white;font-weight:800;border:none;}"
                          "QToolButton::menu-indicator{image:none;width:0;}");
    auto *menu = new QMenu(avatar);
    // dark menu; the highlighted option turns blue with white text
    menu->setStyleSheet("QMenu{background:#0f1b33;color:#8fb2ff;border:1px solid #26324f;border-radius:8px;padding:4px;}"
                        "QMenu::item{padding:8px 18px 8px 12px;border-radius:6px;}"
                        "QMenu::item:selected{background:#2554d8;color:#ffffff;}"
                        "QMenu::separator{height:1px;background:#26324f;margin:4px 8px;}");
    auto *who = new QWidget;
    auto *wv = new QVBoxLayout(who);
    wv->setContentsMargins(14, 10, 14, 8);
    wv->setSpacing(2);
    auto *nm = new QLabel(m_userName);
    nm->setStyleSheet("font-weight:800; font-size:14px; color:#ffffff;");
    auto *rl = new QLabel(RolePolicy::title(m_role));
    rl->setStyleSheet("color:#9aa7c7;");
    wv->addWidget(nm);
    wv->addWidget(rl);
    auto *header = new QWidgetAction(menu);
    header->setDefaultWidget(who);
    menu->addAction(header);
    menu->addSeparator();
    QIcon logoutIcon(Theme::iconPixmap("logout", Theme::themed("#8fb2ff"), 16));
    logoutIcon.addPixmap(Theme::iconPixmap("logout", Theme::themed("#ffffff"), 16), QIcon::Active); // white when highlighted
    QAction *logout = menu->addAction(logoutIcon, "Log out (change role)");
    connect(logout, &QAction::triggered, this, [this]
            { m_loggedOut = true; if (!close()) m_loggedOut = false; });
    avatar->setMenu(menu);
=======
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setPixmap(Theme::iconPixmap("user", QColor("white"), 18));
>>>>>>> be434a4b20a77502664cedba4647512c83bc9a5f
    h->addWidget(avatar, 0, Qt::AlignVCenter);

    auto *timer = new QTimer(this);
    auto tick = [this]
    {
        const auto now = QDateTime::currentDateTime();
        m_clockTime->setText(now.toString("HH:mm:ss"));
        m_clockDate->setText("• " + now.toString("d MMM yyyy"));
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
    for (int i = 0; i < m_nav.size(); ++i)
    {
        m_nav[i]->setProperty("active", i == page);
        if (auto *ic = m_nav[i]->findChild<QLabel *>("navIcon"))
            ic->setPixmap(Theme::iconPixmap(kNav[i].icon, i == page ? QColor("white") : Theme::themed("#94a3b8"), 20));
        Theme::repolish(m_nav[i]);
        for (auto *c : m_nav[i]->findChildren<QLabel *>())
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
    m_toast->move((root->width() - m_toast->width()) / 2 + 125, root->height() - m_toast->height() - 36);
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
