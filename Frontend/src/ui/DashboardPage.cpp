#include "DashboardPage.h"
#include "AppState.h"
#include "Dialogs.h"
#include "Theme.h"
#include "Widgets.h"
#include <QButtonGroup>
#include <QDate>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QLocale>
#include <QScrollArea>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
    // One KPI tile: icon, caption, big number, one supporting line.
    QFrame *statCard(const QString &caption, const QString &icon, const char *tone, const QColor &ink,
                     QLabel *&value, QLabel *&sub)
    {
        auto *c = W::card(nullptr, "dashboardPanel");
        auto *h = new QHBoxLayout(c);
        h->setContentsMargins(14, 14, 14, 14);
        auto *tile = new QLabel;
        tile->setObjectName("dashboardIcon");
        tile->setProperty("tone", tone);
        tile->setFixedSize(32, 32);
        tile->setAlignment(Qt::AlignCenter);
        tile->setPixmap(Theme::iconPixmap(icon, ink, 18));
        h->addWidget(tile, 0, Qt::AlignTop);
        auto *v = new QVBoxLayout;
        v->setSpacing(3);
        v->addWidget(W::label(caption, "caption"));
        value = W::label("", "dashboardKpi");
        value->setWordWrap(true);
        v->addWidget(value);
        sub = W::label("", "muted");
        sub->setWordWrap(true);
        v->addWidget(sub);
        v->addStretch();
        h->addLayout(v, 1);
        return c;
    }

    QTableWidget *table(const QStringList &headers)
    {
        auto *t = new QTableWidget(0, headers.size());
        t->setHorizontalHeaderLabels(headers);
        t->verticalHeader()->hide();
        t->setShowGrid(false);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setSelectionBehavior(QAbstractItemView::SelectRows);
        t->setFocusPolicy(Qt::NoFocus);
        t->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        t->horizontalHeader()->setHighlightSections(false);
        t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        return t;
    }
}

DashboardPage::DashboardPage(QWidget *parent) : QWidget(parent)
{
    build();
    connect(&AppState::instance(), &AppState::changed, this, &DashboardPage::refresh);
    refresh();
    auto *dateTimer = new QTimer(this);
    connect(dateTimer, &QTimer::timeout, this, [this]
            { if (m_dashboardDate != QDate::currentDate()) refresh(); });
    dateTimer->start(1000);
}

void DashboardPage::focusLookup()
{
    m_search->setFocus();
    m_search->selectAll();
}

void DashboardPage::build()
{
    setObjectName("page");
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *inner = new QWidget;
    inner->setObjectName("dashboardBody");
    inner->setAttribute(Qt::WA_StyledBackground);
    scroll->setWidget(inner);
    outer->addWidget(scroll);
    auto *root = new QVBoxLayout(inner);
    root->setContentsMargins(20, 18, 20, 20);
    root->setSpacing(14);

    auto *hero = W::card(nullptr, "dashboardHero");
    auto *heroLayout = new QHBoxLayout(hero);
    heroLayout->setContentsMargins(24, 20, 24, 20);
    heroLayout->setSpacing(20);
    auto *overview = new QVBoxLayout;
    overview->setSpacing(5);
    overview->addWidget(W::label("Store activity", "dashboardHeroTitle"));
    m_date = W::label({}, "dashboardHeroDate");
    overview->addWidget(m_date);
    auto *checkout = W::button("Open checkout", "primary", "cart");
    checkout->setObjectName("dashboardCheckout");
    overview->addSpacing(5);
    overview->addWidget(checkout, 0, Qt::AlignLeft);
    connect(checkout, &QPushButton::clicked, this, [this] { emit navigate(Pages::RecordOrder); });
    heroLayout->addLayout(overview, 1);

    auto *searchPanel = W::card(nullptr, "dashboardSearch");
    auto *searchLayout = new QVBoxLayout(searchPanel);
    searchLayout->setContentsMargins(13, 12, 13, 12);
    auto *searchTitle = W::label("Find an order or product", "dashboardSearchTitle");
    m_search = new QLineEdit;
    m_search->setAccessibleName(searchTitle->text());
    m_search->setPlaceholderText("Order ID or barcode");
    m_search->setMinimumWidth(210);
    m_search->setMinimumHeight(36);
    m_search->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    searchLayout->addWidget(searchTitle);
    searchLayout->addWidget(m_search);
    heroLayout->addWidget(searchPanel, 0, Qt::AlignVCenter);
    root->addWidget(hero);
    connect(m_search, &QLineEdit::returnPressed, this, [this]
            { emit lookup(m_search->text()); m_search->clear(); });

    // ---- KPI row
    auto *stats = new QHBoxLayout;
    stats->setSpacing(12);
    stats->addWidget(statCard("Sales today", "cash", "blue", "#244fc4", m_sales, m_salesSub), 1);
    stats->addWidget(statCard("Orders today", "orders", "green", "#116944", m_orders, m_ordersSub), 1);
    stats->addWidget(statCard("Pending", "clock", "amber", "#865210", m_pending, m_pendingSub), 1);
    stats->addWidget(statCard("Low stock", "warn", "rose", "#9f344b", m_lowCount, m_lowSub), 1);
    root->addLayout(stats);

    auto *mid = new QHBoxLayout;
    mid->setSpacing(14);
    auto *cc = W::card(nullptr, "dashboardPanel");
    auto *cv = new QVBoxLayout(cc);
    cv->setContentsMargins(18, 14, 18, 12);
    cv->setSpacing(6);
    auto *chartHeader = new QHBoxLayout;
    m_revenueTitle = W::label("Hourly revenue", "h1");
    chartHeader->addWidget(m_revenueTitle, 1);
    auto *periods = new QButtonGroup(this);
    for (const auto &text : {QString("Today"), QString("Last 7 days")})
    {
        auto *period = W::button(text, "tab");
        period->setCheckable(true);
        period->setProperty("reportTab", true);
        periods->addButton(period, periods->buttons().size());
        chartHeader->addWidget(period);
    }
    periods->button(0)->setChecked(true);
    connect(periods, &QButtonGroup::idClicked, this, [this](int id)
            {
                m_showRecentDays = id == 1;
                refreshRevenue();
            });
    cv->addLayout(chartHeader);
    cv->addWidget(W::label("Paid orders · PKR", "muted"));
    m_chart = new RevenueChart;
    m_chart->setObjectName("dashboardRevenue");
    m_chart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cv->addWidget(m_chart, 1);
    mid->addWidget(cc, 3);

    auto *quick = W::card(nullptr, "dashboardPanel");
    auto *actions = new QVBoxLayout(quick);
    actions->setContentsMargins(16, 14, 16, 14);
    actions->setSpacing(8);
    actions->addWidget(W::label("Quick access", "h1"));
    const auto addAction = [this, actions](const QString &text, Pages::Id page)
    {
        auto *button = W::button(text, "soft");
        button->setObjectName("dashboardAction");
        button->setMinimumHeight(40);
        actions->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, page] { emit navigate(page); });
    };
    addAction("Review orders", Pages::Orders);
    addAction("Browse products", Pages::Products);
    addAction("Barcode scanner", Pages::Scanner);
    actions->addStretch();
    mid->addWidget(quick, 1);
    root->addLayout(mid, 2);

    // ---- recent orders
    auto *rc = W::card(nullptr, "dashboardPanel");
    auto *rv = new QVBoxLayout(rc);
    rv->setContentsMargins(18, 14, 18, 8);
    rv->setSpacing(6);
    auto *all = W::button("View all", "");
    all->setObjectName("link");
    all->setCursor(Qt::PointingHandCursor);
    connect(all, &QPushButton::clicked, this, [this]
            { emit navigate(Pages::Orders); });
    W::headerRow(rv, "", "Recent orders", all);
    m_recent = table({"ORDER", "TIME", "CUSTOMER", "ITEMS", "TOTAL", "STATUS", ""});
    m_recent->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_recent->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_recent->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    m_recent->setColumnWidth(6, 120);
    m_recent->setMinimumHeight(154);
    rv->addWidget(m_recent, 1);
    m_showing = W::label("", "muted");
    m_showing->setStyleSheet("font-size:12px;");
    rv->addWidget(m_showing);
    root->addWidget(rc, 1);
}

void DashboardPage::refresh()
{
    auto &st = AppState::instance();
    const auto &orders = st.orders();
    m_dashboardDate = QDate::currentDate();
    m_date->setText(QLocale().toString(m_dashboardDate, "dddd, d MMMM yyyy"));
    int pending = 0;
    int ordersToday = 0;
    int paidToday = 0;
    double salesToday = 0.0;
    for (const auto &o : orders)
    {
        if (o.time.date() == m_dashboardDate)
            ++ordersToday;
        if (o.status == "On Hold" || o.status == "In Progress")
            ++pending;
        if (o.status == "Paid" && o.paymentTime().date() == m_dashboardDate)
        {
            ++paidToday;
            salesToday += o.total;
        }
    }
    m_sales->setText(W::money(salesToday));
    m_salesSub->setText("Net revenue so far today");
    m_orders->setText(QString::number(ordersToday));
    m_ordersSub->setText("Average " + W::money(salesToday / qMax(1, paidToday)) + " per paid order");
    refreshRevenue();
    m_pending->setText(QString::number(pending));
    m_pendingSub->setText("On hold or in progress");
    const auto low = st.lowStock();
    m_lowCount->setText(QString::number(low.size()));
    m_lowSub->setText("At or below stock limit");

    const int m = qMin(5, orders.size());
    m_recent->setRowCount(m);
    for (int r = 0; r < m; ++r)
    {
        const auto &o = orders[r];
        m_recent->setRowHeight(r, 48);
        auto id = new QTableWidgetItem(o.id);
        id->setForeground(Theme::readable(QColor("#2554d8")));
        QFont f = id->font();
        f.setBold(true);
        id->setFont(f);
        m_recent->setItem(r, 0, id);
        m_recent->setItem(r, 1, new QTableWidgetItem(o.time.toString("HH:mm")));
        m_recent->setItem(r, 2, new QTableWidgetItem(o.customer));
        m_recent->setItem(r, 3, new QTableWidgetItem(QString::number(o.itemCount())));
        auto *am = new QTableWidgetItem(W::money(o.total));
        am->setFont(f);
        m_recent->setItem(r, 4, am);
        auto *sw = new QWidget;
        auto *swl = new QHBoxLayout(sw);
        swl->setContentsMargins(4, 0, 4, 0);
        swl->addWidget(W::badge(o.status, W::statusBadgeKind(o.status)), 0, Qt::AlignVCenter);
        swl->addStretch();
        m_recent->setCellWidget(r, 5, sw);
        auto *ab = W::button("View", "mini");
        auto *aw = new QWidget;
        auto *al = new QHBoxLayout(aw);
        al->setContentsMargins(0, 0, 4, 0);
        al->addWidget(ab);
        m_recent->setCellWidget(r, 6, aw);
        const OrderRecord copy = o;
        connect(ab, &QPushButton::clicked, this, [this, copy]
                { Dialogs::orderDetails(this, copy); });
    }
    m_showing->setText(m == 0 ? QStringLiteral("No saved orders yet")
                             : QString("Showing the latest %1 of %2 saved orders").arg(m).arg(orders.size()));
}

void DashboardPage::refreshRevenue()
{
    const QDate today = QDate::currentDate();
    const QDate start = m_showRecentDays ? today.addDays(-6) : today;
    const int bins = m_showRecentDays ? 7 : 24;
    QVector<double> totals(bins, 0.0);
    QStringList labels;
    for (int i = 0; i < bins; ++i)
        labels << (m_showRecentDays ? QLocale().toString(start.addDays(i), "dd MMM")
                                   : QTime(i, 0).toString("HH:mm"));

    bool hasPaidOrders = false;
    for (const auto &order : AppState::instance().orders())
    {
        const auto paid = order.paymentTime();
        if (order.status != "Paid" || !paid.isValid() || paid.date() < start || paid.date() > today)
            continue;
        const auto bin = m_showRecentDays ? start.daysTo(paid.date()) : paid.time().hour();
        totals[bin] += order.total;
        hasPaidOrders = true;
    }

    m_revenueTitle->setText(m_showRecentDays ? "Daily revenue" : "Hourly revenue");
    m_chart->setAccessibleName(m_revenueTitle->text());
    m_chart->setData(labels, totals, hasPaidOrders);
}
