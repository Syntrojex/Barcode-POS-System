#include "RecordOrderPage.h"
#include "AppState.h"
#include "Dialogs.h"
#include "Theme.h"
#include "Widgets.h"
#include <QApplication>
#include <QDateTime>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QPainter>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

namespace
{
    constexpr int kCols = 8;
    QWidget *twoLine(const QString &top, const QString &bottom, bool accent = false)
    {
        auto *w = new QWidget;
        auto *v = new QVBoxLayout(w);
        v->setContentsMargins(4, 4, 4, 4);
        v->setSpacing(1);
        auto *t = W::label(top);
        t->setStyleSheet(QString("font-weight:600; font-size:13.5px; %1").arg(accent ? "color:#004ac6;" : ""));
        t->setWordWrap(true);
        auto *b = W::label(bottom, "muted");
        b->setStyleSheet("color:#737686; font-size:11.5px;");
        v->addWidget(t);
        v->addWidget(b);
        return w;
    }
    QTableWidgetItem *cell(const QString &text, Qt::Alignment a = Qt::AlignRight | Qt::AlignVCenter,
                           const QString &color = "", bool bold = false)
    {
        auto *i = new QTableWidgetItem(text);
        i->setTextAlignment(a);
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
    buildUi();
    connect(&AppState::instance(), &AppState::scanned, this, [this](const QString &ean)
            { showCode(ean, true); });
    loadDemoDraft();
}

void RecordOrderPage::buildUi()
{
    setObjectName("page");
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 12);
    root->setSpacing(12);
    root->addWidget(buildCustomerStrip());

    auto *body = new QHBoxLayout;
    body->setSpacing(12);
    body->addWidget(buildLeft(), 33);
    body->addWidget(buildRight(), 67);
    root->addLayout(body, 1);

    // shortcut legend
    auto *bar = W::card(this, "shortcutBar");
    auto *h = new QHBoxLayout(bar);
    h->setContentsMargins(12, 6, 12, 6);
    h->setSpacing(14);
    const QPair<QString, QString> keys[] = {{"F2", "Catalog"}, {"F5", "Scanner"}, {"F9", "Scan Ingest"},
                                            {"F10", "Instant Payment"}, {"Ctrl+N", "New Draft"}};
    for (const auto &k : keys)
    {
        auto *kc = W::label(k.first, "keycap");
        h->addWidget(kc);
        h->addWidget(W::label(k.second, "muted"));
    }
    h->addStretch();
    h->addWidget(W::dot(QColor("#16a34a")));
    auto *ready = W::label("Scanner Interface: simulated (demo)");
    ready->setStyleSheet("color:#006329; font-weight:600;");
    h->addWidget(ready);
    root->addWidget(bar);
}

QWidget *RecordOrderPage::buildCustomerStrip()
{
    auto *strip = W::card(this, "strip");
    auto *h = new QHBoxLayout(strip);
    h->setContentsMargins(12, 10, 12, 10);
    h->setSpacing(10);

    auto *ib = new QLabel;
    ib->setFixedSize(40, 40);
    ib->setAlignment(Qt::AlignCenter);
    ib->setStyleSheet("background:#dce9ff; border-radius:8px;");
    ib->setPixmap(Theme::iconPixmap("user", Theme::primary, 20));
    h->addWidget(ib);

    auto field = [&](const QString &cap, QWidget *content, int stretch = 1)
    {
        auto *f = new QFrame;
        f->setObjectName("field");
        auto *v = new QVBoxLayout(f);
        v->setContentsMargins(10, 5, 10, 5);
        v->setSpacing(0);
        v->addWidget(W::label(cap, "caption"));
        v->addWidget(content);
        h->addWidget(f, stretch);
        return f;
    };
    m_customer = new QLineEdit("Marcus Vance");
    m_customer->setProperty("flat", true);
    m_customer->setPlaceholderText("Walk-in Customer");
    field("CUSTOMER NAME", m_customer, 3);
    m_contact = new QLineEdit("+1 (555) 019-2834");
    m_contact->setProperty("flat", true);
    field("CONTACT MOBILE", m_contact, 3);

    auto *mem = new QWidget;
    auto *mh = new QHBoxLayout(mem);
    mh->setContentsMargins(0, 0, 0, 0);
    auto *tier = W::label("Tier #8821 Gold");
    tier->setStyleSheet("font-weight:600; font-size:14px; color:#004ac6;");
    m_tier = W::badge("5% VIP", "green");
    auto *toggle = new QPushButton;
    toggle->setCheckable(true);
    toggle->setChecked(true);
    toggle->setProperty("kind", "flat");
    toggle->setIcon(Theme::icon("tag", Theme::primary, 16));
    toggle->setToolTip("Toggle loyalty discount");
    toggle->setCursor(Qt::PointingHandCursor);
    mh->addWidget(tier);
    mh->addStretch();
    mh->addWidget(m_tier);
    mh->addWidget(toggle);
    field("MEMBERSHIP STATUS", mem, 3);
    connect(toggle, &QPushButton::toggled, this, [this](bool on)
            {
        m_vip = on;
        W::setBadge(m_tier, on ? QString("%1% VIP").arg(AppState::instance().settings().vipDiscountPercent, 0, 'g', 3) : "No discount",
                    on ? "green" : "grey");
        rebuildTable(); });

    auto *sess = new QWidget;
    auto *sv = new QVBoxLayout(sess);
    sv->setContentsMargins(0, 0, 0, 0);
    sv->setSpacing(0);
    sv->addWidget(W::label("ACTIVE SESSION", "caption"));
    m_sessionId = W::label("#" + AppState::instance().nextOrderId(), "mono");
    m_sessionId->setStyleSheet("font-weight:700; font-size:14px;");
    sv->addWidget(m_sessionId);
    h->addWidget(sess, 2);

    auto *clock = new QFrame;
    clock->setObjectName("field");
    auto *ch = new QHBoxLayout(clock);
    ch->setContentsMargins(10, 6, 10, 6);
    ch->addWidget(W::iconLabel("clock", Theme::primary, 16));
    m_sessionTime = W::label(QDateTime::currentDateTime().toString("dd MMM • HH:mm"));
    m_sessionTime->setStyleSheet("color:#003ea8; font-weight:600;");
    ch->addWidget(m_sessionTime);
    h->addWidget(clock, 2);
    auto *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [this]
            { m_sessionTime->setText(QDateTime::currentDateTime().toString("dd MMM • HH:mm")); });
    t->start(15000);
    return strip;
}

QWidget *RecordOrderPage::buildLeft()
{
    auto *col = new QWidget;
    auto *v = new QVBoxLayout(col);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(12);

    // ---- scanner card
    auto *cam = W::card(col);
    auto *cv = new QVBoxLayout(cam);
    cv->setContentsMargins(12, 12, 12, 12);
    cv->setSpacing(10);
    auto *head = new QHBoxLayout;
    head->setSpacing(8);
    head->addWidget(W::dot(QColor("#16a34a")));
    head->addWidget(W::label("Optical Barcode Feed", "h1"));
    head->addStretch();
    head->addWidget(W::badge("640p @ 30fps", "blue"));
    head->addWidget(W::badge("AF LOCKED", "green"));
    cv->addLayout(head);

    m_feed = new CameraFeedWidget;
    m_feed->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cv->addWidget(m_feed, 1);

    auto *btns = new QHBoxLayout;
    btns->setSpacing(8);
    m_startCam = W::button("Start Cam", "soft", "play");
    m_stopCam = W::button("Stop Cam", "soft", "stop");
    auto *scan = W::button("Scan [F9]", "primary", "barcode");
    m_startCam->setEnabled(false);
    btns->addWidget(m_startCam, 1);
    btns->addWidget(m_stopCam, 1);
    btns->addWidget(scan, 1);
    cv->addLayout(btns);
    connect(m_startCam, &QPushButton::clicked, this, [this]
            { m_feed->setRunning(true); m_startCam->setEnabled(false); m_stopCam->setEnabled(true); });
    connect(m_stopCam, &QPushButton::clicked, this, [this]
            { m_feed->setRunning(false); m_startCam->setEnabled(true); m_stopCam->setEnabled(false); });
    connect(scan, &QPushButton::clicked, this, &RecordOrderPage::simulateScan);

    auto *lr = new QHBoxLayout;
    lr->addWidget(W::label("MANUAL EAN / BARCODE INGEST", "caption"));
    lr->addStretch();
    auto *ql = W::button("Quick Lookup [F4]", "");
    ql->setObjectName("link");
    ql->setCursor(Qt::PointingHandCursor);
    lr->addWidget(ql);
    cv->addLayout(lr);
    connect(ql, &QPushButton::clicked, this, &RecordOrderPage::quickLookup);

    auto *mr = new QHBoxLayout;
    mr->setSpacing(8);
    m_manual = new QLineEdit;
    m_manual->setPlaceholderText("Type 8-digit EAN (or 6-digit code) and press Enter");
    m_manual->setMaxLength(12);
    m_manual->setValidator(new QRegularExpressionValidator(QRegularExpression("[A-Za-z0-9-]*"), m_manual));
    m_manual->setMinimumHeight(38);
    auto *submit = W::button("Submit", "primary", "arrow-right");
    submit->setMinimumHeight(38);
    mr->addWidget(m_manual, 1);
    mr->addWidget(submit);
    cv->addLayout(mr);
    connect(m_manual, &QLineEdit::returnPressed, this, [this]
            { showCode(m_manual->text(), false); });
    connect(submit, &QPushButton::clicked, this, [this]
            { showCode(m_manual->text(), false); });
    v->addWidget(cam, 5);

    // ---- preview card
    auto *pc = W::card(col);
    auto *pv = new QVBoxLayout(pc);
    pv->setContentsMargins(12, 12, 12, 12);
    pv->setSpacing(10);
    m_pStockBadge = W::badge("—", "grey");
    W::headerRow(pv, "shield", "Target Detected Preview", m_pStockBadge);

    auto *info = W::card(pc, "subCard");
    auto *ih = new QHBoxLayout(info);
    ih->setContentsMargins(10, 10, 10, 10);
    ih->setSpacing(12);
    m_pThumb = new QLabel;
    m_pThumb->setFixedSize(64, 76);
    m_pThumb->setAlignment(Qt::AlignCenter);
    m_pThumb->setStyleSheet("background:white; border:1px solid #dce9ff; border-radius:6px;");
    ih->addWidget(m_pThumb);
    auto *iv = new QVBoxLayout;
    iv->setSpacing(3);
    auto *nr = new QHBoxLayout;
    m_pName = W::label("No product scanned", "h1");
    m_pName->setWordWrap(true);
    m_pPrice = W::label("", "h1");
    m_pPrice->setStyleSheet("color:#004ac6; font-size:16px; font-weight:700;");
    nr->addWidget(m_pName, 1);
    nr->addWidget(m_pPrice);
    iv->addLayout(nr);
    m_pMeta = W::label("Scan a barcode or enter an EAN to preview a product.", "muted");
    m_pMeta->setWordWrap(true);
    m_pMeta->setStyleSheet("color:#434655; font-size:12px;");
    iv->addWidget(m_pMeta);
    iv->addStretch();
    auto *sr = new QHBoxLayout;
    m_pStock = W::label("");
    m_pStock->setStyleSheet("color:#006329; font-weight:700;");
    m_pSku = W::label("");
    m_pSku->setStyleSheet("font-weight:700;");
    sr->addWidget(m_pStock);
    sr->addStretch();
    sr->addWidget(m_pSku);
    iv->addLayout(sr);
    ih->addLayout(iv, 1);
    pv->addWidget(info);

    auto *ar = new QHBoxLayout;
    ar->setSpacing(8);
    m_addBtn = W::button("Add Item to Order (Enter)", "primary", "cart");
    m_addBtn->setMinimumHeight(40);
    m_addBtn->setEnabled(false);
    auto *mod = W::button("Modify", "soft", "edit");
    mod->setMinimumHeight(40);
    ar->addWidget(m_addBtn, 3);
    ar->addWidget(mod, 1);
    pv->addLayout(ar);
    connect(m_addBtn, &QPushButton::clicked, this, [this]
            { if (m_preview) addToCart(*m_preview); });
    connect(mod, &QPushButton::clicked, this, [this]
            {
        if (!m_preview) { emit toast("Nothing to modify - scan a product first"); return; }
        bool ok = false;
        const int q = QInputDialog::getInt(this, "Modify quantity", "Quantity to add for " + m_preview->name,
                                           1, 1, qMax(1, m_preview->stock), 1, &ok);
        if (ok) addToCart(*m_preview, q); });

    m_notice = W::card(pc, "notice");
    auto *nh = new QHBoxLayout(m_notice);
    nh->setContentsMargins(10, 6, 10, 6);
    nh->addWidget(W::iconLabel("warn", Theme::error, 16));
    m_noticeText = W::label("");
    m_noticeText->setWordWrap(true);
    nh->addWidget(m_noticeText, 1);
    m_noticeReorder = W::button("Reorder", "");
    m_noticeReorder->setObjectName("link");
    m_noticeReorder->setCursor(Qt::PointingHandCursor);
    m_noticeReorder->setStyleSheet("color:#93000a; font-weight:700; text-decoration: underline; background:transparent; border:none;");
    nh->addWidget(m_noticeReorder);
    pv->addWidget(m_notice);
    connect(m_noticeReorder, &QPushButton::clicked, this, [this]
            {
        AppState::instance().reorder(m_noticeSku);
        emit toast("Purchase order drafted for " + m_noticeSku);
        updateNotice(); });
    v->addWidget(pc, 3);
    return col;
}

QWidget *RecordOrderPage::buildRight()
{
    auto *col = new QWidget;
    auto *v = new QVBoxLayout(col);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(12);

    auto *oc = W::card(col);
    auto *ov = new QVBoxLayout(oc);
    ov->setContentsMargins(0, 12, 0, 8);
    ov->setSpacing(6);

    auto *head = new QHBoxLayout;
    head->setContentsMargins(14, 0, 14, 0);
    head->setSpacing(8);
    head->addWidget(W::iconLabel("orders", Theme::primary, 20));
    head->addWidget(W::label("Current Order Draft", "h1"));
    m_itemsBadge = W::badge("0 Items", "grey");
    head->addWidget(m_itemsBadge);
    head->addStretch();
    auto *clear = W::button("Clear", "flat", "trash");
    clear->setToolTip("Discard the current draft");
    head->addWidget(clear);
    ov->addLayout(head);
    connect(clear, &QPushButton::clicked, this, &RecordOrderPage::newDraft);

    m_table = new QTableWidget(0, kCols);
    m_table->setHorizontalHeaderLabels({"CODE", "PRODUCT DESCRIPTION", "PRICE", "QTY", "DISC.", "TAX", "SUBTOTAL", "ACTION"});
    m_table->verticalHeader()->hide();
    m_table->setShowGrid(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setFocusPolicy(Qt::NoFocus);
    auto *hh = m_table->horizontalHeader();
    hh->setSectionResizeMode(QHeaderView::Fixed);
    hh->setSectionResizeMode(1, QHeaderView::Stretch);
    hh->setHighlightSections(false);
    m_table->setColumnWidth(0, 80);
    m_table->setColumnWidth(2, 66);
    m_table->setColumnWidth(3, 104);
    m_table->setColumnWidth(4, 66);
    m_table->setColumnWidth(5, 64);
    m_table->setWordWrap(false);
    m_table->setColumnWidth(6, 74);
    m_table->setColumnWidth(7, 56);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    hh->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ov->addWidget(m_table, 1);

    auto *acts = new QHBoxLayout;
    acts->setContentsMargins(12, 0, 12, 0);
    acts->setSpacing(8);
    auto *addCustom = W::button("Add Custom Line", "", "plus");
    auto *inc = W::button("+1 Increase Qty", "");
    auto *dec = W::button("−1 Decrease Qty", "");
    auto *rem = W::button("Remove Selected", "danger", "cart");
    acts->addWidget(addCustom);
    acts->addWidget(inc);
    acts->addWidget(dec);
    acts->addStretch();
    acts->addWidget(rem);
    ov->addLayout(acts);
    v->addWidget(oc, 1);

    connect(addCustom, &QPushButton::clicked, this, [this]
            {
        bool ok = false;
        const QString name = QInputDialog::getText(this, "Add custom line", "Description:", QLineEdit::Normal, "", &ok);
        if (!ok || name.trimmed().isEmpty()) return;
        const double price = QInputDialog::getDouble(this, "Add custom line", "Unit price ($):", 1.0, 0.01, 100000, 2, &ok);
        if (!ok) return;
        Product p;
        p.sku = QString("CUSTOM-%1").arg(m_cart.size() + 1, 2, 10, QChar('0'));
        p.name = name.trimmed();
        p.category = "Custom";
        p.location = "Manual entry";
        p.price = price;
        p.stock = 999;
        m_cart.push_back({p, 1, true});
        rebuildTable(); });
    connect(inc, &QPushButton::clicked, this, [this]
            { changeQty(currentRow(), +1); });
    connect(dec, &QPushButton::clicked, this, [this]
            { changeQty(currentRow(), -1); });
    connect(rem, &QPushButton::clicked, this, [this]
            { removeRow(currentRow()); });

    // ---- totals
    auto *tc = W::card(col, "totals");
    auto *th = new QHBoxLayout(tc);
    th->setContentsMargins(16, 12, 16, 12);
    th->setSpacing(20);
    auto *grid = new QGridLayout;
    grid->setVerticalSpacing(6);
    auto row = [&](int r, const QString &k, QLabel *&val, bool muted = false)
    {
        auto *kl = W::label(k, muted ? "muted" : nullptr);
        val = W::label("$0.00");
        val->setAlignment(Qt::AlignRight);
        val->setStyleSheet("font-weight:700;");
        grid->addWidget(kl, r, 0);
        grid->addWidget(val, r, 1);
    };
    row(0, "Gross Subtotal:", m_gross);
    row(1, "Loyalty Discount Applied:", m_disc);
    row(2, "Total Tax (GST included):", m_tax);
    row(3, "Rounding Adjustment:", m_round, true);
    grid->setColumnStretch(0, 1);
    th->addLayout(grid, 1);

    auto *tb = W::card(tc, "totalBox");
    auto *tv = new QVBoxLayout(tb);
    tv->setContentsMargins(16, 10, 16, 10);
    tv->setSpacing(2);
    auto *cap = W::label("TOTAL PAYABLE NET", "caption");
    cap->setAlignment(Qt::AlignRight);
    tv->addWidget(cap);
    m_total = W::label("$0.00", "bigTotal");
    m_total->setAlignment(Qt::AlignRight);
    tv->addWidget(m_total);
    m_tender = W::label("");
    m_tender->setAlignment(Qt::AlignRight);
    m_tender->setStyleSheet("color:#006329; font-weight:600;");
    tv->addWidget(m_tender);
    m_checkout = W::button("Checkout [F10]", "primary", "check");
    m_checkout->setMinimumHeight(38);
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
        emit toast("Camera is stopped - press Start Cam first");
        return;
    }
    const auto &ps = AppState::instance().products();
    if (ps.isEmpty())
        return;
    const Product &p = ps[m_scanIndex++ % ps.size()];
    showCode(p.ean, true);
}

void RecordOrderPage::quickLookup()
{
    QStringList items;
    for (const auto &p : AppState::instance().products())
        items << QString("%1  -  %2").arg(p.ean, p.name);
    bool ok = false;
    const QString pick = QInputDialog::getItem(this, "Quick Lookup", "Find product:", items, 0, false, &ok);
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
    else
        emit toast((fromScanner ? "Scanned " : "Found ") + p->name);
}

void RecordOrderPage::setPreview(const std::optional<Product> &p)
{
    m_preview = p;
    m_addBtn->setEnabled(p.has_value());
    if (!p)
    {
        m_pName->setText("No product detected");
        m_pPrice->clear();
        m_pMeta->setText("Scan a barcode or enter an EAN to preview a product.");
        m_pStock->clear();
        m_pSku->clear();
        W::setBadge(m_pStockBadge, "—", "grey");
        m_pThumb->clear();
        return;
    }
    m_pName->setText(p->name);
    m_pPrice->setText(W::money(p->price));
    m_pMeta->setText(QString("EAN: %1 • %2 • %3").arg(p->ean, p->category, p->location));
    m_pStock->setText(QString("Stock: %1 in warehouse").arg(p->stock));
    m_pSku->setText("SKU: " + p->sku);
    const bool low = p->stock < p->reorderLimit;
    W::setBadge(m_pStockBadge, p->stock == 0 ? "Out of Stock" : low ? "Low Stock" : "Adequate Stock",
                p->stock == 0 ? "red" : low ? "amber" : "green");
    m_pStock->setStyleSheet(QString("font-weight:700; color:%1;").arg(low ? "#93000a" : "#006329"));

    QPixmap pm(64, 76);
    pm.fill(Qt::transparent);
    QPainter pa(&pm);
    pa.setRenderHint(QPainter::Antialiasing);
    pa.setPen(Qt::NoPen);
    pa.setBrush(QColor("#2b3f5c"));
    pa.drawRoundedRect(QRectF(14, 8, 36, 58), 5, 5);
    pa.setBrush(QColor("#f4e3c1"));
    pa.drawRoundedRect(QRectF(19, 24, 26, 26), 3, 3);
    pa.setPen(QColor("#0b1c30"));
    QFont f("Inter");
    f.setPixelSize(7);
    f.setBold(true);
    pa.setFont(f);
    pa.drawText(QRectF(19, 28, 26, 18), Qt::AlignCenter, p->name.section(' ', 0, 0).left(6).toUpper());
    pa.end();
    m_pThumb->setPixmap(pm);
    updateNotice();
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
        m_cart.push_back({p, qty, false});
    rebuildTable();
    m_table->selectRow(found ? m_table->rowCount() - 1 : m_table->rowCount() - 1);
    emit toast(QString("Added %1 × %2").arg(qty).arg(p.name));
}

int RecordOrderPage::currentRow() const
{
    const int r = m_table->currentRow();
    if (r >= 0)
        return r;
    return m_cart.isEmpty() ? -1 : m_cart.size() - 1;
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
    m_table->selectRow(row);
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
        const double sub = gross - disc;
        const double tax = sub * st.taxPercent / (100.0 + st.taxPercent);
        m_table->setRowHeight(r, 60);
        m_table->setCellWidget(r, 0, twoLine(l.p.sku, "", true));
        m_table->setCellWidget(r, 1, twoLine(l.p.name, QString("%1 • %2").arg(l.p.category, l.p.location)));
        m_table->setItem(r, 2, cell(W::money(l.p.price), Qt::AlignRight | Qt::AlignVCenter, "", true));
        auto *st2 = new QtyStepper(l.qty);
        m_table->setCellWidget(r, 3, st2);
        connect(st2, &QtyStepper::minus, this, [this, r]
                { changeQty(r, -1); });
        connect(st2, &QtyStepper::plus, this, [this, r]
                { changeQty(r, +1); });
        m_table->setItem(r, 4, cell(disc > 0 ? W::money(-disc) : "$0.00", Qt::AlignRight | Qt::AlignVCenter,
                                    disc > 0 ? "#006329" : "#737686"));
        m_table->setItem(r, 5, cell(W::money(tax), Qt::AlignRight | Qt::AlignVCenter, "#737686"));
        m_table->setItem(r, 6, cell(W::money(sub), Qt::AlignRight | Qt::AlignVCenter, "#004ac6", true));
        auto *del = new QPushButton;
        del->setProperty("kind", "flat");
        del->setIcon(Theme::icon("trash", Theme::error, 18));
        del->setCursor(Qt::PointingHandCursor);
        del->setToolTip("Remove line");
        auto *wrap = new QWidget;
        auto *wl = new QHBoxLayout(wrap);
        wl->setContentsMargins(0, 0, 0, 0);
        wl->setAlignment(Qt::AlignCenter);
        wl->addWidget(del);
        m_table->setCellWidget(r, 7, wrap);
        connect(del, &QPushButton::clicked, this, [this, r]
                { removeRow(r); });
    }
    updateTotals();
    updateNotice();
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
    const double tax = total * st.taxPercent / (100.0 + st.taxPercent);
    W::setBadge(m_itemsBadge, QString("%1 Item%2").arg(items).arg(items == 1 ? "" : "s"), items ? "blue" : "grey");
    m_gross->setText(W::money(gross));
    m_disc->setText(W::money(-disc));
    m_disc->setStyleSheet("font-weight:700; color:#006329;");
    m_tax->setText(W::money(tax));
    m_round->setText("+$0.00");
    m_total->setText(W::money(total));
    m_tender->setText(items ? "Exact Cash Tender: Ready" : "Add items to begin");
    m_checkout->setEnabled(items > 0);
}

void RecordOrderPage::updateNotice()
{
    const auto &ai = AppState::instance();
    QString sku, name;
    int limit = 0;
    auto consider = [&](const Product &p)
    {
        if (sku.isEmpty() && !p.sku.startsWith("CUSTOM") && p.stock < p.reorderLimit)
        {
            sku = p.sku;
            name = p.name;
            limit = p.reorderLimit;
        }
    };
    if (m_preview)
        consider(*m_preview);
    for (const auto &l : m_cart)
        consider(l.p);
    m_notice->setVisible(!sku.isEmpty());
    if (sku.isEmpty())
        return;
    m_noticeSku = sku;
    m_noticeText->setText(QString("Notice: <b>%1</b> is running low (&lt; %2 units).").arg(name).arg(limit));
    const bool done = ai.isReordered(sku);
    m_noticeReorder->setText(done ? "Ordered ✓" : "Reorder");
    m_noticeReorder->setEnabled(!done);
}

void RecordOrderPage::loadDemoDraft()
{
    // Mirrors the Stitch design so the screen is not empty on first launch.
    for (auto [sku, qty] : {std::pair<const char *, int>{"PRD-102", 2}, {"PRD-214", 1}, {"PRD-8840", 1}, {"PRD-512", 3}})
        if (auto p = AppState::instance().findByCode(sku))
            m_cart.push_back({*p, qty, false});
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
    emit toast("New draft started");
}

void RecordOrderPage::checkout()
{
    if (m_cart.isEmpty())
    {
        emit toast("The order draft is empty");
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
