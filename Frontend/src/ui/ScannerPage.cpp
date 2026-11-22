#include "ScannerPage.h"
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


// =================================================================== ScannerPage
ScannerPage::ScannerPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root;
    pageShell(this, root);
    auto* h = new QHBoxLayout;
    h->setSpacing(12);

    auto* cam = W::card(this);
    auto* cv = new QVBoxLayout(cam);
    cv->setContentsMargins(18, 14, 18, 14);
    cv->setSpacing(10);
    W::headerRow(cv, "barcode", "Barcode Scanner");
    m_feed = new CameraFeedWidget;
    m_feed->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_feed->setMinimumHeight(300);
    cv->addWidget(m_feed, 1);
    auto* btns = new QHBoxLayout;
    m_toggle = W::button("Start camera", "primary", "play");
    btns->addWidget(m_toggle);
    btns->addStretch();
    cv->addLayout(btns);
    auto* mr = new QHBoxLayout;
    m_manual = new QLineEdit;
    m_manual->setPlaceholderText("Or type an 8-digit EAN and press Enter");
    m_manual->setMaxLength(12);
    auto* go = W::button("Lookup", "soft", "search");
    mr->addWidget(m_manual, 1);
    mr->addWidget(go);
    cv->addLayout(mr);
    h->addWidget(cam, 6);

    auto* side = W::card(this);
    auto* sv = new QVBoxLayout(side);
    sv->setContentsMargins(18, 14, 18, 14);
    sv->setSpacing(10);
    W::headerRow(sv, "clock", "Recent Scans");
    m_last = W::label("No scans yet", "h2");
    sv->addWidget(m_last);
    m_history = new QListWidget;
    m_history->setStyleSheet("QListWidget{border:1px solid #dbe4f7;border-radius:10px;padding:4px;} QListWidget::item{padding:9px;border-bottom:1px solid #e8eefb;}");
    sv->addWidget(m_history, 1);
    m_describe = W::button("Show description", "soft", "search");
    m_describe->setEnabled(false);
    sv->addWidget(m_describe);
    m_send = W::button("Add last scan to order", "primary", "cart");
    m_send->setEnabled(false);
    sv->addWidget(m_send);
    h->addWidget(side, 4);
    root->addLayout(h, 1);

    connect(m_toggle, &QPushButton::clicked, this, [this]
        {
            const bool on = !m_feed->running();
            m_feed->setRunning(on);
            const bool running = m_feed->running();
            m_toggle->setText(running ? "Stop camera" : "Start camera");
            m_toggle->setIcon(Theme::icon(running ? "stop" : "play", QColor("white"), 16)); });
    connect(m_feed, &CameraFeedWidget::scanError, this, &ScannerPage::toast);
    connect(m_feed, &CameraFeedWidget::barcodeDetected, this, [this](const QString& code) {
        const auto product = AppState::instance().findByCode(code);
        if (product) { m_feed->showResult(code + "  -  " + product->name, true); show(*product); }
        else { m_feed->showResult(code + "  -  not in Product.txt", false); emit toast("Barcode " + code + " not found in Product.txt"); }
        });
    auto lookup = [this]
        {
            const auto p = AppState::instance().findByCode(m_manual->text());
            if (!p)
            {
                emit toast("No product found for \"" + m_manual->text() + "\"");
                return;
            }
            show(*p);
            m_manual->clear();
        };
    connect(go, &QPushButton::clicked, this, lookup);
    connect(m_manual, &QLineEdit::returnPressed, this, lookup);
    connect(m_send, &QPushButton::clicked, this, [this]
        { emit sendToOrder(m_lastEan); });
    connect(m_describe, &QPushButton::clicked, this, [this]
        { Dialogs::productDescription(this, m_lastId); });
}

void ScannerPage::scanNext()
{
    if (!m_feed->running()) m_feed->setRunning(true);
    if (!m_feed->running()) emit toast("Unable to start camera scanning");
    else {
        m_toggle->setText("Stop camera");
        m_toggle->setIcon(Theme::icon("stop", QColor("white"), 16));
    }
}

void ScannerPage::show(const ProductInfo& p)
{
    m_feed->setCode(p.ean);
    m_lastEan = p.ean;
    m_lastId = p.id;
    m_last->setText(p.name + " • " + W::money(p.price));
    m_history->insertItem(0, QString("%1   %2   %3").arg(QTime::currentTime().toString("HH:mm:ss"), p.ean, p.name));
    m_send->setEnabled(true);
    m_describe->setEnabled(true);
    // Show description only on request; opening a modal on every camera frame is disruptive.
}
void ScannerPage::setViewOnly(bool on)
{
    m_send->setVisible(!on);
}
