#pragma once
#include "AppState.h"
#include <QPair>
#include <QTime>
#include <QVector>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QStackedWidget;
class CameraFeedWidget;

class ScannerPage : public QWidget
{
    Q_OBJECT
public:
    struct ScanEntry
    {
        ProductInfo product;
        int qty = 1;
        bool checked = true;
        QTime lastScanned;
    };

    explicit ScannerPage(QWidget *parent = nullptr);
    void setViewOnly(bool on); // hide checkout batch actions, keep Description

    // Exposed helpers for interactive use and automated UI tests.
    const QVector<ScanEntry> &entries() const { return m_entries; }
    void recordScan(const ProductInfo &p);
    void selectAll();
    void deselectAll();
    void clearHistory();
    void sendCheckedToOrder();
    void removeTransferredItems(const QVector<QPair<QString, int>> &items);

signals:
    void toast(const QString &message);
    void sendToOrder(const QString &ean);
    void sendBatchToOrder(const QVector<QPair<QString, int>> &items);
    void described(int productId);

private:
    void updateCameraDisplay(bool unavailable = false);
    bool show(const ProductInfo &p);
    void rebuildChecklist();
    void updateSelectionSummary();
    void changeEntryQty(int row, int delta);
    void removeEntry(int row);
    void setRowSelected(int row);

    CameraFeedWidget *m_feed = nullptr;
    QStackedWidget *m_cameraStack = nullptr;
    QLabel *m_cameraStatus = nullptr;
    QLabel *m_cameraMessage = nullptr;
    QTableWidget *m_checklist = nullptr;
    QLabel *m_countBadge = nullptr;
    QLabel *m_last = nullptr;
    QLabel *m_productDetails = nullptr;
    QLabel *m_selectedSummary = nullptr;
    QLabel *m_selectedTotal = nullptr;
    QPushButton *m_toggle = nullptr;
    QPushButton *m_selectAll = nullptr;
    QPushButton *m_deselectAll = nullptr;
    QPushButton *m_clearAll = nullptr;
    QPushButton *m_send = nullptr;
    QPushButton *m_describe = nullptr;
    QLineEdit *m_manual = nullptr;
    QWidget *m_selectionBar = nullptr;

    QVector<ScanEntry> m_entries;
    QString m_lastEan;
    int m_lastId = 0;
    bool m_viewOnly = false;
};
