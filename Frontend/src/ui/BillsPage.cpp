#include "BillsPage.h"
#include "Dialogs.h"
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
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextStream>
#include <QTime>
#include <QVBoxLayout>

using namespace Ui; // makeTable, item, wrapW, pageShell, writeCsv (see UiKit.h)


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

void BillsPage::showOrder(const QString& orderId) { m_search->setText(orderId); }
