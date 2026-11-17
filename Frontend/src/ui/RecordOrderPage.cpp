#include "RecordOrderPage.h"
#include "AppState.h"
#include "Dialogs.h"
#include "Theme.h"
#include "Widgets.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QPainter>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QtMath>

namespace
{
    QWidget *twoLine(const QString &top, const QString &bottom)
    {
        auto *w = new QWidget;
        auto *v = new QVBoxLayout(w);
        v->setContentsMargins(4, 4, 4, 4);
        v->setSpacing(1);
        auto *t = W::label(top);
        t->setStyleSheet("font-weight:600; font-size:13.5px;");
        t->setWordWrap(true);
        auto *b = W::label(bottom);
        b->setObjectName("subtle");
        b->setStyleSheet("font-size:11.5px;");
        v->addWidget(t);
        v->addWidget(b);
        return w;
    }
    QTableWidgetItem *cell(const QString &text, const QString &color = "", bool bold = false)
    {
        auto *i = new QTableWidgetItem(text);
        i->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (!color.isEmpty())
            i->setForeground(QColor(color));
        if (bold)
        {
            QFont f = i->font();
            f.setBold(true);
            i->setFont(f);
        }
        return i;
    }
}

RecordOrderPage::RecordOrderPage(QWidget *parent) : QWidget(parent)
{
    auto *root = W::pageLayout(this);
    root->addWidget(buildCustomerStrip());
    auto *body = new QHBoxLayout;
    body->setSpacing(0);
    body->addWidget(buildLeft(), 36);
    body->addWidget(buildRight(), 64);
    root->addLayout(body, 1);

    connect(&AppState::instance(), &AppState::scanned, this, [this](const QString &ean)
            { showCode(ean, true); });
    loadDemoDraft();
    // cell colours and the thumbnail are painted in code, so redraw them when the mode flips
    connect(&Theme::notifier(), &ThemeNotifier::changed, this, [this]
            { rebuildTable(); setPreview(m_preview); });
}

QWidget *RecordOrderPage::buildCustomerStrip()
{
    auto *strip = W::card(this, "strip");
    auto *h = new QHBoxLayout(strip);
    h->setContentsMargins(14, 8, 14, 8);
    h->setSpacing(12);
    h->addWidget(W::iconTile("user", Theme::primary, 40));

    auto field = [&](const QString &cap, QWidget *content, int stretch)
    {
        auto *f = new QFrame;
        f->setObjectName("field");
        auto *v = new QVBoxLayout(f);
        v->setContentsMargins(12, 5, 12, 6);
        v->setSpacing(0);
        v->addWidget(W::label(cap, "caption"));
        v->addWidget(content);
        h->addWidget(f, stretch);
    };
    m_customer = new QLineEdit("Marcus Vance");
    m_customer->setProperty("flat", true);
    m_customer->setPlaceholderText("Walk-in customer");
    field("CUSTOMER", m_customer, 3);
    m_contact = new QLineEdit("+1 (555) 019-2834");
    m_contact->setProperty("flat", true);
    m_contact->setPlaceholderText("Phone (optional)");
    field("PHONE", m_contact, 3);

    m_vipBtn = W::button("VIP 5% off", "tab", "tag");
    m_vipBtn->setCheckable(true);
    m_vipBtn->setChecked(true);
    m_vipBtn->setToolTip("Toggle the loyalty discount");
    h->addWidget(m_vipBtn);
    connect(m_vipBtn, &QPushButton::toggled, this, [this](bool on)
            {
        m_vip = on;
        m_vipBtn->setIcon(Theme::icon("tag", on ? QColor("white") : Theme::primary, 16));
        rebuildTable(); });
    m_vipBtn->setIcon(Theme::icon("tag", QColor("white"), 16));

    h->addStretch(1);
    auto *sv = new QVBoxLayout;
    sv->setSpacing(0);
    sv->addWidget(W::label("ORDER", "caption"));
    m_sessionId = W::label("#" + AppState::instance().nextOrderId(), "mono");
    m_sessionId->setStyleSheet("font-weight:700; font-size:14px;");
    sv->addWidget(m_sessionId);
    h->addLayout(sv);
    return strip;
}

QWidget *RecordOrderPage::buildLeft()
{
    auto *col = new QWidget;
    auto *v = new QVBoxLayout(col);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    auto *cam = W::card(col);
    auto *cv = new QVBoxLayout(cam);
    cv->setContentsMargins(16, 14, 16, 14);
    cv->setSpacing(10);
    W::headerRow(cv, "camera", "Scanner");
    m_feed = new CameraFeedWidget;
    m_feed->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cv->addWidget(m_feed, 1);

    auto *btns = new QHBoxLayout;
    btns->setSpacing(8);
    m_camToggle = W::button("Stop camera", "soft", "stop");
    auto *scan = W::button("Scan  [F9]", "primary", "barcode");
    btns->addWidget(m_camToggle, 1);
    btns->addWidget(scan, 1);
    cv->addLayout(btns);
    connect(m_camToggle, &QPushButton::clicked, this, [this]
            {
        const bool on = !m_feed->running();
        m_feed->setRunning(on);
        m_camToggle->setText(on ? "Stop camera" : "Start camera");
        m_camToggle->setIcon(Theme::icon(on ? "stop" : "play", Theme::primary, 16)); });
    connect(scan, &QPushButton::clicked, this, &RecordOrderPage::simulateScan);

    auto *mr = new QHBoxLayout;
    mr->setSpacing(8);
    m_manual = new QLineEdit;
    m_manual->setPlaceholderText("Type a barcode and press Enter");
    m_manual->setMaxLength(12);
    m_manual->setMinimumHeight(40);
    m_manual->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    auto *browse = W::button("Browse", "soft", "products");
    browse->setMinimumHeight(40);
    mr->addWidget(m_manual, 1);
    mr->addWidget(browse);
    cv->addLayout(mr);
    connect(m_manual, &QLineEdit::returnPressed, this, [this]
            { showCode(m_manual->text(), false); });
    connect(browse, &QPushButton::clicked, this, &RecordOrderPage::quickLookup);
    v->addWidget(cam, 5);

    auto *pc = W::card(col);
    auto *pv = new QVBoxLayout(pc);
    pv->setContentsMargins(16, 14, 16, 14);
    pv->setSpacing(10);
    m_pBadge = W::badge("—", "grey");
    W::headerRow(pv, "shield", "Detected product", m_pBadge);

    auto *info = W::card(pc, "subCard");
    auto *ih = new QHBoxLayout(info);
    ih->setContentsMargins(12, 10, 12, 10);
    ih->setSpacing(12);
    m_pThumb = new QLabel;
    m_pThumb->setFixedSize(60, 72);
    m_pThumb->setAlignment(Qt::AlignCenter);
    ih->addWidget(m_pThumb);
    auto *iv = new QVBoxLayout;
    iv->setSpacing(2);
    iv->addStretch(1);
    auto *nr = new QHBoxLayout;
    m_pName = W::label("Nothing scanned yet", "h1");
    m_pName->setWordWrap(true);
    m_pPrice = W::label("", "price");
    nr->addWidget(m_pName, 1);
    nr->addWidget(m_pPrice);
    iv->addLayout(nr);
    m_pMeta = W::label("Scan a barcode or type one above.", "muted");
    m_pMeta->setWordWrap(true);
    iv->addWidget(m_pMeta);
    m_pStock = W::label("");
    m_pStock->setStyleSheet("font-weight:700;");
    iv->addWidget(m_pStock);
    iv->addStretch(1);
    ih->addLayout(iv, 1);
    pv->addWidget(info, 1);

    m_addBtn = W::button("Add to order", "primary", "cart");
    m_addBtn->setMinimumHeight(42);
    m_addBtn->setEnabled(false);
    pv->addWidget(m_addBtn);
    connect(m_addBtn, &QPushButton::clicked, this, [this]
            { if (m_preview) addToCart(*m_preview); });
    v->addWidget(pc, 3);
    return col;
}

QWidget *RecordOrderPage::buildRight()
{
    auto *col = new QWidget;
    auto *v = new QVBoxLayout(col);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    auto *oc = W::card(col);
    auto *ov = new QVBoxLayout(oc);
    ov->setContentsMargins(0, 14, 0, 10);
    ov->setSpacing(6);

    auto *head = new QHBoxLayout;
    head->setContentsMargins(18, 0, 14, 0);
    head->setSpacing(10);
    head->addWidget(W::iconLabel("orders", Theme::primary, 20));
    head->addWidget(W::label("Current order", "h1"));
    m_itemsBadge = W::badge("0 items", "grey");
    head->addWidget(m_itemsBadge);
    head->addStretch();
    auto *custom = W::button("Custom item", "flat", "plus");
    auto *clear = W::button("Clear", "flat", "trash");
    head->addWidget(custom);
    head->addWidget(clear);
    ov->addLayout(head);
    connect(clear, &QPushButton::clicked, this, &RecordOrderPage::newDraft);
    connect(custom, &QPushButton::clicked, this, [this]
            {
        bool ok = false;
        const QString name = QInputDialog::getText(this, "Custom item", "Description:", QLineEdit::Normal, "", &ok);
        if (!ok || name.trimmed().isEmpty()) return;
        const double price = QInputDialog::getDouble(this, "Custom item", "Unit price ($):", 1.0, 0.01, 100000, 2, &ok);
        if (!ok) return;
        Product p;
        p.sku = QString("CUSTOM-%1").arg(m_cart.size() + 1, 2, 10, QChar('0'));
        p.name = name.trimmed();
        p.category = "Custom";
        p.price = price;
        p.stock = 999;
        m_cart.push_back({p, 1});
        rebuildTable(); });

    m_table = new QTableWidget(0, 6);
    m_table->setHorizontalHeaderLabels({"PRODUCT", "PRICE", "QTY", "DISCOUNT", "TOTAL", ""});
    m_table->verticalHeader()->hide();
    m_table->setShowGrid(false);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *hh = m_table->horizontalHeader();
    hh->setSectionResizeMode(QHeaderView::Fixed);
    hh->setSectionResizeMode(0, QHeaderView::Stretch);
    hh->setHighlightSections(false);
    hh->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_table->setColumnWidth(1, 72);
    m_table->setColumnWidth(2, 104);
    m_table->setColumnWidth(3, 92);
    m_table->setColumnWidth(4, 84);
    m_table->setColumnWidth(5, 46);
    W::alignHeaders(m_table, {1, 3, 4}, Qt::AlignRight);
    W::alignHeaders(m_table, {2}, Qt::AlignHCenter);
    m_table->setWordWrap(false);
    ov->addWidget(m_table, 1);
    v->addWidget(oc, 1);

    auto *tc = W::card(col, "totals");
    auto *th = new QHBoxLayout(tc);
    th->setContentsMargins(18, 14, 18, 14);
    th->setSpacing(18);
    auto *grid = new QVBoxLayout;
    grid->setSpacing(8);
    auto row = [&](const QString &k, QLabel *&val)
    {
        auto *r = new QHBoxLayout;
        r->addWidget(W::label(k, "muted"));
        r->addStretch();
        val = W::label("$0.00");
        val->setStyleSheet("font-weight:700; font-size:14px;");
        r->addWidget(val);
        grid->addLayout(r);
    };
    row("Subtotal", m_gross);
    row("VIP discount", m_disc);
    row("GST included", m_tax);
    grid->addStretch();
    th->addLayout(grid, 1);

    auto *tb = W::card(tc, "totalBox");
    auto *tv = new QVBoxLayout(tb);
    tv->setContentsMargins(18, 12, 18, 14);
    tv->setSpacing(2);
    auto *cap = W::label("TOTAL", "caption");
    tv->addWidget(cap);
    m_total = W::label("$0.00");
    m_total->setStyleSheet("font-size:32px; font-weight:700; color:white;");
    tv->addWidget(m_total);
    m_checkout = W::button("Checkout  [F10]", "primary", "check");
    m_checkout->setMinimumHeight(44);
    tv->addWidget(m_checkout);
    th->addWidget(tb, 1);
    connect(m_checkout, &QPushButton::clicked, this, &RecordOrderPage::checkout);
    v->addWidget(tc);
    return col;
}

// ---------------------------------------------------------------- behaviour
void RecordOrderPage::focusInput()
{
    m_manual->setFocus();
    m_manual->selectAll();
}

void RecordOrderPage::simulateScan()
{
    if (!m_feed->running())
    {
        emit toast("Camera is stopped - start it first");
        return;
    }
    const auto &ps = AppState::instance().products();
    if (ps.isEmpty())
        return;
    showCode(ps[m_scanIndex++ % ps.size()].ean, true);
}

void RecordOrderPage::quickLookup()
{
    QStringList items;
    for (const auto &p : AppState::instance().products())
        items << QString("%1  -  %2").arg(p.ean, p.name);
    bool ok = false;
    const QString pick = QInputDialog::getItem(this, "Browse products", "Choose a product:", items, 0, false, &ok);
    if (ok)
        showCode(pick.section("  -  ", 0, 0), false);
}

void RecordOrderPage::showCode(const QString &codeIn, bool fromScanner)
{
    const QString code = codeIn.trimmed();
    if (code.isEmpty())
    {
        emit toast("Enter a barcode first");
        return;
    }
    const auto p = AppState::instance().findByCode(code);
    if (!p)
    {
        setPreview(std::nullopt);
        emit toast("No product found for \"" + code + "\"");
        return;
    }
    m_feed->setCode(p->ean);
    setPreview(p);
    m_manual->clear();
    if (fromScanner && AppState::instance().settings().autoAddScans)
        addToCart(*p);
}

void RecordOrderPage::setPreview(const std::optional<Product> &p)
{
    m_preview = p;
    m_addBtn->setEnabled(p.has_value());
    if (!p)
    {
        m_pName->setText("Nothing scanned yet");
        m_pPrice->clear();
        m_pMeta->setText("Scan a barcode or type one above.");
        m_pStock->clear();
        W::setBadge(m_pBadge, "—", "grey");
        m_pThumb->clear();
        return;
    }
    m_pName->setText(p->name);
    m_pPrice->setText(W::money(p->price));
    m_pMeta->setText(QString("%1  •  %2  •  %3").arg(p->ean, p->category, p->sku));
    const bool low = p->stock < p->reorderLimit;
    m_pStock->setText(QString("%1 in stock").arg(p->stock));
    m_pStock->setStyleSheet(QString("font-weight:700; color:%1;")
                                .arg((low ? Theme::dangerText() : Theme::successText()).name()));
    W::setBadge(m_pBadge, p->stock == 0 ? "Out of stock" : low ? "Low stock" : "In stock",
                p->stock == 0 ? "red" : low ? "amber" : "green");

    // flat product tile: tinted rounded square with the product's initials
    QPixmap pm(120, 144);
    pm.setDevicePixelRatio(2);
    pm.fill(Qt::transparent);
    QPainter pa(&pm);
    pa.setRenderHint(QPainter::Antialiasing);
    pa.setPen(Qt::NoPen);
    pa.setBrush(Theme::palette().thumbBg);
    pa.drawRoundedRect(QRectF(2, 6, 56, 56), 12, 12);
    pa.setPen(Theme::accentText());
    QFont f = pa.font();
    f.setPixelSize(19);
    f.setBold(true);
    pa.setFont(f);
    QString initials;
    for (const QString &w : p->name.split(' ', Qt::SkipEmptyParts))
        if (initials.size() < 2 && w[0].isLetter())
            initials += w[0].toUpper();
    pa.drawText(QRectF(2, 6, 56, 56), Qt::AlignCenter, initials);
    pa.end();
    m_pThumb->setPixmap(pm);
}

void RecordOrderPage::addToCart(const Product &p, int qty)
{
    int have = 0;
    for (const auto &l : m_cart)
        if (l.p.sku == p.sku)
            have = l.qty;
    if (have + qty > p.stock)
    {
        emit toast(QString("Only %1 in stock for %2").arg(p.stock).arg(p.name));
        return;
    }
    bool found = false;
    for (auto &l : m_cart)
        if (l.p.sku == p.sku)
        {
            l.qty += qty;
            found = true;
        }
    if (!found)
        m_cart.push_back({p, qty});
    rebuildTable();
    emit toast(QString("Added %1 × %2").arg(qty).arg(p.name));
}

void RecordOrderPage::changeQty(int row, int delta)
{
    if (row < 0 || row >= m_cart.size())
        return;
    auto &l = m_cart[row];
    const int nq = l.qty + delta;
    if (nq < 1)
    {
        removeRow(row);
        return;
    }
    if (nq > l.p.stock)
    {
        emit toast(QString("Only %1 in stock for %2").arg(l.p.stock).arg(l.p.name));
        return;
    }
    l.qty = nq;
    rebuildTable();
}

void RecordOrderPage::removeRow(int row)
{
    if (row < 0 || row >= m_cart.size())
        return;
    m_cart.removeAt(row);
    rebuildTable();
}

void RecordOrderPage::rebuildTable()
{
    const auto &st = AppState::instance().settings();
    const double vipPct = m_vip ? st.vipDiscountPercent : 0;
    m_table->setRowCount(0);
    m_table->setRowCount(m_cart.size());
    for (int r = 0; r < m_cart.size(); ++r)
    {
        const auto &l = m_cart[r];
        const double gross = l.p.price * l.qty;
        const double disc = gross * vipPct / 100.0;
        m_table->setRowHeight(r, 62);
        m_table->setCellWidget(r, 0, twoLine(l.p.name, l.p.sku + "  •  " + l.p.category));
        m_table->setItem(r, 1, cell(W::money(l.p.price), "", true));
        auto *stp = new QtyStepper(l.qty);
        m_table->setCellWidget(r, 2, stp);
        connect(stp, &QtyStepper::minus, this, [this, r]
                { changeQty(r, -1); });
        connect(stp, &QtyStepper::plus, this, [this, r]
                { changeQty(r, +1); });
        m_table->setItem(r, 3, cell(disc > 0 ? W::money(-disc) : "—", (disc > 0 ? Theme::successText() : Theme::subtleText()).name()));
        m_table->setItem(r, 4, cell(W::money(gross - disc), Theme::accentText().name(), true));
        auto *del = new QPushButton;
        del->setProperty("kind", "flat");
        del->setIcon(Theme::icon("trash", Theme::error, 18));
        del->setCursor(Qt::PointingHandCursor);
        del->setToolTip("Remove");
        auto *wrap = new QWidget;
        auto *wl = new QHBoxLayout(wrap);
        wl->setContentsMargins(0, 0, 0, 0);
        wl->setAlignment(Qt::AlignCenter);
        wl->addWidget(del);
        m_table->setCellWidget(r, 5, wrap);
        connect(del, &QPushButton::clicked, this, [this, r]
                { removeRow(r); });
    }
    updateTotals();
}

void RecordOrderPage::updateTotals()
{
    const auto &st = AppState::instance().settings();
    double gross = 0;
    int items = 0;
    for (const auto &l : m_cart)
    {
        gross += l.p.price * l.qty;
        items += l.qty;
    }
    const double disc = m_vip ? gross * st.vipDiscountPercent / 100.0 : 0;
    const double total = gross - disc;
    W::setBadge(m_itemsBadge, QString("%1 item%2").arg(items).arg(items == 1 ? "" : "s"), items ? "blue" : "grey");
    m_gross->setText(W::money(gross));
    m_disc->setText(disc > 0 ? W::money(-disc) : "$0.00");
    m_tax->setText(W::money(total * st.taxPercent / (100.0 + st.taxPercent)));
    m_total->setText(W::money(total));
    m_checkout->setEnabled(items > 0);
}

void RecordOrderPage::loadDemoDraft()
{
    for (auto [sku, qty] : {std::pair<const char *, int>{"PRD-102", 2}, {"PRD-214", 1}, {"PRD-8840", 1}, {"PRD-512", 3}})
        if (auto p = AppState::instance().findByCode(sku))
            m_cart.push_back({*p, qty});
    rebuildTable();
    setPreview(AppState::instance().findByCode("PRD-8840"));
    m_feed->setCode(m_preview ? m_preview->ean : QString());
}

void RecordOrderPage::newDraft()
{
    m_cart.clear();
    setPreview(std::nullopt);
    rebuildTable();
    m_sessionId->setText("#" + AppState::instance().nextOrderId());
    emit toast("New order started");
}

void RecordOrderPage::checkout()
{
    if (m_cart.isEmpty())
    {
        emit toast("The order is empty");
        return;
    }
    const auto &st = AppState::instance().settings();
    double gross = 0;
    OrderRecord o;
    for (const auto &l : m_cart)
    {
        gross += l.p.price * l.qty;
        o.lines.push_back({l.p.name, l.qty, l.p.price});
    }
    o.discount = m_vip ? gross * st.vipDiscountPercent / 100.0 : 0;
    o.total = gross - o.discount;

    QString mode;
    double tendered = 0;
    if (!Dialogs::payment(this, o.total, mode, tendered))
        return;
    o.id = AppState::instance().nextOrderId();
    o.time = QDateTime::currentDateTime();
    o.customer = m_customer->text().trimmed().isEmpty() ? "Walk-in Customer" : m_customer->text().trimmed();
    o.contact = m_contact->text().trimmed().isEmpty() ? "-" : m_contact->text().trimmed();
    o.tier = m_vip ? "VIP" : "Standard";
    o.payment = mode;
    o.tendered = tendered;
    o.status = "Paid";
    o.shift = "POS #04 • Shift A";
    AppState::instance().addOrder(o);

    Dialogs::receipt(this, o);
    m_cart.clear();
    setPreview(std::nullopt);
    rebuildTable();
    m_sessionId->setText("#" + AppState::instance().nextOrderId());
    emit toast("Order " + o.id + " paid - " + W::money(o.total));
}
