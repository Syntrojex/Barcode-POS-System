#include "OrdersPage.h"
#include "Dialogs.h"
#include "Theme.h"
#include "UiKit.h"
#include "UiStore.h"
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

using namespace Ui; // makeTable, item, wrapW, pageShell, writeCsv (see UiKit.h)


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

    m_table = makeTable({ "ORDER ID", "DATE & TIME", "CUSTOMER", "ITEMS SUMMARY", "PAYMENT", "AMOUNT", "STATUS", "EDIT WINDOW", "ACTIONS" });
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    for (int c : {0, 1, 4, 5})
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    for (int c : {2, 6, 7, 8})
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
    m_table->setColumnWidth(2, 175);
    m_table->setColumnWidth(6, 120);
    m_table->setColumnWidth(7, 170);
    m_table->setColumnWidth(8, 190);
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
        m_table->setRowHeight(r, 60);
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
        // ---- edit window (requirement 3): paid orders can be edited for 7 days after payment, with manager approval
        const int daysLeft = st.editDaysLeft(o);
        QLabel* windowBadge = nullptr;
        if (o.status == "Paid")
            windowBadge = daysLeft > 0
                ? W::badge(QString("Editable for %1 day%2").arg(daysLeft).arg(daysLeft == 1 ? "" : "s"), daysLeft <= 2 ? "amber" : "green")
                : W::badge("Locked (7 days over)", "grey");
        else if (o.status == "In Progress" || o.status == "On Hold")
            windowBadge = W::badge("Not paid yet", "blue");
        else
            windowBadge = W::badge("Archived", "grey");
        m_table->setCellWidget(r, 7, wrapW(windowBadge));

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
        connect(rc, &QPushButton::clicked, this, [this, copy]
            { Dialogs::receipt(this, copy); });
        if (o.status == "In Progress" || o.status == "On Hold")
        {
            auto* resume = W::button("Resume", "mini", "play");
            resume->setToolTip("Open this order in Record Order");
            h->addWidget(resume);
            connect(resume, &QPushButton::clicked, this, [this, copy]
                { emit resumeOrder(copy.id); });
        }
        else if (o.status == "Paid")
        {
            auto* open = W::button(daysLeft > 0 ? "Open" : "Locked", "mini", daysLeft > 0 ? "unlock" : "lock");
            open->setEnabled(daysLeft > 0);
            open->setToolTip(daysLeft > 0 ? "Edit this order (needs a manager's approval)"
                                          : "The 7-day edit window is over");
            h->addWidget(open);
            connect(open, &QPushButton::clicked, this, [this, copy]
                { openPaid(copy); });
        }
        h->addStretch();
        m_table->setCellWidget(r, 8, w);
    }
    m_showing->setText(QString("%1 of %2 orders").arg(rows.size()).arg(all.size()));
}

void OrdersPage::setRole(UserRole role) { m_role = role; }

// A paid order is opened for editing only after a manager approved it (a signed-in manager approves it himself).
void OrdersPage::openPaid(const OrderRecord& order)
{
    if (AppState::instance().editDaysLeft(order) <= 0)
    {
        emit toast("Order " + order.id + " can no longer be edited (7 days after payment are over)");
        return;
    }
    QString manager, reason;
    if (m_role == UserRole::Manager)
    {
        manager = "Manager (signed in)";
        reason = "Edited by manager";
    }
    else if (!Dialogs::managerApproval(this, order, &manager, &reason))
        return;
    UiStore::instance().grantApproval(order.id, manager, reason);
    emit toast("Approved by " + manager + " - opening order " + order.id);
    emit editOrder(order.id);
}
