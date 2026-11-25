#include "RecordOrderPage.h"
#include "AppState.h"
#include "Dialogs.h"
#include "Pricing.h"
#include "Theme.h"
#include "UiKit.h"
#include "UiStore.h"
#include "Widgets.h"
#include <QApplication>
#include <QDateTime>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QScrollArea>
#include <QShortcut>
#include <QTimer>
#include <QTableWidget>
#include <QVBoxLayout>

class ElidedLabel final : public QLabel
{
public:
    ElidedLabel()
    {
        setTextFormat(Qt::PlainText);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    }

    void setFullText(const QString &text)
    {
        m_fullText = text;
        setAccessibleName(text);
        updateText();
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QLabel::resizeEvent(event);
        updateText();
    }

private:
    void updateText()
    {
        const auto visible = fontMetrics().elidedText(m_fullText, Qt::ElideRight, contentsRect().width());
        setText(visible);
        setToolTip(visible == m_fullText ? QString() : m_fullText);
    }

    QString m_fullText;
};

namespace
{
    QWidget* twoLine(const QString& top, const QString& bottom)
    {
        auto* w = new QWidget;
        auto* v = new QVBoxLayout(w);
        v->setContentsMargins(4, 4, 4, 4);
        v->setSpacing(1);
        auto* t = new ElidedLabel;
        t->setStyleSheet("font-weight:700; font-size:13.5px;");
        t->setFullText(top);
        auto* b = new ElidedLabel;
        b->setObjectName("muted");
        b->setFullText(bottom);
        v->addWidget(t);
        v->addWidget(b);
        return w;
    }
    QTableWidgetItem* cell(const QString& text, const QString& color = "", bool bold = false)
    {
        auto* i = new QTableWidgetItem(text);
        i->setToolTip(text);
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
    root->setContentsMargins(10, 6, 10, 6);
    root->setSpacing(0);
    root->addWidget(buildBarcodeStrip());
    root->addWidget(buildCustomerStrip());
    root->addWidget(buildParkedStrip());
    auto* body = new QHBoxLayout;
    body->setSpacing(0);
    body->addWidget(buildProductPane(), 27);
    body->addWidget(buildCart(), 73);
    root->addLayout(body, 1);
    root->addWidget(buildSummary());

    setTabOrder(m_manual, m_addBtn);
    setTabOrder(m_addBtn, m_descBtn);

    connect(&AppState::instance(), &AppState::scanned, this, [this](const QString& ean)
        { showCode(ean, true); });
    connect(m_feed, &CameraFeedWidget::barcodeDetected, this, [this](const QString& ean) {
        const auto product = AppState::instance().findByCode(ean);
        if (product) m_feed->showResult(ean + "  -  " + product->name, true);
        else m_feed->showResult(ean + "  -  not in Product.txt", false);
        AppState::instance().emitScan(ean);
        });
    connect(m_feed, &CameraFeedWidget::scanError, this, [this](const QString& error)
        {
            setCameraActive(false);
            emit toast(error);
        });
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
    connect(&UiStore::instance(), &UiStore::seasonsChanged, this, &RecordOrderPage::updateTotals);
    auto* ageTimer = new QTimer(this);
    ageTimer->setInterval(30000);
    connect(ageTimer, &QTimer::timeout, this, &RecordOrderPage::rebuildParked);
    ageTimer->start();
    rebuildParked();
}

QWidget* RecordOrderPage::buildBarcodeStrip()
{
    auto* strip = W::card(this, "strip");
    auto* h = new QHBoxLayout(strip);
    h->setContentsMargins(16, 6, 16, 6);
    h->setSpacing(10);

    auto* label = W::label("Barcode", "h1");
    h->addWidget(label);
    m_manual = new QLineEdit;
    m_manual->setObjectName("checkoutBarcode");
    m_manual->setPlaceholderText("Scan or type a barcode, then press Enter");
    m_manual->setAccessibleName("Barcode");
    m_manual->setMaxLength(12);
    m_manual->setMinimumHeight(42);
    m_manual->addAction(Theme::icon("barcode", Theme::outline, 16), QLineEdit::LeadingPosition);
    label->setBuddy(m_manual);
    h->addWidget(m_manual, 1);
    connect(m_manual, &QLineEdit::returnPressed, this, [this]
        { showCode(m_manual->text(), false); });

    auto* browse = W::button("Browse products", "soft", "products");
    m_camToggle = W::button("Camera  [F9]", "soft", "camera");
    for (auto* button : {browse, m_camToggle})
    {
        button->setMinimumHeight(40);
        h->addWidget(button);
    }
    connect(browse, &QPushButton::clicked, this, &RecordOrderPage::quickLookup);
    connect(m_camToggle, &QPushButton::clicked, this, [this]
        { setCameraActive(m_cameraPanel->isHidden()); });
    return strip;
}

QWidget* RecordOrderPage::buildCustomerStrip()
{
    auto* strip = W::card(this, "strip");
    auto* h = new QHBoxLayout(strip);
    h->setContentsMargins(16, 5, 16, 5);
    h->setSpacing(12);
    auto field = [&](const QString& cap, QWidget* content, int stretch)
        {
            auto* f = new QFrame;
            f->setObjectName("field");
            auto* v = new QVBoxLayout(f);
            v->setContentsMargins(12, 5, 12, 6);
            v->setSpacing(0);
            auto* label = W::label(cap, "caption");
            label->setBuddy(content);
            v->addWidget(label);
            v->addWidget(content);
            h->addWidget(f, stretch);
        };
    m_customer = new QLineEdit;
    m_customer->setProperty("flat", true);
    m_customer->setPlaceholderText("Walk-in customer");
    field("CUSTOMER", m_customer, 3);
    m_contact = new QLineEdit;
    m_contact->setProperty("flat", true);
    m_contact->setPlaceholderText("Phone (optional)");
    field("PHONE", m_contact, 3);


    auto* sv = new QVBoxLayout;
    sv->setSpacing(0);
    sv->addWidget(W::label("ORDER", "caption"));
    m_sessionId = W::label("#" + AppState::instance().nextOrderId(), "mono");
    m_sessionId->setStyleSheet("font-weight:800; font-size:14px;");
    sv->addWidget(m_sessionId);
    h->addLayout(sv);
    return strip;
}

QWidget* RecordOrderPage::buildProductPane()
{
    auto* col = new QWidget;
    auto* v = new QVBoxLayout(col);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    auto* pc = W::card(col);
    auto* pv = new QVBoxLayout(pc);
    pv->setContentsMargins(14, 7, 14, 7);
    pv->setSpacing(4);
    m_pBadge = W::badge("—", "grey");
    W::headerRow(pv, "products", "Product preview", m_pBadge);

    auto* info = W::card(pc, "subCard");
    auto* detail = new QVBoxLayout(info);
    detail->setContentsMargins(10, 5, 10, 5);
    detail->setSpacing(2);
    m_pName = new ElidedLabel;
    m_pName->setObjectName("h1");
    detail->addWidget(m_pName);
    m_pMeta = W::label("Scan or enter a barcode.", "muted");
    m_pMeta->setWordWrap(true);
    detail->addWidget(m_pMeta);
    auto* amounts = new QHBoxLayout;
    m_pPrice = W::label("", "checkoutPrice");
    m_pStock = W::label("", "checkoutStock");
    amounts->addWidget(m_pPrice);
    amounts->addWidget(m_pStock, 0, Qt::AlignRight);
    detail->addLayout(amounts);
    pv->addWidget(info);

    m_addBtn = W::button("Add to order", "primary", "cart");
    m_addBtn->setMinimumHeight(36);
    m_addBtn->setEnabled(false);
    m_descBtn = W::button("Description  [F4]", "flat", "info");
    m_descBtn->setEnabled(false);
    pv->addWidget(m_addBtn);
    pv->addWidget(m_descBtn);
    connect(m_descBtn, &QPushButton::clicked, this, &RecordOrderPage::describeCurrent);
    connect(m_addBtn, &QPushButton::clicked, this, [this]
        {
            if (m_preview)
                addToCart(*m_preview);
            focusInput();
        });
    v->addWidget(pc, 0, Qt::AlignTop);

    m_cameraPanel = W::card(col);
    auto* cv = new QVBoxLayout(m_cameraPanel);
    cv->setContentsMargins(10, 5, 10, 5);
    m_feed = new CameraFeedWidget;
    m_feed->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    m_feed->setFixedHeight(90);
    cv->addWidget(m_feed);
    m_cameraPanel->hide();
    v->addWidget(m_cameraPanel);
    v->addStretch();
    return col;
}

QWidget* RecordOrderPage::buildCart()
{
    auto* oc = W::card(this);
    auto* ov = new QVBoxLayout(oc);
    ov->setContentsMargins(0, 14, 0, 10);
    ov->setSpacing(6);

    auto* head = new QHBoxLayout;
    head->setContentsMargins(18, 0, 14, 0);
    head->setSpacing(10);
    head->addWidget(W::label("Cart", "h1"));
    m_itemsBadge = W::badge("0 items", "grey");
    head->addWidget(m_itemsBadge);
    head->addStretch();
    auto* clear = W::button("Clear", "flat", "trash");
    head->addWidget(clear);
    ov->addLayout(head);
    connect(clear, &QPushButton::clicked, this, &RecordOrderPage::newDraft);

    m_table = Ui::makeTable({"PRODUCT", "UNIT PRICE", "QTY", "DISCOUNT", "LINE TOTAL", ""});
    m_table->setAccessibleName("Current order");
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    auto* hh = m_table->horizontalHeader();
    hh->setSectionResizeMode(QHeaderView::Fixed);
    hh->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->setColumnWidth(1, 95);
    m_table->setColumnWidth(2, 116);
    m_table->setColumnWidth(3, 95);
    m_table->setColumnWidth(4, 104);
    m_table->setColumnWidth(5, 42);
    m_table->horizontalHeaderItem(4)->setToolTip("Includes item discounts and tax. Order discounts appear in the summary.");
    m_table->setWordWrap(false);
    ov->addWidget(m_table, 1);
    return oc;
}

QWidget* RecordOrderPage::buildSummary()
{
    auto* tc = W::card(this, "totals");
    tc->setObjectName("checkoutSummary");
    auto* th = new QHBoxLayout(tc);
    th->setContentsMargins(14, 8, 14, 8);
    th->setSpacing(12);
    auto row = [&](const QString& k, QLabel*& val)
        {
            auto* r = new QVBoxLayout;
            r->setSpacing(2);
            r->addWidget(W::label(k, "muted"));
            val = W::label("PKR 0.00");
            val->setStyleSheet("font-weight:700; font-size:14px;");
            r->addWidget(val);
            th->addLayout(r);
        };
    row("Subtotal", m_gross);
    row("Discounts", m_disc);
    row("Tax", m_tax);
    th->addStretch(1);

    auto* tb = W::card(tc, "totalBox");
    auto* tv = new QVBoxLayout(tb);
    tv->setContentsMargins(14, 5, 14, 5);
    tv->setSpacing(0);
    auto* cap = W::label("NET PAYABLE", "caption");
    tv->addWidget(cap);
    m_total = W::label("PKR 0.00", "checkoutPayable");
    m_total->setStyleSheet("font-size:28px; font-weight:800;");
    tv->addWidget(m_total);
    th->addWidget(tb);
    m_hold = W::button("Hold  [F2]", "soft", "pause");
    m_hold->setMinimumHeight(44);
    m_hold->setToolTip("Put this order on hold and start a new one (one press, no questions)");
    m_checkout = W::button("Checkout  [F10]", "primary", "check");
    m_checkout->setMinimumHeight(44);
    th->addWidget(m_hold);
    th->addWidget(m_checkout);
    connect(m_hold, &QPushButton::clicked, this, &RecordOrderPage::holdOrder);
    connect(m_checkout, &QPushButton::clicked, this, &RecordOrderPage::checkout);
    return tc;
}

// ---------------------------------------------------------------- behaviour
void RecordOrderPage::focusInput()
{
    m_manual->setFocus();
    m_manual->selectAll();
}

void RecordOrderPage::simulateScan()
{
    setCameraActive(true);
}

void RecordOrderPage::setCameraActive(bool active)
{
    m_cameraPanel->setVisible(active);
    m_camToggle->setText(active ? "Stop camera  [F9]" : "Camera  [F9]");
    m_feed->setRunning(active);
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
    focusInput();
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
    m_pPrice->setVisible(p.has_value());
    m_pStock->setVisible(p.has_value());
    if (!p)
    {
        m_pName->setFullText("No product selected");
        m_pPrice->clear();
        m_pMeta->setText("Scan or enter a barcode.");
        m_pMeta->setToolTip({});
        m_pStock->clear();
        W::setBadge(m_pBadge, "—", "grey");
        return;
    }
    m_pName->setFullText(p->name);
    m_pPrice->setText(W::money(p->price));
    m_pMeta->setText(QString("%1  •  %2").arg(p->ean, p->unit));
    m_pMeta->setToolTip(QString("Tax %1 · Discount %2").arg(W::money(p->tax), W::money(p->discount)));
    const bool low = p->stock <= AppState::instance().settings().lowStockThreshold;
    m_pStock->setText(QString("%1 in stock").arg(p->stock));
    m_pStock->setProperty("lowStock", low);
    Theme::repolish(m_pStock);
    W::setBadge(m_pBadge, p->stock == 0 ? "Out of stock" : low ? "Low stock" : "In stock",
        p->stock == 0 ? "red" : low ? "amber" : "green");

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
    const auto* stepper = m_table->cellWidget(row, 2);
    const auto* focused = QApplication::focusWidget();
    const QString control = focused && stepper && stepper->isAncestorOf(focused) ? focused->objectName() : QString();
    l.qty = nq;
    rebuildTable();
    if (!control.isEmpty())
        if (auto* button = m_table->cellWidget(row, 2)->findChild<QPushButton*>(control))
            button->setFocus();
}

void RecordOrderPage::removeRow(int row)
{
    if (row < 0 || row >= m_cart.size())
        return;
    m_cart.removeAt(row);
    rebuildTable();
    focusInput();
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
        m_table->setRowHeight(r, 54);
        m_table->setCellWidget(r, 0, twoLine(l.p.name, l.p.sku + "  •  " + l.p.unit));
        m_table->setItem(r, 1, cell(W::money(l.p.price), "", true));
        auto* stp = new QtyStepper(l.qty);
        for (auto* button : stp->findChildren<QPushButton*>())
        {
            const QString action = button->text() == "+" ? "Increase quantity" : "Decrease quantity";
            button->setObjectName(action);
            button->setAccessibleName(action + " for " + l.p.name);
        }
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
        del->setAccessibleName("Remove " + l.p.name);
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
    const auto quote = Pricing::quote(cartLines(), 0);
    const int items = quote.units;
    W::setBadge(m_itemsBadge, QString("%1 item%2").arg(items).arg(items == 1 ? "" : "s"), items ? "blue" : "grey");
    m_gross->setText(W::money(quote.gross));
    m_disc->setText(W::money(-quote.totalDiscount()));
    m_disc->setToolTip(QString("Item discounts %1 · Seasonal discounts %2")
        .arg(W::money(quote.itemDiscount), W::money(quote.seasonal)));
    m_tax->setText(W::money(quote.tax));
    m_total->setText(W::money(quote.payable));
    m_total->setMinimumWidth(qMax(176, m_total->sizeHint().width() + 4));
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
    focusInput();
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
    {
        focusInput();
        return;
    }
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
    focusInput();
    emit toast("Order " + o.id + " paid - " + W::money(o.total));
}
// ---- placeholder bodies for the Increment 3 actions (the redesigned screen replaces them) ----
void RecordOrderPage::setRole(UserRole role) { m_role = role; }
void RecordOrderPage::describeCurrent()
{
    if (m_preview && m_preview->id > 0)
    {
        Dialogs::productDescription(this, m_preview->id);
        focusInput();
    }
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
    scroll->setFixedHeight(46);
    scroll->setStyleSheet("QScrollArea, QScrollArea > QWidget > QWidget { background: transparent; border: none; }");
    auto* inner = new QWidget;
    m_parkChips = new QHBoxLayout(inner);
    m_parkChips->setContentsMargins(0, 0, 0, 0);
    m_parkChips->setSpacing(8);
    scroll->setWidget(inner);
    h->addWidget(scroll, 1);
    m_parkedList = scroll;
    m_parkEmpty = W::label("No held orders", "muted");
    h->addWidget(m_parkEmpty, 1);
    return strip;
}

void RecordOrderPage::rebuildParked()
{
    // drop the old chips
    while (QLayoutItem* it = m_parkChips->takeAt(0))
    {
        if (QWidget* w = it->widget())
        {
            w->hide();
            w->deleteLater();
        }
        delete it;
    }
    const auto& parked = UiStore::instance().parked();
    m_parkTitle->setText(QString("ON HOLD (%1)").arg(parked.size()));
    m_parkedList->setVisible(!parked.isEmpty());
    m_parkEmpty->setVisible(parked.isEmpty());
    const QDateTime now = QDateTime::currentDateTime();
    for (const auto& p : parked)
    {
        const qint64 secs = p.parkedAt.secsTo(now);
        const QString ago = secs < 60 ? QString("just now")
            : secs < 3600 ? QString("%1m ago").arg(secs / 60)
            : QString("%1h ago").arg(secs / 3600);
        const QString text = QString("#%1 · %2").arg(p.code, p.customer.left(18));
        auto* chip = W::button(text, "soft", "arrow-right");
        chip->setMinimumHeight(36);
        chip->setToolTip(QString("Resume %1 · %2 items · %3 · %4")
            .arg(p.customer).arg(p.itemCount()).arg(W::money(p.total()), ago));
        const QString code = p.code;
        connect(chip, &QPushButton::clicked, this, [this, code] { resumeParked(code); });
        auto* del = W::button("", "flat", "trash");
        del->setToolTip("Discard this held order");
        del->setAccessibleName("Discard held order " + code);
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
