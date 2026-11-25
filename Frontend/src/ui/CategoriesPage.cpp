#include "CategoriesPage.h"
#include "Theme.h"
#include "UiKit.h"
#include "UiStore.h"
#include "Widgets.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QRegularExpressionValidator>
#include <QTableWidget>
#include <QVBoxLayout>

using namespace Ui;

CategoriesPage::CategoriesPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root;
    pageShell(this, root, true);
    root->setSpacing(8);

    m_summary = W::label("", "muted");
    root->addWidget(sectionHeader({}, "Product Categories", "Manage category prefixes and barcode routing."));

    auto *info = W::card(this, "subCard");
    auto *ih = new QHBoxLayout(info);
    ih->setContentsMargins(16, 12, 16, 12);
    ih->setSpacing(12);
    ih->addWidget(W::iconLabel("info", Theme::primary, 24), 0, Qt::AlignTop);
    auto *it = new QVBoxLayout;
    it->setSpacing(2);
    auto *t = W::label("The first two barcode digits identify the product category.", "h2");
    t->setWordWrap(true);
    auto *d = W::label("Linked category IDs are protected. The barcode uses the category ID, a 5-digit serial, and a check digit.", "muted");
    d->setWordWrap(true);
    it->addWidget(t);
    it->addWidget(d);
    ih->addLayout(it, 1);
    root->addWidget(info);

    auto *row = new QHBoxLayout;
    row->setSpacing(8);

    // ---- list
    auto *list = W::card(this, "subCard");
    auto *lv = new QVBoxLayout(list);
    lv->setContentsMargins(16, 14, 16, 12);
    lv->setSpacing(8);
    auto *lh = new QHBoxLayout;
    lh->setSpacing(8);
    lh->addWidget(W::label("Department Index", "h1"));
    lh->addStretch();
    m_filter = new QLineEdit;
    m_filter->setPlaceholderText("Filter categories by prefix or name");
    m_filter->setAccessibleName("Filter categories by prefix or title");
    m_filter->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    m_filter->setClearButtonEnabled(true);
    lh->addWidget(m_filter, 1);
    lv->addLayout(lh);
    m_table = makeTable({"ID", "CATEGORY NAME", "ACRONYM", "PRODUCTS", "BARCODE RANGE", "ACTIONS"});
    m_table->setProperty("managementTable", true);
    m_table->setAlternatingRowColors(true);
    m_table->setAccessibleName("Department index");
    m_table->setWordWrap(false);
    auto *hh = m_table->horizontalHeader();
    hh->setSectionResizeMode(1, QHeaderView::Interactive);
    m_table->setColumnWidth(1, 126);
    for (int c : {0, 2, 3, 4})
        hh->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(5, QHeaderView::Fixed);
    m_table->setColumnWidth(5, 93);
    hh->setStretchLastSection(true);
    lv->addWidget(m_table, 1);
    lv->addWidget(m_summary);
    row->addWidget(list, 62);

    // ---- form
    auto *form = W::card(this, "subCard");
    auto *fv = new QVBoxLayout(form);
    fv->setContentsMargins(16, 16, 16, 16);
    fv->setSpacing(6);
    m_formTitle = W::label("Create New Category", "h1");
    fv->addWidget(m_formTitle);
    auto *description = W::label("Assign a two-digit barcode prefix.", "muted");
    description->setWordWrap(true);
    fv->addWidget(description);

    fv->addWidget(W::label("CATEGORY ID (2 DIGITS)", "caption"));
    m_id = new QLineEdit;
    m_id->setMaxLength(2);
    m_id->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d{0,2}"), m_id));
    m_id->setMinimumHeight(36);
    m_id->setAccessibleName("Category ID");
    m_id->setPlaceholderText("e.g. 80");
    fv->addWidget(m_id);

    m_errorBox = W::card(form, "notice");
    auto *eh = new QHBoxLayout(m_errorBox);
    eh->setContentsMargins(12, 8, 12, 8);
    eh->addWidget(W::iconLabel("warn", Theme::error, 18), 0, Qt::AlignTop);
    m_error = W::label("");
    m_error->setWordWrap(true);
    eh->addWidget(m_error, 1);
    fv->addWidget(m_errorBox);
    m_suggest = W::label("", "muted");
    m_suggest->setWordWrap(true);
    fv->addWidget(m_suggest);

    fv->addWidget(W::label("CATEGORY NAME", "caption"));
    m_name = new QLineEdit;
    m_name->setMinimumHeight(36);
    m_name->setAccessibleName("Category name");
    m_name->setPlaceholderText("e.g. Frozen Foods");
    fv->addWidget(m_name);
    fv->addWidget(W::label("DEPARTMENT ACRONYM (MAX 3)", "caption"));
    m_acr = new QLineEdit;
    m_acr->setMaxLength(3);
    m_acr->setMinimumHeight(36);
    m_acr->setAccessibleName("Department acronym");
    m_acr->setPlaceholderText("e.g. FRZ");
    fv->addWidget(m_acr);
    auto *rangeCard = W::card(form, "field");
    auto *rangeLayout = new QVBoxLayout(rangeCard);
    rangeLayout->setContentsMargins(10, 8, 10, 8);
    m_range = W::label("", "mono");
    m_range->setWordWrap(true);
    rangeLayout->addWidget(m_range);
    fv->addWidget(rangeCard);
    fv->addStretch();

    auto *btns = new QHBoxLayout;
    auto *reset = W::button("Reset Form", "soft");
    m_submit = W::button("Add Category", "primary", "plus");
    m_submit->setMinimumHeight(40);
    btns->addWidget(reset);
    btns->addWidget(m_submit, 1);
    fv->addLayout(btns);
    row->addWidget(form, 38);
    root->addLayout(row, 1);

    connect(m_filter, &QLineEdit::textChanged, this, [this] { rebuild(); });
    for (auto *e : {m_id, m_name, m_acr})
        connect(e, &QLineEdit::textChanged, this, [this] { validate(); });
    connect(reset, &QPushButton::clicked, this, &CategoriesPage::resetForm);
    connect(m_submit, &QPushButton::clicked, this, &CategoriesPage::submit);
    connect(&UiStore::instance(), &UiStore::categoriesChanged, this, &CategoriesPage::refresh);
    connect(&AppState::instance(), &AppState::changed, this, &CategoriesPage::refresh);
    resetForm();
    refresh();
}

void CategoriesPage::refresh() { rebuild(); validate(); }

void CategoriesPage::rebuild()
{
    auto &store = UiStore::instance();
    const QString q = m_filter->text().trimmed();
    QVector<Category> rows;
    int skus = 0;
    for (const auto &c : store.categories())
    {
        skus += store.productCount(c.id);
        if (!q.isEmpty() && !c.id.contains(q) && !c.name.contains(q, Qt::CaseInsensitive) &&
            !c.acronym.contains(q, Qt::CaseInsensitive))
            continue;
        rows << c;
    }
    m_summary->setText(QString("%1 of %2 categories · %3 linked products").arg(rows.size()).arg(store.categories().size()).arg(skus));
    m_table->setRowCount(0);
    m_table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r)
    {
        const Category c = rows[r];
        m_table->setRowHeight(r, 46);
        m_table->setItem(r, 0, item(c.id, Qt::AlignLeft | Qt::AlignVCenter, "#004ac6", true));
        auto *name = item(c.name, Qt::AlignLeft | Qt::AlignVCenter, "", true);
        name->setToolTip(c.name);
        m_table->setItem(r, 1, name);
        m_table->setCellWidget(r, 2, wrapW(W::badge(c.acronym, "blue")));
        m_table->setItem(r, 3, item(QString::number(store.productCount(c.id))));
        m_table->setItem(r, 4, item(store.rangeText(c.id)));
        auto *cell = new QWidget;
        auto *h = new QHBoxLayout(cell);
        h->setContentsMargins(4, 0, 4, 0);
        h->setSpacing(4);
        auto *e = W::button("", "flat", "edit");
        auto *x = W::button("", "flat", "trash");
        e->setToolTip("Edit");
        x->setToolTip("Delete");
        e->setAccessibleName("Edit " + c.name);
        x->setAccessibleName("Delete " + c.name);
        h->addWidget(e);
        h->addWidget(x);
        h->addStretch();
        m_table->setCellWidget(r, 5, cell);
        const QString id = c.id;
        connect(e, &QPushButton::clicked, this, [this, id] { edit(id); });
        connect(x, &QPushButton::clicked, this, [this, id]
                {
            QString err;
            if (QMessageBox::question(this, "Delete category", "Delete category " + id + "?") != QMessageBox::Yes)
                return;
            if (!UiStore::instance().removeCategory(id, &err)) { emit toast(err); return; }
            emit toast("Category " + id + " deleted"); });
    }
}

void CategoriesPage::validate()
{
    const QString id = m_id->text().trimmed();
    m_range->setText(id.size() == 2 ? "Barcode range: " + UiStore::instance().rangeText(id)
                                   : "Enter a category ID to see its barcode range.");
    const bool pristine = id.isEmpty() && m_name->text().isEmpty() && m_acr->text().isEmpty();
    QString why;
    if (!pristine)
        why = UiStore::instance().validateCategory(id, m_name->text(), m_acr->text(), m_editing);
    // do not nag about fields the user has not reached yet
    if (id.size() == 2 && m_name->text().trimmed().isEmpty() && m_acr->text().trimmed().isEmpty() && why.startsWith("Category name"))
        why.clear();
    if (why.startsWith("Acronym must") && m_acr->text().isEmpty())
        why.clear();
    m_errorBox->setVisible(!why.isEmpty());
    m_error->setText(why);
    const bool idTaken = why.contains("already in use");
    m_suggest->setVisible(idTaken);
    m_suggest->setText("Suggested free ids:  " + UiStore::instance().freeIds(3).join("   "));
    m_submit->setEnabled(!pristine && UiStore::instance().validateCategory(id, m_name->text(), m_acr->text(), m_editing).isEmpty());
}

void CategoriesPage::resetForm()
{
    m_editing.clear();
    m_id->setEnabled(true);
    m_id->clear();
    m_name->clear();
    m_acr->clear();
    m_formTitle->setText("Create New Category");
    m_submit->setText("Add Category");
    validate();
}

void CategoriesPage::edit(const QString &id)
{
    const Category *c = UiStore::instance().category(id);
    if (!c)
        return;
    m_editing = id;
    m_id->setText(c->id);
    m_name->setText(c->name);
    m_acr->setText(c->acronym);
    m_formTitle->setText("Edit Category " + id);
    m_submit->setText("Save Category");
    validate();
}

void CategoriesPage::submit()
{
    Category c;
    c.id = m_id->text().trimmed();
    c.name = m_name->text().trimmed();
    c.acronym = m_acr->text().trimmed().toUpper();
    c.color = "#2554d8";
    QString err;
    const bool editing = !m_editing.isEmpty();
    const bool ok = editing ? UiStore::instance().updateCategory(m_editing, c, &err)
                            : UiStore::instance().addCategory(c, &err);
    if (!ok)
    {
        emit toast(err);
        return;
    }
    emit toast(QString("Category %1 %2").arg(c.id, editing ? "updated" : "added"));
    resetForm();
}
