#include "ReportsPage.h"
#include "AppState.h"
#include "Theme.h"
#include "UiKit.h"
#include "UiStore.h"
#include "Widgets.h"
#include <QDateEdit>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QPageSize>
#include <QPdfWriter>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QTableWidget>
#include <QTextDocument>
#include <QVBoxLayout>
#include <algorithm>

using namespace Ui; // makeTable, item, wrapW, sectionHeader, kbd, writeCsv, csvField (see UiKit.h)

namespace
{
    // ---------------------------------------------------------------- data helpers (no widgets)
    bool isCompleted(const OrderRecord &o) { return o.status == "Paid" || o.status == "Archived"; }
    bool isCash(const OrderRecord &o) { return o.status == "Paid" && o.payment.compare("cash", Qt::CaseInsensitive) == 0; }
    QString pct(double part, double whole) { return whole > 0 ? QString::number(part * 100.0 / whole, 'f', 1) + "%" : QString("0.0%"); }
    QString when(const QDateTime &t) { return t.toString("dd-MMM-yyyy HH:mm"); }
    QString customerOf(const OrderRecord &o) { return o.customer.isEmpty() ? QString("Walk-in Customer") : o.customer; }

    // Orders parked with F2 are incomplete orders too (they live in UiStore, not in Order.txt).
    QVector<OrderRecord> parkedAsOrders()
    {
        QVector<OrderRecord> out;
        for (const auto &p : UiStore::instance().parked())
        {
            OrderRecord h;
            h.id = p.code;
            h.time = p.parkedAt;
            h.customer = p.customer;
            h.contact = p.contact;
            h.status = "On Hold";
            h.lines = p.lines;
            h.recompute();
            out << h;
        }
        return out;
    }

    // One product's sales over all completed orders.
    struct Sold
    {
        QString name, acronym;
        int productId = 0;
        int qty = 0;
        double revenue = 0; // (price - discount) * qty, without tax
    };

    QVector<Sold> soldTotals()
    {
        QMap<QString, Sold> byKey;
        for (const auto &o : AppState::instance().orders())
        {
            if (!isCompleted(o))
                continue;
            for (const auto &l : o.lines)
            {
                const QString key = l.productId > 0 ? QString("#%1").arg(l.productId) : "n:" + l.name;
                Sold &s = byKey[key];
                s.name = l.name;
                s.acronym = l.acronym;
                s.productId = l.productId;
                s.qty += l.qty;
                s.revenue += (l.price - l.discount) * l.qty;
            }
        }
        QVector<Sold> out;
        for (auto it = byKey.cbegin(); it != byKey.cend(); ++it)
            out.push_back(it.value());
        return out;
    }

    const ProductInfo *productOf(int productId)
    {
        if (productId <= 0)
            return nullptr;
        for (const auto &p : AppState::instance().products())
            if (p.id == productId)
                return &p;
        return nullptr;
    }

    QString stockText(int stock)
    {
        const int low = AppState::instance().settings().lowStockThreshold;
        if (stock <= 0)
            return "0  (depleted)";
        if (stock <= low)
            return QString("%1  (low)").arg(stock);
        return QString::number(stock);
    }

    // ---------------------------------------------------------------- widget helpers
    // Summary tile with a caption that changes per report.
    QFrame *tile(const QString &icon, const QString &tint, QLabel *&cap, QLabel *&val, QLabel *&sub)
    {
        auto *c = W::card(nullptr);
        auto *h = new QHBoxLayout(c);
        h->setContentsMargins(16, 12, 16, 12);
        h->setSpacing(10);
        auto *v = new QVBoxLayout;
        v->setSpacing(2);
        cap = W::label("", "caption");
        val = W::label("", "bigValue");
        sub = W::label("", "muted");
        v->addWidget(cap);
        v->addWidget(val);
        v->addWidget(sub);
        h->addLayout(v, 1);
        QString top = "#eaf1ff", bottom = "#d9e6ff";
        QColor fg = Theme::primary;
        if (tint == "green") { top = "#e2fbec"; bottom = "#c9f3d9"; fg = Theme::tertiary; }
        else if (tint == "amber") { top = "#fff4d6"; bottom = "#ffe8a8"; fg = Theme::themed("#8a5a00"); }
        h->addWidget(W::iconTile(icon, fg, top, bottom, 38), 0, Qt::AlignTop);
        return c;
    }

    QWidget *barCell(double fraction)
    {
        auto *bar = new QProgressBar;
        bar->setRange(0, 1000);
        bar->setValue(int(qBound(0.0, fraction, 1.0) * 1000));
        bar->setTextVisible(false);
        bar->setFixedHeight(8);
        bar->setMinimumWidth(130);
        bar->setStyleSheet("QProgressBar{background:#dfe8fb;border:none;border-radius:4px;}"
                           "QProgressBar::chunk{background:#2554d8;border-radius:4px;}");
        return wrapW(bar);
    }

    QString htmlEscape(const QString &s) { return s.toHtmlEscaped(); }
}

// ======================================================================= page
ReportsPage::ReportsPage(QWidget *parent) : QWidget(parent)
{
    setObjectName("page");
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *inner = new QWidget;
    inner->setMinimumHeight(860);
    scroll->setWidget(inner);
    outer->addWidget(scroll);
    auto *root = W::pageLayout(inner);

    // ---- header: title + exports
    auto *csv = W::button("Export CSV Data", "soft", "export");
    auto *pdf = W::button("Export / Print as PDF", "primary", "print");
    root->addWidget(sectionHeader("MANAGER REPORTS", "Business Reports & Analytics",
                                  "Audit orders, cash sales and product turnover. Every figure comes from the saved orders and products.",
                                  {csv, pdf, kbd("Ctrl+P")}));
    connect(csv, &QPushButton::clicked, this, &ReportsPage::exportCsv);
    connect(pdf, &QPushButton::clicked, this, &ReportsPage::printPdf);
    auto *printKey = new QShortcut(QKeySequence("Ctrl+P"), this);
    printKey->setContext(Qt::WidgetWithChildrenShortcut);
    connect(printKey, &QShortcut::activated, this, &ReportsPage::printPdf);

    // ---- the six report tabs
    auto *tabCard = W::card(this);
    auto *th = new QHBoxLayout(tabCard);
    th->setContentsMargins(12, 10, 12, 10);
    th->setSpacing(6);
    const QStringList names = {"Incomplete Orders", "Completed Orders", "Cash Sales", "All Sales", "Top 10 Sold",
                               "Top 10 Remaining Stock"};
    for (int i = 0; i < names.size(); ++i)
    {
        auto *b = W::button(names[i], "tab");
        b->setCheckable(true);
        b->setChecked(i == 0);
        th->addWidget(b);
        m_tabs << b;
        connect(b, &QPushButton::clicked, this, [this, i] { selectTab(i); });
    }
    th->addStretch();
    root->addWidget(tabCard);

    // ---- three summary tiles (captions change with the report)
    auto *tiles = new QHBoxLayout;
    tiles->setSpacing(0);
    const char *icons[3] = {"orders", "cash", "percent"};
    const char *tints[3] = {"blue", "green", "amber"};
    for (int i = 0; i < 3; ++i)
        tiles->addWidget(tile(icons[i], tints[i], m_cap[i], m_val[i], m_sub[i]));
    root->addLayout(tiles);

    // ---- time period (Cash Sales only): Daily / Weekly / Monthly / Yearly or a custom range
    m_periodCard = W::card(this);
    auto *ph = new QHBoxLayout(m_periodCard);
    ph->setContentsMargins(16, 10, 16, 10);
    ph->setSpacing(8);
    ph->addWidget(W::label("TIME PERIOD", "caption"));
    for (const QString &name : {"Daily", "Weekly", "Monthly", "Yearly"})
    {
        auto *b = W::button(name, "tab");
        b->setCheckable(true);
        ph->addWidget(b);
        m_periods << b;
        connect(b, &QPushButton::clicked, this, [this, name] { selectPeriod(name); });
    }
    ph->addSpacing(18);
    ph->addWidget(W::label("CUSTOM RANGE", "caption"));
    m_from = new QDateEdit(QDate::currentDate());
    m_to = new QDateEdit(QDate::currentDate());
    for (QDateEdit *d : {m_from, m_to})
    {
        d->setCalendarPopup(true);
        d->setDisplayFormat("dd-MMM-yyyy");
        d->setMinimumHeight(34);
    }
    ph->addWidget(m_from);
    ph->addWidget(W::label("to", "muted"));
    ph->addWidget(m_to);
    auto *apply = W::button("Filter", "primary", "filter");
    ph->addWidget(apply);
    ph->addStretch();
    connect(apply, &QPushButton::clicked, this, &ReportsPage::applyCustomRange);
    root->addWidget(m_periodCard);

    // ---- the report table
    auto *card = W::card(this);
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(18, 14, 18, 14);
    v->setSpacing(8);
    auto *head = new QVBoxLayout;
    head->setSpacing(0);
    m_tableTitle = W::label("", "h1");
    m_tableSub = W::label("", "muted");
    head->addWidget(m_tableTitle);
    head->addWidget(m_tableSub);
    v->addLayout(head);
    m_table = makeTable({"-"});
    v->addWidget(m_table, 1);
    m_empty = W::label("", "muted");
    m_empty->setAlignment(Qt::AlignCenter);
    m_empty->setMinimumHeight(70);
    v->addWidget(m_empty);

    auto *foot = new QFrame;
    foot->setObjectName("reportFooter");
    foot->setStyleSheet("#reportFooter{background:#0f1b33;border-radius:12px;}"
                        "#reportFooter QLabel{background:transparent;}");
    auto *fh = new QHBoxLayout(foot);
    fh->setContentsMargins(18, 12, 18, 12);
    fh->setSpacing(12);
    m_footLabel = new QLabel;
    m_footLabel->setStyleSheet("color:#c9d3ee;font-weight:700;");
    m_footValue = new QLabel;
    m_footValue->setStyleSheet("color:#5be3a0;font-size:20px;font-weight:800;");
    m_footExtra = new QLabel;
    m_footExtra->setStyleSheet("color:#c9d3ee;");
    fh->addWidget(m_footLabel);
    fh->addWidget(m_footValue);
    fh->addStretch();
    fh->addWidget(m_footExtra);
    v->addWidget(foot);
    root->addWidget(card, 1);

    selectPeriod("Daily"); // also renders
    selectTab(0);
    connect(&AppState::instance(), &AppState::changed, this, &ReportsPage::refresh);
    connect(&UiStore::instance(), &UiStore::parkedChanged, this, &ReportsPage::refresh);
}

void ReportsPage::refresh() { render(); }

void ReportsPage::selectTab(int index)
{
    m_tab = index;
    for (int i = 0; i < m_tabs.size(); ++i)
        m_tabs[i]->setChecked(i == index);
    m_periodCard->setVisible(index == 2);
    render();
}

void ReportsPage::selectPeriod(const QString &name)
{
    const QDate today = QDate::currentDate();
    m_rangeTo = today;
    if (name == "Daily")
        m_rangeFrom = today;
    else if (name == "Weekly")
        m_rangeFrom = today.addDays(-6);
    else if (name == "Monthly")
        m_rangeFrom = QDate(today.year(), today.month(), 1);
    else
        m_rangeFrom = QDate(today.year(), 1, 1);
    for (auto *b : m_periods)
        b->setChecked(b->text() == name);
    m_from->setDate(m_rangeFrom);
    m_to->setDate(m_rangeTo);
    render();
}

void ReportsPage::applyCustomRange()
{
    QDate a = m_from->date(), b = m_to->date();
    if (b < a)
        std::swap(a, b);
    m_rangeFrom = a;
    m_rangeTo = b;
    for (auto *p : m_periods)
        p->setChecked(false);
    render();
}

// ======================================================================= the six reports
// 1. Orders that are not paid or archived (in progress, on hold, parked), with the total of all such bills.
ReportsPage::Report ReportsPage::incomplete() const
{
    Report r;
    r.title = "Incomplete Orders";
    r.subtitle = "Orders that are not paid or archived";
    QVector<OrderRecord> list = parkedAsOrders();
    for (const auto &o : AppState::instance().orders())
        if (!isCompleted(o))
            list << o;
    std::sort(list.begin(), list.end(), [](const OrderRecord &a, const OrderRecord &b) { return a.time > b.time; });

    double total = 0;
    int hold = 0, progress = 0;
    r.headers = {"ORDER ID", "DATE & TIME", "CUSTOMER", "ITEMS", "STATUS", "AMOUNT"};
    r.rightCols = {5};
    r.statusCol = 4;
    for (const auto &o : list)
    {
        total += o.total;
        (o.status == "On Hold" ? hold : progress)++;
        r.rows << QStringList{"#" + o.id, when(o.time), customerOf(o), o.summary(), o.status, W::money(o.total)};
    }
    r.captions[0] = "INCOMPLETE ORDERS";
    r.values[0] = QString::number(list.size());
    r.subs[0] = "not paid or archived";
    r.captions[1] = "TOTAL AMOUNT PENDING";
    r.values[1] = W::money(total);
    r.subs[1] = "sum of all incomplete bills";
    r.captions[2] = "ON HOLD";
    r.values[2] = QString::number(hold);
    r.subs[2] = QString("%1 in progress").arg(progress);
    r.footerLabel = "Total of all incomplete orders:";
    r.footerValue = W::money(total);
    r.footerExtra = QString("%1 order%2").arg(list.size()).arg(list.size() == 1 ? "" : "s");
    r.emptyText = "No incomplete orders. Everything is paid or archived.";
    return r;
}

// 2. Orders that have been completed (paid or archived), with the total amount.
ReportsPage::Report ReportsPage::completed() const
{
    Report r;
    r.title = "Completed Orders";
    r.subtitle = "Paid and archived orders";
    double total = 0, discount = 0, gross = 0;
    int n = 0;
    r.headers = {"ORDER ID", "DATE & TIME", "CUSTOMER", "ITEMS", "PAYMENT", "STATUS", "AMOUNT"};
    r.rightCols = {6};
    r.statusCol = 5;
    for (const auto &o : AppState::instance().orders())
    {
        if (!isCompleted(o))
            continue;
        ++n;
        total += o.total;
        discount += o.discount;
        gross += o.subtotal;
        r.rows << QStringList{"#" + o.id, when(o.time), customerOf(o), o.summary(), o.payment, o.status, W::money(o.total)};
    }
    r.captions[0] = "COMPLETED ORDERS";
    r.values[0] = QString::number(n);
    r.subs[0] = "paid or archived";
    r.captions[1] = "TOTAL AMOUNT";
    r.values[1] = W::money(total);
    r.subs[1] = "sum of all completed bills";
    r.captions[2] = "DISCOUNTS GIVEN";
    r.values[2] = W::money(discount);
    r.subs[2] = QString("%1 of gross sales").arg(pct(discount, gross));
    r.footerLabel = "Total of all completed orders:";
    r.footerValue = W::money(total);
    r.footerExtra = QString("%1 order%2").arg(n).arg(n == 1 ? "" : "s");
    r.emptyText = "No completed orders yet.";
    return r;
}

// 3. Cash sales for the chosen period (daily, weekly, monthly, yearly or a specified time period).
ReportsPage::Report ReportsPage::cashSales() const
{
    Report r;
    r.title = "Cash Sales Ledger";
    r.subtitle = m_rangeFrom == m_rangeTo ? m_rangeFrom.toString("dd-MMM-yyyy")
                                          : m_rangeFrom.toString("dd-MMM-yyyy") + "  to  " + m_rangeTo.toString("dd-MMM-yyyy");
    double total = 0, tendered = 0, change = 0, discount = 0, gross = 0;
    int n = 0;
    r.headers = {"DATE & TIME", "ORDER ID", "CUSTOMER", "AMOUNT", "CASH RECEIVED", "BALANCE RETURNED", "DISCOUNT"};
    r.rightCols = {3, 4, 5, 6};
    for (const auto &o : AppState::instance().orders())
    {
        if (!isCash(o) || o.time.date() < m_rangeFrom || o.time.date() > m_rangeTo)
            continue;
        ++n;
        total += o.total;
        tendered += o.tendered;
        change += o.change();
        discount += o.discount;
        gross += o.subtotal;
        r.rows << QStringList{when(o.time), "#" + o.id, customerOf(o), W::money(o.total),
                              o.tendered > 0 ? W::money(o.tendered) : QString("-"),
                              o.tendered > 0 ? W::money(o.change()) : QString("-"), W::money(o.discount)};
    }
    r.captions[0] = "TOTAL CASH SALES";
    r.values[0] = W::money(total);
    r.subs[0] = QString("%1 cash order%2").arg(n).arg(n == 1 ? "" : "s");
    r.captions[1] = "CASH TENDER RECEIVED";
    r.values[1] = W::money(tendered);
    r.subs[1] = "balance returned " + W::money(change);
    r.captions[2] = "DISCOUNTS GRANTED";
    r.values[2] = W::money(discount);
    r.subs[2] = QString("avg %1 per order").arg(pct(discount, gross));
    r.footerLabel = "Cash sales in this period:";
    r.footerValue = W::money(total);
    r.footerExtra = QString("Discounts given %1  •  %2 transaction%3").arg(W::money(discount)).arg(n).arg(n == 1 ? "" : "s");
    r.emptyText = "No cash sales in this period.";
    return r;
}

// 4. All sales with the total amount and the discounts.
ReportsPage::Report ReportsPage::allSales() const
{
    Report r;
    r.title = "All Sales";
    r.subtitle = "Every completed sale with its discount and tax";
    double total = 0, discount = 0, gross = 0, tax = 0;
    int n = 0;
    r.headers = {"ORDER ID", "DATE & TIME", "CUSTOMER", "PAYMENT", "GROSS", "DISCOUNT", "TAX", "TOTAL"};
    r.rightCols = {4, 5, 6, 7};
    for (const auto &o : AppState::instance().orders())
    {
        if (!isCompleted(o))
            continue;
        ++n;
        total += o.total;
        discount += o.discount;
        gross += o.subtotal;
        tax += o.tax;
        r.rows << QStringList{"#" + o.id, when(o.time), customerOf(o), o.payment, W::money(o.subtotal),
                              W::money(o.discount), W::money(o.tax), W::money(o.total)};
    }
    r.captions[0] = "TOTAL SALES";
    r.values[0] = W::money(total);
    r.subs[0] = QString("%1 sale%2").arg(n).arg(n == 1 ? "" : "s");
    r.captions[1] = "TOTAL DISCOUNTS";
    r.values[1] = W::money(discount);
    r.subs[1] = QString("%1 of gross sales").arg(pct(discount, gross));
    r.captions[2] = "TAX COLLECTED";
    r.values[2] = W::money(tax);
    r.subs[2] = "included in the totals";
    r.footerLabel = "Total of all sales:";
    r.footerValue = W::money(total);
    r.footerExtra = QString("Discounts given %1").arg(W::money(discount));
    r.emptyText = "No sales recorded yet.";
    return r;
}

// 5. Ten most sold items in descending order, with their remaining stock.
ReportsPage::Report ReportsPage::topSold() const
{
    Report r;
    r.title = "Top 10 Most Sold Items";
    r.subtitle = "Descending by units sold, with remaining stock";
    QVector<Sold> sold = soldTotals();
    std::sort(sold.begin(), sold.end(), [](const Sold &a, const Sold &b) { return a.qty > b.qty; });
    if (sold.size() > 10)
        sold.resize(10);

    int units = 0, restock = 0;
    const int most = sold.isEmpty() ? 0 : sold.first().qty;
    const int low = AppState::instance().settings().lowStockThreshold;
    r.headers = {"#", "PRODUCT", "ACRONYM", "UNITS SOLD", "REVENUE", "REMAINING STOCK", "SALES VOLUME"};
    r.rightCols = {3, 4, 5};
    r.barCol = 6;
    for (int i = 0; i < sold.size(); ++i)
    {
        const Sold &s = sold[i];
        units += s.qty;
        const ProductInfo *p = productOf(s.productId);
        if (p && p->stock <= low)
            ++restock;
        r.rows << QStringList{QString::number(i + 1), s.name, s.acronym, QString::number(s.qty), W::money(s.revenue),
                              p ? stockText(p->stock) : QString("-"), QString()};
        r.bars << (most > 0 ? double(s.qty) / most : 0.0);
    }
    r.captions[0] = "UNITS SOLD (TOP 10)";
    r.values[0] = QString::number(units);
    r.subs[0] = QString("across %1 product%2").arg(sold.size()).arg(sold.size() == 1 ? "" : "s");
    r.captions[1] = "BEST SELLER";
    r.values[1] = sold.isEmpty() ? QString("-") : sold.first().acronym;
    r.subs[1] = sold.isEmpty() ? QString("no sales yet") : QString("%1 - %2 units").arg(sold.first().name).arg(sold.first().qty);
    r.captions[2] = "NEEDS RESTOCK";
    r.values[2] = QString::number(restock);
    r.subs[2] = "top sellers that are low or depleted";
    r.footerLabel = "Units sold by the top 10:";
    r.footerValue = QString::number(units);
    r.footerExtra = restock > 0 ? QString("%1 need restocking").arg(restock) : QString("stock levels healthy");
    r.emptyText = "No sales yet, so there is no ranking.";
    return r;
}

// 6. Ten items with the most remaining stock, descending, so their purchase is not started.
ReportsPage::Report ReportsPage::topStock() const
{
    Report r;
    r.title = "Top 10 Items by Remaining Stock";
    r.subtitle = "Descending by quantity on hand - do not start purchasing these";
    QVector<ProductInfo> list = AppState::instance().products();
    std::sort(list.begin(), list.end(), [](const ProductInfo &a, const ProductInfo &b) { return a.stock > b.stock; });
    if (list.size() > 10)
        list.resize(10);

    QMap<int, int> soldQty;
    for (const auto &s : soldTotals())
        soldQty[s.productId] = s.qty;

    int units = 0;
    double value = 0;
    const int most = list.isEmpty() ? 0 : list.first().stock;
    r.headers = {"#", "PRODUCT", "ACRONYM", "CATEGORY", "REMAINING STOCK", "UNITS SOLD", "STOCK LEVEL"};
    r.rightCols = {4, 5};
    r.barCol = 6;
    for (int i = 0; i < list.size(); ++i)
    {
        const ProductInfo &p = list[i];
        units += p.stock;
        value += p.stock * p.price;
        const Category *c = UiStore::instance().category(p.ean.left(2));
        r.rows << QStringList{QString::number(i + 1), p.name, p.sku, c ? c->name : p.ean.left(2),
                              QString::number(p.stock), QString::number(soldQty.value(p.id, 0)), QString()};
        r.bars << (most > 0 ? double(p.stock) / most : 0.0);
    }
    r.captions[0] = "UNITS IN TOP 10";
    r.values[0] = QString::number(units);
    r.subs[0] = "quantity on hand";
    r.captions[1] = "HIGHEST STOCK";
    r.values[1] = list.isEmpty() ? QString("-") : list.first().sku;
    r.subs[1] = list.isEmpty() ? QString("no products") : QString("%1 - %2 units").arg(list.first().name).arg(list.first().stock);
    r.captions[2] = "STOCK VALUE";
    r.values[2] = W::money(value);
    r.subs[2] = "at unit price, without tax";
    r.footerLabel = "Units in stock (top 10):";
    r.footerValue = QString::number(units);
    r.footerExtra = "Hold purchase orders for these items";
    r.emptyText = "No products in Product.txt.";
    return r;
}

// ======================================================================= drawing
void ReportsPage::render()
{
    if (!m_table)
        return;
    const Builder builders[6] = {&ReportsPage::incomplete, &ReportsPage::completed, &ReportsPage::cashSales,
                                 &ReportsPage::allSales,   &ReportsPage::topSold,   &ReportsPage::topStock};
    m_current = (this->*builders[qBound(0, m_tab, 5)])();
    const Report &r = m_current;

    for (int i = 0; i < 3; ++i)
    {
        m_cap[i]->setText(r.captions[i]);
        m_val[i]->setText(r.values[i]);
        m_sub[i]->setText(r.subs[i]);
    }
    m_tableTitle->setText(r.title);
    m_tableSub->setText(r.subtitle);

    m_table->clear();
    m_table->setColumnCount(r.headers.size());
    m_table->setHorizontalHeaderLabels(r.headers);
    m_table->setRowCount(r.rows.size());
    auto *hh = m_table->horizontalHeader();
    hh->setSectionResizeMode(QHeaderView::Stretch);
    if (r.barCol >= 0)
    {
        hh->setSectionResizeMode(r.barCol, QHeaderView::Fixed);
        m_table->setColumnWidth(r.barCol, 190);
    }
    const bool orderIdFirst = r.headers.value(0).startsWith("ORDER");
    for (int row = 0; row < r.rows.size(); ++row)
    {
        m_table->setRowHeight(row, 48);
        const QStringList &cells = r.rows[row];
        for (int c = 0; c < cells.size(); ++c)
        {
            if (c == r.statusCol)
                m_table->setCellWidget(row, c, wrapW(W::badge(cells[c], W::statusBadgeKind(cells[c]))));
            else if (c == r.barCol)
                m_table->setCellWidget(row, c, barCell(r.bars.value(row)));
            else
            {
                const Qt::Alignment a = (r.rightCols.contains(c) ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter;
                const bool firstOrderId = c == 0 && orderIdFirst;
                const bool amount = c == r.headers.size() - 1 && r.barCol < 0;
                m_table->setItem(row, c, item(cells[c], a, firstOrderId ? "#004ac6" : "", firstOrderId || amount));
            }
        }
    }

    const bool empty = r.rows.isEmpty();
    m_table->setVisible(!empty);
    m_empty->setVisible(empty);
    m_empty->setText(r.emptyText);
    m_footLabel->setText(r.footerLabel);
    m_footValue->setText(r.footerValue);
    m_footExtra->setText(r.footerExtra);
    m_footLabel->parentWidget()->setVisible(!empty);
}

// ======================================================================= exports
void ReportsPage::exportCsv()
{
    if (m_current.rows.isEmpty())
    {
        emit toast("Nothing to export - this report is empty");
        return;
    }
    QStringList lines;
    QStringList head;
    for (const QString &h : m_current.headers)
        head << csvField(h);
    lines << head.join(',');
    for (const QStringList &row : m_current.rows)
    {
        QStringList cells;
        for (const QString &c : row)
            cells << csvField(c);
        lines << cells.join(',');
    }
    QString slug = m_current.title.toLower();
    slug.replace(' ', '-');
    if (writeCsv(this, "report-" + slug + ".csv", lines))
        emit toast(m_current.title + " exported");
}

void ReportsPage::printPdf()
{
    if (m_current.rows.isEmpty())
    {
        emit toast("Nothing to print - this report is empty");
        return;
    }
    QString slug = m_current.title.toLower();
    slug.replace(' ', '-');
    const QString path = QFileDialog::getSaveFileName(this, "Save report as PDF", "report-" + slug + ".pdf", "PDF (*.pdf)");
    if (path.isEmpty())
        return;

    QString html = "<h2 style='color:#0f1b33'>" + htmlEscape(m_current.title) + "</h2><p style='color:#64708a'>" +
                   htmlEscape(m_current.subtitle) + "  -  printed " + QDateTime::currentDateTime().toString("dd-MMM-yyyy HH:mm") +
                   "</p><table width='100%' cellspacing='0' cellpadding='5' style='border-collapse:collapse'><tr>";
    for (int c = 0; c < m_current.headers.size(); ++c)
        html += QString("<th align='%1' style='background:#eaf1ff;border-bottom:1px solid #c3d5ff'>%2</th>")
                    .arg(m_current.rightCols.contains(c) ? "right" : "left", htmlEscape(m_current.headers[c]));
    html += "</tr>";
    for (const QStringList &row : m_current.rows)
    {
        html += "<tr>";
        for (int c = 0; c < row.size(); ++c)
            html += QString("<td align='%1' style='border-bottom:1px solid #e3eaf8'>%2</td>")
                        .arg(m_current.rightCols.contains(c) ? "right" : "left", htmlEscape(row[c]));
        html += "</tr>";
    }
    html += "</table><p><b>" + htmlEscape(m_current.footerLabel) + " " + htmlEscape(m_current.footerValue) + "</b>  " +
            htmlEscape(m_current.footerExtra) + "</p>";

    QPdfWriter pdf(path);
    pdf.setPageSize(QPageSize(QPageSize::A4));
    pdf.setPageMargins(QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);
    pdf.setTitle(m_current.title);
    pdf.setCreator("Syntro-Check");
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&pdf);
    emit toast(QFileInfo(path).exists() ? m_current.title + " saved as PDF" : QString("The PDF could not be written"));
}
