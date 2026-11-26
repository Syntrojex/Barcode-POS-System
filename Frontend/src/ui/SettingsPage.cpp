#include "SettingsPage.h"
#include "AppState.h"
#include "DarkModeManager.h"
#include "ScanAudio.h"
#include "UiKit.h"
#include "Widgets.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QTabBar>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace
{
    void addSetting(QVBoxLayout *page, const QString &title, const QString &description, QWidget *control)
    {
        auto *card = W::card(nullptr, "field");
        auto *row = new QHBoxLayout(card);
        row->setContentsMargins(14, 12, 14, 12);
        row->setSpacing(16);
        auto *text = new QVBoxLayout;
        text->setSpacing(4);
        auto *name = W::label(title, "h2");
        name->setBuddy(control);
        control->setAccessibleName(title);
        text->addWidget(name);
        if (!description.isEmpty())
        {
            auto *help = W::label(description, "muted");
            help->setWordWrap(true);
            text->addWidget(help);
        }
        row->addLayout(text, 1);
        row->addWidget(control, 0, Qt::AlignVCenter);
        page->addWidget(card);
    }
}

SettingsPage::SettingsPage(QWidget *parent, UserRole role) : QWidget(parent)
{
    QVBoxLayout *root;
    Ui::pageShell(this, root, true);
    root->setSpacing(12);
    root->addWidget(Ui::sectionHeader({}, "Terminal Settings", "Configure the way this terminal handles checkout, scanning, and receipts."));

    auto *tabs = new QTabWidget;
    tabs->setObjectName("terminalSettingsTabs");
    tabs->tabBar()->setDrawBase(false);
    auto addTab = [tabs](const QString &title, const QString &description)
    {
        auto *panel = W::card(nullptr, "subCard");
        tabs->addTab(panel, title);
        auto *layout = new QVBoxLayout(panel);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(12);
        auto *intro = W::label(description, "muted");
        intro->setWordWrap(true);
        layout->addWidget(intro);
        return layout;
    };

    auto *checkout = addTab("Checkout", "Choose the payment option selected when checkout opens.");
    auto *payment = new QComboBox;
    using Payment = barcodehub::order::PaymentMode;
    payment->addItem("Cash", int(Payment::Cash));
    payment->addItem("Card", int(Payment::Card));
    payment->addItem("Wallet", int(Payment::Wallet));
    addSetting(checkout, "Default payment method", "The cashier can select a different method for each sale.", payment);
    checkout->addStretch();

    auto *scanner = addTab("Scanner", "Set camera scan behavior and check confirmation audio.");
    auto *autoAdd = new QCheckBox("Enabled");
    addSetting(scanner, "Add scans directly to the cart", "A camera scan, or a full barcode from a USB scanner / typed with Enter, adds the item to the cart at once.", autoAdd);
    auto *beep = new QCheckBox("Enabled");
    addSetting(scanner, "Beep on successful scan", "Play confirmation audio when a scan is accepted.", beep);
    auto *testBeep = W::button("Play test sound", "soft");
    addSetting(scanner, "Scan confirmation sound", "Preview the sound through the terminal's current audio output.", testBeep);
    connect(testBeep, &QPushButton::clicked, this, [this]
            { emit toast(ScanAudio::instance().playIfEnabled(true) ? "Test sound requested" : "Could not play the test sound"); });
    scanner->addStretch();

    auto *inventory = addTab("Inventory", "Set the stock level used for low-stock indicators.");
    auto *low = new QSpinBox;
    low->setRange(0, 1000);
    low->setSuffix(" units");
    addSetting(inventory, "Low-stock limit", "Products at or below this quantity are flagged. This setting leaves quantities unchanged.", low);
    inventory->addStretch();

    auto *appearance = addTab("Appearance", "Choose a theme for the entire terminal.");
    auto *theme = new QComboBox;
    theme->addItem("Light mode", false);
    theme->addItem("Dark mode", true);
    addSetting(appearance, "Color theme", "This uses the same theme preference as the top-bar toggle.", theme);
    appearance->addStretch();

    auto *receipts = addTab("Receipts", "Set the defaults for bills saved as PDF.");
    auto *paper = new QComboBox;
    paper->addItem("A5", false);
    paper->addItem("A4", true);
    addSetting(receipts, "PDF paper size", {}, paper);
    auto *folder = new QLineEdit;
    folder->setAccessibleName("Receipt export folder");
    folder->setPlaceholderText("Documents folder");
    auto *browse = W::button("Browse…", "soft");
    auto *folderRow = new QHBoxLayout;
    folderRow->addWidget(folder, 1);
    folderRow->addWidget(browse);
    auto *folderCard = W::card(nullptr, "field");
    auto *folderLayout = new QVBoxLayout(folderCard);
    folderLayout->setContentsMargins(14, 12, 14, 12);
    folderLayout->setSpacing(8);
    auto *folderLabel = W::label("Default export folder", "h2");
    folderLabel->setBuddy(folder);
    folderLayout->addWidget(folderLabel);
    folderLayout->addLayout(folderRow);
    auto *folderHelp = W::label("Leave blank to use Documents. You can pick another location when saving a bill.", "muted");
    folderHelp->setWordWrap(true);
    folderLayout->addWidget(folderHelp);
    receipts->addWidget(folderCard);
    connect(browse, &QPushButton::clicked, this, [this, folder]
            {
                const auto chosen = QFileDialog::getExistingDirectory(this, "Choose receipt export folder", folder->text());
                if (!chosen.isEmpty())
                    folder->setText(QDir::toNativeSeparators(chosen));
            });
    receipts->addStretch();

    if (RolePolicy::canOpen(role, Pages::Inventory))
    {
        auto *data = addTab("Data", "Check which catalogue and transaction files this terminal is using.");
        const QString dataPath = AppState::instance().dataDir();
        auto *open = W::button("Open data folder", "soft");
        open->setEnabled(!dataPath.isEmpty());
        addSetting(data, "Active data folder", dataPath.isEmpty() ? QString("No data directory configured.")
                                                               : QDir::toNativeSeparators(dataPath), open);
        connect(open, &QPushButton::clicked, this, [this, dataPath]
                {
                    if (!QFileInfo(dataPath).isDir() || !QDesktopServices::openUrl(QUrl::fromLocalFile(dataPath)))
                        emit toast("Could not open the data folder");
                });
        data->addStretch();
    }

    root->addWidget(tabs);
    auto *buttons = new QHBoxLayout;
    auto *status = W::label({}, "muted");
    auto *discard = W::button("Discard changes", "soft");
    auto *apply = W::button("Apply", "primary", "check");
    apply->setMinimumWidth(112);
    buttons->addWidget(status, 1);
    buttons->addWidget(discard);
    buttons->addWidget(apply);
    root->addLayout(buttons);
    root->addStretch();

    auto &appearanceManager = barcodehub::ui::DarkModeManager::instance();
    auto updateButtons = [=, &appearanceManager]
    {
        const auto &saved = AppState::instance().settings();
        const bool changed = low->value() != saved.lowStockThreshold || autoAdd->isChecked() != saved.autoAddScans ||
                             beep->isChecked() != saved.beepOnScan || payment->currentData().toInt() != int(saved.defaultPayment) ||
                             paper->currentData().toBool() != saved.receiptA4 ||
                             QDir::fromNativeSeparators(folder->text().trimmed()) != saved.receiptFolder ||
                             theme->currentData().toBool() != appearanceManager.isDarkMode();
        status->setText(changed ? "Unsaved changes" : "Preferences saved on this terminal");
        apply->setEnabled(changed);
        discard->setEnabled(changed);
    };
    auto reset = [=, &appearanceManager]
    {
        const auto &saved = AppState::instance().settings();
        low->setValue(saved.lowStockThreshold);
        autoAdd->setChecked(saved.autoAddScans);
        beep->setChecked(saved.beepOnScan);
        payment->setCurrentIndex(payment->findData(int(saved.defaultPayment)));
        paper->setCurrentIndex(paper->findData(saved.receiptA4));
        folder->setText(QDir::toNativeSeparators(saved.receiptFolder));
        theme->setCurrentIndex(theme->findData(appearanceManager.isDarkMode()));
        updateButtons();
    };

    for (auto *combo : {payment, paper, theme})
        connect(combo, &QComboBox::currentIndexChanged, this, updateButtons);
    connect(low, &QSpinBox::valueChanged, this, updateButtons);
    connect(autoAdd, &QCheckBox::toggled, this, updateButtons);
    connect(beep, &QCheckBox::toggled, this, updateButtons);
    connect(folder, &QLineEdit::textChanged, this, updateButtons);
    connect(&appearanceManager, &barcodehub::ui::DarkModeManager::themeChanged, this,
            [theme, updateButtons](bool dark)
            {
                theme->setCurrentIndex(theme->findData(dark));
                updateButtons();
            });
    connect(discard, &QPushButton::clicked, this, reset);
    connect(apply, &QPushButton::clicked, this, [=, this, &appearanceManager]
            {
                Settings settings = AppState::instance().settings();
                settings.lowStockThreshold = low->value();
                settings.autoAddScans = autoAdd->isChecked();
                settings.beepOnScan = beep->isChecked();
                settings.defaultPayment = static_cast<Payment>(payment->currentData().toInt());
                settings.receiptA4 = paper->currentData().toBool();
                settings.receiptFolder = QDir::fromNativeSeparators(folder->text().trimmed());
                if (!settings.receiptFolder.isEmpty())
                {
                    const QFileInfo selected(settings.receiptFolder);
                    if (!selected.isAbsolute() || !selected.isDir())
                    {
                        tabs->setCurrentWidget(receipts->parentWidget());
                        folder->setFocus();
                        emit toast("Choose an existing receipt export folder");
                        return;
                    }
                    settings.receiptFolder = QDir::cleanPath(settings.receiptFolder);
                }
                if (!settings.save())
                {
                    emit toast("Could not save terminal preferences");
                    return;
                }
                auto &app = AppState::instance();
                app.settings() = settings;
                const bool dark = theme->currentData().toBool();
                if (appearanceManager.isDarkMode() != dark)
                    appearanceManager.setDarkMode(dark);
                else
                    app.emitChanged();
                reset();
                emit toast("Settings applied");
            });
    reset();
}
