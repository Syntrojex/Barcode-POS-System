#include "ReportsPage.h"
#include "AppState.h"
#include "Theme.h"
#include "UiKit.h"
#include "Widgets.h"
#include "billing/BillRepository.h"
#include "common/RecordText.h"
#include <QDateEdit>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHash>
#include <QLabel>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QProgressBar>
#include <QPushButton>
#include <QSaveFile>
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
    QString quantity(double value)
    {
        return QString::fromStdString(barcodehub::common::formatQuantity(value));
    }

    QDateTime reportTime(const OrderRecord &order)
    {
        return order.status == "Paid" || order.status == "Archived" ? order.paymentTime() : order.time;
    }

    std::vector<barcodehub::billing::Bill> savedBills(const AppState &app)
    {
        std::vector<barcodehub::billing::Bill> bills;
        if (!app.dataDir().isEmpty())
        {
            const auto path = QDir(app.dataDir()).filePath("Bill.txt");
            if (!barcodehub::billing::BillRepository(path.toStdWString()).loadAll(bills))
                bills.clear();
        }
        return bills;
    }

    const char *kTabNames[] = {"Incomplete Orders", "Completed Orders", "Cash Sales", "All Sales", "Top 10 Sold",
                               "Top 10 Remaining Stock"};
    const char *kPeriodNames[] = {"All", "Daily", "Weekly", "Monthly", "Yearly"};

    void clearLayout(QLayout *l)
    {
        while (QLayoutItem *it = l->takeAt(0))
        {
            if (QWidget *w = it->widget())
            {
                w->hide();
                w->deleteLater();
            }
            delete it;
        }
    }
}

// ============================================================================ page
ReportsPage::ReportsPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root;
    pageShell(this, root, true);
    root->setSpacing(8);

    auto *csv = W::button("Export CSV", "soft", "export");
    auto *pdf = W::button("Export / Print PDF", "primary", "print");
    pdf->setToolTip("Export the current report to PDF (Ctrl+P)");
    root->addWidget(sectionHeader({}, "Business Reports & Analytics",
                                  "Audit orders, sales, and product turnover.", {csv, pdf}));
    connect(csv, &QPushButton::clicked, this, &ReportsPage::exportCsv);
    connect(pdf, &QPushButton::clicked, this, &ReportsPage::exportPdf);
    auto *print = new QShortcut(QKeySequence("Ctrl+P"), this);
    print->setContext(Qt::WidgetWithChildrenShortcut);
    connect(print, &QShortcut::activated, this, &ReportsPage::exportPdf);

    // ---- tabs
    auto *tabCard = W::card(this, "subCard");
    auto *th = new QHBoxLayout(tabCard);
    th->setContentsMargins(12, 8, 12, 8);
    th->setSpacing(6);
    for (int i = 0; i < TabCount; ++i)
    {
        auto *b = W::button(kTabNames[i], "tab");
        b->setProperty("reportTab", true);
        b->setCheckable(true);
        th->addWidget(b, 1);
        m_tabButtons << b;
        connect(b, &QPushButton::clicked, this, [this, i] { setTab(i); });
    }
    root->addWidget(tabCard);

    // ---- summary tiles
    m_tiles = new QHBoxLayout;
    m_tiles->setSpacing(8);
    root->addLayout(m_tiles);

    // Keep the period buttons and custom dates on separate rows at minimum width.
    m_periodBar = W::card(this, "subCard");
    auto *periodLayout = new QVBoxLayout(m_periodBar);
    periodLayout->setContentsMargins(12, 8, 12, 8);
    periodLayout->setSpacing(6);
    auto *ph = new QHBoxLayout;
    ph->setSpacing(8);
    ph->addWidget(W::label("TIME PERIOD", "caption"));
    for (int i = 0; i < 5; ++i)
    {
        auto *b = W::button(kPeriodNames[i], "tab");
        b->setProperty("reportTab", true);
        b->setCheckable(true);
        ph->addWidget(b);
        m_periodButtons << b;
        connect(b, &QPushButton::clicked, this, [this, i] { setPeriod(i); });
    }
    ph->addStretch();
    periodLayout->addLayout(ph);
    auto *range = new QHBoxLayout;
    range->setSpacing(8);
    range->addWidget(W::label("CUSTOM RANGE", "caption"));
    m_fromEdit = dateEdit(QDate::currentDate(), "dd MMM yyyy", 34);
    m_toEdit = dateEdit(QDate::currentDate(), "dd MMM yyyy", 34);
    m_fromEdit->setAccessibleName("Report start date");
    m_toEdit->setAccessibleName("Report end date");
    range->addWidget(m_fromEdit);
    range->addWidget(W::label("to", "muted"));
    range->addWidget(m_toEdit);
    auto *apply = W::button("Filter", "primary", "filter");
    range->addWidget(apply);
    range->addStretch();
    periodLayout->addLayout(range);
    connect(apply, &QPushButton::clicked, this, [this]
        {
            m_from = m_fromEdit->date();
            m_to = m_toEdit->date();
            if (m_to < m_from)
                std::swap(m_from, m_to);
            setPeriod(Custom);
        });
    root->addWidget(m_periodBar);

    m_outstandingNotice = W::card(this, "subCard");
    auto *notice = new QVBoxLayout(m_outstandingNotice);
    notice->setContentsMargins(12, 8, 12, 8);
    notice->setSpacing(2);
    notice->addWidget(W::label("OUTSTANDING ORDERS · ALL DATES", "caption"));
    notice->addWidget(W::label("Open and held orders are shown regardless of when they were created.", "muted"));
    root->addWidget(m_outstandingNotice);

    // ---- ledger + side panel
    auto *body = new QHBoxLayout;
    body->setSpacing(8);
    auto *ledger = W::card(this, "subCard");
    auto *lv = new QVBoxLayout(ledger);
    lv->setContentsMargins(12, 12, 12, 12);
    lv->setSpacing(8);
    auto *lh = new QVBoxLayout;
    lh->setSpacing(3);
    m_ledgerTitle = W::label("", "h1");
    m_ledgerSub = W::label("", "muted");
    m_ledgerSub->setWordWrap(true);
    lh->addWidget(m_ledgerTitle);
    lh->addWidget(m_ledgerSub);
    lv->addLayout(lh);
    m_table = makeTable({"-"});
    m_table->setProperty("managementTable", true);
    m_table->setAlternatingRowColors(true);
    m_table->setWordWrap(false);
    m_table->setAccessibleName("Report data");
    m_table->setMinimumHeight(150);
    lv->addWidget(m_table, 1);
    m_empty = W::label("No records in this report.", "muted");
    m_empty->setAlignment(Qt::AlignCenter);
    lv->addWidget(m_empty);
    auto *foot = W::card(ledger, "totalBox");
    auto *fh = new QHBoxLayout(foot);
    fh->setContentsMargins(12, 8, 12, 8);
    fh->setSpacing(12);
    m_footLabel = W::label("");
    m_footLabel->setStyleSheet("font-weight:700; color:#c9d6f5; background:transparent;");
    m_footValue = W::label("");
    m_footValue->setStyleSheet("font-size:20px; font-weight:800; color:#7ffc97; background:transparent;");
    m_footNote = W::label("");
    m_footNote->setWordWrap(true);
    m_footNote->setStyleSheet("color:#c9d6f5; background:transparent;");
    fh->addWidget(m_footLabel);
    fh->addWidget(m_footValue);
    fh->addWidget(m_footNote, 1);
    lv->addWidget(foot);
    body->addWidget(ledger, 7);

    m_side = W::card(this, "subCard");
    auto *sv = new QVBoxLayout(m_side);
    sv->setContentsMargins(12, 12, 12, 12);
    sv->setSpacing(8);
    sv->addWidget(W::label("Top sold staples", "h1"));
    auto *sideDescription = W::label("Units sold in the selected period", "muted");
    sideDescription->setWordWrap(true);
    sv->addWidget(sideDescription);
    auto *listHost = new QWidget;
    m_sideList = new QVBoxLayout(listHost);
    m_sideList->setContentsMargins(0, 0, 0, 0);
    m_sideList->setSpacing(8);
    sv->addWidget(listHost);
    sv->addStretch();
    body->addWidget(m_side, 3);
    root->addLayout(body, 1);

    connect(&AppState::instance(), &AppState::changed, this, &ReportsPage::refresh);
    m_periodButtons[All]->setChecked(true);
    setTab(Incomplete);
}

void ReportsPage::refresh() { render(); }

void ReportsPage::setTab(int tab)
{
    m_tab = tab;
    for (int i = 0; i < m_tabButtons.size(); ++i)
        m_tabButtons[i]->setChecked(i == tab);
    m_periodBar->setVisible(tab != Incomplete); // outstanding orders are shown whatever their date
    m_outstandingNotice->setVisible(tab == Incomplete);
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
    m_fromEdit->setDate(m_from.isValid() ? m_from : today);
    m_toEdit->setDate(m_to.isValid() ? m_to : today);
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
QVector<ReportsPage::TopItem> ReportsPage::topSold(int limit, bool cashOnly) const
{
    auto &app = AppState::instance();
    // The checkout view model uses integer quantities. Read the saved bill lines
    // through the existing repository so a weight sale keeps its decimal quantity.
    const auto bills = savedBills(app);
    QHash<QString, const barcodehub::billing::Bill *> billsById;
    for (const auto &bill : bills)
        billsById.insert(QString::fromStdString(bill.id), &bill);

    QVector<TopItem> items;
    QHash<QString, qsizetype> byAcronym;
    auto addLine = [&](const QString &acronym, const QString &name, double qty, double value)
    {
        const auto key = acronym.toUpper();
        if (!byAcronym.contains(key))
        {
            byAcronym.insert(key, items.size());
            items.push_back({name, acronym});
        }
        auto &item = items[byAcronym.value(key)];
        item.units += qty;
        item.lineValue += value;
    };
    for (const auto &o : app.orders())
    {
        if (o.status != "Paid" || (cashOnly && !isCash(o)))
            continue;
        const QDate d = o.paymentTime().date();
        if (m_from.isValid() && (d < m_from || d > m_to))
            continue;
        if (const auto *bill = billsById.value(o.billId); bill && !bill->lines.empty())
        {
            for (const auto &line : bill->lines)
            {
                const auto acronym = QString::fromStdString(line.productAcronym);
                addLine(acronym, acronym, line.quantity, (line.unitPrice - line.discountPerUnit) * line.quantity);
            }
        }
        else
            for (const auto &line : o.lines)
                addLine(line.acronym, line.name, line.qty, (line.price - line.discount) * line.qty);
    }
    for (auto &t : items)
        if (const auto p = app.findByCode(t.acronym))
        {
            t.stock = p->stock;
            t.name = p->name;
        }
    std::stable_sort(items.begin(), items.end(), [](const TopItem &a, const TopItem &b) { return a.units > b.units; });
    if (limit > 0 && items.size() > limit)
        items.resize(limit);
    return items;
}

ReportsPage::Report ReportsPage::build(int tab, const QHash<QString, double> &paidUnits) const
{
    auto &app = AppState::instance();
    const auto &all = app.orders();
    const int low = app.settings().lowStockThreshold;
    auto inRange = [&](const OrderRecord &o)
    {
        if (!m_from.isValid())
            return true;
        const QDate d = reportTime(o).date();
        return d >= m_from && d <= m_to;
    };
    const QString dt = "dd MMM yyyy HH:mm";
    Report r;
    double sum = 0, disc = 0;
    int n = 0;

    switch (tab)
    {
    case Incomplete:
    {
        r.title = "Incomplete Orders";
        r.subtitle = "Open and held orders · All dates";
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
        r.tiles = {{"INCOMPLETE ORDERS", "orders", QString::number(n), "not paid, not archived"},
                   {"OUTSTANDING AMOUNT", "bills", W::money(sum), "total of all incomplete bills"},
                   {"ON HOLD", "pause", QString::number(hold), QString("%1 in progress").arg(n - hold)}};
        r.footLabel = "Outstanding total:";
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
            const double units = paidUnits.value(o.billId, o.itemCount());
            r.rows.push_back({"#" + o.id, reportTime(o).toString(dt), o.customer, cap(o.payment), quantity(units), o.status, W::money(o.total)});
            if (o.status == "Paid")
            {
                sum += o.total;
                ++n;
            }
            else
                ++archived;
        }
        r.tiles = {{"COMPLETED (PAID)", "check", QString::number(n), periodText()},
                   {"TOTAL AMOUNT", "bills", W::money(sum), "paid orders only"},
                   {"ARCHIVED", "history", QString::number(archived), "Excluded from paid total"}};
        r.footLabel = "Paid total:";
        r.footValue = W::money(sum);
        r.footNote = QString("%1 paid  •  %2 archived (not counted)").arg(n).arg(archived);
        break;
    }
    case CashSales:
    case AllSales:
    {
        const bool cash = (tab == CashSales);
        r.title = cash ? "Cash Sales Ledger" : "All Sales Ledger";
        r.subtitle = (cash ? "Paid cash orders · " : "All paid payment methods · ") + periodText();
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
            else if (o.payment.compare("wallet", Qt::CaseInsensitive) == 0) ++walletN;
            const QString units = quantity(paidUnits.value(o.billId, o.itemCount()));
            if (cash)
                r.rows.push_back({o.paymentTime().toString(dt), "#" + o.id, o.customer, units,
                                  W::money(o.total), W::money(-o.discount)});
            else
                r.rows.push_back({o.paymentTime().toString(dt), "#" + o.id, o.customer, cap(o.payment), units,
                                  W::money(o.total), W::money(-o.discount)});
            sum += o.total;
            disc += o.discount;
            ++n;
        }
        r.tiles = {{cash ? "TOTAL CASH SALES" : "TOTAL SALES", "cash", W::money(sum), QString("%1 order%2").arg(n).arg(n == 1 ? "" : "s")},
                   cash ? Tile{"AVERAGE CASH BILL", "bills", W::money(n ? sum / n : 0.0), "per cash order"}
                        : Tile{"PAYMENT MIX", "card", QString("%1 / %2 / %3").arg(cashN).arg(cardN).arg(walletN), "cash / card / wallet"},
                   {"DISCOUNTS GRANTED", "percent", W::money(disc), "Item, seasonal, and special discounts"}};
        r.footLabel = cash ? "Cash sales total:" : "Sales total:";
        r.footValue = W::money(sum);
        r.footNote = QString("Discounts given: %1  •  %2 paid").arg(W::money(disc)).arg(n);
        break;
    }
    case TopSold:
    {
        r.title = "Top 10 Most Sold Items";
        r.subtitle = "Descending by units sold  •  " + periodText();
        r.headers = {"RANK", "PRODUCT", "ACRONYM", "UNITS SOLD", "LINE VALUE", "STOCK LEFT", "STATUS"};
        r.right = {false, false, false, true, true, true, false};
        r.accent = {2, 3};
        const auto items = topSold(10);
        double units = 0;
        int lowCount = 0;
        for (int i = 0; i < items.size(); ++i)
        {
            const auto &t = items[i];
            const QString status = t.stock < 0 ? "Not in catalogue" : t.stock == 0 ? "Out of stock" : t.stock <= low ? "Low stock" : "OK";
            if (t.stock >= 0 && t.stock <= low)
                ++lowCount;
            r.rows.push_back({QString::number(i + 1), t.name, t.acronym, quantity(t.units), W::money(t.lineValue),
                              t.stock < 0 ? QString("—") : QString::number(t.stock), status});
            units += t.units;
            sum += t.lineValue;
        }
        r.tiles = {{"BEST SELLER", "star", items.isEmpty() ? QString("-") : items[0].acronym,
                    items.isEmpty() ? QString("No sales in this period") : QString("%1 units sold").arg(quantity(items[0].units))},
                   {"UNITS SOLD (TOP 10)", "cart", quantity(units), "Across ranked products"},
                   {"NEED RESTOCK", "warn", QString::number(lowCount), "top sellers with low or no stock"}};
        r.footLabel = "Top ten line value:";
        r.footValue = W::money(sum);
        r.footNote = "After item discounts, before tax and bill-level discounts";
        break;
    }
    case TopRemaining:
    {
        r.title = "Top 10 Items by Remaining Stock";
        r.subtitle = "Ranked by current stock · Sales: " + periodText();
        r.headers = {"RANK", "PRODUCT", "ACRONYM", "STOCK LEFT", "SOLD (PERIOD)", "ADVICE"};
        r.right = {false, false, false, true, true, false};
        r.accent = {2, 3};
        QVector<ProductInfo> products = app.products();
        std::stable_sort(products.begin(), products.end(), [](const ProductInfo &a, const ProductInfo &b) { return a.stock > b.stock; });
        if (products.size() > 10)
            products.resize(10);
        const auto sold = topSold(0);
        int stockSum = 0;
        for (int i = 0; i < products.size(); ++i)
        {
            const auto &p = products[i];
            double s = 0;
            for (const auto &t : sold)
                if (t.acronym.compare(p.sku, Qt::CaseInsensitive) == 0)
                    s = t.units;
            const QString advice = p.stock <= 0 ? "Out of stock"
                                 : p.stock <= low ? "Review for restock" : "Stock sufficient";
            r.rows.push_back({QString::number(i + 1), p.name, p.sku, QString::number(p.stock), quantity(s), advice});
            stockSum += p.stock;
        }
        r.tiles = {{"MOST STOCKED", "inventory", products.isEmpty() ? QString("-") : products[0].sku,
                    products.isEmpty() ? QString() : QString("%1 units on hand").arg(products[0].stock)},
                   {"UNITS ON HAND (TOP 10)", "products", QString::number(stockSum), "held by the ten items"},
                   {"CATALOGUE", "barcode", QString::number(app.products().size()), "products in Product.txt"}};
        r.footLabel = "Units on hand (top ten):";
        r.footValue = QString::number(stockSum);
        r.footNote = "Stock quantities are current snapshots";
        break;
    }
    }
    return r;
}

// ============================================================================ view
void ReportsPage::render()
{
    QHash<QString, double> paidUnits;
    if (m_tab == Completed || m_tab == CashSales || m_tab == AllSales)
        for (const auto &bill : savedBills(AppState::instance()))
        {
            if (bill.lines.empty())
                continue;
            double units = 0;
            for (const auto &line : bill.lines)
                units += line.quantity;
            paidUnits.insert(QString::fromStdString(bill.id), units);
        }
    m_report = build(m_tab, paidUnits);
    const Report &r = m_report;

    clearLayout(m_tiles);
    for (const auto &t : r.tiles)
    {
        QLabel *value = nullptr, *sub = nullptr;
        auto *tile = W::card(this, "reportMetric");
        auto *layout = new QHBoxLayout(tile);
        layout->setContentsMargins(14, 8, 10, 8);
        layout->setSpacing(10);
        layout->addWidget(W::iconLabel(t.icon, Theme::readable(Theme::primary), 24), 0, Qt::AlignTop);
        auto *labels = new QVBoxLayout;
        labels->setSpacing(2);
        labels->addWidget(W::label(t.caption, "caption"));
        value = W::label("", "bigValue");
        sub = W::label("", "muted");
        sub->setWordWrap(true);
        labels->addWidget(value);
        labels->addWidget(sub);
        layout->addLayout(labels, 1);
        value->setText(t.value);
        sub->setText(t.sub);
        m_tiles->addWidget(tile, 1);
    }

    m_ledgerTitle->setText(r.title);
    m_ledgerSub->setText(r.subtitle);
    m_table->setRowCount(0);
    m_table->setColumnCount(r.headers.size());
    m_table->setHorizontalHeaderLabels(r.headers);
    auto *header = m_table->horizontalHeader();
    header->setMinimumSectionSize(52);
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(false);
    const bool narrow = r.showTopSold;
    for (int column = 0; column < r.headers.size(); ++column)
    {
        const auto &name = r.headers[column];
        const int width = name == "PRODUCT" || name == "CUSTOMER" ? (narrow ? 142 : 168)
                         : name == "DATE & TIME" ? 133
                         : name == "RANK" ? 56
                         : name == "ITEMS" && narrow ? 60
                         : name == "ITEMS" && m_tab == Incomplete ? 174
                         : name == "ADVICE" ? 150 : 104;
        m_table->setColumnWidth(column, width);
        m_table->horizontalHeaderItem(column)->setTextAlignment(
            (r.right.value(column) ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter);
        if (name == "PRODUCT" || (name == "CUSTOMER" && !narrow))
            header->setSectionResizeMode(column, QHeaderView::Stretch);
        if (name == "AMOUNT" || name == "DISCOUNT" || name == "LINE VALUE")
            header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    }
    header->setStretchLastSection(narrow);
    m_table->setRowCount(r.rows.size());
    for (int row = 0; row < r.rows.size(); ++row)
    {
        m_table->setRowHeight(row, 40);
        for (int c = 0; c < r.headers.size() && c < r.rows[row].size(); ++c)
        {
            if (r.headers[c] == "STATUS")
            {
                const auto &status = r.rows[row][c];
                const auto kind = status == "Low stock" ? QString("amber")
                                 : status == "Out of stock" ? QString("red")
                                 : status == "OK" ? QString("green") : W::statusBadgeKind(status);
                m_table->setCellWidget(row, c, wrapW(W::badge(status, kind)));
                continue;
            }
            const bool accent = r.accent.contains(c);
            const Qt::Alignment a = (r.right.value(c) ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter;
            auto *cell = item(r.rows[row][c], a, accent ? "#004ac6" : "", accent);
            cell->setToolTip(r.rows[row][c]);
            m_table->setItem(row, c, cell);
        }
    }
    m_footLabel->setText(r.footLabel);
    m_footValue->setText(r.footValue);
    m_footNote->setText(r.footNote);
    m_empty->setVisible(r.rows.isEmpty());

    // top sold staples (side panel)
    m_side->setVisible(r.showTopSold);
    if (r.showTopSold)
    {
        clearLayout(m_sideList);
        const auto items = topSold(5, m_tab == CashSales);
        const double maxUnits = items.isEmpty() ? 1.0 : qMax(1.0, items.first().units);
        if (items.isEmpty())
            m_sideList->addWidget(W::label("No sales in this period.", "muted"));
        for (int i = 0; i < items.size(); ++i)
        {
            const auto &t = items[i];
            auto *rowW = new QWidget;
            auto *v = new QVBoxLayout(rowW);
            v->setContentsMargins(0, 0, 0, 0);
            v->setSpacing(3);
            auto *name = W::label(t.name);
            name->setStyleSheet("font-weight:700;");
            name->setWordWrap(true);
            v->addWidget(name);
            auto *h = new QHBoxLayout;
            h->setSpacing(8);
            h->addWidget(W::label(QString("%1 sold").arg(quantity(t.units)), "muted"));
            v->addLayout(h);
            auto *bar = new QProgressBar;
            bar->setRange(0, 1000);
            bar->setValue(qRound(t.units * 1000 / maxUnits));
            bar->setTextVisible(false);
            bar->setFixedHeight(8);
            bar->setObjectName("reportRankingBar");
            v->addWidget(bar);
            m_sideList->addWidget(rowW);
        }
    }
}

// ============================================================================ export
void ReportsPage::exportCsv()
{
    const Report r = m_report;
    auto field = [](QString value) { return csvField(value.replace("\r\n", "\n").replace('\r', '\n')); };
    QStringList rows;
    QStringList head;
    for (const auto &h : r.headers)
        head << csvField(h);
    rows << head.join(',');
    for (const auto &row : r.rows)
    {
        QStringList cells;
        for (const auto &c : row)
            cells << field(c);
        rows << cells.join(',');
    }
    rows << QString();
    rows << csvField(r.footLabel) + "," + csvField(r.footValue);
    rows << csvField("Report") + "," + csvField(r.title);
    rows << csvField("Scope") + "," + csvField(r.subtitle);
    rows << csvField("Notes") + "," + csvField(r.footNote);
    QString name = r.title.toLower();
    name.replace(' ', '-');
    if (writeCsv(this, "report-" + name + ".csv", rows))
        emit toast("Report exported as CSV");
}

void ReportsPage::exportPdf()
{
    const Report r = m_report;
    QString name = r.title.toLower();
    name.replace(' ', '-');
    const QString path = QFileDialog::getSaveFileName(this, "Export / Print report as PDF", "report-" + name + ".pdf", "PDF (*.pdf)");
    if (path.isEmpty())
        return;
    QString html = QString("<h2 style='margin-bottom:2px'>%1</h2><p style='color:#555'>%2 &nbsp;|&nbsp; exported %3</p>"
                           "<table width='100%' border='1' cellspacing='0' cellpadding='5' style='border-collapse:collapse'><thead><tr>")
                       .arg(r.title.toHtmlEscaped(), r.subtitle.toHtmlEscaped(), QDateTime::currentDateTime().toString("dd-MMM-yyyy HH:mm"));
    for (const auto &h : r.headers)
        html += "<th bgcolor='#dce9ff' align='left'>" + h.toHtmlEscaped() + "</th>";
    html += "</tr></thead>";
    for (const auto &row : r.rows)
    {
        html += "<tr>";
        for (int c = 0; c < row.size(); ++c)
            html += QString("<td align='%1'>%2</td>").arg(r.right.value(c) ? "right" : "left", row[c].toHtmlEscaped());
        html += "</tr>";
    }
    if (r.rows.isEmpty())
        html += QString("<tr><td colspan='%1'>No records in this report.</td></tr>").arg(r.headers.size());
    html += QString("</table><p><b>%1 %2</b> &nbsp; %3</p>")
                .arg(r.footLabel.toHtmlEscaped(), r.footValue.toHtmlEscaped(), r.footNote.toHtmlEscaped());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        emit toast("Could not open the report PDF for writing");
        return;
    }
    {
        QPdfWriter pdf(&file);
        pdf.setPageSize(QPageSize(QPageSize::A4));
        pdf.setPageOrientation(QPageLayout::Landscape);
        pdf.setPageMargins(QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);
        pdf.setCreator("Syntro-Check");
        pdf.setTitle(r.title);
        QTextDocument doc;
        doc.setHtml(html);
        doc.print(&pdf);
    }
    emit toast(file.size() > 0 && file.commit() ? "Report saved as PDF" : "Could not save the report PDF");
}
