#include "OrdersPage.h"
#include "Dialogs.h"
#include "Theme.h"
#include "UiKit.h"
#include "UiStore.h"
#include "Widgets.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QShowEvent>
#include <QTableWidget>
#include <QVBoxLayout>

using namespace Ui; // makeTable, item, wrapW, pageShell, writeCsv (see UiKit.h)


// =================================================================== OrdersPage
OrdersPage::OrdersPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root;
    pageShell(this, root);
    root->setSpacing(8);
    root->addWidget(sectionHeader({}, "Orders Management", "Search, review, and resume customer orders."));

    // ---- manager: edit requests waiting for approval (hidden when there are none)
    m_requestsCard = W::card(this, "subCard");
    auto* rv = new QVBoxLayout(m_requestsCard);
    rv->setContentsMargins(16, 12, 16, 12);
    rv->setSpacing(8);
    auto* rh = new QHBoxLayout;
    rh->setSpacing(8);
    rh->addWidget(W::iconLabel("clock", Theme::primary, 18));
    m_requestsTitle = W::label("", "h2");
    rh->addWidget(m_requestsTitle);
    rh->addStretch();
    rv->addLayout(rh);
    m_requestsList = new QVBoxLayout;
    m_requestsList->setSpacing(6);
    rv->addLayout(m_requestsList);
    m_requestsCard->hide();
    root->addWidget(m_requestsCard);

    auto* card = W::card(this, "subCard");
    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(16, 16, 16, 12);
    v->setSpacing(8);

    auto* tools = new QHBoxLayout;
    tools->setSpacing(8);
    m_orderSearch = new QLineEdit;
    m_orderSearch->setAccessibleName("Search orders");
    m_orderSearch->setClearButtonEnabled(true);
    m_orderSearch->setPlaceholderText("Search by order number, customer or phone…");
    m_orderSearch->setMinimumHeight(40);
    m_orderSearch->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    m_custSearch = new QLineEdit; // kept hidden: legacy field, not shown
    m_custSearch->hide();
    tools->addWidget(m_orderSearch, 1);
    auto* exp = W::button("Export CSV", "soft", "export");
    exp->setMinimumHeight(40);
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
    m_table->setProperty("managementTable", true);
    m_table->setAlternatingRowColors(true);
    m_table->setAccessibleName("Orders");
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setMinimumSectionSize(70);
    for (int c : {0, 1, 4, 5})
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    for (int c : {2, 6, 7, 8})
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
    m_table->setColumnWidth(2, 152);
    m_table->setColumnWidth(6, 112);
    m_table->setColumnWidth(7, 132);
    m_table->setColumnWidth(8, 204);
    v->addWidget(m_table, 1);
    m_empty = W::label("", "muted");
    m_empty->setAlignment(Qt::AlignCenter);
    m_empty->setMinimumHeight(64);
    m_empty->setWordWrap(true);
    v->addWidget(m_empty);
    m_showing = W::label("", "muted");
    v->addWidget(m_showing);
    root->addWidget(card, 1);

    connect(m_orderSearch, &QLineEdit::textChanged, this, [this]
        { rebuild(); });
    connect(m_custSearch, &QLineEdit::textChanged, this, [this]
        { rebuild(); });
    connect(&AppState::instance(), &AppState::changed, this, &OrdersPage::rebuild);
    connect(&UiStore::instance(), &UiStore::editRequestsChanged, this, &OrdersPage::rebuild);
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
    rebuildRequests();
    const auto& all = AppState::instance().orders();
    QString q = m_orderSearch->text().trimmed();
    q.remove('#');
    QVector<OrderRecord> rows;
    for (const auto& o : all)
    {
        if (m_filter != "All" && o.status != m_filter)
            continue;
        if (!q.isEmpty() && !o.id.contains(q, Qt::CaseInsensitive) && !o.customer.contains(q, Qt::CaseInsensitive) &&
            !o.contact.contains(q))
            continue;
        rows << o;
    }
    auto& st = AppState::instance();
    m_table->setRowCount(0);
    m_table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r)
    {
        const auto& o = rows[r];
        m_table->setRowHeight(r, 54);
        m_table->setItem(r, 0, item("#" + o.id, Qt::AlignLeft | Qt::AlignVCenter, "#004ac6", true));
        m_table->setItem(r, 1, item(o.time.toString("yyyy-MM-dd HH:mm") + "\n" + o.shift));
        {
            auto* cw = new QWidget;
            auto* cl = new QVBoxLayout(cw);
            cl->setContentsMargins(8, 4, 8, 4);
            cl->setSpacing(0);
            auto* n = W::label(o.customer + (o.tier == "VIP" ? "  [VIP]" : ""));
            n->setStyleSheet("font-weight:700;");
            cw->setToolTip(n->text() + "\n" + o.contact);
            auto* c = W::label(o.contact, "muted");
            cl->addWidget(n);
            cl->addWidget(c);
            m_table->setCellWidget(r, 2, cw);
        }
        auto* summary = item(o.summary());
        summary->setToolTip(o.summary());
        m_table->setItem(r, 3, summary);
        m_table->setItem(r, 4, item(o.payment));
        m_table->setItem(r, 5, item(W::money(o.total), Qt::AlignRight | Qt::AlignVCenter, "", true));
        m_table->setCellWidget(r, 6, wrapW(W::badge(o.status, W::statusBadgeKind(o.status))));
        // ---- edit window (requirement 3): paid orders can be edited for 7 days after payment, with manager approval
        const int daysLeft = st.editDaysLeft(o);
        QLabel* windowBadge = nullptr;
        const UiStore::EditRequest* req = UiStore::instance().latestEditRequest(o.id);
        const QString reqStatus = (o.status == "Paid" && daysLeft > 0 && req) ? req->status : QString();
        if (reqStatus == "pending")
            windowBadge = W::badge("Requested", "amber");
        else if (reqStatus == "approved")
            windowBadge = W::badge("Approved", "green");
        else if (reqStatus == "rejected")
            windowBadge = W::badge("Rejected", "red");
        else if (o.status == "Paid")
            windowBadge = daysLeft > 0
                ? W::badge(QString("Editable · %1d").arg(daysLeft), daysLeft <= 2 ? "amber" : "green")
                : W::badge("Window closed", "grey");
        if (req && !reqStatus.isEmpty())
            windowBadge->setToolTip(QString("Requested by %1 (%2)%3").arg(req->cashier, req->reason,
                req->manager.isEmpty() ? QString() : " - " + req->status + " by " + req->manager));
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
        view->setFixedWidth(34);
        view->setToolTip("View details");
        view->setAccessibleName("View details for order " + o.id);
        h->addWidget(view);
        connect(view, &QPushButton::clicked, this, [this, copy]
            { Dialogs::orderDetails(this, copy); });
        auto* rc = W::button("", "flat", "bills");
        rc->setFixedWidth(34);
        rc->setToolTip(o.billId.isEmpty() ? "Receipt" : "View Bill");
        rc->setAccessibleName(rc->toolTip() + " for order " + o.id);
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
            QString text = "Open", icon = "unlock", tip = "Edit this order";
            bool enabled = daysLeft > 0;
            if (daysLeft <= 0)
            {
                text = "Locked";
                icon = "lock";
                tip = "The 7-day edit window is over";
            }
            else if (m_role != UserRole::Manager)
            {
                if (reqStatus == "pending")
                {
                    text = "Waiting";
                    icon = "clock";
                    tip = "Waiting for a manager to approve";
                    enabled = false;
                }
                else if (reqStatus == "approved")
                    tip = "Approved by " + req->manager + " - open it to edit";
                else
                {
                    text = "Request edit";
                    tip = reqStatus == "rejected" ? "The last request was rejected by " + req->manager + ". Ask again."
                                                  : "Ask a manager for permission to edit this paid order";
                }
            }
            auto* open = W::button(text, "mini", icon);
            open->setEnabled(enabled);
            open->setToolTip(tip);
            h->addWidget(open);
            connect(open, &QPushButton::clicked, this, [this, copy]
                { openPaid(copy); });
        }
        h->addStretch();
        m_table->setCellWidget(r, 8, w);
    }
    m_empty->setText(all.isEmpty() ? "No orders recorded yet." : "No orders match the current search and status.");
    m_empty->setVisible(rows.isEmpty());
    m_showing->setText(QString("Showing %1 of %2 orders").arg(rows.size()).arg(all.size()));
}

void OrdersPage::setRole(UserRole role)
{
    m_role = role;
    rebuild();
}

// Manager: edits straight away (his own approval is recorded). Cashier: opens it when a manager approved the request,
// otherwise sends a request - no PIN on the cashier's screen.
void OrdersPage::openPaid(const OrderRecord& order)
{
    auto& store = UiStore::instance();
    if (AppState::instance().editDaysLeft(order) <= 0)
    {
        emit toast("Order " + order.id + " can no longer be edited (7 days after payment are over)");
        return;
    }
    if (m_role == UserRole::Manager)
    {
        store.grantApproval(order.id, m_user.isEmpty() ? QString("Manager") : m_user, "Edited by manager");
        emit editOrder(order.id);
        return;
    }
    store.reloadEditRequests();
    if (store.approvalFor(order.id))
    {
        emit editOrder(order.id);
        return;
    }
    QString reason;
    if (!Dialogs::requestEditReason(this, order, &reason))
        return;
    const QString error = store.requestEdit(order.id, m_user, reason);
    emit toast(error.isEmpty() ? "Request sent - order " + order.id + " opens once a manager approves it" : error);
}

void OrdersPage::showEvent(QShowEvent* e)
{
    UiStore::instance().reloadEditRequests(); // another terminal may have approved / requested meanwhile
    QWidget::showEvent(e);
}

void OrdersPage::rebuildRequests()
{
    while (auto* item = m_requestsList->takeAt(0))
    {
        if (auto* w = item->widget())
            w->deleteLater();
        delete item;
    }
    const auto pending = UiStore::instance().pendingEditRequests();
    m_requestsCard->setVisible(m_role == UserRole::Manager && !pending.isEmpty());
    if (m_role != UserRole::Manager)
        return;
    m_requestsTitle->setText(QString("Waiting for your approval (%1)").arg(pending.size()));
    for (const auto& r : pending)
    {
        OrderRecord order;
        for (const auto& o : AppState::instance().orders())
            if (o.id == r.orderId) { order = o; break; }
        auto* row = new QWidget;
        auto* h = new QHBoxLayout(row);
        h->setContentsMargins(0, 2, 0, 2);
        h->setSpacing(10);
        auto* what = W::label(QString("<b>#%1</b> &nbsp;%2 &nbsp;·&nbsp; %3").arg(r.orderId,
            (order.customer.isEmpty() ? QString("Walk-in Customer") : order.customer).toHtmlEscaped(), W::money(order.total)));
        what->setTextFormat(Qt::RichText);
        h->addWidget(what);
        auto* why = W::label(QString("%1 asked: %2  ·  %3").arg(r.cashier, r.reason, r.requestedAt.toString("h:mm AP")), "muted");
        h->addWidget(why, 1);
        auto* reject = W::button("Reject", "flat", "close");
        auto* approve = W::button("Approve", "primary", "check");
        h->addWidget(reject);
        h->addWidget(approve);
        const QString id = r.orderId;
        connect(approve, &QPushButton::clicked, this, [this, id] {
            const QString err = UiStore::instance().decideEdit(id, m_user, true);
            emit toast(err.isEmpty() ? "Approved - the cashier can now open order " + id : err); });
        connect(reject, &QPushButton::clicked, this, [this, id] {
            const QString err = UiStore::instance().decideEdit(id, m_user, false);
            emit toast(err.isEmpty() ? "Request for order " + id + " rejected" : err); });
        m_requestsList->addWidget(row);
    }
}
