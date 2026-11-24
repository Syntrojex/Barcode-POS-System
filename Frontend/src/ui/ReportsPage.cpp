#include "ReportsPage.h"
#include "AppState.h"
#include "Theme.h"
#include "UiKit.h"
#include "Widgets.h"
#include <QDateEdit>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QTableWidget>
#include <QTextDocument>
#include <QVBoxLayout>
#include <algorithm>

using namespace Ui;

namespace
{
    QString cap(const QString &s) { return s.isEmpty() ? s : s.left(1).toUpper() + s.mid(1).toLower(); }
    bool isCash(const OrderRecord &o) { return o.payment.compare("cash", Qt::CaseInsensitive) == 0; }
    double lineNet(const ReceiptLine &l) { return (l.price - l.discount + l.tax) * l.qty; }

    const char *kTabNames[] = {"Incomplete Orders", "Completed Orders", "Cash Sales", "All Sales", "Top 10 Sold",
                               "Top 10 Remaining Stock"};
    const char *kPeriodNames[] = {"All", "Daily", "Weekly", "Monthly", "Yearly"};

    void clearLayout(QLayout *l)
    {
        while (QLayoutItem *it = l->takeAt(0))
        {
            if (QWidget *w = it->widget())
                w->deleteLater();
            delete it;
        }
    }
}

// ============================================================================ page
ReportsPage::ReportsPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root;
    pageShell(this, root);
    root->setSpacing(12);

    auto *csv = W::button("Export CSV Data", "soft", "export");
    auto *pdf = W::button("Export / Print as PDF  Ctrl+P", "primary", "print");
    root->addWidget(sectionHeader("SHIFT", "Business Reports & Analytics",
                                  "Audit register velocity, cashier cash reconciliation and product turnover", {csv, pdf}));
    connect(csv, &QPushButton::clicked, this, &ReportsPage::exportCsv);
    connect(pdf, &QPushButton::clicked, this, &ReportsPage::exportPdf);
    auto *print = new QShortcut(QKeySequence("Ctrl+P"), this);
    print->setContext(Qt::WidgetWithChildrenShortcut);
    connect(print, &QShortcut::activated, this, &ReportsPage::exportPdf);

    // ---- tabs
    auto *tabCard = W::card(this);
    auto *th = new QHBoxLayout(tabCard);
    th->setContentsMargins(12, 8, 12, 8);
    th->setSpacing(6);
    for (int i = 0; i < TabCount; ++i)
    {
        auto *b = W::button(kTabNames[i], "tab");
        b->setCheckable(true);
        th->addWidget(b);
        m_tabButtons << b;
        connect(b, &QPushButton::clicked, this, [this, i] { setTab(i); });
    }
    th->addStretch();
    root->addWidget(tabCard);

    // ---- summary tiles
    m_tiles = new QHBoxLayout;
    m_tiles->setSpacing(12);
    root->addLayout(m_tiles);

    // ---- period bar (cash sales / all sales / completed / top sold)
    m_periodBar = W::card(this);
    auto *ph = new QHBoxLayout(m_periodBar);
    ph->setContentsMargins(14, 8, 14, 8);
    ph->setSpacing(6);
    ph->addWidget(W::label("TIME PERIOD", "caption"));
    for (int i = 0; i < 5; ++i)
    {
        auto *b = W::button(kPeriodNames[i], "tab");
        b->setCheckable(true);
        ph->addWidget(b);
        m_periodButtons << b;
        connect(b, &QPushButton::clicked, this, [this, i] { setPeriod(i); });
    }
    ph->addSpacing(16);
    ph->addWidget(W::label("CUSTOM RANGE", "caption"));
    m_fromEdit = new QDateEdit(QDate::currentDate());
    m_toEdit = new QDateEdit(QDate::currentDate());
    for (auto *e : {m_fromEdit, m_toEdit})
    {
        e->setCalendarPopup(true);
        e->setDisplayFormat("dd-MMM-yyyy");
        e->setMinimumHeight(36);
    }
    ph->addWidget(m_fromEdit);
    ph->addWidget(W::label("to", "muted"));
    ph->addWidget(m_toEdit);
    auto *filter = W::button("Filter", "primary", "filter");
    ph->addWidget(filter);
    ph->addStretch();
    connect(filter, &QPushButton::clicked, this, [this]
            { m_from = m_fromEdit->date(); m_to = m_toEdit->date(); if (m_to < m_from) std::swap(m_from, m_to); setPeriod(Custom); });
    root->addWidget(m_periodBar);

    // ---- ledger + side panel
    auto *body = new QHBoxLayout;
    body->setSpacing(12);
    auto *ledger = W::card(this);
    auto *lv = new QVBoxLayout(ledger);
    lv->setContentsMargins(0, 14, 0, 0);
    lv->setSpacing(8);
    auto *lh = new QHBoxLayout;
    lh->setContentsMargins(18, 0, 18, 0);
    m_ledgerTitle = W::label("", "h1");
    m_ledgerSub = W::label("", "muted");
    lh->addWidget(m_ledgerTitle);
    lh->addStretch();
    lh->addWidget(m_ledgerSub);
    lv->addLayout(lh);
    m_table = makeTable({"-"});
    lv->addWidget(m_table, 1);
    auto *foot = new QFrame;
    foot->setStyleSheet("QFrame{background:#0f1b33; border-bottom-left-radius:14px; border-bottom-right-radius:14px;}"
                        "QLabel{background:transparent; color:white;}");
    auto *fh = new QHBoxLayout(foot);
    fh->setContentsMargins(18, 12, 18, 12);
    m_footLabel = W::label("");
    m_footLabel->setStyleSheet("font-weight:700; color:#c9d6f5; background:transparent;");
    m_footValue = W::label("");
    m_footValue->setStyleSheet("font-size:20px; font-weight:800; color:#7ffc97; background:transparent;");
    m_footNote = W::label("");
    m_footNote->setStyleSheet("color:#c9d6f5; background:transparent;");
    fh->addWidget(m_footLabel);
    fh->addWidget(m_footValue);
    fh->addStretch();
    fh->addWidget(m_footNote);
    lv->addWidget(foot);
    body->addWidget(ledger, 7);

    m_side = W::card(this);
    auto *sv = new QVBoxLayout(m_side);
    sv->setContentsMargins(16, 14, 16, 14);
    sv->setSpacing(8);
    sv->addWidget(W::label("VELOCITY MATRIX", "caption"));
    sv->addWidget(W::label("Top sold staples", "h1"));
    auto *listHost = new QWidget;
    m_sideList = new QVBoxLayout(listHost);
    m_sideList->setContentsMargins(0, 0, 0, 0);
    m_sideList->setSpacing(8);
    sv->addWidget(listHost);
    sv->addStretch();
    body->addWidget(m_side, 3);
    root->addLayout(body, 1);

    connect(&AppState::instance(), &AppState::changed, this, &ReportsPage::refresh);
    setPeriod(All);
    setTab(CashSales);
}

void ReportsPage::refresh() { render(); }

void ReportsPage::setTab(int tab)
{
    m_tab = tab;
    for (int i = 0; i < m_tabButtons.size(); ++i)
        m_tabButtons[i]->setChecked(i == tab);
    m_periodBar->setVisible(tab != Incomplete); // outstanding orders are shown whatever their date
    render();
}

void ReportsPage::setPeriod(int period)
{
    m_period = period;
    const QDate today = QDate::currentDate();
    switch (period)
    {
    case Daily: m_from = today; m_to = today; break;
    case Weekly: m_from = today.addDays(-(today.dayOfWeek() - 1)); m_to = today; break;
    case Monthly: m_from = QDate(today.year(), today.month(), 1); m_to = today; break;
    case Yearly: m_from = QDate(today.year(), 1, 1); m_to = today; break;
    case Custom: break; // m_from / m_to were set by the Filter button
    default: m_from = QDate(); m_to = QDate(); break;
    }
    if (period != All && period != Custom)
    {
        m_fromEdit->setDate(m_from);
        m_toEdit->setDate(m_to);
    }
    for (int i = 0; i < m_periodButtons.size(); ++i)
        m_periodButtons[i]->setChecked(i == period);
    render();
}

QString ReportsPage::periodText() const
{
    if (m_period == All || !m_from.isValid())
        return "All dates";
    if (m_from == m_to)
        return m_from.toString("dd-MMM-yyyy");
    return m_from.toString("dd-MMM-yyyy") + " to " + m_to.toString("dd-MMM-yyyy");
}

// ============================================================================ data
QVector<ReportsPage::TopItem> ReportsPage::topSold(int limit) const
{
    auto &app = AppState::instance();
    QVector<TopItem> items;
    for (const auto &o : app.orders())
    {
        if (o.status != "Paid")
            continue;
        const QDate d = o.time.date();
        if (m_from.isValid() && (d < m_from || d > m_to))
            continue;
        for (const auto &l : o.lines)
        {
            auto it = std::find_if(items.begin(), items.end(), [&](const TopItem &t) { return t.acronym == l.acronym; });
            if (it == items.end())
            {
                TopItem t;
                t.name = l.name;
                t.acronym = l.acronym;
                items.push_back(t);
                it = items.end() - 1;
            }
            it->units += l.qty;
            it->revenue += lineNet(l);
        }
    }
    for (auto &t : items)
        if (const auto p = app.findByCode(t.acronym))
            t.stock = p->stock;
    std::stable_sort(items.begin(), items.end(), [](const TopItem &a, const TopItem &b) { return a.units > b.units; });
    if (items.size() > limit)
        items.resize(limit);
    return items;
}

ReportsPage::Report ReportsPage::build(int tab) const
{
    auto &app = AppState::instance();
    const auto &all = app.orders();
    const int low = app.settings().lowStockThreshold;
    auto inRange = [&](const OrderRecord &o)
    {
        if (!m_from.isValid())
            return true;
        const QDate d = o.time.date();
        return d >= m_from && d <= m_to;
    };
    const QString dt = "yyyy-MM-dd HH:mm";
    Report r;
    double sum = 0, disc = 0;
    int n = 0;

    switch (tab)
    {
    case Incomplete:
    {
        r.title = "Incomplete Orders";
        r.subtitle = "Orders that are not paid and not archived";
        r.headers = {"ORDER ID", "DATE & TIME", "CUSTOMER", "ITEMS", "STATUS", "AMOUNT"};
        r.right = {false, false, false, false, false, true};
        r.accent = {0, 5};
        int hold = 0;
        for (const auto &o : all)
            if (o.status == "In Progress" || o.status == "On Hold")
            {
                r.rows.push_back({"#" + o.id, o.time.toString(dt), o.customer, o.summary(), o.status, W::money(o.total)});
                sum += o.total;
                ++n;
                if (o.status == "On Hold")
                    ++hold;
            }
        r.tiles = {{"INCOMPLETE ORDERS", "orders", QString::number(n), "not paid, not archived", "amber"},
                   {"OUTSTANDING AMOUNT", "bills", W::money(sum), "total of all incomplete bills", "blue"},
                   {"ON HOLD", "pause", QString::number(hold), QString("%1 in progress").arg(n - hold), "green"}};
        r.footLabel = "Total of incomplete bills:";
        r.footValue = W::money(sum);
        r.footNote = QString("%1 order%2").arg(n).arg(n == 1 ? "" : "s");
        break;
    }
    case Completed:
    {
        r.title = "Completed Orders";
        r.subtitle = "Paid and archived orders  •  " + periodText();
        r.headers = {"ORDER ID", "DATE & TIME", "CUSTOMER", "PAYMENT", "ITEMS", "STATUS", "AMOUNT"};
        r.right = {false, false, false, false, false, false, true};
        r.accent = {0, 6};
        int archived = 0;
        for (const auto &o : all)
        {
            if ((o.status != "Paid" && o.status != "Archived") || !inRange(o))
                continue;
            r.rows.push_back({"#" + o.id, o.time.toString(dt), o.customer, cap(o.payment), o.summary(), o.status, W::money(o.total)});
            if (o.status == "Paid")
            {
                sum += o.total;
                ++n;
            }
            else
                ++archived;
        }
        r.tiles = {{"COMPLETED (PAID)", "check", QString::number(n), periodText(), "green"},
                   {"TOTAL AMOUNT", "bills", W::money(sum), "paid orders only", "blue"},
                   {"ARCHIVED", "history", QString::number(archived), "replaced by an edited order, not counted", "amber"}};
        r.footLabel = "Total amount of completed bills:";
        r.footValue = W::money(sum);
        r.footNote = QString("%1 paid  •  %2 archived (not counted)").arg(n).arg(archived);
        break;
    }
    case CashSales:
    case AllSales:
    {
        const bool cash = (tab == CashSales);
        r.title = cash ? "Cash Sales Ledger" : "All Sales Ledger";
        r.subtitle = periodText();
        r.showTopSold = true;
        if (cash)
        {
            r.headers = {"DATE & TIME", "ORDER ID", "CUSTOMER", "ITEMS", "AMOUNT", "DISCOUNT"};
            r.right = {false, false, false, false, true, true};
            r.accent = {1, 4};
        }
        else
        {
            r.headers = {"DATE & TIME", "ORDER ID", "CUSTOMER", "PAYMENT", "ITEMS", "AMOUNT", "DISCOUNT"};
            r.right = {false, false, false, false, false, true, true};
            r.accent = {1, 5};
        }
        int cashN = 0, cardN = 0, walletN = 0;
        for (const auto &o : all)
        {
            if (o.status != "Paid" || !inRange(o))
                continue;
            if (cash && !isCash(o))
                continue;
            if (isCash(o)) ++cashN;
            else if (o.payment.compare("card", Qt::CaseInsensitive) == 0) ++cardN;
            else ++walletN;
            if (cash)
                r.rows.push_back({o.time.toString(dt), "#" + o.id, o.customer, QString::number(o.itemCount()),
                                  W::money(o.total), W::money(-o.discount)});
            else
                r.rows.push_back({o.time.toString(dt), "#" + o.id, o.customer, cap(o.payment), QString::number(o.itemCount()),
                                  W::money(o.total), W::money(-o.discount)});
            sum += o.total;
            disc += o.discount;
            ++n;
        }
        const double avgDisc = (sum + disc) > 0 ? disc * 100.0 / (sum + disc) : 0.0;
        r.tiles = {{cash ? "TOTAL CASH SALES" : "TOTAL SALES", "cash", W::money(sum), QString("%1 order%2").arg(n).arg(n == 1 ? "" : "s"), "blue"},
                   cash ? Tile{"AVERAGE CASH BILL", "bills", W::money(n ? sum / n : 0.0), "per cash order", "green"}
                        : Tile{"PAYMENT MIX", "card", QString("%1 / %2 / %3").arg(cashN).arg(cardN).arg(walletN), "cash / card / wallet", "green"},
                   {"DISCOUNTS GRANTED", "percent", W::money(disc), QString("%1% of gross").arg(avgDisc, 0, 'f', 1), "red"}};
        r.footLabel = cash ? "Cash sales total:" : "Sales total:";
        r.footValue = W::money(sum);
        r.footNote = QString("Discounts given: %1  •  %2 orders").arg(W::money(disc)).arg(n);
        break;
    }
    case TopSold:
    {
        r.title = "Top 10 Most Sold Items";
        r.subtitle = "Descending by units sold  •  " + periodText();
        r.headers = {"RANK", "PRODUCT", "ACRONYM", "UNITS SOLD", "REVENUE", "STOCK LEFT", "STATUS"};
        r.right = {false, false, false, true, true, true, false};
        r.accent = {2, 3};
        const auto items = topSold(10);
        int units = 0, lowCount = 0;
        for (int i = 0; i < items.size(); ++i)
        {
            const auto &t = items[i];
            const QString status = t.stock <= 0 ? "Out of stock" : t.stock <= low ? "Low stock" : "OK";
            if (t.stock <= low)
                ++lowCount;
            r.rows.push_back({QString::number(i + 1), t.name, t.acronym, QString::number(t.units), W::money(t.revenue),
                              QString::number(t.stock), status});
            units += t.units;
            sum += t.revenue;
        }
        r.tiles = {{"BEST SELLER", "star", items.isEmpty() ? QString("-") : items[0].acronym,
                    items.isEmpty() ? QString("no sales in this period") : QString("%1 units sold").arg(items[0].units), "blue"},
                   {"UNITS SOLD (TOP 10)", "cart", QString::number(units), "across the ten items", "green"},
                   {"NEED RESTOCK", "warn", QString::number(lowCount), "top sellers with low or no stock", lowCount ? "red" : "amber"}};
        r.footLabel = "Revenue of the top ten:";
        r.footValue = W::money(sum);
        r.footNote = QString("%1 units").arg(units);
        break;
    }
    case TopRemaining:
    {
        r.title = "Top 10 Items by Remaining Stock";
        r.subtitle = "Descending by stock left  -  no need to purchase these now  •  sales: " + periodText();
        r.headers = {"RANK", "PRODUCT", "ACRONYM", "STOCK LEFT", "SOLD (PERIOD)", "ADVICE"};
        r.right = {false, false, false, true, true, false};
        r.accent = {2, 3};
        QVector<ProductInfo> products = app.products();
        std::stable_sort(products.begin(), products.end(), [](const ProductInfo &a, const ProductInfo &b) { return a.stock > b.stock; });
        if (products.size() > 10)
            products.resize(10);
        const auto sold = topSold(100000);
        int stockSum = 0;
        for (int i = 0; i < products.size(); ++i)
        {
            const auto &p = products[i];
            int s = 0;
            for (const auto &t : sold)
                if (t.acronym == p.sku)
                    s = t.units;
            r.rows.push_back({QString::number(i + 1), p.name, p.sku, QString::number(p.stock), QString::number(s), "Do not re-order yet"});
            stockSum += p.stock;
        }
        r.tiles = {{"MOST STOCKED", "inventory", products.isEmpty() ? QString("-") : products[0].sku,
                    products.isEmpty() ? QString() : QString("%1 units on hand").arg(products[0].stock), "blue"},
                   {"UNITS ON HAND (TOP 10)", "products", QString::number(stockSum), "held by the ten items", "green"},
                   {"CATALOGUE", "barcode", QString::number(app.products().size()), "products in Product.txt", "amber"}};
        r.footLabel = "Units on hand (top ten):";
        r.footValue = QString::number(stockSum);
        r.footNote = "Purchase of these items should not be started";
        break;
    }
    }
    return r;
}

// ============================================================================ view
void ReportsPage::render()
{
    m_report = build(m_tab);
    const Report &r = m_report;

    clearLayout(m_tiles);
    for (const auto &t : r.tiles)
    {
        QLabel *value = nullptr, *sub = nullptr;
        auto *tile = statTile(t.caption, t.icon, value, sub, t.tint);
        value->setText(t.value);
        sub->setText(t.sub);
        m_tiles->addWidget(tile, 1);
    }

    m_ledgerTitle->setText(r.title);
    m_ledgerSub->setText(r.subtitle);
    m_table->clear();
    m_table->setColumnCount(r.headers.size());
    m_table->setHorizontalHeaderLabels(r.headers);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setRowCount(r.rows.size());
    for (int row = 0; row < r.rows.size(); ++row)
    {
        m_table->setRowHeight(row, 46);
        for (int c = 0; c < r.headers.size() && c < r.rows[row].size(); ++c)
        {
            const bool accent = r.accent.contains(c);
            const Qt::Alignment a = (r.right.value(c) ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter;
            m_table->setItem(row, c, item(r.rows[row][c], a, accent ? "#004ac6" : "", accent));
        }
    }
    m_footLabel->setText(r.footLabel);
    m_footValue->setText(r.footValue);
    m_footNote->setText(r.footNote);

    // top sold staples (side panel)
    m_side->setVisible(r.showTopSold);
    if (r.showTopSold)
    {
        clearLayout(m_sideList);
        const auto items = topSold(6);
        const int low = AppState::instance().settings().lowStockThreshold;
        const int maxUnits = items.isEmpty() ? 1 : qMax(1, items[0].units);
        if (items.isEmpty())
            m_sideList->addWidget(W::label("No sales in this period.", "muted"));
        for (int i = 0; i < items.size(); ++i)
        {
            const auto &t = items[i];
            auto *rowW = new QWidget;
            auto *v = new QVBoxLayout(rowW);
            v->setContentsMargins(0, 0, 0, 0);
            v->setSpacing(3);
            auto *h = new QHBoxLayout;
            h->setSpacing(8);
            h->addWidget(W::badge(QString::number(i + 1), "blue"));
            auto *name = W::label(t.name);
            name->setStyleSheet("font-weight:700;");
            h->addWidget(name, 1);
            h->addWidget(W::label(QString("%1 sold").arg(t.units), "muted"));
            h->addWidget(W::badge(t.stock <= 0 ? "DEPLETED" : QString("Stock: %1").arg(t.stock),
                                  t.stock <= 0 ? "red" : t.stock <= low ? "amber" : "grey"));
            v->addLayout(h);
            auto *bar = new QProgressBar;
            bar->setRange(0, maxUnits);
            bar->setValue(t.units);
            bar->setTextVisible(false);
            bar->setFixedHeight(8);
            bar->setStyleSheet("QProgressBar{background:#dce9ff; border:none; border-radius:4px;}"
                               "QProgressBar::chunk{background:#2554d8; border-radius:4px;}");
            v->addWidget(bar);
            m_sideList->addWidget(rowW);
        }
    }
}

// ============================================================================ export
void ReportsPage::exportCsv()
{
    const Report &r = m_report;
    QStringList rows;
    QStringList head;
    for (const auto &h : r.headers)
        head << csvField(h);
    rows << head.join(',');
    for (const auto &row : r.rows)
    {
        QStringList cells;
        for (const auto &c : row)
            cells << csvField(c);
        rows << cells.join(',');
    }
    rows << QString();
    rows << csvField(r.footLabel) + "," + csvField(r.footValue);
    QString name = r.title.toLower();
    name.replace(' ', '-');
    if (writeCsv(this, "report-" + name + ".csv", rows))
        emit toast("Report exported as CSV");
}

void ReportsPage::exportPdf()
{
    const Report &r = m_report;
    QString name = r.title.toLower();
    name.replace(' ', '-');
    const QString path = QFileDialog::getSaveFileName(this, "Export / Print report as PDF", "report-" + name + ".pdf", "PDF (*.pdf)");
    if (path.isEmpty())
        return;
    QString html = QString("<h2 style='margin-bottom:2px'>%1</h2><p style='color:#555'>%2 &nbsp;|&nbsp; printed %3</p>"
                           "<table width='100%' border='1' cellspacing='0' cellpadding='5' style='border-collapse:collapse'><tr>")
                       .arg(r.title.toHtmlEscaped(), r.subtitle.toHtmlEscaped(), QDateTime::currentDateTime().toString("dd-MMM-yyyy HH:mm"));
    for (const auto &h : r.headers)
        html += "<th bgcolor='#dce9ff' align='left'>" + h.toHtmlEscaped() + "</th>";
    html += "</tr>";
    for (const auto &row : r.rows)
    {
        html += "<tr>";
        for (int c = 0; c < row.size(); ++c)
            html += QString("<td align='%1'>%2</td>").arg(r.right.value(c) ? "right" : "left", row[c].toHtmlEscaped());
        html += "</tr>";
    }
    html += QString("</table><p><b>%1 %2</b> &nbsp; %3</p>")
                .arg(r.footLabel.toHtmlEscaped(), r.footValue.toHtmlEscaped(), r.footNote.toHtmlEscaped());
    QPdfWriter pdf(path);
    pdf.setPageSize(QPageSize(QPageSize::A4));
    pdf.setPageOrientation(QPageLayout::Landscape);
    pdf.setPageMargins(QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);
    pdf.setCreator("Syntro-Check");
    pdf.setTitle(r.title);
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&pdf);
    emit toast("Report saved as PDF");
}
