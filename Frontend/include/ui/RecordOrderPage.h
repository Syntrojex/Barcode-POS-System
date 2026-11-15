#pragma once
#include "AppState.h"
#include <QVector>
#include <QWidget>
#include <optional>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTimer;
class CameraFeedWidget;

// "Record Order" screen: scanner feed, manual EAN entry, product preview and the order draft.
class RecordOrderPage : public QWidget
{
    Q_OBJECT
public:
    explicit RecordOrderPage(QWidget *parent = nullptr);

public slots:
    void focusInput();
    void simulateScan();    // F9
    void quickLookup();     // F4
    void checkout();        // F10
    void showCode(const QString &code, bool fromScanner);
    void newDraft();

signals:
    void toast(const QString &message);

private:
    struct CartLine
    {
        Product p;
        int qty = 1;
        bool custom = false;
    };

    void buildUi();
    QWidget *buildCustomerStrip();
    QWidget *buildLeft();
    QWidget *buildRight();
    void setPreview(const std::optional<Product> &p);
    void addToCart(const Product &p, int qty = 1);
    void changeQty(int row, int delta);
    void removeRow(int row);
    int currentRow() const;
    void rebuildTable();
    void updateTotals();
    void updateNotice();
    void loadDemoDraft();

    QVector<CartLine> m_cart;
    std::optional<Product> m_preview;
    int m_scanIndex = 0;

    // customer strip
    QLineEdit *m_customer, *m_contact;
    QLabel *m_tier, *m_sessionId, *m_sessionTime;
    bool m_vip = true;
    // left
    CameraFeedWidget *m_feed;
    QLineEdit *m_manual;
    QPushButton *m_startCam, *m_stopCam;
    QLabel *m_pName, *m_pPrice, *m_pMeta, *m_pStock, *m_pSku, *m_pStockBadge, *m_pThumb;
    QPushButton *m_addBtn;
    QWidget *m_notice;
    QLabel *m_noticeText;
    QPushButton *m_noticeReorder;
    QString m_noticeSku;
    // right
    QTableWidget *m_table;
    QLabel *m_itemsBadge, *m_gross, *m_disc, *m_tax, *m_round, *m_total, *m_tender;
    QPushButton *m_checkout;
};
