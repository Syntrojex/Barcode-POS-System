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
    const char *kSub[] = {"Today's performance at a glance", "Scan or search products and build the order",
                          "Every order from this shift", "Catalog and stock levels", "Scan a barcode to look up a product",
                          "Printable bills for paid orders", "Tax, discount and scanning preferences"};
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("BarcodeHub - SmartPOS");
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
    sb->setFixedWidth(232);
    auto *v = new QVBoxLayout(sb);
    v->setContentsMargins(14, 18, 14, 16);
    v->setSpacing(6);

    auto *brand = new QHBoxLayout;
    brand->setContentsMargins(4, 0, 4, 0);
    brand->setSpacing(11);
    auto *logo = new QFrame;
    logo->setObjectName("logoBox");
    logo->setFixedSize(42, 42);
    auto *ll = new QVBoxLayout(logo);
    ll->setContentsMargins(0, 0, 0, 0);
    auto *ls = new QLabel("S");
    ls->setAlignment(Qt::AlignCenter);
    ll->addWidget(ls);
    brand->addWidget(logo);
    auto *bt = new QVBoxLayout;
    bt->setSpacing(0);
    bt->addWidget(W::label("SmartPOS", "brand"));
    bt->addWidget(W::label("Retail workspace", "brandSub"));
    brand->addLayout(bt);
    brand->addStretch();
    v->addLayout(brand);
    v->addSpacing(18);

    for (int i = 0; i < PageCount; ++i)
    {
        auto *b = new QPushButton;
        b->setObjectName("nav");
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedHeight(44);
        b->setToolTip(QString("%1  (%2)").arg(kNav[i].text, kNav[i].key));
        auto *bh = new QHBoxLayout(b);
        bh->setContentsMargins(14, 0, 14, 0);
        bh->setSpacing(13);
        auto *ic = new QLabel;
        ic->setObjectName("navIcon");
        ic->setFixedSize(20, 20);
        ic->setPixmap(Theme::iconPixmap(kNav[i].icon, QColor("#b8c3e0"), 20));
        auto *tx = new QLabel(kNav[i].text);
        for (QWidget *l : {static_cast<QWidget *>(ic), static_cast<QWidget *>(tx)})
            l->setAttribute(Qt::WA_TransparentForMouseEvents);
        bh->addWidget(ic);
        bh->addWidget(tx, 1);
        v->addWidget(b);
        m_nav << b;
        connect(b, &QPushButton::clicked, this, [this, i]
                { goTo(i); });
    }

    v->addStretch();
    auto *user = W::card(sb, "userCard");
    auto *uh = new QHBoxLayout(user);
    uh->setContentsMargins(10, 9, 10, 9);
    uh->setSpacing(10);
    auto *av = new QLabel("AR");
    av->setObjectName("avatar");
    av->setFixedSize(34, 34);
    av->setAlignment(Qt::AlignCenter);
    uh->addWidget(av);
    auto *un = new QVBoxLayout;
    un->setSpacing(0);
    un->addWidget(W::label("Alex Rivera", "userName"));
    un->addWidget(W::label("Cashier • Terminal 04", "userRole"));
    uh->addLayout(un, 1);
    v->addWidget(user);
    v->addSpacing(4);
    auto *exitB = new QPushButton("  Exit");
    exitB->setObjectName("exitBtn");
    exitB->setIcon(Theme::icon("logout", QColor("#c9d3ee"), 16));
    exitB->setCursor(Qt::PointingHandCursor);
    v->addWidget(exitB);
    connect(exitB, &QPushButton::clicked, this, &QWidget::close);
    return sb;
}

QWidget *MainWindow::buildTopBar()
{
    auto *bar = new QFrame;
    bar->setObjectName("topbar");
    bar->setFixedHeight(72);
    auto *h = new QHBoxLayout(bar);
    h->setContentsMargins(24, 0, 20, 0);
    h->setSpacing(14);
    auto *t = new QVBoxLayout;
    t->setSpacing(0);
    m_title = W::label("", "pageTitle");
    m_subtitle = W::label("", "pageSub");
    t->addStretch();
    t->addWidget(m_title);
    t->addWidget(m_subtitle);
    t->addStretch();
    h->addLayout(t);
    h->addStretch();

    auto *chip = new QFrame;
    chip->setObjectName("chip");
    auto *ch = new QHBoxLayout(chip);
    ch->setContentsMargins(14, 5, 14, 7);
    ch->setSpacing(10);
    ch->addWidget(W::iconLabel("clock", Theme::primary, 18));
    auto *cl = new QVBoxLayout;
    cl->setSpacing(0);
    m_clockTime = W::label("", "clockTime");
    m_clockDate = W::label("", "clockDate");
    cl->addWidget(m_clockTime);
    cl->addWidget(m_clockDate);
    ch->addLayout(cl);
    h->addWidget(chip, 0, Qt::AlignVCenter);
    auto *timer = new QTimer(this);
    auto tick = [this]
    {
        const auto now = QDateTime::currentDateTime();
        m_clockTime->setText(now.toString("HH:mm:ss"));
        m_clockDate->setText(now.toString("ddd, d MMM yyyy"));
    };
    connect(timer, &QTimer::timeout, this, tick);
    timer->start(1000);
    tick();
    return bar;
}

void MainWindow::goTo(int page)
{
    if (page < 0 || page >= PageCount)
        return;
    m_stack->setCurrentIndex(page);
    m_title->setText(kNav[page].text);
    m_subtitle->setText(kSub[page]);
    for (int i = 0; i < m_nav.size(); ++i)
    {
        m_nav[i]->setProperty("active", i == page);
        if (auto *ic = m_nav[i]->findChild<QLabel *>("navIcon"))
            ic->setPixmap(Theme::iconPixmap(kNav[i].icon, i == page ? QColor("white") : QColor("#b8c3e0"), 20));
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
    m_toast->move((root->width() - m_toast->width()) / 2 + 116, root->height() - m_toast->height() - 36);
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
