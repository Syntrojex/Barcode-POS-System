#include "ProductsPage.h"
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
#include <QTimer>
#include <QVBoxLayout>

using namespace Ui; // makeTable, item, wrapW, pageShell, writeCsv (see UiKit.h)


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
    m_search->setPlaceholderText("Search products");
    m_search->setToolTip("Search by name, acronym, product ID, or barcode.");
    m_search->setMinimumWidth(280);
    m_search->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    h->addWidget(m_search);
    v->addLayout(h);
    m_issues = W::button("", "soft", "warn");
    connect(m_issues, &QPushButton::clicked, this, [this] { Dialogs::loadIssues(this); });
    h->insertWidget(3, m_issues);
    m_newBtn = W::button("New Product", "primary", "plus");
    m_newBtn->setVisible(false);
    connect(m_newBtn, &QPushButton::clicked, this, &ProductsPage::addNew);
    h->addWidget(m_newBtn);
    m_table = makeTable({ "ID", "ACRONYM", "BARCODE", "PRODUCT", "UNIT", "PRICE", "TAX", "DISC.", "STOCK", "STATUS", "" });
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    for (int c : {0, 1, 2, 4, 5, 6, 7, 8, 9})
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(10, QHeaderView::Fixed);
    m_table->setColumnWidth(10, 234);
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
        auto* d = W::button("Details", "soft");
        auto* cell = new QWidget;
        auto* cl = new QHBoxLayout(cell);
        cl->setContentsMargins(0, 0, 4, 0);
        cl->setSpacing(6);
        cl->addWidget(d);
        const QString ean = p.ean;
        const int pid = p.id;
        if (RolePolicy::canEditCatalogue(m_role)) // manager: edit the product in the same dialog as "New Product"
        {
            auto* e = W::button("Edit", "mini", "edit");
            cl->addWidget(e);
            const ProductInfo copy = p;
            connect(e, &QPushButton::clicked, this, [this, copy]
                {
                    // open after the click handler returns: saving rebuilds this table (and this button)
                    QTimer::singleShot(0, this, [this, copy]
                        {
                            if (Dialogs::editProduct(this, &copy))
                                emit toast("Product " + copy.sku + " updated in Product.txt");
                        });
                });
        }
        if (!m_viewOnly) // Manager views products only: no "Add" button
        {
            auto* b = W::button("Add", "mini");
            b->setEnabled(p.stock > 0);
            cl->addWidget(b);
            connect(b, &QPushButton::clicked, this, [this, ean]
                { emit addToOrder(ean); });
        }
        m_table->setCellWidget(r, 10, cell);
        connect(d, &QPushButton::clicked, this, [this, pid]
            { Dialogs::productDescription(this, pid); });
    }
}
void ProductsPage::setViewOnly(bool on)
{
    m_viewOnly = on;
    m_table->setRowCount(0); // drop the old rows (with their buttons) before rebuilding
    rebuild();
}

void ProductsPage::setRole(UserRole role)
{
    m_role = role;
    m_newBtn->setVisible(RolePolicy::canEditCatalogue(role));
    m_table->setRowCount(0);
    rebuild();
}

void ProductsPage::addNew()
{
    if (RolePolicy::canEditCatalogue(m_role) && Dialogs::editProduct(this, nullptr))
        emit toast("Product saved to Product.txt");
}
