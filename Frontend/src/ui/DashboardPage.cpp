#include "DashboardPage.h"
#include "AppState.h"
#include "Dialogs.h"
#include "Theme.h"
#include "Widgets.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QLocale>
#include <QScrollArea>
#include <QTableWidget>
#include <QVBoxLayout>

namespace
{
    // One KPI tile: icon, caption, big number, one supporting line.
    QFrame *statCard(const QString &caption, const QString &icon, const QString &top, const QString &bottom,
                     QLabel *&value, QLabel *&sub, bool alert = false)
    {
        auto *c = W::card(nullptr, alert ? "cardAlert" : "card");
        auto *h = new QHBoxLayout(c);
        h->setContentsMargins(16, 12, 16, 12);
        h->setSpacing(14);
        h->addWidget(W::iconTile(icon, Qt::white, top, bottom, 46));
        auto *v = new QVBoxLayout;
        v->setSpacing(0);
        v->addWidget(W::label(caption, "caption"));
        value = W::label("", "bigValue");
        if (alert)
            value->setStyleSheet("color:#c0271d;");
        v->addWidget(value);
        sub = W::label("", "muted");
        sub->setStyleSheet("font-size:12px;");
        v->addWidget(sub);
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
    inner->setMinimumHeight(900);
    scroll->setWidget(inner);
    outer->addWidget(scroll);
    auto *root = W::pageLayout(inner);

    // ---- KPI row
    auto *stats = new QHBoxLayout;
    stats->setSpacing(0);
    stats->addWidget(statCard("SALES TODAY", "cash", "#7fb0ff", "#2b5fe8", m_sales, m_salesSub));
    stats->addWidget(statCard("ORDERS", "orders", "#5be3a0", "#15a362", m_orders, m_ordersSub));
    stats->addWidget(statCard("PENDING", "clock", "#ffd479", "#f09a1a", m_pending, m_pendingSub));
    stats->addWidget(statCard("LOW STOCK", "warn", "#ff9a8f", "#d9392b", m_lowCount, m_lowSub, true));
    root->addLayout(stats);

    // ---- quick actions
    auto *qa = new QHBoxLayout;
    qa->setContentsMargins(8, 8, 8, 4);
    qa->setSpacing(10);
    // (quick buttons removed: the sidebar already opens these pages)
    qa->addStretch(1);
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Find an order or product…  (Ctrl+K)");
    m_search->setMinimumWidth(300);
    m_search->setMinimumHeight(40);
    m_search->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    qa->addWidget(m_search);
    root->addLayout(qa);
    connect(m_search, &QLineEdit::returnPressed, this, [this]
            { emit lookup(m_search->text()); m_search->clear(); });

    // ---- chart (full width)
    auto *mid = new QHBoxLayout;
    mid->setSpacing(0);
    auto *cc = W::card(this);
    auto *cv = new QVBoxLayout(cc);
    cv->setContentsMargins(18, 14, 18, 12);
    cv->setSpacing(6);
    auto *legend = new QLabel("Revenue from paid orders");
    legend->setObjectName("muted");
    W::headerRow(cv, "", "Hourly revenue", legend);
    m_chart = new RevenueChart;
    m_chart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_chart->setData({}, {}, {}); // populated from real paid orders in refresh()
    cv->addWidget(m_chart, 1);
    mid->addWidget(cc, 1);
    root->addLayout(mid, 5);

    // ---- recent orders
    auto *rc = W::card(this);
    auto *rv = new QVBoxLayout(rc);
    rv->setContentsMargins(18, 14, 18, 8);
    rv->setSpacing(6);
    auto *all = W::button("View all", "");
    all->setObjectName("link");
    all->setCursor(Qt::PointingHandCursor);
    connect(all, &QPushButton::clicked, this, [this]
            { emit navigate(2); });
    W::headerRow(rv, "orders", "Recent orders", all);
    m_recent = table({"ORDER", "TIME", "CUSTOMER", "ITEMS", "TOTAL", "STATUS", ""});
    m_recent->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_recent->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_recent->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    m_recent->setColumnWidth(6, 120);
    rv->addWidget(m_recent, 1);
    m_showing = W::label("", "muted");
    m_showing->setStyleSheet("font-size:12px;");
    rv->addWidget(m_showing);
    root->addWidget(rc, 6);
}

void DashboardPage::refresh()
{
    auto &st = AppState::instance();
    const auto &orders = st.orders();
    int pending = 0;
    for (const auto &o : orders)
        if (o.status != "Paid")
            ++pending;
    m_sales->setText(W::money(st.salesToday()));
    m_salesSub->setText("Revenue so far today");
    m_orders->setText(QString::number(st.ordersToday()));
    int paidToday = 0;
    for (const auto &o : orders)
        if (o.status == "Paid" && o.time.date() == QDate::currentDate()) ++paidToday;
    m_ordersSub->setText("Average " + W::money(st.salesToday() / qMax(1, paidToday)) + " per paid order");

    QStringList hours;
    QVector<double> earned(13, 0.0), target(13, 0.0);
    for (int hour = 8; hour <= 20; ++hour) hours << QString("%1").arg(hour, 2, 10, QChar('0'));
    for (const auto &o : orders)
        if (o.status == "Paid" && o.time.date() == QDate::currentDate() &&
            o.time.time().hour() >= 8 && o.time.time().hour() <= 20)
            earned[o.time.time().hour() - 8] += o.total;
    m_chart->setData(hours, earned, target);
    m_pending->setText(QString::number(pending));
    m_pendingSub->setText("On hold or in progress");
    const auto low = st.lowStock();
    m_lowCount->setText(QString::number(low.size()));
    m_lowSub->setText("Items at or below the low-stock limit");

    const int m = qMin(5, orders.size());
    m_recent->setRowCount(m);
    for (int r = 0; r < m; ++r)
    {
        const auto &o = orders[r];
        m_recent->setRowHeight(r, 48);
        auto id = new QTableWidgetItem(o.id);
        id->setForeground(Theme::readable(Theme::themed("#2554d8")));
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
    m_showing->setText(QString("Showing the latest %1 of %2 saved orders").arg(m).arg(orders.size()));
}
