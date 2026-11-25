#include "SettingsPage.h"
#include "Dialogs.h"
#include "Theme.h"
#include "UiKit.h"
#include "Widgets.h"
#include <QApplication>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextStream>
#include <QTime>
#include <QVBoxLayout>

using namespace Ui; // makeTable, item, wrapW, pageShell, writeCsv (see UiKit.h)


// =================================================================== SettingsPage
SettingsPage::SettingsPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root;
    pageShell(this, root);
    auto* card = W::card(this);
    card->setMaximumWidth(620);
    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(20, 16, 20, 16);
    v->setSpacing(14);
    W::headerRow(v, "settings", "Terminal Settings");
    auto& s = AppState::instance().settings();

    auto* form = new QFormLayout;
    form->setHorizontalSpacing(20);
    form->setVerticalSpacing(12);
    auto* low = new QSpinBox;
    low->setRange(0, 1000);
    low->setSuffix(" units");
    low->setValue(s.lowStockThreshold);
    auto* autoAdd = new QCheckBox("Add scanned items to the order automatically");
    autoAdd->setChecked(s.autoAddScans);
    auto* beep = new QCheckBox("Play a beep on successful scan");
    beep->setChecked(s.beepOnScan);
    form->addRow("Low-stock limit", low);
    form->addRow("Scanning", autoAdd);
    form->addRow("", beep);
    v->addLayout(form);
    auto *note = W::label("Tax and discounts come from the product catalog. Change the theme from the top bar.", "muted");
    note->setWordWrap(true);
    v->addWidget(note);
    v->addStretch();
    auto* h = new QHBoxLayout;
    h->addStretch();
    auto* save = W::button("Apply", "primary", "check");
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
    root->addWidget(card);
    root->addStretch();
}
