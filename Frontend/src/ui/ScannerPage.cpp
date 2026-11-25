#include "ScannerPage.h"
#include "Dialogs.h"
#include "Pricing.h"
#include "ScanAudio.h"
#include "Theme.h"
#include "UiKit.h"
#include "UiStore.h"
#include "Widgets.h"
#include <QAbstractItemView>
#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTime>
#include <QVBoxLayout>
#include <algorithm>

using namespace Ui; // makeTable, item, wrapW, pageShell, writeCsv (see UiKit.h)

namespace
{
    class ElidedLineLabel final : public QLabel
    {
    public:
        explicit ElidedLineLabel(const QString &text, const char *objectName = nullptr, const QString &style = QString())
        {
            setTextFormat(Qt::PlainText);
            setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
            if (objectName)
                setObjectName(objectName);
            if (!style.isEmpty())
                setStyleSheet(style);
            m_full = text;
            setAccessibleName(text);
            updateElided();
        }

    protected:
        void resizeEvent(QResizeEvent *event) override
        {
            QLabel::resizeEvent(event);
            updateElided();
        }

    private:
        void updateElided()
        {
            const QString elided = fontMetrics().elidedText(m_full, Qt::ElideRight, contentsRect().width());
            setText(elided);
            setToolTip(elided == m_full ? QString() : m_full);
        }

        QString m_full;
    };

    double unitGrossWithTax(const ProductInfo &p)
    {
        return p.price - p.discount + p.tax;
    }

    QWidget *makeChecklistProductCell(const ScannerPage::ScanEntry &entry)
    {
        auto *w = new QWidget;
        auto *v = new QVBoxLayout(w);
        v->setContentsMargins(2, 4, 2, 4);
        v->setSpacing(1);

        auto *top = new ElidedLineLabel(entry.product.name, nullptr, QStringLiteral("font-weight:700; font-size:13px;"));
        const QString subtitle = entry.product.stock <= 0
            ? QString("%1  •  %2  •  Out of stock").arg(entry.product.ean, W::money(entry.product.price))
            : QString("%1  •  %2  •  Line %3")
                  .arg(entry.product.ean,
                       W::money(entry.product.price),
                       W::money(unitGrossWithTax(entry.product) * entry.qty));
        auto *bottom = new ElidedLineLabel(
            subtitle,
            entry.product.stock <= 0 ? "errorText" : "muted",
            QStringLiteral("font-size:11.5px;"));
        v->addWidget(top);
        v->addWidget(bottom);
        return w;
    }
}

// =================================================================== ScannerPage
ScannerPage::ScannerPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *root;
    pageShell(this, root);
    root->setSpacing(8);
    root->addWidget(sectionHeader({}, "Barcode Scanner", "Scan EAN-8 products using the camera or manual lookup."));
    auto *h = new QHBoxLayout;
    h->setSpacing(8);

    auto *cam = W::card(this, "subCard");
    auto *cv = new QVBoxLayout(cam);
    cv->setContentsMargins(16, 16, 16, 16);
    cv->setSpacing(8);
    m_cameraStatus = W::badge("Camera off", "grey");
    W::headerRow(cv, "camera", "Barcode Camera", m_cameraStatus);
    auto *preview = W::card(cam, "field");
    auto *pv = new QVBoxLayout(preview);
    pv->setContentsMargins(8, 8, 8, 8);
    pv->setSpacing(8);
    m_cameraStack = new QStackedWidget;
    auto *idle = new QWidget;
    auto *iv = new QVBoxLayout(idle);
    iv->setAlignment(Qt::AlignCenter);
    iv->addWidget(W::iconLabel("barcode", Theme::primary, 64), 0, Qt::AlignHCenter);
    m_cameraMessage = W::label("Camera off", "h2");
    m_cameraMessage->setAlignment(Qt::AlignCenter);
    iv->addWidget(m_cameraMessage);
    m_cameraStack->addWidget(idle);
    m_feed = new CameraFeedWidget;
    m_feed->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_feed->setMinimumHeight(176);
    m_cameraStack->addWidget(m_feed);
    pv->addWidget(m_cameraStack, 1);
    m_toggle = W::button("Start camera", "primary", "play");
    m_toggle->setMinimumWidth(152);
    pv->addWidget(m_toggle, 0, Qt::AlignHCenter);
    cv->addWidget(preview, 1);
    cv->addWidget(W::label("MANUAL BARCODE ENTRY", "caption"));
    auto *mr = new QHBoxLayout;
    m_manual = new QLineEdit;
    m_manual->setObjectName("scannerManualInput");
    m_manual->setPlaceholderText("Enter an 8-digit EAN");
    m_manual->setAccessibleName("Manual barcode entry");
    m_manual->setClearButtonEnabled(true);
    m_manual->setMaxLength(12);
    auto *go = W::button("Lookup", "soft", "search");
    mr->addWidget(m_manual, 1);
    mr->addWidget(go);
    cv->addLayout(mr);
    cv->addWidget(W::label("Manual lookup is available while the camera is off.", "muted"));
    h->addWidget(cam, 5);

    auto *side = W::card(this, "subCard");
    auto *sv = new QVBoxLayout(side);
    sv->setContentsMargins(16, 16, 16, 16);
    sv->setSpacing(8);
    m_countBadge = W::badge("0 items", "grey");
    W::headerRow(sv, "clock", "Recent scans", m_countBadge);

    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(6);
    m_selectAll = W::button("Select all", "mini");
    m_selectAll->setObjectName("scannerSelectAll");
    m_deselectAll = W::button("Unselect", "mini");
    m_deselectAll->setObjectName("scannerDeselectAll");
    m_clearAll = W::button("Clear", "flat", "trash");
    m_clearAll->setObjectName("scannerClearAll");
    toolbar->addWidget(m_selectAll);
    toolbar->addWidget(m_deselectAll);
    toolbar->addStretch();
    toolbar->addWidget(m_clearAll);
    sv->addLayout(toolbar);

    m_checklist = makeTable({"", "PRODUCT", "QTY", ""});
    m_checklist->setObjectName("scannerChecklist");
    m_checklist->setAccessibleName("Scanned items checklist");
    m_checklist->setProperty("managementTable", true);
    m_checklist->setAlternatingRowColors(true);
    m_checklist->setSelectionMode(QAbstractItemView::SingleSelection);
    m_checklist->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_checklist->setWordWrap(false);
    auto *hh = m_checklist->horizontalHeader();
    hh->setSectionResizeMode(QHeaderView::Fixed);
    hh->setSectionResizeMode(1, QHeaderView::Stretch);
    m_checklist->setColumnWidth(0, 42);
    m_checklist->setColumnWidth(2, 116);
    m_checklist->setColumnWidth(3, 38);
    sv->addWidget(m_checklist, 1);

    auto *product = W::card(side, "field");
    auto *details = new QVBoxLayout(product);
    details->setContentsMargins(10, 8, 10, 8);
    details->setSpacing(3);
    details->addWidget(W::label("SELECTED PRODUCT", "caption"));
    m_last = W::label("No scans yet", "h2");
    m_last->setWordWrap(true);
    details->addWidget(m_last);
    m_productDetails = W::label("", "muted");
    m_productDetails->setWordWrap(true);
    details->addWidget(m_productDetails);
    sv->addWidget(product);

    auto *summaryBox = new QFrame;
    summaryBox->setObjectName("field");
    auto *sbLayout = new QHBoxLayout(summaryBox);
    sbLayout->setContentsMargins(10, 7, 10, 7);
    sbLayout->setSpacing(8);
    m_selectedSummary = W::label("0 selected", "muted");
    m_selectedTotal = W::label("PKR 0.00", "h2");
    sbLayout->addWidget(m_selectedSummary, 1);
    sbLayout->addWidget(m_selectedTotal, 0, Qt::AlignRight);
    m_selectionBar = summaryBox;
    sv->addWidget(summaryBox);

    m_describe = W::button("Show description", "soft", "search");
    m_describe->setEnabled(false);
    sv->addWidget(m_describe);

    m_send = W::button("Add checked to checkout", "primary", "cart");
    m_send->setObjectName("scannerAddChecked");
    m_send->setEnabled(false);
    sv->addWidget(m_send);

    h->addWidget(side, 5);
    root->addLayout(h, 1);

    connect(m_toggle, &QPushButton::clicked, this, [this]
        {
            const bool starting = !m_feed->running();
            m_feed->setRunning(starting);
            if (!starting || m_feed->running())
                updateCameraDisplay();
        });
    connect(m_feed, &CameraFeedWidget::scanError, this, [this](const QString &message)
        {
            updateCameraDisplay(true);
            emit toast(message);
        });
    connect(m_feed, &CameraFeedWidget::barcodeDetected, this, [this](const QString &code)
        {
            const auto product = AppState::instance().findByCode(code);
            if (product)
            {
                const bool added = show(*product);
                m_feed->showResult(code + "  -  " + product->name, added);
                if (added)
                    ScanAudio::instance().playSuccess();
            }
            else
            {
                m_feed->showResult(code + "  -  not in Product.txt", false);
                emit toast("Barcode " + code + " not found in Product.txt");
            }
        });

    auto lookup = [this]
        {
            const QString query = m_manual->text().trimmed();
            if (query.isEmpty())
            {
                emit toast("Enter a barcode first");
                return;
            }
            const auto p = AppState::instance().findByCode(query);
            if (!p)
            {
                emit toast("No product found for \"" + query + "\"");
                return;
            }
            if (show(*p))
                ScanAudio::instance().playSuccess();
            m_manual->clear();
        };
    connect(go, &QPushButton::clicked, this, lookup);
    connect(m_manual, &QLineEdit::returnPressed, this, lookup);

    connect(m_selectAll, &QPushButton::clicked, this, &ScannerPage::selectAll);
    connect(m_deselectAll, &QPushButton::clicked, this, &ScannerPage::deselectAll);
    connect(m_clearAll, &QPushButton::clicked, this, &ScannerPage::clearHistory);
    connect(m_send, &QPushButton::clicked, this, &ScannerPage::sendCheckedToOrder);
    connect(m_describe, &QPushButton::clicked, this, [this]
        {
            if (m_lastId > 0)
            {
                Dialogs::productDescription(this, m_lastId);
                emit described(m_lastId);
            }
        });
    connect(m_checklist, &QTableWidget::currentCellChanged, this,
        [this](int currentRow, int, int, int) { setRowSelected(currentRow); });
    connect(&AppState::instance(), &AppState::changed, this, [this]
        {
            for (auto &e : m_entries)
            {
                if (const auto latest = AppState::instance().findByCode(e.product.ean))
                {
                    e.product = *latest;
                    if (e.product.stock <= 0)
                        e.checked = false;
                }
                else
                {
                    e.product.stock = 0;
                    e.checked = false;
                }
            }
            rebuildChecklist();
        });
    connect(&UiStore::instance(), &UiStore::seasonsChanged, this, &ScannerPage::updateSelectionSummary);

    rebuildChecklist();
}

void ScannerPage::updateCameraDisplay(bool unavailable)
{
    const bool running = m_feed->running() && !unavailable;
    const QString state = unavailable ? "Camera unavailable" : running ? "Camera started" : "Camera off";
    W::setBadge(m_cameraStatus, state, unavailable ? "amber" : running ? "blue" : "grey");
    m_cameraMessage->setText(state);
    m_cameraStack->setCurrentIndex(running ? 1 : 0);
    m_toggle->setText(running ? "Stop camera" : "Start camera");
    m_toggle->setIcon(Theme::icon(running ? "stop" : "play", Qt::white, 16));
}

void ScannerPage::recordScan(const ProductInfo &p)
{
    show(p);
}

bool ScannerPage::show(const ProductInfo &p)
{
    m_feed->setCode(p.ean);
    m_lastEan = p.ean;
    m_lastId = p.id;

    int existingIndex = -1;
    for (int i = 0; i < m_entries.size(); ++i)
    {
        if (m_entries[i].product.id == p.id && m_entries[i].product.ean == p.ean)
        {
            existingIndex = i;
            break;
        }
    }

    bool added = p.stock > 0;
    if (existingIndex >= 0)
    {
        ScanEntry updated = m_entries.takeAt(existingIndex);
        updated.product = p;
        updated.lastScanned = QTime::currentTime();
        if (p.stock > 0)
        {
            if (updated.qty < p.stock)
                ++updated.qty;
            else
            {
                added = false;
                emit toast(QString("Maximum stock (%1) reached for %2").arg(p.stock).arg(p.name));
            }
            updated.checked = true;
        }
        else
        {
            updated.checked = false;
        }
        m_entries.prepend(updated);
    }
    else
    {
        ScanEntry entry;
        entry.product = p;
        entry.qty = 1;
        entry.checked = (p.stock > 0);
        entry.lastScanned = QTime::currentTime();
        m_entries.prepend(entry);
    }

    rebuildChecklist();
    m_checklist->selectRow(0);
    setRowSelected(0);
    // Show description only on request; opening a modal on every camera frame is disruptive.
    return added;
}

void ScannerPage::rebuildChecklist()
{
    m_checklist->setRowCount(0);
    m_checklist->setRowCount(m_entries.size());

    for (int r = 0; r < m_entries.size(); ++r)
    {
        const auto &e = m_entries[r];
        m_checklist->setRowHeight(r, 52);

        auto *cb = new QCheckBox;
        cb->setStyleSheet("QCheckBox { spacing: 0px; padding: 0px; }");
        cb->setFixedSize(20, 20);
        cb->setAccessibleName("Select " + e.product.name);
        cb->setChecked(e.checked && e.product.stock > 0);
        cb->setEnabled(e.product.stock > 0);
        if (e.product.stock <= 0)
            cb->setToolTip("Out of stock — cannot be added to checkout");
        connect(cb, &QCheckBox::toggled, this, [this, r](bool on)
            {
                if (r >= 0 && r < m_entries.size())
                {
                    m_entries[r].checked = on && (m_entries[r].product.stock > 0);
                    updateSelectionSummary();
                }
            });
        m_checklist->setCellWidget(r, 0, wrapW(cb, Qt::AlignCenter));
        m_checklist->setCellWidget(r, 1, makeChecklistProductCell(e));

        auto *stp = new QtyStepper(e.qty);
        const auto buttons = stp->findChildren<QPushButton *>();
        for (auto *button : buttons)
        {
            const bool isPlus = (button->text() == "+");
            const QString action = isPlus ? "Increase quantity" : "Decrease quantity";
            button->setObjectName(action);
            button->setAccessibleName(action + " for " + e.product.name);
            if (isPlus)
                button->setEnabled(e.product.stock > 0 && e.qty < e.product.stock);
            else
                button->setEnabled(e.qty > 1);
        }
        connect(stp, &QtyStepper::minus, this, [this, r] { changeEntryQty(r, -1); });
        connect(stp, &QtyStepper::plus, this, [this, r] { changeEntryQty(r, +1); });
        m_checklist->setCellWidget(r, 2, stp);

        auto *del = W::button("", "flat", "trash");
        del->setToolTip("Remove from list");
        del->setAccessibleName("Remove " + e.product.name);
        connect(del, &QPushButton::clicked, this, [this, r] { removeEntry(r); });
        m_checklist->setCellWidget(r, 3, wrapW(del, Qt::AlignCenter));
    }

    if (m_entries.isEmpty())
    {
        m_lastEan.clear();
        m_lastId = 0;
        m_last->setText("No scans yet");
        m_productDetails->clear();
        m_describe->setEnabled(false);
    }
    else
    {
        m_describe->setEnabled(m_lastId > 0);
    }

    updateSelectionSummary();
}

void ScannerPage::updateSelectionSummary()
{
    int totalItems = m_entries.size();
    int totalUnits = 0;
    int checkableItems = 0;
    int checkedItems = 0;
    int checkedUnits = 0;
    QVector<ReceiptLine> checkedLines;

    for (const auto &e : m_entries)
    {
        totalUnits += e.qty;
        if (e.product.stock > 0)
            ++checkableItems;
        if (e.checked && e.product.stock > 0 && e.qty > 0)
        {
            ++checkedItems;
            checkedUnits += e.qty;
            const auto &p = e.product;
            checkedLines.push_back({p.name, p.sku, e.qty, p.price, p.tax, p.discount, p.id});
        }
    }

    if (totalItems == 0)
        W::setBadge(m_countBadge, "0 items", "grey");
    else
        W::setBadge(m_countBadge,
                    QString("%1 product%2 · %3 unit%4")
                        .arg(totalItems)
                        .arg(totalItems == 1 ? "" : "s")
                        .arg(totalUnits)
                        .arg(totalUnits == 1 ? "" : "s"),
                    "blue");

    m_selectAll->setEnabled(checkedItems < checkableItems);
    m_deselectAll->setEnabled(checkedItems > 0);
    m_clearAll->setEnabled(!m_entries.isEmpty());

    m_selectedSummary->setText(checkedItems == 0
        ? QString("No items selected")
        : QString("Checked: %1 product%2 (%3 unit%4)")
              .arg(checkedItems)
              .arg(checkedItems == 1 ? "" : "s")
              .arg(checkedUnits)
              .arg(checkedUnits == 1 ? "" : "s"));
    m_selectedTotal->setText(W::money(Pricing::quote(checkedLines).payable));

    m_send->setEnabled(!m_viewOnly && checkedUnits > 0);
    m_send->setText(checkedUnits > 0
        ? QString("Add checked to checkout (%1)").arg(checkedUnits)
        : QString("Add checked to checkout"));
}

void ScannerPage::changeEntryQty(int row, int delta)
{
    if (row < 0 || row >= m_entries.size())
        return;

    auto &entry = m_entries[row];
    const int nextQty = entry.qty + delta;
    if (nextQty < 1)
        return;
    if (delta > 0 && entry.product.stock > 0 && nextQty > entry.product.stock)
    {
        emit toast(QString("Only %1 in stock for %2").arg(entry.product.stock).arg(entry.product.name));
        return;
    }

    entry.qty = nextQty;
    if (delta > 0 && entry.product.stock > 0)
        entry.checked = true;
    rebuildChecklist();
    m_checklist->selectRow(row);
}

void ScannerPage::removeEntry(int row)
{
    if (row < 0 || row >= m_entries.size())
        return;
    m_entries.removeAt(row);
    rebuildChecklist();
    if (!m_entries.isEmpty())
    {
        const int nextRow = std::min(row, static_cast<int>(m_entries.size()) - 1);
        m_checklist->selectRow(nextRow);
        setRowSelected(nextRow);
    }
}

void ScannerPage::setRowSelected(int row)
{
    if (row < 0 || row >= m_entries.size())
        return;
    const auto &p = m_entries[row].product;
    m_lastEan = p.ean;
    m_lastId = p.id;
    m_last->setText(p.name);
    m_productDetails->setText(QString("%1  ·  %2  ·  %3 in stock")
                                  .arg(p.ean, W::money(p.price)).arg(p.stock));
    m_describe->setEnabled(p.id > 0);
}

void ScannerPage::selectAll()
{
    for (auto &e : m_entries)
        e.checked = (e.product.stock > 0);
    rebuildChecklist();
}

void ScannerPage::deselectAll()
{
    for (auto &e : m_entries)
        e.checked = false;
    rebuildChecklist();
}

void ScannerPage::clearHistory()
{
    m_entries.clear();
    rebuildChecklist();
}

void ScannerPage::sendCheckedToOrder()
{
    if (m_viewOnly)
        return;
    QVector<QPair<QString, int>> batch;

    for (const auto &e : m_entries)
    {
        if (e.checked && e.qty > 0 && e.product.stock > 0)
            batch.push_back({e.product.ean, e.qty});
    }

    if (batch.isEmpty())
    {
        emit toast("Select at least one in-stock product to add to checkout");
        return;
    }

    emit sendBatchToOrder(batch);
}

void ScannerPage::removeTransferredItems(const QVector<QPair<QString, int>> &items)
{
    for (const auto &item : items)
    {
        const auto entry = std::find_if(m_entries.begin(), m_entries.end(), [&item](const ScanEntry &candidate)
            { return candidate.product.ean == item.first; });
        if (entry == m_entries.end())
            continue;
        entry->qty -= item.second;
        if (entry->qty <= 0)
            m_entries.erase(entry);
    }
    rebuildChecklist();
    if (!m_entries.isEmpty())
        setRowSelected(0);
}

void ScannerPage::setViewOnly(bool on)
{
    m_viewOnly = on;
    m_send->setVisible(!on);
    m_selectionBar->setVisible(!on);
    m_selectAll->setVisible(!on);
    m_deselectAll->setVisible(!on);
    m_checklist->setColumnHidden(0, on);
    updateSelectionSummary();
}
