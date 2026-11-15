#include "OtherPages.h"
#include "Dialogs.h"
#include "Theme.h"
#include "Widgets.h"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QListWidget>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>

namespace
{
    QTableWidget *makeTable(const QStringList &headers)
    {
        auto *t = new QTableWidget(0, headers.size());
        t->setHorizontalHeaderLabels(headers);
        t->verticalHeader()->hide();
        t->setShowGrid(false);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setSelectionBehavior(QAbstractItemView::SelectRows);
        t->setSelectionMode(QAbstractItemView::SingleSelection);
        t->setFocusPolicy(Qt::NoFocus);
        t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        t->horizontalHeader()->setHighlightSections(false);
        t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        t->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        return t;
    }
    QTableWidgetItem *item(const QString &s, Qt::Alignment a = Qt::AlignLeft | Qt::AlignVCenter,
                           const QString &color = "", bool bold = false)
    {
        auto *i = new QTableWidgetItem(s);
        i->setTextAlignment(a);
        if (!color.isEmpty())
            i->setForeground(QColor(color));
        if (bold)
        {
            QFont f = i->font();
            f.setBold(true);
            i->setFont(f);
        }
        return i;
    }
    QWidget *wrapW(QWidget *w, Qt::Alignment a = Qt::AlignLeft)
    {
        auto *c = new QWidget;
        auto *h = new QHBoxLayout(c);
        h->setContentsMargins(6, 0, 6, 0);
        h->setAlignment(a | Qt::AlignVCenter);
        h->addWidget(w);
        return c;
    }
    QWidget *pageShell(QWidget *page, QVBoxLayout *&body)
    {
        page->setObjectName("page");
        auto *root = new QVBoxLayout(page);
        root->setContentsMargins(16, 14, 16, 12);
        root->setSpacing(12);
        body = root;
        return page;
    }
    QFrame *kpiCard(const QString &cap, const QString &icon, QLabel *&value)
    {
        auto *c = W::card(nullptr);
        auto *h = new QHBoxLayout(c);
        h->setContentsMargins(14, 10, 14, 10);
        auto *v = new QVBoxLayout;
        v->setSpacing(0);
        v->addWidget(W::label(cap, "caption"));
        value = W::label("", "bigValue");
        value->setStyleSheet("font-size:20px; font-weight:700;");
        v->addWidget(value);
        h->addLayout(v);
        h->addStretch();
        auto *ic = new QLabel;
        ic->setFixedSize(32, 32);
        ic->setAlignment(Qt::AlignCenter);
        ic->setStyleSheet("background:#dce9ff; border-radius:6px;");
        ic->setPixmap(Theme::iconPixmap(icon, Theme::primary, 16));
        h->addWidget(ic);
        return c;
    }
    bool writeCsv(QWidget *parent, const QString &suggested, const QStringList &rows)
    {
        const QString path = QFileDialog::getSaveFileName(parent, "Export CSV", suggested, "CSV (*.csv)");
        if (path.isEmpty())
            return false;
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;
        QTextStream out(&f);
        for (const auto &r : rows)
            out << r << '\n';
        return true;
    }
}

// =================================================================== OrdersPage
OrdersPage::OrdersPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root;
    pageShell(this, root);

    auto *kp = new QHBoxLayout;
    kp->setSpacing(12);
    kp->addWidget(kpiCard("TODAY'S NET VOLUME", "cash", m_vol));
    kp->addWidget(kpiCard("ORDERS PROCESSED", "orders", m_count));
    QLabel *regs;
    kp->addWidget(kpiCard("ACTIVE REGISTERS", "cart", regs));
    regs->setText("4 / 4");
    kp->addWidget(kpiCard("PENDING / HOLD", "clock", m_hold));
    root->addLayout(kp);

    auto *card = W::card(this);
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(14, 12, 14, 8);
    v->setSpacing(10);

    auto *tools = new QHBoxLayout;
    tools->setSpacing(8);
    m_orderSearch = new QLineEdit;
    m_orderSearch->setPlaceholderText("Search Order ID (#ORD-892...)");
    m_orderSearch->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    m_custSearch = new QLineEdit;
    m_custSearch->setPlaceholderText("Customer name / contact phone");
    m_custSearch->addAction(Theme::icon("user", Theme::outline, 16), QLineEdit::LeadingPosition);
    tools->addWidget(m_orderSearch, 2);
    tools->addWidget(m_custSearch, 2);
    tools->addStretch(1);
    auto *exp = W::button("Export CSV", "soft", "export");
    tools->addWidget(exp);
    v->addLayout(tools);

    auto *tabs = new QHBoxLayout;
    tabs->setSpacing(6);
    for (const QString &name : {"All", "Paid", "In Progress", "On Hold"})
    {
        auto *b = W::button(name, "tab");
        b->setCheckable(true);
        b->setChecked(name == "All");
        tabs->addWidget(b);
        m_tabs << b;
        connect(b, &QPushButton::clicked, this, [this, name]
                {
            m_filter = name;
            for (auto *t : m_tabs) t->setChecked(t->text() == name);
            rebuild(); });
    }
    tabs->addStretch();
    v->addLayout(tabs);

    m_table = makeTable({"ORDER ID", "DATE & TIME", "CUSTOMER", "ITEMS SUMMARY", "PAYMENT", "AMOUNT", "STATUS", "ACTIONS"});
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    for (int c : {0, 1, 4, 5})
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    for (int c : {2, 6, 7})
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
    m_table->setColumnWidth(2, 175);
    m_table->setColumnWidth(6, 104);
    m_table->setColumnWidth(7, 150);
    v->addWidget(m_table, 1);
    m_showing = W::label("", "muted");
    v->addWidget(m_showing);
    root->addWidget(card, 1);

    connect(m_orderSearch, &QLineEdit::textChanged, this, [this]
            { rebuild(); });
    connect(m_custSearch, &QLineEdit::textChanged, this, [this]
            { rebuild(); });
    connect(&AppState::instance(), &AppState::changed, this, &OrdersPage::rebuild);
    connect(exp, &QPushButton::clicked, this, [this]
            {
        QStringList rows{"Order ID,Time,Customer,Contact,Items,Payment,Amount,Status"};
        for (const auto &o : AppState::instance().orders())
            rows << QString("%1,%2,\"%3\",\"%4\",%5,\"%6\",%7,%8").arg(o.id, o.time.toString("yyyy-MM-dd HH:mm:ss"), o.customer, o.contact)
                        .arg(o.itemCount()).arg(o.payment).arg(o.total, 0, 'f', 2).arg(o.status);
        if (writeCsv(this, "orders.csv", rows)) emit toast("Orders exported"); });
    rebuild();
}

void OrdersPage::setSearch(const QString &text)
{
    m_custSearch->clear();
    m_orderSearch->setText(text);
}

void OrdersPage::rebuild()
{
    const auto &all = AppState::instance().orders();
    QString q = m_orderSearch->text().trimmed();
    q.remove('#');
    const QString cq = m_custSearch->text().trimmed();
    QVector<OrderRecord> rows;
    int hold = 0;
    for (const auto &o : all)
    {
        if (o.status == "On Hold" || o.status == "In Progress")
            ++hold;
        if (m_filter != "All" && o.status != m_filter)
            continue;
        if (!q.isEmpty() && !o.id.contains(q, Qt::CaseInsensitive))
            continue;
        if (!cq.isEmpty() && !o.customer.contains(cq, Qt::CaseInsensitive) && !o.contact.contains(cq))
            continue;
        rows << o;
    }
    auto &st = AppState::instance();
    m_vol->setText(W::money(st.salesToday()));
    m_count->setText(QString::number(st.ordersToday()));
    m_hold->setText(QString::number(hold));
    m_table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r)
    {
        const auto &o = rows[r];
        m_table->setRowHeight(r, 50);
        m_table->setItem(r, 0, item("#" + o.id, Qt::AlignLeft | Qt::AlignVCenter, "#004ac6", true));
        m_table->setItem(r, 1, item(o.time.toString("yyyy-MM-dd HH:mm") + "\n" + o.shift));
        {
            auto *cw = new QWidget;
            auto *cl = new QVBoxLayout(cw);
            cl->setContentsMargins(8, 4, 8, 4);
            cl->setSpacing(0);
            auto *n = W::label(o.customer + (o.tier == "VIP" ? "  [VIP]" : ""));
            n->setStyleSheet("font-weight:700;");
            auto *c = W::label(o.contact);
            c->setStyleSheet("color:#737686; font-size:11.5px;");
            cl->addWidget(n);
            cl->addWidget(c);
            m_table->setCellWidget(r, 2, cw);
        }
        m_table->setItem(r, 3, item(o.summary()));
        m_table->setItem(r, 4, item(o.payment));
        m_table->setItem(r, 5, item(W::money(o.total), Qt::AlignRight | Qt::AlignVCenter, "", true));
        m_table->setCellWidget(r, 6, wrapW(W::badge(o.status, W::statusBadgeKind(o.status))));
        auto *w = new QWidget;
        auto *h = new QHBoxLayout(w);
        h->setContentsMargins(4, 0, 4, 0);
        h->setSpacing(4);
        const OrderRecord copy = o;
        if (o.status == "On Hold")
        {
            auto *res = W::button("Resume", "primary");
            res->setMinimumHeight(26);
            res->setMinimumWidth(72);
            res->setStyleSheet("padding:3px 8px;");
            h->addWidget(res);
            connect(res, &QPushButton::clicked, this, [this, copy]
                    { emit resumeOrder(copy.id); });
        }
        else
        {
            auto *view = W::button("", "flat", "eye");
            view->setToolTip("View details");
            h->addWidget(view);
            connect(view, &QPushButton::clicked, this, [this, copy]
                    { Dialogs::orderDetails(this, copy); });
        }
        auto *rc = W::button("", "flat", "bills");
        rc->setToolTip("Receipt (F6)");
        auto *pr = W::button("", "flat", "print");
        pr->setToolTip("Print (Ctrl+P) - demo");
        h->addWidget(rc);
        h->addWidget(pr);
        h->addStretch();
        connect(rc, &QPushButton::clicked, this, [this, copy]
                { Dialogs::receipt(this, copy); });
        connect(pr, &QPushButton::clicked, this, [this, copy]
                { emit toast("Receipt for " + copy.id + " sent to printer (demo)"); });
        m_table->setCellWidget(r, 7, w);
    }
    m_showing->setText(QString("Showing %1 of %2 recent orders").arg(rows.size()).arg(all.size()));
}

// =================================================================== ProductsPage
ProductsPage::ProductsPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root;
    pageShell(this, root);
    auto *card = W::card(this);
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(14, 12, 14, 8);
    v->setSpacing(10);
    auto *h = new QHBoxLayout;
    h->addWidget(W::iconLabel("products", Theme::primary, 20));
    h->addWidget(W::label("Product Catalog", "h1"));
    m_count = W::badge("", "blue");
    h->addWidget(m_count);
    h->addStretch();
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Search name, SKU or EAN...");
    m_search->setMinimumWidth(280);
    m_search->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    h->addWidget(m_search);
    v->addLayout(h);
    m_table = makeTable({"SKU", "EAN-8", "PRODUCT", "CATEGORY", "LOCATION", "PRICE", "STOCK", "STATUS", ""});
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    for (int c : {0, 1, 3, 4, 5, 6, 7})
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Fixed);
    m_table->setColumnWidth(8, 130);
    v->addWidget(m_table, 1);
    root->addWidget(card, 1);
    connect(m_search, &QLineEdit::textChanged, this, [this]
            { rebuild(); });
    connect(&AppState::instance(), &AppState::changed, this, &ProductsPage::rebuild);
    rebuild();
}

void ProductsPage::setSearch(const QString &text) { m_search->setText(text); }

void ProductsPage::rebuild()
{
    const QString q = m_search->text().trimmed();
    QVector<Product> rows;
    for (const auto &p : AppState::instance().products())
        if (q.isEmpty() || p.name.contains(q, Qt::CaseInsensitive) || p.sku.contains(q, Qt::CaseInsensitive) || p.ean.contains(q))
            rows << p;
    m_count->setText(QString("%1 SKUs").arg(rows.size()));
    m_table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r)
    {
        const auto &p = rows[r];
        m_table->setRowHeight(r, 44);
        m_table->setItem(r, 0, item(p.sku, Qt::AlignLeft | Qt::AlignVCenter, "#004ac6", true));
        m_table->setItem(r, 1, item(p.ean));
        m_table->setItem(r, 2, item(p.name, Qt::AlignLeft | Qt::AlignVCenter, "", true));
        m_table->setItem(r, 3, item(p.category));
        m_table->setItem(r, 4, item(p.location));
        m_table->setItem(r, 5, item(W::money(p.price), Qt::AlignRight | Qt::AlignVCenter, "", true));
        const bool low = p.stock < p.reorderLimit;
        m_table->setItem(r, 6, item(QString::number(p.stock), Qt::AlignRight | Qt::AlignVCenter, low ? "#ba1a1a" : "", low));
        m_table->setCellWidget(r, 7, wrapW(W::badge(p.stock == 0 ? "Out" : low ? "Low" : "OK", p.stock == 0 ? "red" : low ? "amber" : "green")));
        auto *b = W::button("Add to Order", "mini");
        b->setEnabled(p.stock > 0);
        m_table->setCellWidget(r, 8, wrapW(b));
        const QString ean = p.ean;
        connect(b, &QPushButton::clicked, this, [this, ean]
                { emit addToOrder(ean); });
    }
}

// =================================================================== BillsPage
BillsPage::BillsPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root;
    pageShell(this, root);
    auto *card = W::card(this);
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(14, 12, 14, 8);
    v->setSpacing(10);
    auto *h = new QHBoxLayout;
    h->addWidget(W::iconLabel("bills", Theme::primary, 20));
    h->addWidget(W::label("Bills", "h1"));
    h->addStretch();
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Search bill or order ID...");
    m_search->setMinimumWidth(280);
    m_search->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    h->addWidget(m_search);
    v->addLayout(h);
    m_table = makeTable({"BILL NO.", "ORDER", "DATE & TIME", "CUSTOMER", "PAYMENT", "TOTAL", "ACTIONS"});
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    m_table->setColumnWidth(6, 160);
    v->addWidget(m_table, 1);
    root->addWidget(card, 1);
    connect(m_search, &QLineEdit::textChanged, this, [this]
            { rebuild(); });
    connect(&AppState::instance(), &AppState::changed, this, &BillsPage::rebuild);
    rebuild();
}

void BillsPage::rebuild()
{
    const QString q = m_search->text().trimmed();
    QVector<OrderRecord> rows;
    for (const auto &o : AppState::instance().orders())
        if (o.status == "Paid" && (q.isEmpty() || o.id.contains(q, Qt::CaseInsensitive) || o.customer.contains(q, Qt::CaseInsensitive)))
            rows << o;
    m_table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r)
    {
        const auto &o = rows[r];
        m_table->setRowHeight(r, 46);
        m_table->setItem(r, 0, item("BILL-" + o.id.right(8), Qt::AlignLeft | Qt::AlignVCenter, "#004ac6", true));
        m_table->setItem(r, 1, item(o.id));
        m_table->setItem(r, 2, item(o.time.toString("yyyy-MM-dd HH:mm")));
        m_table->setItem(r, 3, item(o.customer));
        m_table->setItem(r, 4, item(o.payment));
        m_table->setItem(r, 5, item(W::money(o.total), Qt::AlignRight | Qt::AlignVCenter, "", true));
        auto *w = new QWidget;
        auto *h = new QHBoxLayout(w);
        h->setContentsMargins(4, 0, 4, 0);
        auto *view = W::button("View Bill", "mini");
        auto *pr = W::button("Print", "mini");
        h->addWidget(view);
        h->addWidget(pr);
        h->addStretch();
        m_table->setCellWidget(r, 6, w);
        const OrderRecord copy = o;
        connect(view, &QPushButton::clicked, this, [this, copy]
                { Dialogs::receipt(this, copy); });
        connect(pr, &QPushButton::clicked, this, [this, copy]
                { emit toast("Bill " + copy.id + " sent to printer (demo)"); });
    }
}

// =================================================================== ScannerPage
ScannerPage::ScannerPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root;
    pageShell(this, root);
    auto *h = new QHBoxLayout;
    h->setSpacing(12);

    auto *cam = W::card(this);
    auto *cv = new QVBoxLayout(cam);
    cv->setContentsMargins(14, 12, 14, 12);
    cv->setSpacing(10);
    auto *tb = new QHBoxLayout;
    auto *sim = W::badge("SIMULATED FEED", "amber");
    W::headerRow(cv, "barcode", "Barcode Scanner", sim);
    (void)tb;
    m_feed = new CameraFeedWidget;
    m_feed->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_feed->setMinimumHeight(300);
    cv->addWidget(m_feed, 1);
    auto *btns = new QHBoxLayout;
    m_toggle = W::button("Stop Cam", "soft", "stop");
    auto *scan = W::button("Scan Now [F9]", "primary", "barcode");
    btns->addWidget(m_toggle);
    btns->addWidget(scan);
    btns->addStretch();
    cv->addLayout(btns);
    auto *mr = new QHBoxLayout;
    m_manual = new QLineEdit;
    m_manual->setPlaceholderText("Or type an 8-digit EAN and press Enter");
    m_manual->setMaxLength(12);
    auto *go = W::button("Lookup", "soft", "search");
    mr->addWidget(m_manual, 1);
    mr->addWidget(go);
    cv->addLayout(mr);
    h->addWidget(cam, 6);

    auto *side = W::card(this);
    auto *sv = new QVBoxLayout(side);
    sv->setContentsMargins(14, 12, 14, 12);
    sv->setSpacing(10);
    W::headerRow(sv, "clock", "Recent Scans");
    m_last = W::label("No scans yet", "h2");
    sv->addWidget(m_last);
    m_history = new QListWidget;
    m_history->setStyleSheet("QListWidget{border:1px solid #dce9ff;border-radius:8px;padding:4px;} QListWidget::item{padding:8px;border-bottom:1px solid #eff4ff;}");
    sv->addWidget(m_history, 1);
    m_send = W::button("Add Last Scan to Order", "primary", "cart");
    m_send->setEnabled(false);
    sv->addWidget(m_send);
    h->addWidget(side, 4);
    root->addLayout(h, 1);

    connect(m_toggle, &QPushButton::clicked, this, [this]
            {
        const bool on = !m_feed->running();
        m_feed->setRunning(on);
        m_toggle->setText(on ? "Stop Cam" : "Start Cam");
        m_toggle->setIcon(Theme::icon(on ? "stop" : "play", Theme::primary, 16)); });
    connect(scan, &QPushButton::clicked, this, &ScannerPage::scanNext);
    auto lookup = [this]
    {
        const auto p = AppState::instance().findByCode(m_manual->text());
        if (!p)
        {
            emit toast("No product found for \"" + m_manual->text() + "\"");
            return;
        }
        m_feed->setCode(p->ean);
        m_lastEan = p->ean;
        m_last->setText(p->name + " • " + W::money(p->price));
        m_history->insertItem(0, QString("%1   %2   %3").arg(QTime::currentTime().toString("HH:mm:ss"), p->ean, p->name));
        m_send->setEnabled(true);
        m_manual->clear();
    };
    connect(go, &QPushButton::clicked, this, lookup);
    connect(m_manual, &QLineEdit::returnPressed, this, lookup);
    connect(m_send, &QPushButton::clicked, this, [this]
            { emit sendToOrder(m_lastEan); });
}

void ScannerPage::scanNext()
{
    if (!m_feed->running())
    {
        emit toast("Camera is stopped - press Start Cam first");
        return;
    }
    const auto &ps = AppState::instance().products();
    const Product &p = ps[m_index++ % ps.size()];
    m_feed->setCode(p.ean);
    m_lastEan = p.ean;
    m_last->setText(p.name + " • " + W::money(p.price));
    m_history->insertItem(0, QString("%1   %2   %3").arg(QTime::currentTime().toString("HH:mm:ss"), p.ean, p.name));
    m_send->setEnabled(true);
}

// =================================================================== SettingsPage
SettingsPage::SettingsPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root;
    pageShell(this, root);
    auto *card = W::card(this);
    card->setMaximumWidth(620);
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(20, 16, 20, 16);
    v->setSpacing(14);
    W::headerRow(v, "settings", "Terminal Settings");
    auto &s = AppState::instance().settings();

    auto *form = new QFormLayout;
    form->setHorizontalSpacing(20);
    form->setVerticalSpacing(12);
    auto *tax = new QDoubleSpinBox;
    tax->setSuffix(" %");
    tax->setRange(0, 40);
    tax->setValue(s.taxPercent);
    auto *vip = new QDoubleSpinBox;
    vip->setSuffix(" %");
    vip->setRange(0, 50);
    vip->setValue(s.vipDiscountPercent);
    auto *autoAdd = new QCheckBox("Add scanned items to the order automatically");
    autoAdd->setChecked(s.autoAddScans);
    auto *beep = new QCheckBox("Play a beep on successful scan");
    beep->setChecked(s.beepOnScan);
    form->addRow("GST rate (included in price)", tax);
    form->addRow("VIP loyalty discount", vip);
    form->addRow("Scanning", autoAdd);
    form->addRow("", beep);
    v->addLayout(form);
    v->addWidget(W::label("Changes apply to new calculations immediately. Settings are kept in memory only (no backend).", "muted"));
    v->addStretch();
    auto *h = new QHBoxLayout;
    h->addStretch();
    auto *save = W::button("Apply", "primary", "check");
    h->addWidget(save);
    v->addLayout(h);
    connect(save, &QPushButton::clicked, this, [=, this]
            {
        auto &st = AppState::instance().settings();
        st.taxPercent = tax->value();
        st.vipDiscountPercent = vip->value();
        st.autoAddScans = autoAdd->isChecked();
        st.beepOnScan = beep->isChecked();
        emit toast("Settings applied"); });
    root->addWidget(card);
    root->addStretch();
}
