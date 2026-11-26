#include <iterator>
#include "InventoryPage.h"
#include "Theme.h"
#include "UiKit.h"
#include "UiStore.h"
#include "Widgets.h"
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

using namespace Ui;

namespace
{
    QString stockKind(int qty, int limit)
    {
        return qty <= 0 ? "red" : qty <= limit ? "amber" : "green";
    }
    QString stockText(int qty, int limit)
    {
        return qty <= 0 ? "Out of stock" : qty <= limit ? "Low stock" : "In stock";
    }
}

InventoryPage::InventoryPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root;
    pageShell(this, root);
    root->setSpacing(8);

    auto *exportBtn = W::button("Export Stock Ledger", "soft", "export");
    auto *newBtn = W::button("New SKU Entry", "primary", "plus");
    root->addWidget(sectionHeader({}, "Inventory Management",
                                  "Monitor stock and stage quantity adjustments.",
                                  {exportBtn, newBtn}));
    connect(newBtn, &QPushButton::clicked, this, &InventoryPage::newProduct);

    auto *tiles = new QHBoxLayout;
    tiles->setSpacing(8);
    tiles->addWidget(statTile("CATALOGUE DEPTH", "products", m_total, m_totalSub, "blue"));
    tiles->addWidget(statTile("LOW STOCK", "warn", m_low, m_lowSub, "amber"));
    tiles->addWidget(statTile("OUT OF STOCK", "cart", m_out, m_outSub, "red"));
    root->addLayout(tiles);

    auto *card = W::card(this, "subCard");
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(16, 14, 16, 12);
    v->setSpacing(8);

    auto *tools = new QHBoxLayout;
    tools->setSpacing(8);
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Search inventory");
    m_search->setToolTip("Search by product ID, name, acronym, or barcode.");
    m_search->setAccessibleName("Search inventory");
    m_search->setClearButtonEnabled(true);
    m_search->setMinimumHeight(40);
    m_search->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    m_category = new QComboBox;
    m_category->setAccessibleName("Inventory category");
    m_category->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_category->setMinimumContentsLength(14);
    m_category->setMinimumHeight(40);
    m_lowOnly = new QCheckBox("Low stock only");
    m_saveTop = W::button("Save Changes", "primary", "check");
    m_saveTop->setMinimumHeight(40);
    tools->addWidget(m_search, 3);
    tools->addWidget(m_category, 2);
    tools->addWidget(m_lowOnly);
    tools->addWidget(m_saveTop);
    v->addLayout(tools);

    m_table = makeTable({"PRODUCT ID", "PRODUCT", "CATEGORY / ACRONYM", "AVAILABLE QUANTITY", "STATUS", "UNIT PRICE"});
    m_table->setProperty("managementTable", true);
    m_table->setAlternatingRowColors(true);
    m_table->setAccessibleName("Inventory stock");
    auto *hh = m_table->horizontalHeader();
    hh->setSectionResizeMode(1, QHeaderView::Interactive);
    m_table->setColumnWidth(1, 224);
    for (int c : {0, 5})
        hh->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(2, QHeaderView::Interactive);
    m_table->setColumnWidth(2, 142);
    hh->setSectionResizeMode(3, QHeaderView::Fixed);
    hh->setSectionResizeMode(4, QHeaderView::Fixed);
    m_table->setColumnWidth(3, 155);
    m_table->setColumnWidth(4, 112);
    hh->setStretchLastSection(true);
    v->addWidget(m_table, 1);

    m_footer = W::label("", "muted");
    v->addWidget(m_footer);

    // Unsaved changes bar
    m_bar = new QFrame;
    m_bar->setObjectName("totalBox");
    auto *bh = new QHBoxLayout(m_bar);
    bh->setContentsMargins(16, 10, 12, 10);
    bh->setSpacing(10);
    bh->addWidget(W::iconLabel("warn", QColor("#ffc24a"), 20));
    m_barText = W::label("");
    m_barText->setStyleSheet("color:white; font-weight:700;");
    bh->addWidget(m_barText, 1);
    auto *disc = W::button("Discard All", "soft", "refresh");
    auto *save = W::button("Save && Commit Stock", "primary", "check");
    bh->addWidget(disc);
    bh->addWidget(save);
    v->addWidget(m_bar);
    m_bar->hide();
    root->addWidget(card, 1);

    connect(m_search, &QLineEdit::textChanged, this, [this] { rebuild(); });
    connect(m_category, &QComboBox::currentIndexChanged, this, [this] { if (!m_building) rebuild(); });
    connect(m_lowOnly, &QCheckBox::toggled, this, [this] { rebuild(); });
    connect(m_saveTop, &QPushButton::clicked, this, &InventoryPage::save);
    connect(save, &QPushButton::clicked, this, &InventoryPage::save);
    connect(disc, &QPushButton::clicked, this, &InventoryPage::discard);
    connect(exportBtn, &QPushButton::clicked, this, [this]
            {
        QStringList rows{"Product ID,Barcode,Acronym,Name,Category,Quantity,Status"};
        const int limit = AppState::instance().settings().lowStockThreshold;
        for (const auto &p : AppState::instance().products())
        {
            const int q = m_edits.value(p.id, p.stock);
            rows << QString("%1,%2,%3,%4,%5,%6,%7").arg(p.id).arg(p.ean, p.sku, csvField(p.name), p.categoryId).arg(q).arg(stockText(q, limit).toUpper());
        }
        if (writeCsv(this, "inventory.csv", rows)) emit toast("Stock ledger exported"); });
    connect(&AppState::instance(), &AppState::changed, this, &InventoryPage::refresh);
    connect(&UiStore::instance(), &UiStore::categoriesChanged, this, &InventoryPage::refresh);
    refresh();
}

void InventoryPage::refresh()
{
    m_building = true;
    const QString keep = m_category->currentData().toString();
    m_category->clear();
    m_category->addItem("All Categories", QString());
    for (const auto &c : UiStore::instance().categories())
        m_category->addItem(QString("%1 - %2").arg(c.id, c.name), c.id);
    const int idx = m_category->findData(keep);
    m_category->setCurrentIndex(idx < 0 ? 0 : idx);
    m_building = false;
    rebuild();
}

void InventoryPage::rebuild()
{
    auto &st = AppState::instance();
    const int limit = st.settings().lowStockThreshold;
    const QString q = m_search->text().trimmed();
    const QString cat = m_category->currentData().toString();

    // drop staged values for products that no longer exist
    for (auto it = m_edits.begin(); it != m_edits.end();)
    {
        bool found = false;
        for (const auto &p : st.products())
            if (p.id == it.key()) { found = true; break; }
        it = found ? std::next(it) : m_edits.erase(it);
    }

    int low = 0, out = 0;
    double value = 0;
    for (const auto &p : st.products())
    {
        if (p.stock <= 0) ++out;
        else if (p.stock <= limit) ++low;
        value += p.price * p.stock;
    }
    m_total->setText(QString::number(st.products().size()) + " SKUs");
    m_totalSub->setText("Stock value: " + W::money(value));
    m_low->setText(QString::number(low) + " items");
    m_lowSub->setText(QString("At or below %1 units").arg(limit));
    m_out->setText(QString("%1 SKU%2").arg(out).arg(out == 1 ? "" : "s"));
    m_outSub->setText("Zero units available");

    QVector<ProductInfo> rows;
    const QVector<ProductInfo> base = q.isEmpty() ? st.products() : st.searchText(q);
    for (const auto &p : base)
    {
        const int qty = m_edits.value(p.id, p.stock);
        if (!cat.isEmpty() && p.categoryId != cat) continue;
        if (m_lowOnly->isChecked() && qty > limit) continue;
        rows << p;
    }

    m_table->setRowCount(0);
    m_table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r)
    {
        const ProductInfo p = rows[r];
        m_table->setRowHeight(r, 52);
        const Category *c = UiStore::instance().category(p.categoryId);
        m_table->setItem(r, 0, item("PRD-" + QString::number(p.id).rightJustified(4, '0'), Qt::AlignLeft | Qt::AlignVCenter, "#004ac6", true));
        auto *product = item(p.name + "\nEAN: " + p.ean + "  -  " + p.unit);
        product->setToolTip(product->text());
        m_table->setItem(r, 1, product);
        m_table->setItem(r, 2, item(p.sku + "\n" + (c ? c->name : QString("Category " + p.categoryId))));
        m_table->setItem(r, 5, item(W::money(p.price), Qt::AlignRight | Qt::AlignVCenter));

        auto *cell = new QWidget;
        auto *h = new QHBoxLayout(cell);
        h->setContentsMargins(6, 0, 6, 0);
        h->setSpacing(4);
        auto *minus = W::button("-", "step");
        auto *plus = W::button("+", "step");
        auto *spin = new QSpinBox;
        spin->setAccessibleName("Quantity for " + p.name);
        minus->setAccessibleName("Decrease quantity for " + p.name);
        plus->setAccessibleName("Increase quantity for " + p.name);
        spin->setRange(0, 999999);
        spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
        spin->setAlignment(Qt::AlignCenter);
        spin->setValue(m_edits.value(p.id, p.stock));
        spin->setMinimumWidth(64);
        h->addWidget(minus);
        h->addWidget(spin);
        h->addWidget(plus);
        m_table->setCellWidget(r, 3, cell);
        auto *badge = W::badge(stockText(spin->value(), limit), stockKind(spin->value(), limit));
        m_table->setCellWidget(r, 4, wrapW(badge));
        connect(minus, &QPushButton::clicked, spin, [spin] { spin->setValue(spin->value() - 1); });
        connect(plus, &QPushButton::clicked, spin, [spin] { spin->setValue(spin->value() + 1); });
        connect(spin, qOverload<int>(&QSpinBox::valueChanged), this, [this, spin, badge, p, limit](int v)
                {
            if (v == p.stock) m_edits.remove(p.id); else m_edits[p.id] = v;
            W::setBadge(badge, stockText(v, limit), stockKind(v, limit));
            spin->setProperty("staged", v != p.stock);
            Theme::repolish(spin);
            updateBar(); });
        spin->setProperty("staged", m_edits.contains(p.id));
        Theme::repolish(spin);
    }
    m_footer->setText(QString("Displaying %1 of %2 registered items").arg(rows.size()).arg(st.products().size()));
    updateBar();
}

void InventoryPage::updateBar()
{
    const int n = m_edits.size();
    m_bar->setVisible(n > 0);
    m_barText->setText(QString("Unsaved quantity changes: %1 item%2 modified").arg(n).arg(n == 1 ? "" : "s"));
    m_saveTop->setEnabled(n > 0);
}

void InventoryPage::save()
{
    if (m_edits.isEmpty())
        return;
    QString err;
    const int n = m_edits.size();
    if (!AppState::instance().setStockLevels(m_edits, &err))
    {
        emit toast(err.isEmpty() ? "Could not save stock levels" : err);
        return;
    }
    m_edits.clear();
    rebuild();
    emit toast(QString("Stock updated for %1 product%2").arg(n).arg(n == 1 ? "" : "s"));
}

void InventoryPage::discard()
{
    m_edits.clear();
    rebuild();
    emit toast("Changes discarded");
}
