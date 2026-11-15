#include "DashboardPage.h"
#include "AppState.h"
#include "Dialogs.h"
#include "Theme.h"
#include "Widgets.h"
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QScrollArea>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>

namespace
{
    QFrame *statCard(const QString &title, const QColor &dotColor, const QString &icon, QLabel *&value,
                     QWidget *bottom, bool alert = false)
    {
        auto *c = W::card(nullptr, alert ? "cardAlert" : "card");
        auto *v = new QVBoxLayout(c);
        v->setContentsMargins(14, 12, 14, 12);
        v->setSpacing(6);
        auto *h = new QHBoxLayout;
        h->setSpacing(6);
        h->addWidget(W::dot(dotColor));
        h->addWidget(W::label(title, "caption"));
        h->addStretch();
        auto *ic = new QLabel;
        ic->setFixedSize(30, 30);
        ic->setAlignment(Qt::AlignCenter);
        ic->setStyleSheet(QString("background:%1; border-radius:6px;").arg(alert ? "#ffdad6" : "#dce9ff"));
        ic->setPixmap(Theme::iconPixmap(icon, alert ? Theme::error : Theme::primary, 16));
        h->addWidget(ic);
        v->addLayout(h);
        value = W::label("", "bigValue");
        if (alert)
            value->setStyleSheet("color:#ba1a1a;");
        v->addWidget(value);
        v->addWidget(bottom);
        return c;
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
    // The dashboard is taller than a 800px window: put it in a scroll area.
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *inner = new QWidget;
    inner->setObjectName("page");
    inner->setMinimumHeight(1000);
    scroll->setWidget(inner);
    outer->addWidget(scroll);
    auto *root = new QVBoxLayout(inner);
    root->setContentsMargins(16, 14, 16, 12);
    root->setSpacing(12);

    // ---- stat cards
    auto *stats = new QHBoxLayout;
    stats->setSpacing(12);
    {
        auto *b = new QWidget;
        auto *h = new QHBoxLayout(b);
        h->setContentsMargins(0, 0, 0, 0);
        m_salesDelta = W::badge("↗ +14.2%", "green");
        h->addWidget(m_salesDelta);
        h->addStretch();
        h->addWidget(W::label("vs. Yest $3,364", "muted"));
        stats->addWidget(statCard("TOTAL SALES TODAY", QColor("#2563eb"), "cash", m_sales, b));
    }
    {
        auto *b = new QWidget;
        auto *h = new QHBoxLayout(b);
        h->setContentsMargins(0, 0, 0, 0);
        h->addWidget(W::label("Ticket Avg:", "muted"));
        m_ticket = W::badge("$25.96", "blue");
        h->addWidget(m_ticket);
        h->addStretch();
        h->addWidget(W::badge("+4.1%", "green"));
        stats->addWidget(statCard("TOTAL ORDERS TODAY", QColor("#16a34a"), "orders", m_orders, b));
    }
    {
        auto *b = new QWidget;
        auto *h = new QHBoxLayout(b);
        h->setContentsMargins(0, 0, 0, 0);
        auto *s = W::label("● 98.6% Synced");
        s->setStyleSheet("color:#006329; font-weight:600;");
        h->addWidget(s);
        h->addStretch();
        h->addWidget(W::label("12 Groups", "muted"));
        QLabel *dummy;
        auto *c = statCard("CATALOG INVENTORY", QColor("#737686"), "products", dummy, b);
        dummy->setText(QString("%1 <span style='font-size:12px;color:#434655;'>SKUS</span>")
                           .arg(QLocale(QLocale::English).toString(1240)));
        stats->addWidget(c);
    }
    {
        auto *b = new QWidget;
        auto *h = new QHBoxLayout(b);
        h->setContentsMargins(0, 0, 0, 0);
        h->addWidget(W::label("Action Needed", "muted"));
        h->addStretch();
        auto *wl = W::button("Watchlist", "");
        wl->setObjectName("link");
        wl->setIcon(Theme::icon("arrow-right", Theme::primary, 14));
        wl->setLayoutDirection(Qt::RightToLeft);
        wl->setCursor(Qt::PointingHandCursor);
        h->addWidget(wl);
        connect(wl, &QPushButton::clicked, this, [this]
                { m_low->setFocus(); m_low->selectRow(0); });
        stats->addWidget(statCard("LOW STOCK ALERT", QColor("#ba1a1a"), "warn", m_lowCount, b, true));
    }
    root->addLayout(stats);

    // ---- quick actions
    auto *qa = new QHBoxLayout;
    qa->setSpacing(10);
    auto quick = [&](const QString &text, const QString &key, const QString &icon, int page, bool primary)
    {
        auto *b = W::button(text + "   " + key, primary ? "primary" : "soft", icon);
        b->setMinimumHeight(38);
        connect(b, &QPushButton::clicked, this, [this, page]
                { emit navigate(page); });
        qa->addWidget(b);
    };
    quick("New Order", "F2", "cart", 1, true);
    quick("Scan Barcode", "F5", "barcode", 4, false);
    quick("View Products", "F4", "products", 3, false);
    quick("View Bills", "F6", "bills", 5, false);
    qa->addStretch(1);
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Lookup Ticket / SKU (Ctrl+K)...");
    m_search->setMinimumWidth(260);
    m_search->setMinimumHeight(38);
    m_search->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    qa->addWidget(m_search);
    auto *rf = W::button("", "soft", "refresh");
    rf->setProperty("kind", "icon");
    rf->setMinimumHeight(38);
    rf->setToolTip("Refresh");
    qa->addWidget(rf);
    root->addLayout(qa);
    connect(m_search, &QLineEdit::returnPressed, this, [this]
            { emit lookup(m_search->text()); m_search->clear(); });
    connect(rf, &QPushButton::clicked, this, [this]
            { refresh(); emit toast("Dashboard refreshed"); });

    // ---- chart + low stock
    auto *mid = new QHBoxLayout;
    mid->setSpacing(12);

    auto *cc = W::card(this);
    auto *cv = new QVBoxLayout(cc);
    cv->setContentsMargins(14, 12, 14, 12);
    cv->setSpacing(8);
    auto *tog = new QWidget;
    auto *th = new QHBoxLayout(tog);
    th->setContentsMargins(0, 0, 0, 0);
    th->setSpacing(4);
    m_todayBtn = W::button("Today", "tab");
    m_yestBtn = W::button("Yesterday", "tab");
    m_todayBtn->setCheckable(true);
    m_yestBtn->setCheckable(true);
    m_todayBtn->setChecked(true);
    th->addWidget(m_todayBtn);
    th->addWidget(m_yestBtn);
    W::headerRow(cv, "", "Hourly Register Velocity", tog);
    cv->addWidget(W::label("Revenue pace across standard operating hours (08:00 - 20:00)", "muted"));

    auto *kp = new QHBoxLayout;
    kp->setSpacing(8);
    auto kpi = [&](const QString &cap, QLabel *&val)
    {
        auto *f = W::card(cc, "subCard");
        auto *v = new QVBoxLayout(f);
        v->setContentsMargins(10, 6, 10, 6);
        v->setSpacing(1);
        v->addWidget(W::label(cap, "caption"));
        val = W::label("");
        val->setTextFormat(Qt::RichText);
        v->addWidget(val);
        kp->addWidget(f);
    };
    kpi("PEAK HOUR WINDOW", m_peak);
    kpi("SHIFT QUOTA PACE", m_quota);
    kpi("SCAN INGEST RATE", m_ingest);
    cv->addLayout(kp);

    m_chart = new RevenueChart;
    m_chart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cv->addWidget(m_chart, 1);

    auto *tr = new QHBoxLayout;
    tr->setSpacing(8);
    tr->addWidget(W::label("TENDERS:", "caption"));
    auto tender = [&](const QString &icon, const QString &t)
    {
        auto *f = W::card(cc, "subCard");
        auto *h = new QHBoxLayout(f);
        h->setContentsMargins(8, 4, 8, 4);
        h->setSpacing(6);
        h->addWidget(W::iconLabel(icon, Theme::tertiary, 16));
        h->addWidget(W::label(t));
        tr->addWidget(f);
    };
    tender("cash", "Cash 35%");
    tender("card", "Card 55%");
    tender("tag", "Digital 10%");
    tr->addStretch();
    auto *bal = W::badge("✓ Drawer Balanced (+$0.00)", "green");
    tr->addWidget(bal);
    cv->addLayout(tr);
    mid->addWidget(cc, 6);

    auto *lc = W::card(this);
    auto *lv = new QVBoxLayout(lc);
    lv->setContentsMargins(14, 12, 14, 12);
    lv->setSpacing(8);
    auto *lowBadge = W::badge("", "red");
    lowBadge->setObjectName("lowBadge");
    auto *lh = new QHBoxLayout;
    auto *ic = new QLabel;
    ic->setFixedSize(30, 30);
    ic->setAlignment(Qt::AlignCenter);
    ic->setStyleSheet("background:#ffdad6; border-radius:6px;");
    ic->setPixmap(Theme::iconPixmap("orders", Theme::error, 16));
    lh->addWidget(ic);
    auto *lt = new QVBoxLayout;
    lt->setSpacing(0);
    lt->addWidget(W::label("Low Stock Watchlist", "h1"));
    lt->addWidget(W::label("Critical replenishment threshold reached", "muted"));
    lh->addLayout(lt);
    lh->addStretch();
    lh->addWidget(lowBadge);
    lv->addLayout(lh);
    m_low = new QTableWidget(0, 4);
    m_low->setHorizontalHeaderLabels({"ITEM SKU & NAME", "STOCK", "LIMIT", "ACTION"});
    m_low->verticalHeader()->hide();
    m_low->setShowGrid(false);
    m_low->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_low->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_low->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_low->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_low->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_low->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_low->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_low->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_low->setColumnWidth(1, 64);
    m_low->setColumnWidth(2, 56);
    m_low->setColumnWidth(3, 90);
    lv->addWidget(m_low, 1);
    auto *lf = new QHBoxLayout;
    lf->addWidget(W::label("PO auto-draft available", "muted"));
    lf->addStretch();
    m_genPO = W::button("Generate POs", "primary", "arrow-right");
    lf->addWidget(m_genPO);
    lv->addLayout(lf);
    mid->addWidget(lc, 4);
    root->addLayout(mid, 5);
    lowBadge->setProperty("lowBadge", true);
    connect(m_genPO, &QPushButton::clicked, this, [this]
            { AppState::instance().reorderAllLow(); emit toast("Purchase orders drafted for all low-stock SKUs"); });

    // ---- recent transactions
    auto *rc = W::card(this);
    auto *rv = new QVBoxLayout(rc);
    rv->setContentsMargins(14, 12, 14, 8);
    rv->setSpacing(6);
    auto *rh = new QHBoxLayout;
    auto *ri = new QLabel;
    ri->setFixedSize(30, 30);
    ri->setAlignment(Qt::AlignCenter);
    ri->setStyleSheet("background:#dce9ff; border-radius:6px;");
    ri->setPixmap(Theme::iconPixmap("orders", Theme::primary, 16));
    rh->addWidget(ri);
    auto *rt = new QVBoxLayout;
    rt->setSpacing(0);
    rt->addWidget(W::label("Recent Shift Transactions", "h1"));
    rt->addWidget(W::label("Live feed of orders processed across Register #01 - #04", "muted"));
    rh->addLayout(rt);
    rh->addStretch();
    auto *exp = W::button("Export CSV", "soft", "export");
    rh->addWidget(exp);
    rv->addLayout(rh);
    m_recent = new QTableWidget(0, 7);
    m_recent->setHorizontalHeaderLabels({"ORDER ID", "TIME", "CUSTOMER", "ITEMS", "AMOUNT", "STATUS", "ACTION"});
    m_recent->verticalHeader()->hide();
    m_recent->setShowGrid(false);
    m_recent->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recent->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recent->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_recent->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_recent->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    m_recent->setColumnWidth(6, 130);
    m_recent->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_recent->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    rv->addWidget(m_recent, 1);
    m_showing = W::label("", "muted");
    rv->addWidget(m_showing);
    root->addWidget(rc, 6);

    connect(exp, &QPushButton::clicked, this, [this]
            {
        const QString path = QFileDialog::getSaveFileName(this, "Export CSV", "recent-orders.csv", "CSV (*.csv)");
        if (path.isEmpty()) return;
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) { emit toast("Could not write file"); return; }
        QTextStream out(&f);
        out << "Order ID,Time,Customer,Items,Amount,Payment,Status\n";
        for (const auto &o : AppState::instance().orders())
            out << o.id << ',' << o.time.toString("yyyy-MM-dd HH:mm:ss") << ",\"" << o.customer << "\","
                << o.itemCount() << ',' << QString::number(o.total, 'f', 2) << ",\"" << o.payment << "\"," << o.status << '\n';
        emit toast("Exported " + QString::number(AppState::instance().orders().size()) + " orders"); });
    connect(m_todayBtn, &QPushButton::clicked, this, [this]
            { setDay(true); });
    connect(m_yestBtn, &QPushButton::clicked, this, [this]
            { setDay(false); });
    setDay(true);
}

void DashboardPage::setDay(bool today)
{
    m_todayBtn->setChecked(today);
    m_yestBtn->setChecked(!today);
    const QStringList labels = {"08h", "09h", "10h", "11h", "12h", "13h", "14h", "15h", "16h", "17h", "18h", "19h", "20h"};
    if (today)
    {
        m_chart->setData(labels, {120, 190, 330, 480, 610, 641, 560, 470, 380, 290, 210, 150, 90},
                         {250, 260, 280, 320, 360, 400, 420, 420, 400, 360, 320, 290, 260});
        m_peak->setText("<b style='font-size:14px'>12:00 - 13:00</b><br><span style='color:#006329;font-size:11px'>$640.80 • 24 txns</span>");
        m_quota->setText("<b style='font-size:14px'>89.4% Completed</b><br><span style='color:#004ac6;font-size:11px'>Target $4,300.00</span>");
        m_ingest->setText("<b style='font-size:14px'>3.2 sec/item</b><br><span style='color:#006329;font-size:11px'>Optimal Ingest</span>");
    }
    else
    {
        m_chart->setData(labels, {100, 170, 290, 420, 540, 580, 520, 430, 350, 270, 190, 130, 80},
                         {250, 260, 280, 320, 360, 400, 420, 420, 400, 360, 320, 290, 260});
        m_peak->setText("<b style='font-size:14px'>13:00 - 14:00</b><br><span style='color:#006329;font-size:11px'>$580.20 • 21 txns</span>");
        m_quota->setText("<b style='font-size:14px'>78.2% Completed</b><br><span style='color:#004ac6;font-size:11px'>Target $4,300.00</span>");
        m_ingest->setText("<b style='font-size:14px'>3.6 sec/item</b><br><span style='color:#5a4300;font-size:11px'>Acceptable</span>");
    }
}

void DashboardPage::refresh()
{
    auto &st = AppState::instance();
    m_sales->setText(W::money(st.salesToday()));
    m_orders->setText(QString("%1 <span style='font-size:12px;color:#434655;'>ORDERS</span>").arg(st.ordersToday()));
    m_ticket->setText(W::money(st.salesToday() / qMax(1, st.ordersToday())));
    const auto low = st.lowStock();
    m_lowCount->setText(QString("%1 <span style='font-size:12px;'>ITEMS URGENT</span>").arg(low.size()));
    if (auto *b = findChild<QLabel *>("lowBadge"))
        b->setText(QString("● %1 SKUs Urgent").arg(low.size()));
    m_genPO->setText(QString("Generate POs (%1)").arg(low.size()));

    // low-stock table (top 5)
    const int n = qMin(5, low.size());
    m_low->setRowCount(n);
    for (int r = 0; r < n; ++r)
    {
        const auto &p = low[r];
        m_low->setRowHeight(r, 44);
        auto *name = new QWidget;
        auto *nv = new QVBoxLayout(name);
        nv->setContentsMargins(4, 2, 4, 2);
        nv->setSpacing(0);
        auto *a = W::label(p.name);
        a->setStyleSheet("font-weight:600;");
        auto *b = W::label(p.sku + " • " + p.ean);
        b->setStyleSheet("color:#737686; font-size:11px;");
        nv->addWidget(a);
        nv->addWidget(b);
        m_low->setCellWidget(r, 0, name);
        auto *sk = new QTableWidgetItem(QString::number(p.stock));
        sk->setTextAlignment(Qt::AlignCenter);
        sk->setForeground(QColor(p.stock <= 5 ? "#ba1a1a" : "#003ea8"));
        QFont f = sk->font();
        f.setBold(true);
        sk->setFont(f);
        m_low->setItem(r, 1, sk);
        auto *lim = new QTableWidgetItem(QString::number(p.reorderLimit));
        lim->setTextAlignment(Qt::AlignCenter);
        lim->setForeground(QColor("#737686"));
        m_low->setItem(r, 2, lim);
        auto *btn = W::button(st.isReordered(p.sku) ? "Ordered ✓" : "Reorder", "mini");
        btn->setEnabled(!st.isReordered(p.sku));
        auto *wrap = new QWidget;
        auto *wl = new QHBoxLayout(wrap);
        wl->setContentsMargins(0, 0, 6, 0);
        wl->addWidget(btn);
        m_low->setCellWidget(r, 3, wrap);
        const QString sku = p.sku, nm = p.name;
        connect(btn, &QPushButton::clicked, this, [this, sku, nm]
                { AppState::instance().reorder(sku); emit toast("Purchase order drafted for " + nm); });
    }

    // recent transactions (latest 5)
    const auto &orders = st.orders();
    const int m = qMin(5, orders.size());
    m_recent->setRowCount(m);
    for (int r = 0; r < m; ++r)
    {
        const auto &o = orders[r];
        m_recent->setRowHeight(r, 48);
        auto id = new QTableWidgetItem(o.id);
        id->setForeground(QColor("#004ac6"));
        QFont f = id->font();
        f.setBold(true);
        id->setFont(f);
        m_recent->setItem(r, 0, id);
        m_recent->setItem(r, 1, new QTableWidgetItem(o.time.toString("HH:mm:ss")));
        m_recent->setItem(r, 2, new QTableWidgetItem(o.customer + (o.tier == "VIP" ? "  [VIP]" : "")));
        auto *it = new QTableWidgetItem(QString::number(o.itemCount()));
        m_recent->setItem(r, 3, it);
        auto *am = new QTableWidgetItem(W::money(o.total));
        am->setFont(f);
        m_recent->setItem(r, 4, am);
        auto *sw = new QWidget;
        auto *swl = new QHBoxLayout(sw);
        swl->setContentsMargins(4, 0, 4, 0);
        swl->addWidget(W::badge("● " + o.status, W::statusBadgeKind(o.status)), 0, Qt::AlignVCenter);
        swl->addStretch();
        m_recent->setCellWidget(r, 5, sw);
        const bool hold = o.status == "On Hold";
        auto *ab = W::button(hold ? "Resume (F8)" : "View Details", hold ? "primary" : "mini");
        auto *aw = new QWidget;
        auto *al = new QHBoxLayout(aw);
        al->setContentsMargins(0, 0, 6, 0);
        al->addWidget(ab);
        m_recent->setCellWidget(r, 6, aw);
        const OrderRecord copy = o;
        connect(ab, &QPushButton::clicked, this, [this, copy, hold]
                {
            if (hold) emit resumeOrder(copy.id);
            else Dialogs::orderDetails(this, copy); });
    }
    m_showing->setText(QString("Showing %1 of %2 orders logged today").arg(m).arg(st.ordersToday()));
}
