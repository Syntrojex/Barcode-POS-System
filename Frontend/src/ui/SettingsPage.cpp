#include "SettingsPage.h"
#include "UiKit.h"
#include "Widgets.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QVBoxLayout>

using namespace Ui; // makeTable, item, wrapW, pageShell, writeCsv (see UiKit.h)


// =================================================================== SettingsPage
SettingsPage::SettingsPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root;
    pageShell(this, root);
    root->setSpacing(8);
    root->addWidget(sectionHeader({}, "Terminal Settings", "Configure scanning behavior and stock alerts for this terminal."));
    auto* panels = new QHBoxLayout;
    panels->setSpacing(8);
    auto* card = W::card(this, "subCard");
    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(16, 16, 16, 16);
    v->setSpacing(12);
    auto& s = AppState::instance().settings();

    v->addWidget(W::label("INVENTORY ALERTS", "caption"));
    auto* inventory = new QHBoxLayout;
    auto* stockText = new QVBoxLayout;
    auto* stockLabel = W::label("Low-stock limit", "h2");
    stockText->addWidget(stockLabel);
    auto* stockHelp = W::label("Products at or below this quantity are marked low stock.", "muted");
    stockHelp->setWordWrap(true);
    stockText->addWidget(stockHelp);
    inventory->addLayout(stockText, 1);
    auto* low = new QSpinBox;
    low->setAccessibleName("Low-stock limit");
    low->setRange(0, 1000);
    low->setSuffix(" units");
    low->setValue(s.lowStockThreshold);
    low->setMinimumHeight(40);
    low->setMinimumWidth(104);
    stockLabel->setBuddy(low);
    inventory->addWidget(low, 0, Qt::AlignVCenter);
    v->addLayout(inventory);
    v->addSpacing(8);
    v->addWidget(W::label("SCANNING", "caption"));

    auto* autoAdd = new QCheckBox("Add scanned items to the order automatically");
    autoAdd->setChecked(s.autoAddScans);
    auto* beep = new QCheckBox("Play a beep on successful scan");
    beep->setChecked(s.beepOnScan);
    auto setting = [v](QCheckBox* control, const QString& description)
    {
        auto* group = W::card(nullptr, "field");
        auto* layout = new QVBoxLayout(group);
        layout->setContentsMargins(12, 8, 12, 8);
        layout->setSpacing(4);
        control->setMinimumHeight(26);
        layout->addWidget(control);
        auto* help = W::label(description, "muted");
        help->setWordWrap(true);
        layout->addWidget(help);
        v->addWidget(group);
    };
    setting(autoAdd, "Matching camera scans add to the cart on Record Order.");
    setting(beep, "Play confirmation audio when a scan is accepted.");

    auto* pricing = W::card(card, "field");
    auto* pricingLayout = new QVBoxLayout(pricing);
    pricingLayout->setContentsMargins(12, 8, 12, 8);
    pricingLayout->setSpacing(4);
    pricingLayout->addWidget(W::label("Product pricing", "h2"));
    auto* note = W::label("Tax and item discounts are configured per product.", "muted");
    note->setWordWrap(true);
    pricingLayout->addWidget(note);
    v->addWidget(pricing);
    auto* h = new QHBoxLayout;
    h->addStretch();
    auto* save = W::button("Apply", "primary", "check");
    save->setMinimumSize(116, 40);
    h->addWidget(save);
    v->addLayout(h);
    connect(save, &QPushButton::clicked, this, [=, this]
        {
            auto& st = AppState::instance().settings();
            st.lowStockThreshold = low->value();
            AppState::instance().emitChanged();
            st.autoAddScans = autoAdd->isChecked();
            st.beepOnScan = beep->isChecked();
            emit toast("Settings applied"); });
    panels->addWidget(card, 7, Qt::AlignTop);

    auto* about = W::card(this, "subCard");
    auto* details = new QVBoxLayout(about);
    details->setContentsMargins(16, 16, 16, 16);
    details->setSpacing(8);
    details->addWidget(W::label("ABOUT THIS TERMINAL", "caption"));
    details->addWidget(W::label("Syntro-Check POS v2", "h1"));
    details->addSpacing(8);
    details->addWidget(W::label("Theme", "h2"));
    auto* theme = W::label("Use the control in the top bar to switch between light and dark mode.", "muted");
    theme->setWordWrap(true);
    details->addWidget(theme);
    panels->addWidget(about, 3, Qt::AlignTop);
    root->addLayout(panels);
    root->addStretch();
}
