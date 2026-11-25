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
#include <QTableWidget>
#include <QVBoxLayout>

using namespace Ui;

DiscountsPage::DiscountsPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root;
    pageShell(this, root, true);
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
    m_bannerTitle->setWordWrap(true);
    m_bannerSub->setWordWrap(true);
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
    lv->addWidget(W::label("Promotional seasons", "h1"));
    auto *lh = new QHBoxLayout;
    for (const QString &name : {"All", "Active", "Scheduled", "Expired"})
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
    m_table = makeTable({"SEASON NAME", "START DATE", "END DATE", "DISCOUNT", "ACTIVE", "STATUS", ""});
    auto *hh = m_table->horizontalHeader();
    hh->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 178);
    for (int cidx : {1, 2, 3, 5})
        hh->setSectionResizeMode(cidx, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(4, QHeaderView::Fixed);
    m_table->setColumnWidth(4, 96);
    hh->setSectionResizeMode(6, QHeaderView::Fixed);
    m_table->setColumnWidth(6, 110);
    lv->addWidget(m_table, 1);
    row->addWidget(list, 62);

    // ---- form
    auto *form = W::card(this);
    auto *fv = new QVBoxLayout(form);
    fv->setContentsMargins(18, 16, 18, 16);
    fv->setSpacing(8);
    m_formTitle = W::label("Add Promotional Season", "h1");
    fv->addWidget(m_formTitle);
    fv->addWidget(W::label("SEASON / CAMPAIGN NAME", "caption"));
    m_name = new QLineEdit;
    m_name->setMinimumHeight(40);
    m_name->setPlaceholderText("e.g. Independence Day Bash");
    fv->addWidget(m_name);
    auto *dates = new QHBoxLayout;
    auto *sv = new QVBoxLayout;
    sv->addWidget(W::label("START DATE", "caption"));
    m_start = dateEdit(QDate::currentDate(), "dd MMM yyyy", 40);
    auto *ev = new QVBoxLayout;
    ev->addWidget(W::label("END DATE", "caption"));
    m_end = dateEdit(QDate::currentDate().addDays(7), "dd MMM yyyy", 40);
    sv->addWidget(m_start);
    ev->addWidget(m_end);
    dates->addLayout(sv);
    dates->addLayout(ev);
    fv->addLayout(dates);
    fv->addWidget(W::label("DISCOUNT PERCENTAGE", "caption"));
    m_percent = new QDoubleSpinBox;
    m_percent->setRange(0, 100);
    m_percent->setDecimals(1);
    m_percent->setSuffix(" % OFF");
    m_percent->setValue(10);
    m_percent->setMinimumHeight(40);
    fv->addWidget(m_percent);
    m_active = new QCheckBox("Apply automatically during this season");
    m_active->setChecked(true);
    fv->addWidget(m_active);
    m_error = W::label("", "errorText");
    m_error->setWordWrap(true);
    fv->addWidget(m_error);
    fv->addStretch();
    auto *nl = W::label(QString("Cashiers can apply up to %1% manually at checkout.")
                            .arg(UiStore::kSpecialDiscountCapPercent), "muted");
    nl->setWordWrap(true);
    fv->addWidget(nl);
    auto *btns = new QHBoxLayout;
    auto *cancel = W::button("Cancel", "soft");
    m_save = W::button("Save Season", "primary", "check");
    m_save->setMinimumHeight(44);
    btns->addWidget(cancel);
    btns->addWidget(m_save, 1);
    fv->addLayout(btns);
    row->addWidget(form, 38);
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
        m_bannerTitle->setText(QString("Seasonal discount active now: %1% (%2, ends %3)")
                                   .arg(act->percent, 0, 'f', 0).arg(act->name, act->end.toString("d MMM yyyy")));
        m_bannerSub->setText("Applied automatically to every bill and shown in the bill's seasonal discount field. " +
                             QString::number(today.daysTo(act->end)) + " days remaining.");
        m_banner->setStyleSheet("#totalBox{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #0b5639,stop:1 #126840);}");
        m_rate->setText(QString("%1%").arg(act->percent, 0, 'f', 0));
        m_rateSub->setText(act->name);
    }
    else
    {
        m_bannerTitle->setText("No seasonal discount is active right now");
        m_bannerSub->setText("Bills are created without a seasonal discount until a season is active and its dates match today.");
        m_banner->setStyleSheet("#totalBox{background:#404c65;}");
        m_rate->setText("0%");
        m_rateSub->setText("No active season");
    }
    int upcoming = 0;
    for (const auto &s : store.seasons())
        if (s.start > today)
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
        m_table->setItem(r, 0, item(s.name + "\n" + s.id, Qt::AlignLeft | Qt::AlignVCenter, dim, true));
        m_table->setItem(r, 1, item(s.start.toString("dd MMM yyyy"), Qt::AlignLeft | Qt::AlignVCenter, dim));
        m_table->setItem(r, 2, item(s.end.toString("dd MMM yyyy"), Qt::AlignLeft | Qt::AlignVCenter, dim));
        m_table->setCellWidget(r, 3, wrapW(W::badge(QString("%1%").arg(s.percent, 0, 'f', 0), expired ? "grey" : "blue")));
        auto *tog = W::button(s.active ? "ON" : "OFF", "tab");
        tog->setCheckable(true);
        tog->setChecked(s.active);
        tog->setMinimumWidth(64);
        tog->setEnabled(!expired);
        m_table->setCellWidget(r, 4, wrapW(tog));
        m_table->setCellWidget(r, 5, wrapW(W::badge(st == "active" ? "ACTIVE NOW" : st.toUpper(),
                                                   st == "active" ? "green" : st == "scheduled" ? "amber" : "grey")));
        auto *cell = new QWidget;
        auto *h = new QHBoxLayout(cell);
        h->setContentsMargins(4, 0, 4, 0);
        h->setSpacing(4);
        auto *e = W::button("", "flat", "edit");
        auto *x = W::button("", "flat", "trash");
        h->addWidget(e);
        h->addWidget(x);
        h->addStretch();
        m_table->setCellWidget(r, 6, cell);
        const QString id = s.id;
        connect(tog, &QPushButton::clicked, this, [this, id](bool on)
                {
            UiStore::instance().setSeasonActive(id, on);
            emit toast(on ? "Season activated" : "Season deactivated"); });
        connect(e, &QPushButton::clicked, this, [this, id] { edit(id); });
        connect(x, &QPushButton::clicked, this, [this, id]
                {
            if (QMessageBox::question(this, "Delete season", "Delete this season?") != QMessageBox::Yes) return;
            UiStore::instance().removeSeason(id);
            emit toast("Season deleted"); });
    }
}

void DiscountsPage::resetForm()
{
    m_editing.clear();
    m_name->clear();
    m_start->setDate(QDate::currentDate());
    m_end->setDate(QDate::currentDate().addDays(7));
    m_percent->setValue(10);
    m_active->setChecked(true);
    m_error->clear();
    m_formTitle->setText("Add Promotional Season");
    m_save->setText("Save Season");
}

void DiscountsPage::edit(const QString &id)
{
    for (const auto &s : UiStore::instance().seasons())
        if (s.id == id)
        {
            m_editing = id;
            m_name->setText(s.name);
            m_start->setDate(s.start);
            m_end->setDate(s.end);
            m_percent->setValue(s.percent);
            m_active->setChecked(s.active);
            m_error->clear();
            m_formTitle->setText("Edit Season " + id);
            m_save->setText("Update Season");
            return;
        }
}

void DiscountsPage::submit()
{
    auto &store = UiStore::instance();
    const QString why = store.validateSeason(m_name->text(), m_start->date(), m_end->date(), m_percent->value());
    m_error->setText(why);
    if (!why.isEmpty())
        return;
    Season s;
    s.name = m_name->text().trimmed();
    s.start = m_start->date();
    s.end = m_end->date();
    s.percent = m_percent->value();
    s.active = m_active->isChecked();
    if (m_editing.isEmpty())
    {
        s.id = store.nextSeasonId();
        store.addSeason(s);
        emit toast("Season added to Seasons.txt");
    }
    else
    {
        s.id = m_editing;
        store.updateSeason(m_editing, s);
        emit toast("Season updated");
    }
    resetForm();
}
