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
    QFrame *statCard(const QString &caption, const QString &icon, const QColor &accent,
                     QLabel *&value, QLabel *&sub, bool alert = false)
    {
        auto *c = W::card(nullptr, alert ? "cardAlert" : "card");
        auto *h = new QHBoxLayout(c);
        h->setContentsMargins(16, 12, 16, 12);
        h->setSpacing(14);
        h->addWidget(W::iconTile(icon, accent, 44));
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
    stats->addWidget(statCard("SALES TODAY", "cash", QColor("#2554d8"), m_sales, m_salesSub));
    stats->addWidget(statCard("ORDERS", "orders", QColor("#15915a"), m_orders, m_ordersSub));
    stats->addWidget(statCard("PENDING", "clock", QColor("#d98a0b"), m_pending, m_pendingSub));
    stats->addWidget(statCard("LOW STOCK", "warn", QColor("#d9392b"), m_lowCount, m_lowSub, true));
    root->addLayout(stats);

    // ---- quick actions
    auto *qa = new QHBoxLayout;
    qa->setContentsMargins(8, 8, 8, 4);
    qa->setSpacing(10);
    auto quick = [&](const QString &text, const QString &icon, int page, bool primary)
    {
        auto *b = W::button(text, primary ? "primary" : "soft", icon);
        b->setMinimumHeight(40);
        connect(b, &QPushButton::clicked, this, [this, page]
                { emit navigate(page); });
        qa->addWidget(b);
    };
    quick("New Order", "cart", 1, true);
    quick("Scan Barcode", "barcode", 4, false);
    quick("Products", "products", 3, false);
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

    // ---- chart + low stock
    auto *mid = new QHBoxLayout;
    mid->setSpacing(0);
    auto *cc = W::card(this);
    auto *cv = new QVBoxLayout(cc);
    cv->setContentsMargins(18, 14, 18, 12);
    cv->setSpacing(6);
    auto *legend = new QLabel("<span style='color:#2554d8'>■</span> Revenue &nbsp;&nbsp; <span style='color:#e08a2c'>╌</span> Target");
    legend->setObjectName("muted");
    W::headerRow(cv, "", "Hourly revenue", legend);
    m_chart = new RevenueChart;
    m_chart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_chart->setData({"08", "09", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20"},
                     {120, 190, 330, 480, 610, 641, 560, 470, 380, 290, 210, 150, 90},
                     {250, 260, 280, 320, 360, 400, 420, 420, 400, 360, 320, 290, 260});
    cv->addWidget(m_chart, 1);
    mid->addWidget(cc, 6);

    auto *lc = W::card(this);
    auto *lv = new QVBoxLayout(lc);
    lv->setContentsMargins(18, 14, 18, 12);
    lv->setSpacing(6);
    m_lowBadge = W::badge("", "red");
    W::headerRow(lv, "warn", "Low stock", m_lowBadge);
    m_low = table({"PRODUCT", "STOCK", "LIMIT", ""});
    m_low->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c : {1, 2, 3})
        m_low->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
    m_low->setColumnWidth(1, 66);
    m_low->setColumnWidth(2, 52);
    m_low->setColumnWidth(3, 92);
    W::alignHeaders(m_low, {1, 2}, Qt::AlignHCenter);
    lv->addWidget(m_low, 1);
    auto *lf = new QHBoxLayout;
    lf->addStretch();
    m_genPO = W::button("Reorder all", "primary", "arrow-right");
    lf->addWidget(m_genPO);
    lv->addLayout(lf);
    mid->addWidget(lc, 4);
    root->addLayout(mid, 5);
    connect(m_genPO, &QPushButton::clicked, this, [this]
            { AppState::instance().reorderAllLow(); emit toast("Purchase orders drafted for all low-stock items"); });

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
    m_ordersSub->setText("Average " + W::money(st.salesToday() / qMax(1, st.ordersToday())) + " per order");
    m_pending->setText(QString::number(pending));
    m_pendingSub->setText("On hold or in progress");
    const auto low = st.lowStock();
    m_lowCount->setText(QString::number(low.size()));
    m_lowSub->setText("Items below reorder limit");
    W::setBadge(m_lowBadge, QString("%1 items").arg(low.size()), "red");
    m_genPO->setText(QString("Reorder all (%1)").arg(low.size()));
    m_genPO->setEnabled(!low.isEmpty());

    const int n = qMin(5, low.size());
    m_low->setRowCount(n);
    for (int r = 0; r < n; ++r)
    {
        const auto &p = low[r];
        m_low->setRowHeight(r, 50);
        auto *name = new QWidget;
        auto *nv = new QVBoxLayout(name);
        nv->setContentsMargins(4, 4, 4, 4);
        nv->setSpacing(0);
        auto *a = W::label(p.name);
        a->setStyleSheet("font-weight:600;");
        auto *b = W::label(p.sku);
        b->setStyleSheet("color:#7b869c; font-size:11px;");
        nv->addWidget(a);
        nv->addWidget(b);
        m_low->setCellWidget(r, 0, name);
        auto *sk = new QTableWidgetItem(QString::number(p.stock));
        sk->setTextAlignment(Qt::AlignCenter);
        sk->setForeground(QColor(p.stock <= 5 ? "#c0271d" : "#b36b00"));
        QFont f = sk->font();
        f.setBold(true);
        sk->setFont(f);
        m_low->setItem(r, 1, sk);
        auto *lim = new QTableWidgetItem(QString::number(p.reorderLimit));
        lim->setTextAlignment(Qt::AlignCenter);
        lim->setForeground(QColor("#7b869c"));
        m_low->setItem(r, 2, lim);
        const bool done = st.isReordered(p.sku);
        auto *btn = W::button(done ? "Ordered ✓" : "Reorder", "mini");
        btn->setEnabled(!done);
        auto *wrap = new QWidget;
        auto *wl = new QHBoxLayout(wrap);
        wl->setContentsMargins(0, 0, 4, 0);
        wl->addWidget(btn);
        m_low->setCellWidget(r, 3, wrap);
        const QString sku = p.sku, nm = p.name;
        connect(btn, &QPushButton::clicked, this, [this, sku, nm]
                { AppState::instance().reorder(sku); emit toast("Purchase order drafted for " + nm); });
    }

    const int m = qMin(5, orders.size());
    m_recent->setRowCount(m);
    for (int r = 0; r < m; ++r)
    {
        const auto &o = orders[r];
        m_recent->setRowHeight(r, 48);
        auto id = new QTableWidgetItem(o.id);
        id->setForeground(QColor("#2554d8"));
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
        const bool hold = o.status == "On Hold";
        auto *ab = W::button(hold ? "Resume" : "View", hold ? "primary" : "mini");
        if (hold)
            ab->setStyleSheet("padding:4px 12px;");
        auto *aw = new QWidget;
        auto *al = new QHBoxLayout(aw);
        al->setContentsMargins(0, 0, 4, 0);
        al->addWidget(ab);
        m_recent->setCellWidget(r, 6, aw);
        const OrderRecord copy = o;
        connect(ab, &QPushButton::clicked, this, [this, copy, hold]
                {
            if (hold) emit resumeOrder(copy.id);
            else Dialogs::orderDetails(this, copy); });
    }
    m_showing->setText(QString("Showing the latest %1 of %2 orders today").arg(m).arg(st.ordersToday()));
}
