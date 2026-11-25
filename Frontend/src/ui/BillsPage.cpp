#include "BillsPage.h"
#include "Dialogs.h"
#include "Theme.h"
#include "UiKit.h"
#include "Widgets.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QTableWidget>
#include <QVBoxLayout>

using namespace Ui; // makeTable, item, wrapW, pageShell, writeCsv (see UiKit.h)


// =================================================================== BillsPage
BillsPage::BillsPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root;
    pageShell(this, root);
    root->setSpacing(8);
    root->addWidget(sectionHeader({}, "Bills & Invoices", "Find paid transactions and open their receipts."));
    auto* card = W::card(this, "subCard");
    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(16, 16, 16, 12);
    v->setSpacing(8);
    auto* h = new QHBoxLayout;
    h->addWidget(W::label("Saved Bills", "h1"));
    h->addStretch();
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Search by bill ID, order ID or customer");
    m_search->setAccessibleName("Search saved bills");
    m_search->setMinimumHeight(40);
    m_search->setClearButtonEnabled(true);
    m_search->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    h->addWidget(m_search, 1);
    v->addLayout(h);
    m_table = makeTable({ "BILL NO.", "ORDER", "DATE & TIME", "CUSTOMER", "PAYMENT", "TOTAL", "ACTIONS" });
    m_table->setProperty("managementTable", true);
    m_table->setAlternatingRowColors(true);
    m_table->setWordWrap(false);
    m_table->setAccessibleName("Saved bills");
    auto *header = m_table->horizontalHeader();
    for (int column : {0, 1, 2, 4, 5})
        header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(6, QHeaderView::Fixed);
    header->setMinimumSectionSize(100);
    m_table->setColumnWidth(6, 110);
    v->addWidget(m_table, 1);
    m_summary = W::label("", "muted");
    v->addWidget(m_summary);
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
    int savedCount = 0;
    for (const auto& o : AppState::instance().orders())
    {
        if (o.status != "Paid" || o.billId.isEmpty())
            continue;
        ++savedCount;
        if (q.isEmpty() || o.id.contains(q, Qt::CaseInsensitive) || o.billId.contains(q, Qt::CaseInsensitive) || o.customer.contains(q, Qt::CaseInsensitive))
            rows << o;
    }
    m_table->setRowCount(0);
    m_table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r)
    {
        const auto& o = rows[r];
        m_table->setRowHeight(r, 52);
        m_table->setItem(r, 0, item(o.billId, Qt::AlignLeft | Qt::AlignVCenter, "#004ac6", true));
        m_table->setItem(r, 1, item(o.id));
        m_table->setItem(r, 2, item(o.paymentTime().toString("d MMM yyyy HH:mm")));
        auto *customer = item(o.customer);
        customer->setToolTip(o.customer);
        m_table->setItem(r, 3, customer);
        m_table->setItem(r, 4, item(o.payment));
        m_table->setItem(r, 5, item(W::money(o.total), Qt::AlignRight | Qt::AlignVCenter, "", true));
        auto* w = new QWidget;
        auto* h = new QHBoxLayout(w);
        h->setContentsMargins(4, 0, 4, 0);
        auto* view = W::button("View Bill", "mini");
        view->setAccessibleName("View bill " + o.billId);
        h->addWidget(view);
        h->addStretch();
        m_table->setCellWidget(r, 6, w);
        const OrderRecord copy = o;
        connect(view, &QPushButton::clicked, this, [this, copy]
            { Dialogs::receipt(this, copy); });
    }
    m_summary->setText(savedCount == 0 ? "No paid bills yet."
                         : rows.isEmpty() ? "No paid bills match your search."
                         : QString("Showing %1 of %2 paid bills").arg(rows.size()).arg(savedCount));
}

void BillsPage::showOrder(const QString& orderId) { m_search->setText(orderId); }
