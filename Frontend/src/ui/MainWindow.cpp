#include "MainWindow.h"
#include "AppState.h"
#include "DashboardPage.h"
#include "OtherPages.h"
#include "RecordOrderPage.h"
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
        const char *text, *icon, *key;
    };
    const NavDef kNav[] = {
        {"Dashboard", "dashboard", "F1"}, {"Record Order", "cart", "F2"}, {"Orders", "orders", "F3"},
        {"Products", "products", "F4"}, {"Barcode Scanner", "barcode", "F5"}, {"Bills", "bills", "F6"},
        {"Settings", "settings", "F10"}};
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("SmartPOS - BarcodeHub");
    setMinimumSize(1100, 700);
    resize(1360, 860);

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
    m_products = new ProductsPage;
    m_scanner = new ScannerPage;
    m_bills = new BillsPage;
    m_settings = new SettingsPage;
    for (QWidget *w : QList<QWidget *>{m_dashboard, m_record, m_orders, m_products, m_scanner, m_bills, m_settings})
        m_stack->addWidget(w);
    rv->addWidget(m_stack, 1);
    h->addWidget(right, 1);

    m_toast = new QLabel(root);
    m_toast->setObjectName("toast");
    m_toast->hide();
    m_toastTimer = new QTimer(this);
    m_toastTimer->setSingleShot(true);
    connect(m_toastTimer, &QTimer::timeout, m_toast, &QWidget::hide);

    // page wiring
    for (QObject *o : QList<QObject *>{m_dashboard, m_record, m_orders, m_products, m_scanner, m_bills, m_settings})
        connect(o, SIGNAL(toast(QString)), this, SLOT(showToast(QString)));
    connect(m_dashboard, &DashboardPage::navigate, this, &MainWindow::goTo);
    connect(m_dashboard, &DashboardPage::lookup, this, [this](const QString &text)
            {
        if (text.trimmed().isEmpty()) return;
        if (AppState::instance().findByCode(text)) { m_products->setSearch(text); goTo(Products); }
        else { m_orders->setSearch(text); goTo(Orders); } });
    auto resume = [this](const QString &id)
    {
        goTo(RecordOrder);
        showToast("Resumed " + id + " (demo: line items are not restored)");
    };
    connect(m_dashboard, &DashboardPage::resumeOrder, this, resume);
    connect(m_orders, &OrdersPage::resumeOrder, this, resume);
    connect(m_products, &ProductsPage::addToOrder, this, [this](const QString &ean)
            { goTo(RecordOrder); m_record->showCode(ean, true); });
    connect(m_scanner, &ScannerPage::sendToOrder, this, [this](const QString &ean)
            { goTo(RecordOrder); m_record->showCode(ean, true); });

    // F-keys
    for (int n : {1, 2, 3, 4, 5, 6, 9, 10})
    {
        auto *sc = new QShortcut(QKeySequence(Qt::Key_F1 + n - 1), this);
        sc->setContext(Qt::ApplicationShortcut);
        connect(sc, &QShortcut::activated, this, [this, n]
                { handleFKey(n); });
    }
    auto *k = new QShortcut(QKeySequence("Ctrl+K"), this);
    connect(k, &QShortcut::activated, this, [this]
            { goTo(Dashboard); m_dashboard->focusLookup(); });
    auto *nd = new QShortcut(QKeySequence("Ctrl+N"), this);
    connect(nd, &QShortcut::activated, this, [this]
            { goTo(RecordOrder); m_record->newDraft(); });

    goTo(RecordOrder);
}

QWidget *MainWindow::buildSidebar()
{
    auto *sb = new QFrame;
    sb->setObjectName("sidebar");
    sb->setFixedWidth(244);
    auto *v = new QVBoxLayout(sb);
    v->setContentsMargins(10, 10, 10, 10);
    v->setSpacing(8);

    auto *brand = new QHBoxLayout;
    brand->setContentsMargins(4, 2, 4, 2);
    auto *logo = new QFrame;
    logo->setObjectName("logoBox");
    logo->setFixedSize(38, 38);
    auto *ll = new QVBoxLayout(logo);
    ll->setContentsMargins(0, 0, 0, 0);
    auto *ls = new QLabel("S");
    ls->setAlignment(Qt::AlignCenter);
    ll->addWidget(ls);
    brand->addWidget(logo);
    auto *bt = new QVBoxLayout;
    bt->setSpacing(0);
    bt->addWidget(W::label("SmartPOS", "brand"));
    auto *ver = W::label("v2.4 LTS Retail");
    ver->setStyleSheet("color:#737686; font-size:11px;");
    bt->addWidget(ver);
    brand->addLayout(bt);
    brand->addStretch();
    brand->addWidget(W::badge("POS#4", "grey"));
    v->addLayout(brand);
    v->addSpacing(4);

    auto *user = W::card(sb, "userCard");
    auto *uv = new QVBoxLayout(user);
    uv->setContentsMargins(10, 10, 10, 10);
    uv->setSpacing(8);
    auto *ur = new QHBoxLayout;
    auto *av = new QLabel("AR");
    av->setObjectName("avatar");
    av->setFixedSize(32, 32);
    av->setAlignment(Qt::AlignCenter);
    ur->addWidget(av);
    auto *un = new QVBoxLayout;
    un->setSpacing(0);
    auto *nm = W::label("Alex Rivera");
    nm->setStyleSheet("font-weight:700; font-size:13.5px;");
    un->addWidget(nm);
    auto *tm = W::label("Terminal #04", "muted");
    tm->setStyleSheet("color:#434655; font-size:12px;");
    un->addWidget(tm);
    ur->addLayout(un);
    ur->addStretch();
    ur->addWidget(W::iconLabel("lock", Theme::outline, 16));
    uv->addLayout(ur);
    auto *rr = new QVBoxLayout;
    rr->setSpacing(4);
    rr->addWidget(W::label("ACTIVE ROLE", "caption"));
    auto *seg = new QFrame;
    seg->setObjectName("segment");
    auto *sh = new QHBoxLayout(seg);
    sh->setContentsMargins(2, 2, 2, 2);
    sh->setSpacing(0);
    auto *cashier = new QPushButton("Cashier");
    auto *manager = new QPushButton("Manager");
    for (auto *b : {cashier, manager})
    {
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sh->addWidget(b);
    }
    manager->setChecked(true);
    rr->addWidget(seg);
    uv->addLayout(rr);
    v->addWidget(user);
    v->addSpacing(6);

    for (int i = 0; i < PageCount; ++i)
    {
        auto *b = new QPushButton;
        b->setObjectName("nav");
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedHeight(40);
        auto *bh = new QHBoxLayout(b);
        bh->setContentsMargins(12, 0, 12, 0);
        bh->setSpacing(12);
        auto *ic = new QLabel;
        ic->setObjectName("navIcon");
        ic->setFixedSize(20, 20);
        ic->setPixmap(Theme::iconPixmap(kNav[i].icon, Theme::onSurface, 20));
        auto *tx = new QLabel(kNav[i].text);
        auto *fk = new QLabel(kNav[i].key);
        fk->setObjectName("fkey");
        for (auto *l : {static_cast<QWidget *>(ic), static_cast<QWidget *>(tx), static_cast<QWidget *>(fk)})
            l->setAttribute(Qt::WA_TransparentForMouseEvents);
        bh->addWidget(ic);
        bh->addWidget(tx, 1);
        bh->addWidget(fk);
        v->addWidget(b);
        m_nav << b;
        connect(b, &QPushButton::clicked, this, [this, i]
                { goTo(i); });
    }
    connect(cashier, &QPushButton::clicked, this, [=, this]
            {
        cashier->setChecked(true); manager->setChecked(false);
        m_nav[Settings]->setProperty("locked", true); Theme::repolish(m_nav[Settings]); for (auto *c : m_nav[Settings]->findChildren<QLabel *>()) Theme::repolish(c);
        if (m_stack->currentIndex() == Settings) goTo(Dashboard);
        showToast("Cashier role: Settings locked"); });
    connect(manager, &QPushButton::clicked, this, [=, this]
            {
        manager->setChecked(true); cashier->setChecked(false);
        m_nav[Settings]->setProperty("locked", false); Theme::repolish(m_nav[Settings]); for (auto *c : m_nav[Settings]->findChildren<QLabel *>()) Theme::repolish(c);
        showToast("Manager role: full access"); });

    v->addStretch();
    auto *foot = new QHBoxLayout;
    foot->setContentsMargins(6, 0, 6, 0);
    foot->addWidget(W::dot(QColor("#16a34a")));
    foot->addWidget(W::label("Demo • local data", "navFooter"));
    foot->addStretch();
    foot->addWidget(W::label("Store #102", "navFooter"));
    v->addLayout(foot);
    auto *exitB = W::button("Exit Terminal", "", "logout");
    exitB->setMinimumHeight(36);
    v->addWidget(exitB);
    connect(exitB, &QPushButton::clicked, this, &QWidget::close);
    return sb;
}

QWidget *MainWindow::buildTopBar()
{
    auto *bar = new QFrame;
    bar->setObjectName("topbar");
    bar->setFixedHeight(58);
    auto *h = new QHBoxLayout(bar);
    h->setContentsMargins(16, 0, 14, 0);
    h->setSpacing(12);
    h->addWidget(W::badge("LIVE SHIFT", "blue"));
    m_title = W::label("SmartPOS Retail Workspace", "topTitle");
    h->addWidget(m_title);
    h->addSpacing(8);
    h->addWidget(W::iconLabel("info", Theme::outline, 15));
    h->addWidget(W::label("Press F1-F12 for fast routing", "topHint"));
    h->addStretch();

    auto *reg = new QFrame;
    reg->setObjectName("chip");
    auto *rh = new QHBoxLayout(reg);
    rh->setContentsMargins(12, 6, 12, 6);
    rh->setSpacing(8);
    rh->addWidget(W::dot(QColor("#16a34a")));
    rh->addWidget(W::label("Register 02"));
    rh->addWidget(W::label("|", "muted"));
    auto *cd = W::label("Cash Drawer: Active ($350.00 Float)");
    cd->setStyleSheet("color:#434655;");
    rh->addWidget(cd);
    h->addWidget(reg);

    h->addWidget(W::iconLabel("wifi", Theme::tertiary, 18));
    auto *ping = W::label("local");
    ping->setStyleSheet("color:#006329; font-weight:700; font-size:12px;");
    h->addWidget(ping);

    auto *clock = new QVBoxLayout;
    clock->setSpacing(0);
    m_clockTime = W::label("", "clockTime");
    m_clockTime->setAlignment(Qt::AlignRight);
    m_clockDate = W::label("", "clockDate");
    m_clockDate->setAlignment(Qt::AlignRight);
    clock->addWidget(m_clockTime);
    clock->addWidget(m_clockDate);
    h->addLayout(clock);
    auto *t = new QTimer(this);
    auto tick = [this]
    {
        const auto now = QDateTime::currentDateTime();
        m_clockTime->setText(now.toString("HH:mm:ss"));
        m_clockDate->setText(now.toString("dd MMM yyyy"));
    };
    connect(t, &QTimer::timeout, this, tick);
    t->start(1000);
    tick();

    auto *av = new QLabel;
    av->setObjectName("topAvatar");
    av->setFixedSize(34, 34);
    av->setAlignment(Qt::AlignCenter);
    av->setPixmap(Theme::iconPixmap("user", Qt::white, 18));
    h->addWidget(av);
    h->addWidget(W::iconLabel("chevron-down", Theme::outline, 14));
    return bar;
}

void MainWindow::goTo(int page)
{
    if (page < 0 || page >= PageCount)
        return;
    if (page == Settings && m_nav[Settings]->property("locked").toBool())
    {
        showToast("Settings are available to the Manager role");
        return;
    }
    m_stack->setCurrentIndex(page);
    for (int i = 0; i < m_nav.size(); ++i)
    {
        m_nav[i]->setProperty("active", i == page);
        if (auto *ic = m_nav[i]->findChild<QLabel *>("navIcon"))
            ic->setPixmap(Theme::iconPixmap(kNav[i].icon, i == page ? QColor("white") : Theme::onSurface, 20));
        Theme::repolish(m_nav[i]);
        for (auto *c : m_nav[i]->findChildren<QLabel *>())
            Theme::repolish(c);
    }
    if (page == RecordOrder)
        m_record->focusInput();
}

QPixmap MainWindow::grabPage(int page)
{
    goTo(page);
    QApplication::processEvents();
    return grab();
}

void MainWindow::handleFKey(int n)
{
    const bool onRecord = m_stack->currentIndex() == RecordOrder;
    switch (n)
    {
    case 1: goTo(Dashboard); break;
    case 2: goTo(RecordOrder); break;
    case 3: goTo(Orders); break;
    case 4:
        if (onRecord) m_record->quickLookup();
        else goTo(Products);
        break;
    case 5: goTo(Scanner); break;
    case 6: goTo(Bills); break;
    case 9:
        if (!onRecord) goTo(RecordOrder);
        m_record->simulateScan();
        break;
    case 10:
        if (onRecord) m_record->checkout();
        else goTo(Settings);
        break;
    }
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
    m_toast->move((root->width() - m_toast->width()) / 2 + 120, root->height() - m_toast->height() - 64);
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    positionToast();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    const auto r = QMessageBox::question(this, "Exit Terminal", "Close SmartPOS? The current draft will be lost.");
    if (r == QMessageBox::Yes)
        e->accept();
    else
        e->ignore();
}
