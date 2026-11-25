#include "OrdersPage.h"
#include "Dialogs.h"
#include "OrderRules.h"
#include "UiStore.h"
#include "Theme.h"
#include "UiKit.h"
#include "Widgets.h"
#include <QApplication>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSet>
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
    m_table->setColumnWidth(6, 110);
    m_table->setColumnWidth(7, 150);
    m_table->setColumnWidth(8, 215);
    v->addWidget(m_table, 1);
    m_showing = W::label("", "muted");
    v->addWidget(m_showing);
    root->addWidget(card, 1);

    connect(m_orderSearch, &QLineEdit::textChanged, this, [this]
        { rebuild(); });
    connect(m_custSearch, &QLineEdit::textChanged, this, [this]
        { rebuild(); });
    connect(&AppState::instance(), &AppState::changed, this, &OrdersPage::rebuild);
    connect(&UiStore::instance(), &UiStore::parkedChanged, this, &OrdersPage::rebuild);
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
    const auto& parked = UiStore::instance().parked();
    QString q = m_orderSearch->text().trimmed();
    q.remove('#');

    // Orders parked with F2 live in UiStore; show them as "On Hold" rows next to the orders from Order.txt.
    QVector<OrderRecord> candidates;
    QSet<QString> holdCodes;
    for (const auto& p : parked)
    {
        OrderRecord h;
        h.id = p.code;
        h.time = p.parkedAt;
        h.customer = p.customer.isEmpty() ? QString("Walk-in Customer") : p.customer;
        h.contact = p.contact.isEmpty() ? QString("-") : p.contact;
        h.tier = "Guest";
        h.shift = "Parked";
        h.payment = "-";
        h.status = "On Hold";
        h.lines = p.lines;
        h.recompute();
        candidates << h;
        holdCodes.insert(p.code);
    }
    candidates += all;

    QVector<OrderRecord> rows;
    for (const auto& o : candidates)
    {
        if (m_filter != "All" && o.status != m_filter)
            continue;
        if (!q.isEmpty() && !o.id.contains(q, Qt::CaseInsensitive) && !o.customer.contains(q, Qt::CaseInsensitive) &&
            !o.contact.contains(q))
            continue;
        rows << o;
    }

    m_table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r)
    {
        const auto& o = rows[r];
        const bool isHold = holdCodes.contains(o.id);
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
            auto* c = W::label(o.contact, "muted");
            cl->addWidget(n);
            cl->addWidget(c);
            m_table->setCellWidget(r, 2, cw);
        }
        m_table->setItem(r, 3, item(o.summary()));
        m_table->setItem(r, 4, item(o.payment));
        m_table->setItem(r, 5, item(W::money(o.total), Qt::AlignRight | Qt::AlignVCenter, "", true));
        m_table->setCellWidget(r, 6, wrapW(W::badge(o.status, W::statusBadgeKind(o.status))));

        // ---- EDIT WINDOW column: "Editable for N days" (paid, inside 7 days), "Window closed", "Locked", "Resume anytime"
        {
            QLabel* b = nullptr;
            if (OrderRules::isPaid(o))
            {
                const int left = OrderRules::editDaysLeft(o);
                if (left > 0)
                {
                    b = W::badge(OrderRules::editBadgeText(left), OrderRules::editBadgeKind(left));
                    b->setToolTip(QString("Paid %1 - editable until %2, with manager approval")
                        .arg(o.time.toString("d MMM HH:mm"), OrderRules::editDeadline(o).toString("d MMM yyyy HH:mm")));
                }
                else
                {
                    b = W::badge("Window closed", "grey");
                    b->setToolTip(QString("Paid %1 days ago - orders can only be edited within %2 days of payment")
                        .arg(OrderRules::daysSincePaid(o)).arg(OrderRules::kEditWindowDays));
                }
            }
            else if (o.status == "Archived")
            {
                b = W::badge("Locked", "grey");
                b->setToolTip("Archived orders cannot be edited");
            }
            else if (OrderRules::isResumable(o))
            {
                b = W::badge("Resume anytime", "blue");
                b->setToolTip("On-hold orders need no approval");
            }
            if (b)
                m_table->setCellWidget(r, 7, wrapW(b));
        }

        // ---- ACTIONS column: Open (paid) / Resume (on hold, in progress), details, receipt
        auto* w = new QWidget;
        auto* h = new QHBoxLayout(w);
        h->setContentsMargins(4, 0, 4, 0);
        h->setSpacing(4);
        const OrderRecord copy = o;
        const QString id = o.id;
        if (OrderRules::isResumable(o))
        {
            auto* resume = W::button("Resume", "mini", "play");
            resume->setToolTip("Continue this order in Record Order");
            h->addWidget(resume);
            if (isHold)
                connect(resume, &QPushButton::clicked, this, [this, id] { emit resumeParked(id); });
            else
                connect(resume, &QPushButton::clicked, this, [this, id] { emit resumeOrder(id); });
        }
        else if (OrderRules::isPaid(o) || o.status == "Archived")
        {
            auto* open = W::button("Open", "mini", OrderRules::isPaid(o) && OrderRules::paidEditable(o) ? "unlock" : "lock");
            open->setEnabled(OrderRules::isPaid(o) && OrderRules::paidEditable(o));
            open->setToolTip(open->isEnabled()
                ? (m_role == UserRole::Manager ? "Open this paid order for editing"
                                               : "Open this paid order for editing (needs manager approval)")
                : "This order can no longer be edited");
            h->addWidget(open);
            connect(open, &QPushButton::clicked, this, [this, copy] { openPaid(copy); });
        }
        auto* view = W::button("", "flat", "eye");
        view->setToolTip("View details");
        h->addWidget(view);
        connect(view, &QPushButton::clicked, this, [this, copy]
            { Dialogs::orderDetails(this, copy); });
        if (!isHold)
        {
            auto* rc = W::button("", "flat", "bills");
            rc->setToolTip("Receipt");
            h->addWidget(rc);
            connect(rc, &QPushButton::clicked, this, [this, copy]
                { Dialogs::receipt(this, copy); });
        }
        h->addStretch();
        m_table->setCellWidget(r, 8, w);
    }
    m_showing->setText(QString("%1 of %2 orders").arg(rows.size()).arg(candidates.size()));
}

// Paid order: inside the 7-day window and approved by a manager -> open it for editing.
void OrdersPage::openPaid(const OrderRecord& order)
{
    if (!OrderRules::paidEditable(order))
    {
        emit toast(QString("Order #%1 was paid %2 days ago - the %3-day edit window has closed")
            .arg(order.id).arg(OrderRules::daysSincePaid(order)).arg(OrderRules::kEditWindowDays));
        return;
    }
    if (m_role == UserRole::Manager)
    {
        emit toast("Edit of order #" + order.id + " approved by manager");
        emit editOrder(order.id);
        return;
    }
    QString manager, reason;
    if (!Dialogs::managerApproval(this, order, &manager, &reason))
        return;
    emit toast(QString("Order #%1 unlocked by %2").arg(order.id, manager));
    emit editOrder(order.id);
}

void OrdersPage::setRole(UserRole role)
{
    m_role = role;
    rebuild();
}
