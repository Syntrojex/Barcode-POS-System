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
#include <QMessageBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTableWidget>
#include <QVBoxLayout>

using namespace Ui;

DiscountsPage::DiscountsPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root;
    pageShell(this, root, true);
    root->setSpacing(8);
    root->addWidget(sectionHeader({}, "Seasonal Discounts", "Manage campaigns and automatic seasonal price reductions."));

    // ---- banner
    m_banner = new QFrame;
    m_banner->setObjectName("seasonBanner");
    auto *bh = new QHBoxLayout(m_banner);
    bh->setContentsMargins(16, 8, 16, 8);
    auto *bv = new QVBoxLayout;
    bv->setSpacing(2);
    bv->addWidget(W::label("CURRENT PROMOTION", "caption"));
    m_bannerTitle = W::label("", "h1");
    m_bannerSub = W::label("", "muted");
    m_bannerTitle->setWordWrap(true);
    m_bannerSub->setWordWrap(true);
    bv->addWidget(m_bannerTitle);
    bv->addWidget(m_bannerSub);
    bh->addLayout(bv, 1);
    root->addWidget(m_banner);

    auto *tiles = new QHBoxLayout;
    tiles->setSpacing(8);
    auto tile = [tiles](const QString &title, QLabel *&value, QLabel *&detail)
    {
        auto *card = W::card(nullptr, "subCard");
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(12, 8, 12, 8);
        layout->setSpacing(2);
        layout->addWidget(W::label(title, "caption"));
        value = W::label("", "bigValue");
        detail = W::label("", "muted");
        layout->addWidget(value);
        layout->addWidget(detail);
        tiles->addWidget(card, 1);
    };
    QLabel *cap, *capDetails;
    tile("ACTIVE RATE", m_rate, m_rateSub);
    tile("UPCOMING CAMPAIGNS", m_upcoming, m_upSub);
    tile("CASHIER SPECIAL CAP", cap, capDetails);
    cap->setText(QString("%1%").arg(UiStore::kSpecialDiscountCapPercent));
    capDetails->setText("At checkout");
    root->addLayout(tiles);

    auto *row = new QHBoxLayout;
    row->setSpacing(8);

    // ---- list
    auto *list = W::card(this, "subCard");
    auto *lv = new QVBoxLayout(list);
    lv->setContentsMargins(16, 14, 16, 12);
    lv->setSpacing(8);
    lv->addWidget(W::label("Configured Promotional Seasons", "h1"));
    auto *lh = new QHBoxLayout;
    lh->setSpacing(4);
    for (const QString &name : {"All", "Active", "Scheduled", "Inactive", "Expired"})
    {
        auto *btn = W::button(name, "tab");
        btn->setProperty("compactSeasonTab", true);
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
    m_table = makeTable({"START DATE", "END DATE", "RATE", "ACTIVE", "STATUS", "ACTIONS"});
    m_table->setProperty("managementTable", true);
    m_table->setAlternatingRowColors(true);
    m_table->setAccessibleName("Configured promotional seasons");
    auto *hh = m_table->horizontalHeader();
    // fixed widths; the last (actions) column takes whatever room is left over
    hh->setSectionResizeMode(QHeaderView::Fixed);
    const int widths[] = {100, 100, 70, 72, 108, 88};
    for (int cidx = 0; cidx <= 5; ++cidx)
        m_table->setColumnWidth(cidx, widths[cidx]);
    hh->setStretchLastSection(true);
    hh->setMinimumSectionSize(60);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); // narrow window: scroll sideways, never clip
    lv->addWidget(m_table, 1);
    lv->addWidget(W::label("Status reflects dates and activation.", "muted"));
    row->addWidget(list, 62);

    // ---- form
    auto *form = W::card(this, "subCard");
    auto *fv = new QVBoxLayout(form);
    fv->setContentsMargins(14, 12, 14, 12);
    fv->setSpacing(8);
    m_formTitle = W::label("Add Promotional Season", "h1");
    m_formTitle->setWordWrap(true);
    fv->addWidget(m_formTitle);
    m_seasonLabel = W::label("", "muted");
    m_seasonLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_seasonLabel->setWordWrap(true);
    fv->addWidget(m_seasonLabel);
    auto *dates = new QHBoxLayout;
    dates->setSpacing(10);
    auto *sv = new QVBoxLayout;
    sv->setSpacing(4);
    sv->addWidget(W::label("START DATE", "caption"));
    m_start = dateEdit(QDate::currentDate(), "dd MMM yyyy", 36);
    m_start->setAccessibleName("Season start date");
    auto *ev = new QVBoxLayout;
    ev->setSpacing(4);
    ev->addWidget(W::label("END DATE", "caption"));
    m_end = dateEdit(QDate::currentDate().addDays(7), "dd MMM yyyy", 36);
    m_end->setAccessibleName("Season end date");
    sv->addWidget(m_start);
    ev->addWidget(m_end);
    dates->addLayout(sv, 1);
    dates->addLayout(ev, 1);
    fv->addLayout(dates);
    const auto updateSeasonLabel = [this] {
        m_seasonLabel->setText(UiStore::seasonLabel(m_start->date(), m_end->date()));
    };
    connect(m_start, &QDateEdit::dateChanged, this, updateSeasonLabel);
    connect(m_end, &QDateEdit::dateChanged, this, updateSeasonLabel);
    updateSeasonLabel();
    fv->addSpacing(4);
    fv->addWidget(W::label("DISCOUNT PERCENTAGE", "caption"));
    m_percent = new QDoubleSpinBox;
    m_percent->setRange(0.5, 100);
    m_percent->setAccessibleName("Seasonal discount percentage");
    m_percent->setDecimals(2);
    m_percent->setSingleStep(0.5);
    m_percent->setSuffix(" %");
    m_percent->setValue(10);
    m_percent->setMinimumHeight(36);
    fv->addWidget(m_percent);
    m_active = new QCheckBox("Apply automatically during this season");
    m_active->setMinimumHeight(30);
    m_active->setChecked(true);
    fv->addWidget(m_active);
    m_error = W::label("", "errorText");
    m_error->setWordWrap(true);
    m_error->hide();
    fv->addWidget(m_error);
    fv->addStretch();
    auto *note = W::card(form, "field");
    auto *noteLayout = new QVBoxLayout(note);
    noteLayout->setContentsMargins(8, 6, 8, 6);
    auto *nl = W::label(QString("The special checkout discount is capped at %1%.")
                            .arg(UiStore::kSpecialDiscountCapPercent), "muted");
    nl->setWordWrap(true);
    noteLayout->addWidget(nl);
    fv->addWidget(note);
    auto *btns = new QHBoxLayout;
    auto *cancel = W::button("Cancel", "soft");
    m_save = W::button("Save Season", "primary", "check");
    m_save->setMinimumHeight(38);
    btns->addWidget(cancel);
    btns->addWidget(m_save, 1);
    fv->addLayout(btns);
    auto *formScroll = new QScrollArea;
    formScroll->setWidgetResizable(true);
    formScroll->setFrameShape(QFrame::NoFrame);
    formScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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
    m_banner->setProperty("activeSeason", act != nullptr);
    Theme::repolish(m_banner);
    if (act)
    {
        m_bannerTitle->setText(QString("%1% seasonal discount active · %2")
                                   .arg(act->percent, 0, 'g', 4).arg(act->name));
        m_bannerSub->setText("Applied automatically to every bill and shown in the bill's seasonal discount field. " +
                             QString::number(today.daysTo(act->end)) + " days remaining.");
        m_rate->setText(QString("%1%").arg(act->percent, 0, 'g', 4));
        m_rateSub->setText("Until " + act->end.toString("d MMM yyyy"));
    }
    else
    {
        m_bannerTitle->setText("No seasonal discount is active right now");
        m_bannerSub->setText("An enabled season applies automatically within its date range.");
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
        m_table->setRowHeight(r, 48);
        m_table->setItem(r, 0, item(s.start.toString("dd MMM yyyy"), Qt::AlignLeft | Qt::AlignVCenter, dim, true));
        m_table->setItem(r, 1, item(s.end.toString("dd MMM yyyy"), Qt::AlignLeft | Qt::AlignVCenter, dim, true));
        m_table->setItem(r, 2, item(QString("%1%").arg(s.percent, 0, 'g', 4),
                                    Qt::AlignLeft | Qt::AlignVCenter, dim, true));
        auto *tog = W::button(s.active ? "ON" : "OFF", "tab");
        tog->setProperty("compactSeasonTab", true);
        tog->setCheckable(true);
        tog->setChecked(s.active);
        tog->setMinimumWidth(54);
        tog->setEnabled(!expired);
        m_table->setCellWidget(r, 3, wrapW(tog));
        QString status = st;
        status[0] = status.at(0).toUpper();
        m_table->setCellWidget(r, 4, wrapW(W::badge(status,
                                                   st == "active" ? "green" : st == "scheduled" ? "amber" : st == "inactive" ? "red" : "grey")));
        auto *cell = new QWidget;
        auto *h = new QHBoxLayout(cell);
        h->setContentsMargins(4, 0, 4, 0);
        h->setSpacing(4);
        auto *e = W::button("", "flat", "edit");
        auto *x = W::button("", "flat", "trash");
        e->setToolTip("Edit season");
        x->setToolTip("Delete season");
        h->addWidget(e);
        h->addWidget(x);
        h->addStretch();
        m_table->setCellWidget(r, 5, cell);
        const QString id = s.id;
        connect(tog, &QPushButton::clicked, this, [this, id](bool on)
                {
            const QString why = UiStore::instance().setSeasonActive(id, on);
            if (!why.isEmpty())
            {
                rebuild();
                QMessageBox::warning(this, "Cannot activate", why);
            }
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
    m_error->hide();
    m_formTitle->setText("Add Promotional Season");
    m_formTitle->setToolTip({});
    m_save->setText("Save Season");
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
            m_error->hide();
            m_formTitle->setText("Edit Promotional Season");
            m_formTitle->setToolTip(s.name);
            m_save->setText("Save Season");
            return;
        }
}

void DiscountsPage::submit()
{
    auto &store = UiStore::instance();
    const QString why = store.validateSeason(m_start->date(), m_end->date(), m_percent->value(), m_active->isChecked(), m_editing);
    m_error->setText(why);
    m_error->setVisible(!why.isEmpty());
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
