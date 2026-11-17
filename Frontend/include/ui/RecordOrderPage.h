#pragma once
#include "AppState.h"
#include <QVector>
#include <QWidget>
#include <optional>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class CameraFeedWidget;

// "Record Order": scanner + manual entry on the left, the order draft and totals on the right.
class RecordOrderPage : public QWidget
{
    Q_OBJECT
public:
    explicit RecordOrderPage(QWidget *parent = nullptr);

public slots:
    void focusInput();
    void simulateScan(); // F9
    void quickLookup();  // F4
    void checkout();     // F10
    void showCode(const QString &code, bool fromScanner);
    void newDraft();

signals:
    void toast(const QString &message);

private:
    struct CartLine
    {
        ProductInfo p;
        int qty = 1;
    };

    QWidget *buildCustomerStrip();
    QWidget *buildLeft();
    QWidget *buildRight();
    void setPreview(const std::optional<ProductInfo> &p);
    void addToCart(const ProductInfo &p, int qty = 1);
    void changeQty(int row, int delta);
    void removeRow(int row);
    void rebuildTable();
    void updateTotals();
    void loadDemoDraft();

    QVector<CartLine> m_cart;
    std::optional<ProductInfo> m_preview;
    int m_scanIndex = 0;
    bool m_vip = true;

    QLineEdit *m_customer, *m_contact, *m_manual;
    QLabel *m_sessionId;
    QPushButton *m_vipBtn;

    CameraFeedWidget *m_feed;
    QPushButton *m_camToggle, *m_addBtn, *m_descBtn;
    QLabel *m_pName, *m_pPrice, *m_pMeta, *m_pStock, *m_pThumb, *m_pBadge;

    QTableWidget *m_table;
    QLabel *m_itemsBadge, *m_gross, *m_disc, *m_tax, *m_total;
    QPushButton *m_checkout;
};
