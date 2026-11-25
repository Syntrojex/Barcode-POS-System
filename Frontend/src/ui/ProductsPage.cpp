#include "ProductsPage.h"
#include "Dialogs.h"
#include "Theme.h"
#include "UiKit.h"
#include "Widgets.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

using namespace Ui; // makeTable, item, wrapW, pageShell, writeCsv (see UiKit.h)


// =================================================================== ProductsPage
ProductsPage::ProductsPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root;
    pageShell(this, root);
    root->setSpacing(8);
    root->addWidget(sectionHeader({}, "Product Catalog", "Find products, inspect barcodes, and manage the catalog."));
    auto* card = W::card(this, "subCard");
    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(16, 16, 16, 12);
    v->setSpacing(8);
    auto* h = new QHBoxLayout;
    h->addWidget(W::label("Registered products", "h1"));
    m_count = W::badge("", "blue");
    h->addWidget(m_count);
    h->addStretch();
    v->addLayout(h);
    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Search products by name, ID, acronym, or barcode…");
    m_search->setToolTip("Search by name, acronym, product ID, or barcode.");
    m_search->setClearButtonEnabled(true);
    m_search->setAccessibleName("Search products");
    m_search->setMinimumHeight(40);
    m_search->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    toolbar->addWidget(m_search, 1);
    m_issues = W::button("", "soft", "warn");
    m_issues->setMinimumHeight(40);
    connect(m_issues, &QPushButton::clicked, this, [this] { Dialogs::loadIssues(this); });
    toolbar->addWidget(m_issues);
    m_newBtn = W::button("New Product", "primary", "plus");
    m_newBtn->setMinimumHeight(40);
    m_newBtn->setVisible(false);
    connect(m_newBtn, &QPushButton::clicked, this, &ProductsPage::addNew);
    toolbar->addWidget(m_newBtn);
    v->addLayout(toolbar);
    m_table = makeTable({ "ID", "ACRONYM", "BARCODE", "PRODUCT", "UNIT", "PRICE", "TAX", "DISC.", "STOCK", "STATUS", "ACTIONS" });
    m_table->setProperty("managementTable", true);
    m_table->setAlternatingRowColors(true);
    m_table->setAccessibleName("Registered products");
    m_table->horizontalHeader()->setMinimumSectionSize(42);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
    m_table->setColumnWidth(3, 196);
    for (int c : {0, 1, 2, 4, 5, 6, 7, 8})
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(9, QHeaderView::Fixed);
    m_table->setColumnWidth(9, 104);
    m_table->horizontalHeader()->setSectionResizeMode(10, QHeaderView::Fixed);
    m_table->setColumnWidth(10, 202);
    m_table->setWordWrap(false);
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
    m_table->setRowCount(0);
    m_table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r)
    {
        const auto& p = rows[r];
        m_table->setRowHeight(r, 44);
        const int limit = st.settings().lowStockThreshold;
        m_table->setItem(r, 0, item(QString::number(p.id)));
        m_table->setItem(r, 1, item(p.sku, Qt::AlignLeft | Qt::AlignVCenter, "#004ac6", true));
        m_table->setItem(r, 2, item(p.ean));
        auto* name = item(p.name, Qt::AlignLeft | Qt::AlignVCenter, "", true);
        name->setToolTip(p.name);
        m_table->setItem(r, 3, name);
        m_table->setItem(r, 4, item(p.unit));
        m_table->setItem(r, 5, item(W::money(p.price), Qt::AlignRight | Qt::AlignVCenter, "", true));
        m_table->setItem(r, 6, item(W::money(p.tax), Qt::AlignRight | Qt::AlignVCenter));
        m_table->setItem(r, 7, item(W::money(p.discount), Qt::AlignRight | Qt::AlignVCenter));
        const bool low = p.stock <= limit;
        m_table->setItem(r, 8, item(QString::number(p.stock), Qt::AlignRight | Qt::AlignVCenter, low ? "#ba1a1a" : "", low));
        m_table->setCellWidget(r, 9, wrapW(W::badge(p.stock == 0 ? "Out" : low ? "Low stock" : "In stock", p.stock == 0 ? "red" : low ? "amber" : "green")));
        auto* d = W::button("Details", "mini");
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
        cl->addStretch();
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
