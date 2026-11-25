#include "DiscountsPage.h"
#include "Theme.h"
#include "UiKit.h"
#include "UiStore.h"
#include "Widgets.h"
#include <QCheckBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QTableWidget>
#include <QVBoxLayout>

using namespace Ui;

DiscountsPage::DiscountsPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root;
    pageShell(this, root);
    root->setSpacing(12);

    // ---- banner
    m_banner = new QFrame;
    m_banner->setObjectName("totalBox");
    auto *bh = new QHBoxLayout(m_banner);
    bh->setContentsMargins(18, 14, 18, 14);
    bh->setSpacing(14);
    bh->addWidget(W::iconTile("tag", Qt::white, "#2fbf71", "#0b7a36", 46));
    auto *bv = new QVBoxLayout;
    bv->setSpacing(2);
    m_bannerTitle = W::label("");
    m_bannerTitle->setStyleSheet("color:white; font-size:17px; font-weight:800;");
    m_bannerSub = W::label("");
    m_bannerSub->setStyleSheet("color:#c9d8fb;");
    bv->addWidget(m_bannerTitle);
    bv->addWidget(m_bannerSub);
    bh->addLayout(bv, 1);
    root->addWidget(m_banner);

    auto *tiles = new QHBoxLayout;
    tiles->setSpacing(12);
    QLabel *a, *b, *c, *d, *e, *f;
    tiles->addWidget(statTile("CURRENT ACTIVE RATE", "tag", m_rate, m_rateSub, "green"));
    tiles->addWidget(statTile("UPCOMING CAMPAIGNS", "clock", m_upcoming, m_upSub, "blue"));
    tiles->addWidget(statTile("CASHIER SPECIAL LIMIT", "lock", a, b, "red"));
    a->setText(QString("%1%").arg(UiStore::kSpecialDiscountCapPercent, 0, 'f', 1));
    b->setText("Special discount cap at checkout");
    (void)c; (void)d; (void)e; (void)f;
    root->addLayout(tiles);

    auto *row = new QHBoxLayout;
    row->setSpacing(12);

    // ---- list
    auto *list = W::card(this);
    auto *lv = new QVBoxLayout(list);
    lv->setContentsMargins(16, 14, 16, 12);
    lv->setSpacing(10);
    lv->addWidget(W::label("Seasons (Seasons.txt)", "h1"));
    auto *sub = W::label("When a bill's date is inside an active season, its discount percentage is applied and saved "
                         "in the bill's seasonal discount field.", "muted");
    sub->setWordWrap(true);
    lv->addWidget(sub);
    auto *lh = new QHBoxLayout; // filter tabs get their own row, so they never squeeze the title
    lh->setSpacing(6);
    for (const QString &name : {"All", "Active", "Scheduled", "Inactive", "Expired"})
    {
        auto *btn = W::button(name, "tab");
        btn->setCheckable(true);
        btn->setChecked(name == "All");
        lh->addWidget(btn);
        m_tabs << btn;
        connect(btn, &QPushButton::clicked, this, [this, name]
                {
            m_filter = name;
            for (auto *t : m_tabs) t->setChecked(t->text() == name);
            rebuild(); });
    }
    lh->addStretch();
    lv->addLayout(lh);
    // the four fields of a Seasons.txt line, then today's status and the actions
    m_table = makeTable({"START DATE", "END DATE", "DISCOUNT %", "IS ACTIVE", "STATUS TODAY", ""});
    auto *hh = m_table->horizontalHeader();
    // fixed widths, so the headers and badges are never cut off
    // fixed widths; the last (actions) column takes whatever room is left over
    hh->setSectionResizeMode(QHeaderView::Fixed);
    const int widths[] = {128, 128, 104, 96, 132, 70};
    for (int cidx = 0; cidx <= 5; ++cidx)
        m_table->setColumnWidth(cidx, widths[cidx]);
    hh->setStretchLastSection(true);
    hh->setMinimumSectionSize(60);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); // narrow window: scroll sideways, never clip
    lv->addWidget(m_table, 1);
    row->addWidget(list, 62);

    // ---- form
    auto *form = W::card(this);
    auto *fv = new QVBoxLayout(form);
    fv->setContentsMargins(18, 16, 18, 16);
    fv->setSpacing(8);
    m_formTitle = W::label("Add Seasonal Discount", "h1");
    fv->addWidget(m_formTitle);
    auto *formSub = W::label("Saved as one line of Seasons.txt: start date, end date, discount percentage, isActive.", "muted");
    formSub->setWordWrap(true);
    fv->addWidget(formSub);
    auto *dates = new QHBoxLayout;
    dates->setSpacing(10);
    auto *sv = new QVBoxLayout;
    sv->setSpacing(4);
    sv->addWidget(W::label("START DATE", "caption"));
    m_start = new QDateEdit(QDate::currentDate());
    auto *ev = new QVBoxLayout;
    ev->setSpacing(4);
    ev->addWidget(W::label("END DATE", "caption"));
    m_end = new QDateEdit(QDate::currentDate().addDays(7));
    for (auto *de : {m_start, m_end})
    {
        de->setCalendarPopup(true);
        de->setDisplayFormat("dd MMM yyyy");
        de->setMinimumHeight(40);
    }
    sv->addWidget(m_start);
    ev->addWidget(m_end);
    dates->addLayout(sv, 1);
    dates->addLayout(ev, 1);
    fv->addLayout(dates);
    fv->addSpacing(4);
    fv->addWidget(W::label("DISCOUNT PERCENTAGE", "caption"));
    m_percent = new QDoubleSpinBox;
    m_percent->setRange(0.5, 100);
    m_percent->setDecimals(2);
    m_percent->setSingleStep(0.5);
    m_percent->setSuffix(" %");
    m_percent->setValue(10);
    m_percent->setMinimumHeight(40);
    fv->addWidget(m_percent);
    m_active = new QCheckBox("Active (isActive = true)");
    m_active->setToolTip("An active season is applied automatically to every bill whose date is inside it.");
    m_active->setMinimumHeight(30);
    m_active->setChecked(true);
    fv->addWidget(m_active);
    m_error = W::label("");
    m_error->setWordWrap(true);
    m_error->setStyleSheet("color:#ba1a1a; font-weight:600;");
    fv->addWidget(m_error);
    fv->addStretch();
    auto *note = W::card(form, "subCard");
    auto *nv = new QVBoxLayout(note);
    nv->setContentsMargins(12, 10, 12, 10);
    nv->addWidget(W::label("POS policy note", "h2"));
    auto *nl = W::label("Special manual discounts entered by cashiers at checkout are capped at 8% maximum.", "muted");
    nl->setWordWrap(true);
    nv->addWidget(nl);
    fv->addWidget(note);
    auto *btns = new QHBoxLayout;
    auto *cancel = W::button("Cancel", "soft");
    m_save = W::button("Save Discount", "primary", "check");
    m_save->setMinimumHeight(44);
    btns->addWidget(cancel);
    btns->addWidget(m_save, 1);
    fv->addLayout(btns);
    auto *formScroll = new QScrollArea;
    formScroll->setWidgetResizable(true);
    formScroll->setFrameShape(QFrame::NoFrame);
    formScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    formScroll->setStyleSheet("QScrollArea, QScrollArea > QWidget > QWidget { background: transparent; border: none; }");
    formScroll->setWidget(form);
    // wide enough for the form plus the vertical scroll bar, so the card is never cut off on the right
    formScroll->setMinimumWidth(form->minimumSizeHint().width() + formScroll->verticalScrollBar()->sizeHint().width() + 4);
    row->addWidget(formScroll, 38);
    root->addLayout(row, 1);

    connect(cancel, &QPushButton::clicked, this, &DiscountsPage::resetForm);
    connect(m_save, &QPushButton::clicked, this, &DiscountsPage::submit);
    connect(&UiStore::instance(), &UiStore::seasonsChanged, this, &DiscountsPage::refresh);
    refresh();
}

void DiscountsPage::refresh() { rebuild(); }

void DiscountsPage::rebuild()
{
    auto &store = UiStore::instance();
    const QDate today = QDate::currentDate();
    const Season *act = store.activeSeason(today);
    if (act)
    {
        m_bannerTitle->setText(QString("Seasonal discount active today: %1% (%2)")
                                   .arg(act->percent, 0, 'g', 4).arg(act->name));
        m_bannerSub->setText("Applied automatically to every bill and shown in the bill's seasonal discount field. " +
                             QString::number(today.daysTo(act->end)) + " days remaining.");
        m_banner->setStyleSheet("#totalBox{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #0b7a36,stop:1 #12a150);}");
        m_rate->setText(QString("%1%").arg(act->percent, 0, 'g', 4));
        m_rateSub->setText("Until " + act->end.toString("d MMM yyyy"));
    }
    else
    {
        m_bannerTitle->setText("No seasonal discount is active right now");
        m_bannerSub->setText("Bills are created without a seasonal discount until a season is active and its dates match today.");
        m_banner->setStyleSheet("#totalBox{background:#55617c;}");
        m_rate->setText("0%");
        m_rateSub->setText("No active season");
    }
    int upcoming = 0;
    for (const auto &s : store.seasons())
        if (s.active && s.start > today)
            ++upcoming;
    m_upcoming->setText(QString::number(upcoming));
    m_upSub->setText("Scheduled");

    QVector<Season> rows;
    for (const auto &s : store.seasons())
    {
        const QString st = s.statusOn(today);
        if (m_filter != "All" && st.compare(m_filter, Qt::CaseInsensitive) != 0)
            continue;
        rows << s;
    }
    m_table->setRowCount(0);
    m_table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r)
    {
        const Season s = rows[r];
        const QString st = s.statusOn(today);
        const bool expired = st == "expired";
        const QString dim = expired ? "#8a94ab" : "";
        m_table->setRowHeight(r, 58);
        m_table->setItem(r, 0, item(s.start.toString("dd MMM yyyy"), Qt::AlignLeft | Qt::AlignVCenter, dim, true));
        m_table->setItem(r, 1, item(s.end.toString("dd MMM yyyy"), Qt::AlignLeft | Qt::AlignVCenter, dim, true));
        m_table->setCellWidget(r, 2, wrapW(W::badge(QString("%1%").arg(s.percent, 0, 'g', 4), expired ? "grey" : "blue")));
        auto *tog = W::button(s.active ? "ON" : "OFF", "tab");
        tog->setCheckable(true);
        tog->setChecked(s.active);
        tog->setMinimumWidth(54);
        tog->setEnabled(!expired);
        m_table->setCellWidget(r, 3, wrapW(tog));
        m_table->setCellWidget(r, 4, wrapW(W::badge(st.toUpper(),
                                                   st == "active" ? "green" : st == "scheduled" ? "amber" : st == "inactive" ? "red" : "grey")));
        auto *cell = new QWidget;
        auto *h = new QHBoxLayout(cell);
        h->setContentsMargins(4, 0, 4, 0);
        h->setSpacing(4);
        auto *e = W::button("", "flat", "edit");
        auto *x = W::button("", "flat", "trash");
        h->addWidget(e);
        h->addWidget(x);
        h->addStretch();
        m_table->setCellWidget(r, 5, cell);
        const QString id = s.id;
        connect(tog, &QPushButton::clicked, this, [this, id](bool on)
                {
            const QString why = UiStore::instance().setSeasonActive(id, on);
            if (!why.isEmpty())
                QMessageBox::warning(this, "Cannot activate", why);
            else
                emit toast(on ? "Seasonal discount set active" : "Seasonal discount set inactive"); });
        connect(e, &QPushButton::clicked, this, [this, id] { edit(id); });
        connect(x, &QPushButton::clicked, this, [this, id]
                {
            if (QMessageBox::question(this, "Delete season", "Delete this seasonal discount from Seasons.txt?") != QMessageBox::Yes) return;
            UiStore::instance().removeSeason(id);
            if (m_editing == id) resetForm();
            emit toast("Seasonal discount deleted"); });
    }
}

void DiscountsPage::resetForm()
{
    m_editing.clear();
    m_start->setDate(QDate::currentDate());
    m_end->setDate(QDate::currentDate().addDays(7));
    m_percent->setValue(10);
    m_active->setChecked(true);
    m_error->clear();
    m_formTitle->setText("Add Seasonal Discount");
    m_save->setText("Save Discount");
}

void DiscountsPage::edit(const QString &id)
{
    for (const auto &s : UiStore::instance().seasons())
        if (s.id == id)
        {
            m_editing = id;
            m_start->setDate(s.start);
            m_end->setDate(s.end);
            m_percent->setValue(s.percent);
            m_active->setChecked(s.active);
            m_error->clear();
            m_formTitle->setText("Edit Seasonal Discount (" + s.name + ")");
            m_save->setText("Update Discount");
            return;
        }
}

void DiscountsPage::submit()
{
    auto &store = UiStore::instance();
    const QString why = store.validateSeason(m_start->date(), m_end->date(), m_percent->value(), m_active->isChecked(), m_editing);
    m_error->setText(why);
    if (!why.isEmpty())
        return;
    Season s;
    s.start = m_start->date();
    s.end = m_end->date();
    s.percent = m_percent->value();
    s.active = m_active->isChecked();
    if (m_editing.isEmpty())
    {
        s.id = store.nextSeasonId();
        store.addSeason(s);
        emit toast("Seasonal discount added to Seasons.txt");
    }
    else
    {
        s.id = m_editing;
        store.updateSeason(m_editing, s);
        emit toast("Seasonal discount updated in Seasons.txt");
    }
    resetForm();
}
