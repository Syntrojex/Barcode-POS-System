#include "RecordOrderPage.h"
#include "AppState.h"
#include "Dialogs.h"
#include "Pricing.h"
#include "Theme.h"
#include "UiStore.h"
#include "Widgets.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QPainter>
#include <QScrollArea>
#include <QShortcut>
#include <QTimer>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QtMath>

namespace
{
    QWidget* twoLine(const QString& top, const QString& bottom)
    {
        auto* w = new QWidget;
        auto* v = new QVBoxLayout(w);
        v->setContentsMargins(4, 4, 4, 4);
        v->setSpacing(1);
        auto* t = W::label(top);
        t->setStyleSheet("font-weight:700; font-size:13.5px;");
        t->setWordWrap(true);
        auto* b = W::label(bottom);
        b->setStyleSheet("color:#8a94ab; font-size:11.5px;");
        v->addWidget(t);
        v->addWidget(b);
        return w;
    }
    QTableWidgetItem* cell(const QString& text, const QString& color = "", bool bold = false)
    {
        auto* i = new QTableWidgetItem(text);
        i->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (!color.isEmpty())
            i->setForeground(Theme::readable(QColor(color)));
        if (bold)
        {
            QFont f = i->font();
            f.setBold(true);
            i->setFont(f);
        }
        return i;
    }
}

RecordOrderPage::RecordOrderPage(QWidget* parent) : QWidget(parent)
{
    auto* root = W::pageLayout(this);
    root->addWidget(buildCustomerStrip());
    auto* body = new QHBoxLayout;
    body->setSpacing(0);
    body->addWidget(buildLeft(), 36);
    body->addWidget(buildRight(), 64);
    root->addLayout(body, 1);

    connect(&AppState::instance(), &AppState::scanned, this, [this](const QString& ean)
        { showCode(ean, true); });
    connect(m_feed, &CameraFeedWidget::barcodeDetected, this, [this](const QString& ean) {
        const auto product = AppState::instance().findByCode(ean);
        if (product) m_feed->showResult(ean + "  -  " + product->name, true);
        else m_feed->showResult(ean + "  -  not in Product.txt", false);
        AppState::instance().emitScan(ean);
        });
    connect(m_feed, &CameraFeedWidget::scanError, this, &RecordOrderPage::toast);
    loadDemoDraft();

    // keyboard: F2 hold, F10 checkout, F4 description, F9 camera (only while this page is visible)
    auto key = [this](const QKeySequence& k)
        {
            auto* s = new QShortcut(k, this);
            s->setContext(Qt::WidgetWithChildrenShortcut);
            return s;
        };
    connect(key(QKeySequence(Qt::Key_F2)), &QShortcut::activated, this, &RecordOrderPage::holdOrder);
    connect(key(QKeySequence(Qt::Key_F10)), &QShortcut::activated, this, &RecordOrderPage::checkout);
    connect(key(QKeySequence(Qt::Key_F4)), &QShortcut::activated, this, &RecordOrderPage::describeCurrent);
    connect(key(QKeySequence(Qt::Key_F9)), &QShortcut::activated, m_camToggle, &QPushButton::click);

    // on-hold chips: redraw when the list changes and every 30 s (the "4m ago" text)
    connect(&UiStore::instance(), &UiStore::parkedChanged, this, &RecordOrderPage::rebuildParked);
    auto* ageTimer = new QTimer(this);
    ageTimer->setInterval(30000);
    connect(ageTimer, &QTimer::timeout, this, &RecordOrderPage::rebuildParked);
    ageTimer->start();
    rebuildParked();
}

QWidget* RecordOrderPage::buildCustomerStrip()
{
    auto* strip = W::card(this, "strip");
    auto* h = new QHBoxLayout(strip);
    h->setContentsMargins(14, 8, 14, 8);
    h->setSpacing(12);
    h->addWidget(W::iconTile("user", Qt::white, "#7fb0ff", "#2b5fe8", 40));

    auto field = [&](const QString& cap, QWidget* content, int stretch)
        {
            auto* f = new QFrame;
            f->setObjectName("field");
            auto* v = new QVBoxLayout(f);
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


    h->addStretch(1);
    auto* sv = new QVBoxLayout;
    sv->setSpacing(0);
    sv->addWidget(W::label("ORDER", "caption"));
    m_sessionId = W::label("#" + AppState::instance().nextOrderId(), "mono");
    m_sessionId->setStyleSheet("font-weight:800; font-size:14px;");
    sv->addWidget(m_sessionId);
    h->addLayout(sv);
    return strip;
}

QWidget* RecordOrderPage::buildLeft()
{
    auto* col = new QWidget;
    auto* v = new QVBoxLayout(col);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    auto* cam = W::card(col);
    auto* cv = new QVBoxLayout(cam);
    cv->setContentsMargins(16, 14, 16, 14);
    cv->setSpacing(10);
    W::headerRow(cv, "camera", "Scanner");
    m_feed = new CameraFeedWidget;
    m_feed->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cv->addWidget(m_feed, 1);

    auto* btns = new QHBoxLayout;
    btns->setSpacing(8);
    m_camToggle = W::button("Start camera  [F9]", "primary", "play");
    btns->addWidget(m_camToggle, 1);
    cv->addLayout(btns);
    connect(m_camToggle, &QPushButton::clicked, this, [this]
        {
            const bool on = !m_feed->running();
            m_feed->setRunning(on);
            const bool running = m_feed->running();
            m_camToggle->setText(running ? "Stop camera" : "Start camera  [F9]");
            m_camToggle->setIcon(Theme::icon(running ? "stop" : "play", QColor("white"), 16)); });

    auto* mr = new QHBoxLayout;
    mr->setSpacing(8);
    m_manual = new QLineEdit;
    m_manual->setPlaceholderText("Type a barcode and press Enter");
    m_manual->setMaxLength(12);
    m_manual->setMinimumHeight(40);
    m_manual->addAction(Theme::icon("search", Theme::outline, 16), QLineEdit::LeadingPosition);
    auto* browse = W::button("Browse", "soft", "products");
    browse->setMinimumHeight(40);
    mr->addWidget(m_manual, 1);
    mr->addWidget(browse);
    cv->addLayout(mr);
    connect(m_manual, &QLineEdit::returnPressed, this, [this]
        { showCode(m_manual->text(), false); });
    connect(browse, &QPushButton::clicked, this, &RecordOrderPage::quickLookup);
    v->addWidget(cam, 5);

    auto* pc = W::card(col);
    auto* pv = new QVBoxLayout(pc);
    pv->setContentsMargins(16, 14, 16, 14);
    pv->setSpacing(10);
    m_pBadge = W::badge("—", "grey");
    W::headerRow(pv, "shield", "Detected product", m_pBadge);

    auto* info = W::card(pc, "subCard");
    auto* ih = new QHBoxLayout(info);
    ih->setContentsMargins(12, 10, 12, 10);
    ih->setSpacing(12);
    m_pThumb = new QLabel;
    m_pThumb->setFixedSize(60, 72);
    m_pThumb->setAlignment(Qt::AlignCenter);
    ih->addWidget(m_pThumb);
    auto* iv = new QVBoxLayout;
    iv->setSpacing(2);
    auto* nr = new QHBoxLayout;
    m_pName = W::label("Nothing scanned yet", "h1");
    m_pName->setWordWrap(true);
    m_pPrice = W::label("");
    m_pPrice->setStyleSheet("color:#2554d8; font-size:17px; font-weight:800;");
    nr->addWidget(m_pName, 1);
    nr->addWidget(m_pPrice);
    iv->addLayout(nr);
    m_pMeta = W::label("Scan a barcode or type one above.", "muted");
    m_pMeta->setWordWrap(true);
    iv->addWidget(m_pMeta);
    m_pStock = W::label("");
    m_pStock->setStyleSheet("font-weight:700;");
    iv->addWidget(m_pStock);
    ih->addLayout(iv, 1);
    pv->addWidget(info);

    m_addBtn = W::button("Add to order", "primary", "cart");
    m_addBtn->setMinimumHeight(42);
    m_addBtn->setEnabled(false);
    m_descBtn = W::button("Description", "soft", "search");
    m_descBtn->setEnabled(false);
    pv->addWidget(m_descBtn);
    pv->addWidget(m_addBtn);
    connect(m_descBtn, &QPushButton::clicked, this, [this]
        { if (m_preview && m_preview->id > 0) Dialogs::productDescription(this, m_preview->id); });
    connect(m_addBtn, &QPushButton::clicked, this, [this]
        { if (m_preview) addToCart(*m_preview); });
    v->addWidget(pc, 3);
    return col;
}

QWidget* RecordOrderPage::buildRight()
{
    auto* col = new QWidget;
    auto* v = new QVBoxLayout(col);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    v->addWidget(buildParkedStrip());

    auto* oc = W::card(col);
    auto* ov = new QVBoxLayout(oc);
    ov->setContentsMargins(0, 14, 0, 10);
    ov->setSpacing(6);

    auto* head = new QHBoxLayout;
    head->setContentsMargins(18, 0, 14, 0);
    head->setSpacing(10);
    head->addWidget(W::iconLabel("orders", Theme::primary, 20));
    head->addWidget(W::label("Current order", "h1"));
    m_itemsBadge = W::badge("0 items", "grey");
    head->addWidget(m_itemsBadge);
    head->addStretch();
    auto* clear = W::button("Clear", "flat", "trash");
    head->addWidget(clear);
    ov->addLayout(head);
    connect(clear, &QPushButton::clicked, this, &RecordOrderPage::newDraft);

    m_table = new QTableWidget(0, 6);
    m_table->setHorizontalHeaderLabels({ "PRODUCT", "PRICE", "QTY", "DISCOUNT", "TOTAL", "" });
    m_table->verticalHeader()->hide();
    m_table->setShowGrid(false);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* hh = m_table->horizontalHeader();
    hh->setSectionResizeMode(QHeaderView::Fixed);
    hh->setSectionResizeMode(0, QHeaderView::Stretch);
    hh->setHighlightSections(false);
    hh->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_table->setColumnWidth(1, 78);
    m_table->setColumnWidth(2, 112);
    m_table->setColumnWidth(3, 92);
    m_table->setColumnWidth(4, 96);
    m_table->setColumnWidth(5, 52);
    m_table->setWordWrap(false);
    ov->addWidget(m_table, 1);
    v->addWidget(oc, 1);

    auto* tc = W::card(col, "totals");
    auto* th = new QHBoxLayout(tc);
    th->setContentsMargins(18, 14, 18, 14);
    th->setSpacing(18);
    auto* grid = new QVBoxLayout;
    grid->setSpacing(8);
    auto row = [&](const QString& k, QLabel*& val)
        {
            auto* r = new QHBoxLayout;
            r->addWidget(W::label(k, "muted"));
            r->addStretch();
            val = W::label("PKR 0.00");
            val->setStyleSheet("font-weight:800; font-size:14px;");
            r->addWidget(val);
            grid->addLayout(r);
        };
    row("Subtotal", m_gross);
    row("Discount", m_disc);
    row("Seasonal", m_seasonal);
    m_seasonalKey = qobject_cast<QLabel*>(grid->itemAt(grid->count() - 1)->layout()->itemAt(0)->widget());
    row("Tax", m_tax);
    // a season switched on/off or edited on the Discounts page -> refresh the totals
    connect(&UiStore::instance(), &UiStore::seasonsChanged, this, &RecordOrderPage::updateTotals);
    grid->addStretch();
    th->addLayout(grid, 1);

    auto* tb = W::card(tc, "totalBox");
    auto* tv = new QVBoxLayout(tb);
    tv->setContentsMargins(18, 12, 18, 14);
    tv->setSpacing(2);
    auto* cap = W::label("TOTAL", "caption");
    tv->addWidget(cap);
    m_total = W::label("PKR 0.00");
    m_total->setStyleSheet("font-size:34px; font-weight:800; color:white;");
    tv->addWidget(m_total);
    auto* actions = new QHBoxLayout;
    actions->setSpacing(8);
    m_hold = W::button("Hold  [F2]", "soft", "pause");
    m_hold->setMinimumHeight(44);
    m_hold->setToolTip("Put this order on hold and start a new one (one press, no questions)");
    m_checkout = W::button("Checkout  [F10]", "primary", "check");
    m_checkout->setMinimumHeight(44);
    actions->addWidget(m_hold, 2);
    actions->addWidget(m_checkout, 3);
    tv->addLayout(actions);
    connect(m_hold, &QPushButton::clicked, this, &RecordOrderPage::holdOrder);
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
    if (!m_feed->running()) m_feed->setRunning(true);
    if (!m_feed->running()) emit toast("Unable to start camera scanning");
    else {
        m_camToggle->setText("Stop camera");
        m_camToggle->setIcon(Theme::icon("stop", QColor("white"), 16));
    }
}

void RecordOrderPage::quickLookup()
{
    QStringList items;
    for (const auto& p : AppState::instance().products())
        items << QString("%1  -  %2").arg(p.ean, p.name);
    bool ok = false;
    const QString pick = QInputDialog::getItem(this, "Browse products", "Choose a product:", items, 0, false, &ok);
    if (ok)
        showCode(pick.section("  -  ", 0, 0), false);
}

void RecordOrderPage::showCode(const QString& codeIn, bool fromScanner)
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

void RecordOrderPage::setPreview(const std::optional<ProductInfo>& p)
{
    m_preview = p;
    m_addBtn->setEnabled(p.has_value() && p->stock > 0);
    m_descBtn->setEnabled(p.has_value() && p->id > 0);
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
    m_pMeta->setText(QString("%1  •  %2  •  tax %3  •  disc %4").arg(p->ean, p->unit, W::money(p->tax), W::money(p->discount)));
    const bool low = p->stock <= AppState::instance().settings().lowStockThreshold;
    m_pStock->setText(QString("%1 in stock").arg(p->stock));
    m_pStock->setStyleSheet(QString("font-weight:700; color:%1;").arg(low ? "#b3261e" : "#0b7a36"));
    W::setBadge(m_pBadge, p->stock == 0 ? "Out of stock" : low ? "Low stock" : "In stock",
        p->stock == 0 ? "red" : low ? "amber" : "green");

    // little 3D "box" thumbnail
    QPixmap pm(120, 144);
    pm.setDevicePixelRatio(2);
    pm.fill(Qt::transparent);
    QPainter pa(&pm);
    pa.setRenderHint(QPainter::Antialiasing);
    pa.setPen(Qt::NoPen);
    pa.setBrush(QColor(20, 40, 110, 40));
    pa.drawEllipse(QRectF(6, 60, 46, 8));
    QLinearGradient fg(10, 8, 10, 62);
    fg.setColorAt(0, Theme::themed("#7fb0ff"));
    fg.setColorAt(1, Theme::themed("#2b5fe8"));
    pa.setBrush(fg);
    pa.drawRoundedRect(QRectF(8, 14, 34, 46), 4, 4);
    pa.setBrush(Theme::themed("#1a3fae"));
    pa.drawPolygon(QPolygonF({ {42, 14}, {50, 8}, {50, 54}, {42, 60} }));
    pa.setBrush(Theme::themed("#a9ccff"));
    pa.drawPolygon(QPolygonF({ {8, 14}, {16, 8}, {50, 8}, {42, 14} }));
    pa.setBrush(QColor(255, 255, 255, 230));
    pa.drawRoundedRect(QRectF(13, 26, 24, 22), 3, 3);
    pa.setPen(Theme::themed("#1a3fae"));
    QFont f = pa.font();
    f.setPixelSize(8);
    f.setBold(true);
    pa.setFont(f);
    pa.drawText(QRectF(13, 26, 24, 22), Qt::AlignCenter, p->name.section(' ', 0, 0).left(5).toUpper());
    pa.end();
    m_pThumb->setPixmap(pm);
}

void RecordOrderPage::addToCart(const ProductInfo& p, int qty)
{
    int have = 0;
    for (const auto& l : m_cart)
        if (l.p.sku == p.sku)
            have = l.qty;
    const QString problem = AppState::instance().stockProblem(p.id, have, qty);
    if (!problem.isEmpty())
    {
        emit toast(problem);
        return;
    }
    bool found = false;
    for (auto& l : m_cart)
        if (l.p.sku == p.sku)
        {
            l.qty += qty;
            found = true;
        }
    if (!found)
        m_cart.push_back({ p, qty });
    rebuildTable();
    emit toast(QString("Added %1 × %2").arg(qty).arg(p.name));
}

void RecordOrderPage::changeQty(int row, int delta)
{
    if (row < 0 || row >= m_cart.size())
        return;
    auto& l = m_cart[row];
    const int nq = l.qty + delta;
    if (nq < 1)
    {
        removeRow(row);
        return;
    }
    const QString problem = AppState::instance().stockProblem(l.p.id, l.qty, delta);
    if (delta > 0 && !problem.isEmpty())
    {
        emit toast(problem);
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
    m_table->setRowCount(0);
    m_table->setRowCount(m_cart.size());
    for (int r = 0; r < m_cart.size(); ++r)
    {
        const auto& l = m_cart[r];
        const double gross = l.p.price * l.qty;
        const double disc = l.p.discount * l.qty;
        const double tax = l.p.tax * l.qty;
        m_table->setRowHeight(r, 62);
        m_table->setCellWidget(r, 0, twoLine(l.p.name, l.p.sku + "  •  " + l.p.unit));
        m_table->setItem(r, 1, cell(W::money(l.p.price), "", true));
        auto* stp = new QtyStepper(l.qty);
        m_table->setCellWidget(r, 2, stp);
        connect(stp, &QtyStepper::minus, this, [this, r]
            { changeQty(r, -1); });
        connect(stp, &QtyStepper::plus, this, [this, r]
            { changeQty(r, +1); });
        m_table->setItem(r, 3, cell(disc > 0 ? W::money(-disc) : "—", disc > 0 ? "#0b7a36" : "#8a94ab"));
        m_table->setItem(r, 4, cell(W::money(gross - disc + tax), "#2554d8", true));
        auto* del = new QPushButton;
        del->setProperty("kind", "flat");
        del->setIcon(Theme::icon("trash", Theme::error, 18));
        del->setCursor(Qt::PointingHandCursor);
        del->setToolTip("Remove");
        auto* wrap = new QWidget;
        auto* wl = new QHBoxLayout(wrap);
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
    // Same calculation as the payment screen (Pricing::quote), so the totals always match.
    // The special discount is chosen at payment, so it is 0 here.
    const Pricing::Quote q = Pricing::quote(cartLines(), 0);
    const int items = q.units;
    const double gross = q.gross, disc = q.itemDiscount, tax = q.tax, total = q.payable;
    m_seasonalKey->setText(q.seasonName.isEmpty() ? "Seasonal"
                                                  : QString("Seasonal (%1%)").arg(q.seasonalPercent, 0, 'g', 4));
    m_seasonal->setText(q.seasonal > 0 ? W::money(-q.seasonal) : "PKR 0.00");
    W::setBadge(m_itemsBadge, QString("%1 item%2").arg(items).arg(items == 1 ? "" : "s"), items ? "blue" : "grey");
    m_gross->setText(W::money(gross));
    m_disc->setText(disc > 0 ? W::money(-disc) : "PKR 0.00");
    m_tax->setText(W::money(tax));
    m_total->setText(W::money(total));
    m_checkout->setEnabled(items > 0);
    m_hold->setEnabled(items > 0);
}

void RecordOrderPage::loadDemoDraft()
{
    // Start with an empty order; products come from Product.txt.
    rebuildTable();
    setPreview(std::nullopt);
}

void RecordOrderPage::newDraft()
{
    m_editingId.clear();
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
    if (!m_editingId.isEmpty())
    {
        // Front-end only for now: saving edits to an existing order (Order.txt / Bill.txt / stock) arrives with the
        // Increment 3 back-end. Nothing is written, so no duplicate order is created.
        emit toast("Saving edits to order #" + m_editingId + " needs the Increment 3 back-end - nothing was changed");
        return;
    }
    OrderRecord o;
    for (const auto& l : m_cart)
        o.lines.push_back({ l.p.name, l.p.sku, l.qty, l.p.price, l.p.tax, l.p.discount, l.p.id });
    o.recompute();

    Dialogs::PaymentRequest req;
    req.orderId = m_sessionId->text().mid(1);
    req.customer = m_customer->text();
    req.contact = m_contact->text();
    req.lines = o.lines;
    Dialogs::PaymentResult res;
    if (!Dialogs::payment(this, req, res))
        return;
    const QString mode = res.mode, merchant = res.merchant;
    const double tendered = res.tendered;
    const auto quote = Pricing::quote(o.lines, res.specialPercent);
    o.lines = Pricing::fold(o.lines, res.specialPercent);
    Pricing::apply(o, quote);

    o.customer = m_customer->text().trimmed().isEmpty() ? "Walk-in Customer" : m_customer->text().trimmed();
    o.contact = m_contact->text().trimmed().isEmpty() ? "-" : m_contact->text().trimmed();
    o.payment = mode;
    o.merchant = merchant;
    o.tendered = tendered;
    o.status = "Paid";
    o.shift = "POS #04 • Shift A";
    QString checkoutError;
    if (!AppState::instance().addOrder(o, &checkoutError))
    {
        emit toast(checkoutError);
        return;
    }

    Dialogs::receipt(this, o);
    m_cart.clear();
    setPreview(std::nullopt);
    rebuildTable();
    m_sessionId->setText("#" + AppState::instance().nextOrderId());
    emit toast("Order " + o.id + " paid - " + W::money(o.total));
}
// ---- placeholder bodies for the Increment 3 actions (the redesigned screen replaces them) ----
void RecordOrderPage::setRole(UserRole role) { m_role = role; }
void RecordOrderPage::describeCurrent()
{
    if (m_preview && m_preview->id > 0)
        Dialogs::productDescription(this, m_preview->id);
}

// ---------------------------------------------------------------- on hold (parked orders)
QVector<ReceiptLine> RecordOrderPage::cartLines() const
{
    QVector<ReceiptLine> lines;
    for (const auto& l : m_cart)
        lines.push_back({ l.p.name, l.p.sku, l.qty, l.p.price, l.p.tax, l.p.discount, l.p.id });
    return lines;
}

QWidget* RecordOrderPage::buildParkedStrip()
{
    auto* strip = W::card(this, "strip");
    strip->setFixedHeight(62);
    auto* h = new QHBoxLayout(strip);
    h->setContentsMargins(14, 6, 14, 6);
    h->setSpacing(10);
    h->addWidget(W::iconLabel("pause", Theme::primary, 18));
    m_parkTitle = W::label("ON HOLD", "caption");
    h->addWidget(m_parkTitle);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setStyleSheet("QScrollArea, QScrollArea > QWidget > QWidget { background: transparent; border: none; }");
    auto* inner = new QWidget;
    m_parkChips = new QHBoxLayout(inner);
    m_parkChips->setContentsMargins(0, 0, 0, 0);
    m_parkChips->setSpacing(8);
    scroll->setWidget(inner);
    h->addWidget(scroll, 1);
    m_parkEmpty = W::label("No orders on hold. Press F2 to park the current order.", "muted");
    h->addWidget(m_parkEmpty);
    return strip;
}

void RecordOrderPage::rebuildParked()
{
    // drop the old chips
    while (QLayoutItem* it = m_parkChips->takeAt(0))
    {
        if (QWidget* w = it->widget())
            w->deleteLater();
        delete it;
    }
    const auto& parked = UiStore::instance().parked();
    m_parkTitle->setText(QString("ON HOLD (%1)").arg(parked.size()));
    m_parkEmpty->setVisible(parked.isEmpty());
    const QDateTime now = QDateTime::currentDateTime();
    for (const auto& p : parked)
    {
        const qint64 secs = p.parkedAt.secsTo(now);
        const QString ago = secs < 60 ? QString("just now")
            : secs < 3600 ? QString("%1m ago").arg(secs / 60)
            : QString("%1h ago").arg(secs / 3600);
        const QString text = QString("#%1   %2  •  %3  •  %4   %5")
            .arg(p.code, p.customer.left(18), QString::number(p.itemCount()), W::money(p.total()), ago);
        auto* chip = W::button(text, "soft", "arrow-right");
        chip->setMinimumHeight(36);
        chip->setToolTip("Click to resume this order");
        const QString code = p.code;
        connect(chip, &QPushButton::clicked, this, [this, code] { resumeParked(code); });
        auto* del = W::button("", "flat", "trash");
        del->setToolTip("Discard this held order");
        connect(del, &QPushButton::clicked, this, [this, code]
            {
                UiStore::instance().discardParked(code);
                emit toast("Held order " + code + " discarded");
            });
        auto* cell = new QWidget;
        auto* cl = new QHBoxLayout(cell);
        cl->setContentsMargins(0, 0, 0, 0);
        cl->setSpacing(2);
        cl->addWidget(chip);
        cl->addWidget(del);
        m_parkChips->addWidget(cell);
    }
    m_parkChips->addStretch();
}

void RecordOrderPage::holdOrder()
{
    if (m_cart.isEmpty())
    {
        emit toast("Nothing to hold - the order is empty");
        return;
    }
    const QString code = UiStore::instance().park(m_customer->text(), m_contact->text(), cartLines());
    m_editingId.clear();
    m_cart.clear();
    m_customer->clear();
    m_contact->clear();
    setPreview(std::nullopt);
    rebuildTable();
    m_sessionId->setText("#" + AppState::instance().nextOrderId());
    focusInput(); // ready for the next customer's first scan
    emit toast("Order put on hold as " + code);
}

void RecordOrderPage::resumeParked(const QString& code)
{
    ParkedOrder po;
    if (!UiStore::instance().takeParked(code, po))
    {
        emit toast("That held order is no longer available");
        return;
    }
    // A cart that is not empty is parked first (swap), so nothing is lost and no question is asked.
    QString swapped;
    if (!m_cart.isEmpty() && m_editingId.isEmpty())
        swapped = UiStore::instance().park(m_customer->text(), m_contact->text(), cartLines());
    if (!m_editingId.isEmpty()) // leaving an order that was open for editing
    {
        m_editingId.clear();
        m_sessionId->setText("#" + AppState::instance().nextOrderId());
    }

    m_cart.clear();
    int adjusted = 0;
    for (const auto& line : po.lines)
    {
        std::optional<ProductInfo> prod;
        for (const auto& p : AppState::instance().products())
            if (line.productId > 0 ? p.id == line.productId : p.sku == line.acronym)
            {
                prod = p;
                break;
            }
        if (!prod)
        {
            ++adjusted;
            continue;
        }
        int qty = line.qty;
        while (qty > 0 && !AppState::instance().stockProblem(prod->id, 0, qty).isEmpty())
            --qty; // stock changed while the order was on hold
        if (qty != line.qty)
            ++adjusted;
        if (qty > 0)
            m_cart.push_back({ *prod, qty });
    }
    m_customer->setText(po.customer == "Walk-in Customer" ? QString() : po.customer);
    m_contact->setText(po.contact == "-" ? QString() : po.contact);
    setPreview(std::nullopt);
    rebuildTable();
    focusInput();

    QString msg = "Resumed " + code;
    if (!swapped.isEmpty())
        msg += "  (previous order held as " + swapped + ")";
    if (adjusted > 0)
        msg += QString("  -  %1 line%2 changed because of stock").arg(adjusted).arg(adjusted == 1 ? "" : "s");
    emit toast(msg);
}
// Opens an existing order (paid + approved from the Orders page, or in progress) in the cart.
void RecordOrderPage::loadOrderForEdit(const QString& orderId)
{
    const OrderRecord* found = nullptr;
    for (const auto& o : AppState::instance().orders())
        if (o.id == orderId)
        {
            found = &o;
            break;
        }
    if (!found)
    {
        emit toast("Order #" + orderId + " was not found");
        return;
    }
    // Nothing is lost: a cart that is not empty is parked first, like when resuming a held order.
    QString swapped;
    if (m_editingId.isEmpty() && !m_cart.isEmpty())
        swapped = UiStore::instance().park(m_customer->text(), m_contact->text(), cartLines());

    m_cart.clear();
    for (const auto& line : found->lines)
        for (const auto& p : AppState::instance().products())
            if (line.productId > 0 ? p.id == line.productId : p.sku == line.acronym)
            {
                m_cart.push_back({ p, line.qty });
                break;
            }
    m_editingId = orderId;
    m_customer->setText(found->customer == "Walk-in Customer" ? QString() : found->customer);
    m_contact->setText(found->contact == "-" ? QString() : found->contact);
    m_sessionId->setText("#" + orderId + "  (editing)");
    setPreview(std::nullopt);
    rebuildTable();
    focusInput();
    QString msg = "Editing order #" + orderId;
    if (!swapped.isEmpty())
        msg += "  (previous order held as " + swapped + ")";
    emit toast(msg);
}
