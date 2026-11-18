#include "OtherPages.h"
#include "Dialogs.h"
#include "Theme.h"
#include "Widgets.h"
#include <QApplication>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextStream>
#include <QTime>
#include <QVBoxLayout>

namespace
{
    QTableWidget* makeTable(const QStringList& headers)
    {
        auto* t = new QTableWidget(0, headers.size());
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
    QTableWidgetItem* item(const QString& s, Qt::Alignment a = Qt::AlignLeft | Qt::AlignVCenter,
        const QString& color = "", bool bold = false)
    {
        auto* i = new QTableWidgetItem(s);
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
    QWidget* wrapW(QWidget* w, Qt::Alignment a = Qt::AlignLeft)
    {
        auto* c = new QWidget;
        auto* h = new QHBoxLayout(c);
        h->setContentsMargins(6, 0, 6, 0);
        h->setAlignment(a | Qt::AlignVCenter);
        h->addWidget(w);
        return c;
    }
    QWidget* pageShell(QWidget* page, QVBoxLayout*& body)
    {
        body = W::pageLayout(page);
        return page;
    }
    [[maybe_unused]] QFrame* kpiCard(const QString& cap, const QString& icon, QLabel*& value)
    {
        auto* c = W::card(nullptr);
        auto* h = new QHBoxLayout(c);
        h->setContentsMargins(14, 10, 14, 10);
        auto* v = new QVBoxLayout;
        v->setSpacing(0);
        v->addWidget(W::label(cap, "caption"));
        value = W::label("", "bigValue");
        value->setStyleSheet("font-size:20px; font-weight:700;");
        v->addWidget(value);
        h->addLayout(v);
        h->addStretch();
        auto* ic = new QLabel;
        ic->setFixedSize(32, 32);
        ic->setAlignment(Qt::AlignCenter);
        ic->setStyleSheet("background:#dce9ff; border-radius:6px;");
        ic->setPixmap(Theme::iconPixmap(icon, Theme::primary, 16));
        h->addWidget(ic);
        return c;
    }
    bool writeCsv(QWidget* parent, const QString& suggested, const QStringList& rows)
    {
        const QString path = QFileDialog::getSaveFileName(parent, "Export CSV", suggested, "CSV (*.csv)");
        if (path.isEmpty())
            return false;
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;
        QTextStream out(&f);
        for (const auto& r : rows)
            out << r << '\n';
        return true;
    }
}

// =================================================================== OrdersPage
OrdersPage::OrdersPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root;
    pageShell(this, root);

    auto* card = W::card(this);
    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(18, 14, 18, 10);
    v->setSpacing(10);

    auto* tools = new QHBoxLayout;
    tools->setSpacing(8);
    m_orderSearch = new QLineEdit;
    m_orderSearch->setPlaceholderText("Search by order number, customer or phone…");
    m_orderSearch->setMinimumHeight(40);
    m_orderSearch->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    m_custSearch = new QLineEdit; // kept hidden: legacy field, not shown
    m_custSearch->hide();
    tools->addWidget(m_orderSearch, 3);
    tools->addStretch(1);
    auto* exp = W::button("Export CSV", "soft", "export");
    tools->addWidget(exp);
    v->addLayout(tools);

    auto* tabs = new QHBoxLayout;
    tabs->setSpacing(6);
    for (const QString& name : { "All", "Paid", "In Progress", "On Hold" })
    {
        auto* b = W::button(name, "tab");
        b->setCheckable(true);
        b->setChecked(name == "All");
        tabs->addWidget(b);
        m_tabs << b;
        connect(b, &QPushButton::clicked, this, [this, name]
            {
                m_filter = name;
                for (auto* t : m_tabs) t->setChecked(t->text() == name);
                rebuild(); });
    }
    tabs->addStretch();
    v->addLayout(tabs);

    m_table = makeTable({ "ORDER ID", "DATE & TIME", "CUSTOMER", "ITEMS SUMMARY", "PAYMENT", "AMOUNT", "STATUS", "ACTIONS" });
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    for (int c : {0, 1, 4, 5})
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    for (int c : {2, 6, 7})
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
    m_table->setColumnWidth(2, 175);
    m_table->setColumnWidth(6, 120);
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
            QStringList rows{ "Order ID,Time,Customer,Contact,Items,Payment,Amount,Status" };
            for (const auto& o : AppState::instance().orders())
                rows << QString("%1,%2,\"%3\",\"%4\",%5,\"%6\",%7,%8").arg(o.id, o.time.toString("yyyy-MM-dd HH:mm:ss"), o.customer, o.contact)
                .arg(o.itemCount()).arg(o.payment).arg(o.total, 0, 'f', 2).arg(o.status);
            if (writeCsv(this, "orders.csv", rows)) emit toast("Orders exported"); });
    rebuild();
}

void OrdersPage::setSearch(const QString& text)
{
    m_custSearch->clear();
    m_orderSearch->setText(text);
}

void OrdersPage::rebuild()
{
    const auto& all = AppState::instance().orders();
    QString q = m_orderSearch->text().trimmed();
    q.remove('#');
    QVector<OrderRecord> rows;
    int hold = 0;
    for (const auto& o : all)
    {
        if (o.status == "On Hold" || o.status == "In Progress")
            ++hold;
        if (m_filter != "All" && o.status != m_filter)
            continue;
        if (!q.isEmpty() && !o.id.contains(q, Qt::CaseInsensitive) && !o.customer.contains(q, Qt::CaseInsensitive) &&
            !o.contact.contains(q))
            continue;
        rows << o;
    }
    auto& st = AppState::instance();
    (void)hold;
    m_table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r)
    {
        const auto& o = rows[r];
        m_table->setRowHeight(r, 56);
        m_table->setItem(r, 0, item("#" + o.id, Qt::AlignLeft | Qt::AlignVCenter, "#004ac6", true));
        m_table->setItem(r, 1, item(o.time.toString("yyyy-MM-dd HH:mm") + "\n" + o.shift));
        {
            auto* cw = new QWidget;
            auto* cl = new QVBoxLayout(cw);
            cl->setContentsMargins(8, 4, 8, 4);
            cl->setSpacing(0);
            auto* n = W::label(o.customer + (o.tier == "VIP" ? "  [VIP]" : ""));
            n->setStyleSheet("font-weight:700;");
            auto* c = W::label(o.contact);
            c->setStyleSheet("color:#737686; font-size:11.5px;");
            cl->addWidget(n);
            cl->addWidget(c);
            m_table->setCellWidget(r, 2, cw);
        }
        m_table->setItem(r, 3, item(o.summary()));
        m_table->setItem(r, 4, item(o.payment));
        m_table->setItem(r, 5, item(W::money(o.total), Qt::AlignRight | Qt::AlignVCenter, "", true));
        m_table->setCellWidget(r, 6, wrapW(W::badge(o.status, W::statusBadgeKind(o.status))));
        auto* w = new QWidget;
        auto* h = new QHBoxLayout(w);
        h->setContentsMargins(4, 0, 4, 0);
        h->setSpacing(4);
        const OrderRecord copy = o;
        auto* view = W::button("", "flat", "eye");
        view->setToolTip("View details");
        h->addWidget(view);
        connect(view, &QPushButton::clicked, this, [this, copy]
            { Dialogs::orderDetails(this, copy); });
        auto* rc = W::button("", "flat", "bills");
        rc->setToolTip("Receipt");
        h->addWidget(rc);
        h->addStretch();
        connect(rc, &QPushButton::clicked, this, [this, copy]
            { Dialogs::receipt(this, copy); });
        m_table->setCellWidget(r, 7, w);
    }
    m_showing->setText(QString("%1 of %2 orders").arg(rows.size()).arg(all.size()));
}

// =================================================================== ProductsPage
ProductsPage::ProductsPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root;
    pageShell(this, root);
    auto* card = W::card(this);
    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(18, 14, 18, 10);
    v->setSpacing(10);
    auto* h = new QHBoxLayout;
    h->addWidget(W::iconLabel("products", Theme::primary, 20));
    h->addWidget(W::label("Product Catalog", "h1"));
    m_count = W::badge("", "blue");
    h->addWidget(m_count);
    h->addStretch();
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Search name, acronym, id or barcode...");
    m_search->setMinimumWidth(280);
    m_search->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    h->addWidget(m_search);
    v->addLayout(h);
    m_issues = W::button("", "soft", "warn");
    connect(m_issues, &QPushButton::clicked, this, [this] { Dialogs::loadIssues(this); });
    h->insertWidget(3, m_issues);
    m_table = makeTable({ "ID", "ACRONYM", "BARCODE", "PRODUCT", "UNIT", "PRICE", "TAX", "DISC.", "STOCK", "STATUS", "" });
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    for (int c : {0, 1, 2, 4, 5, 6, 7, 8, 9})
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(10, QHeaderView::Fixed);
    m_table->setColumnWidth(10, 200);
    v->addWidget(m_table, 1);
    root->addWidget(card, 1);
    connect(m_search, &QLineEdit::textChanged, this, [this]
        { rebuild(); });
    connect(&AppState::instance(), &AppState::changed, this, &ProductsPage::rebuild);
    rebuild();
}

void ProductsPage::setSearch(const QString& text) { m_search->setText(text); }

void ProductsPage::rebuild()
{
    const QString q = m_search->text().trimmed();
    auto& st = AppState::instance();
    const QVector<ProductInfo> rows = q.isEmpty() ? st.products() : st.searchText(q);
    m_count->setText(QString("%1 products").arg(rows.size()));
    const int nIssues = st.productIssues().size();
    m_issues->setVisible(nIssues > 0);
    m_issues->setText(QString("Data issues (%1)").arg(nIssues));
    m_table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r)
    {
        const auto& p = rows[r];
        m_table->setRowHeight(r, 44);
        const int limit = st.settings().lowStockThreshold;
        m_table->setItem(r, 0, item(QString::number(p.id)));
        m_table->setItem(r, 1, item(p.sku, Qt::AlignLeft | Qt::AlignVCenter, "#004ac6", true));
        m_table->setItem(r, 2, item(p.ean));
        m_table->setItem(r, 3, item(p.name, Qt::AlignLeft | Qt::AlignVCenter, "", true));
        m_table->setItem(r, 4, item(p.unit));
        m_table->setItem(r, 5, item(W::money(p.price), Qt::AlignRight | Qt::AlignVCenter, "", true));
        m_table->setItem(r, 6, item(W::money(p.tax), Qt::AlignRight | Qt::AlignVCenter));
        m_table->setItem(r, 7, item(W::money(p.discount), Qt::AlignRight | Qt::AlignVCenter));
        const bool low = p.stock <= limit;
        m_table->setItem(r, 8, item(QString::number(p.stock), Qt::AlignRight | Qt::AlignVCenter, low ? "#ba1a1a" : "", low));
        m_table->setCellWidget(r, 9, wrapW(W::badge(p.stock == 0 ? "Out" : low ? "Low" : "OK", p.stock == 0 ? "red" : low ? "amber" : "green")));
        auto* b = W::button("Add", "mini");
        b->setEnabled(p.stock > 0);
        auto* d = W::button("Details", "soft");
        auto* cell = new QWidget;
        auto* cl = new QHBoxLayout(cell);
        cl->setContentsMargins(0, 0, 4, 0);
        cl->setSpacing(6);
        cl->addWidget(d);
        cl->addWidget(b);
        m_table->setCellWidget(r, 10, cell);
        const QString ean = p.ean;
        const int pid = p.id;
        connect(b, &QPushButton::clicked, this, [this, ean]
            { emit addToOrder(ean); });
        connect(d, &QPushButton::clicked, this, [this, pid]
            { Dialogs::productDescription(this, pid); });
    }
}

// =================================================================== BillsPage
BillsPage::BillsPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root;
    pageShell(this, root);
    auto* card = W::card(this);
    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(18, 14, 18, 10);
    v->setSpacing(10);
    auto* h = new QHBoxLayout;
    h->addWidget(W::iconLabel("bills", Theme::primary, 20));
    h->addWidget(W::label("Bills", "h1"));
    h->addStretch();
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Search bill or order ID...");
    m_search->setMinimumWidth(280);
    m_search->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    h->addWidget(m_search);
    v->addLayout(h);
    m_table = makeTable({ "BILL NO.", "ORDER", "DATE & TIME", "CUSTOMER", "PAYMENT", "TOTAL", "ACTIONS" });
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
    for (const auto& o : AppState::instance().orders())
        if (o.status == "Paid" && !o.billId.isEmpty() &&
            (q.isEmpty() || o.id.contains(q, Qt::CaseInsensitive) || o.billId.contains(q, Qt::CaseInsensitive) || o.customer.contains(q, Qt::CaseInsensitive)))
            rows << o;
    m_table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r)
    {
        const auto& o = rows[r];
        m_table->setRowHeight(r, 46);
        m_table->setItem(r, 0, item(o.billId, Qt::AlignLeft | Qt::AlignVCenter, "#004ac6", true));
        m_table->setItem(r, 1, item(o.id));
        m_table->setItem(r, 2, item(o.time.toString("yyyy-MM-dd HH:mm")));
        m_table->setItem(r, 3, item(o.customer));
        m_table->setItem(r, 4, item(o.payment));
        m_table->setItem(r, 5, item(W::money(o.total), Qt::AlignRight | Qt::AlignVCenter, "", true));
        auto* w = new QWidget;
        auto* h = new QHBoxLayout(w);
        h->setContentsMargins(4, 0, 4, 0);
        auto* view = W::button("View Bill", "mini");
        h->addWidget(view);
        h->addStretch();
        m_table->setCellWidget(r, 6, w);
        const OrderRecord copy = o;
        connect(view, &QPushButton::clicked, this, [this, copy]
            { Dialogs::receipt(this, copy); });
    }
}

// =================================================================== ScannerPage
ScannerPage::ScannerPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root;
    pageShell(this, root);
    auto* h = new QHBoxLayout;
    h->setSpacing(12);

    auto* cam = W::card(this);
    auto* cv = new QVBoxLayout(cam);
    cv->setContentsMargins(18, 14, 18, 14);
    cv->setSpacing(10);
    W::headerRow(cv, "barcode", "Barcode Scanner");
    m_feed = new CameraFeedWidget;
    m_feed->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_feed->setMinimumHeight(300);
    cv->addWidget(m_feed, 1);
    auto* btns = new QHBoxLayout;
    m_toggle = W::button("Start Cam", "soft", "play");
    auto* scan = W::button("Start Scan [F9]", "primary", "barcode");
    btns->addWidget(m_toggle);
    btns->addWidget(scan);
    btns->addStretch();
    cv->addLayout(btns);
    auto* mr = new QHBoxLayout;
    m_manual = new QLineEdit;
    m_manual->setPlaceholderText("Or type an 8-digit EAN and press Enter");
    m_manual->setMaxLength(12);
    auto* go = W::button("Lookup", "soft", "search");
    mr->addWidget(m_manual, 1);
    mr->addWidget(go);
    cv->addLayout(mr);
    h->addWidget(cam, 6);

    auto* side = W::card(this);
    auto* sv = new QVBoxLayout(side);
    sv->setContentsMargins(18, 14, 18, 14);
    sv->setSpacing(10);
    W::headerRow(sv, "clock", "Recent Scans");
    m_last = W::label("No scans yet", "h2");
    sv->addWidget(m_last);
    m_history = new QListWidget;
    m_history->setStyleSheet("QListWidget{border:1px solid #dbe4f7;border-radius:10px;padding:4px;} QListWidget::item{padding:9px;border-bottom:1px solid #e8eefb;}");
    sv->addWidget(m_history, 1);
    m_describe = W::button("Show description", "soft", "search");
    m_describe->setEnabled(false);
    sv->addWidget(m_describe);
    m_send = W::button("Add last scan to order", "primary", "cart");
    m_send->setEnabled(false);
    sv->addWidget(m_send);
    h->addWidget(side, 4);
    root->addLayout(h, 1);

    connect(m_toggle, &QPushButton::clicked, this, [this]
        {
            const bool on = !m_feed->running();
            m_feed->setRunning(on);
            const bool running = m_feed->running();
            m_toggle->setText(running ? "Stop Cam" : "Start Cam");
            m_toggle->setIcon(Theme::icon(running ? "stop" : "play", Theme::primary, 16)); });
    connect(scan, &QPushButton::clicked, this, &ScannerPage::scanNext);
    connect(m_feed, &CameraFeedWidget::scanError, this, &ScannerPage::toast);
    connect(m_feed, &CameraFeedWidget::barcodeDetected, this, [this](const QString& code) {
        const auto product = AppState::instance().findByCode(code);
        if (product) show(*product);
        else emit toast("Barcode " + code + " not found in Product.txt");
        });
    auto lookup = [this]
        {
            const auto p = AppState::instance().findByCode(m_manual->text());
            if (!p)
            {
                emit toast("No product found for \"" + m_manual->text() + "\"");
                return;
            }
            show(*p);
            m_manual->clear();
        };
    connect(go, &QPushButton::clicked, this, lookup);
    connect(m_manual, &QLineEdit::returnPressed, this, lookup);
    connect(m_send, &QPushButton::clicked, this, [this]
        { emit sendToOrder(m_lastEan); });
    connect(m_describe, &QPushButton::clicked, this, [this]
        { Dialogs::productDescription(this, m_lastId); });
}

void ScannerPage::scanNext()
{
    if (!m_feed->running()) m_feed->setRunning(true);
    if (!m_feed->running()) emit toast("Unable to start camera scanning");
    else {
        m_toggle->setText("Stop Cam");
        m_toggle->setIcon(Theme::icon("stop", Theme::primary, 16));
    }
}

void ScannerPage::show(const ProductInfo& p)
{
    m_feed->setCode(p.ean);
    m_lastEan = p.ean;
    m_lastId = p.id;
    m_last->setText(p.name + " • " + W::money(p.price));
    m_history->insertItem(0, QString("%1   %2   %3").arg(QTime::currentTime().toString("HH:mm:ss"), p.ean, p.name));
    m_send->setEnabled(true);
    m_describe->setEnabled(true);
    // Show description only on request; opening a modal on every camera frame is disruptive.
}

// =================================================================== SettingsPage
SettingsPage::SettingsPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root;
    pageShell(this, root);
    auto* card = W::card(this);
    card->setMaximumWidth(620);
    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(20, 16, 20, 16);
    v->setSpacing(14);
    W::headerRow(v, "settings", "Terminal Settings");
    auto& s = AppState::instance().settings();

    auto* form = new QFormLayout;
    form->setHorizontalSpacing(20);
    form->setVerticalSpacing(12);
    auto* low = new QSpinBox;
    low->setRange(0, 1000);
    low->setSuffix(" units");
    low->setValue(s.lowStockThreshold);
    auto* dark = new QCheckBox("Dark mode");
    dark->setChecked(Theme::isDark());
    auto* autoAdd = new QCheckBox("Add scanned items to the order automatically");
    autoAdd->setChecked(s.autoAddScans);
    auto* beep = new QCheckBox("Play a beep on successful scan");
    beep->setChecked(s.beepOnScan);
    form->addRow("Appearance", dark);
    form->addRow("Low-stock limit", low);
    form->addRow("Scanning", autoAdd);
    form->addRow("", beep);
    v->addLayout(form);
    v->addWidget(W::label("Tax and discount come per product from Product.txt. Dark mode is remembered between runs.", "muted"));
    v->addStretch();
    auto* h = new QHBoxLayout;
    h->addStretch();
    auto* save = W::button("Apply", "primary", "check");
    h->addWidget(save);
    v->addLayout(h);
    connect(save, &QPushButton::clicked, this, [=, this]
        {
            auto& st = AppState::instance().settings();
            st.lowStockThreshold = low->value();
            AppState::instance().emitChanged();
            if (dark->isChecked() != Theme::isDark()) { Theme::setDark(dark->isChecked()); qApp->setStyleSheet(Theme::styleSheet()); }
            st.autoAddScans = autoAdd->isChecked();
            st.beepOnScan = beep->isChecked();
            emit toast("Settings applied"); });
    root->addWidget(card);
    root->addStretch();
}