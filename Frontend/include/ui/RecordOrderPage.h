#pragma once

/**
 * @file RecordOrderPage.h
 * @brief Primary interactive POS cashier screen for recording orders and completing checkout.
 * @author Ali Abdullah (25L-3022)
 *
 * Implements the core Point of Sale GUI workflow for Increment 2:
 * - Dual-pane layout: Camera scanner & manual barcode entry on the left, shopping cart & totals on the right.
 * - Keyboard shortcuts: F9 (Camera Scan), F4 (Product Description), F10 (Checkout), F2 (Focus Barcode Input).
 * - Real-time stock validation and dynamic bill subtotal calculation.
 * - Integration with Dialogs::payment() and Dialogs::receipt().
 */

#include "AppState.h"
#include <QVector>
#include <QWidget>
#include <optional>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class CameraFeedWidget;

/**
 * @class RecordOrderPage
 * @brief Main cashier interface managing active order drafts, shopping cart items, and billing settlement.
 */
class RecordOrderPage : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief Constructs the RecordOrderPage and builds its user interface components.
     * @param parent Optional parent widget.
     */
    explicit RecordOrderPage(QWidget* parent = nullptr);

public slots:
    /** @brief Shifts keyboard focus to the barcode input field. */
    void focusInput();

    /** @brief Initiates camera barcode scanning or simulates a scan input (F9 shortcut). */
    void simulateScan();

    /** @brief Opens the Product Description modal for the currently highlighted or entered item (F4 shortcut). */
    void quickLookup();

    /** @brief Triggers order checkout and launches the payment settlement dialog (F10 shortcut). */
    void checkout();

    /**
     * @brief Processes a scanned or typed barcode string, looking up matching inventory.
     * @param code Decoded barcode string.
     * @param fromScanner True if emitted from camera scanner; false if typed manually.
     */
    void showCode(const QString& code, bool fromScanner);

    /** @brief Discards the current cart and resets the interface for a new customer transaction. */
    void newDraft();

signals:
    /** @brief Emits a transient user notification toast message on the main window. */
    void toast(const QString& message);

private:
    /**
     * @struct CartLine
     * @brief In-memory shopping cart item tracking product metadata and selected quantity.
     */
    struct CartLine
    {
        ProductInfo p;  ///< Cached snapshot of product details.
        int qty = 1;    ///< Quantity added to cart.
    };

    QWidget* buildCustomerStrip();
    QWidget* buildLeft();
    QWidget* buildRight();
    void setPreview(const std::optional<ProductInfo>& p);
    void addToCart(const ProductInfo& p, int qty = 1);
    void changeQty(int row, int delta);
    void removeRow(int row);
    void rebuildTable();
    void updateTotals();
    void loadDemoDraft();

    QVector<CartLine> m_cart;
    std::optional<ProductInfo> m_preview;

    QLineEdit* m_codeEdit = nullptr;
    QLineEdit* m_custName = nullptr;
    QLineEdit* m_custContact = nullptr;
    QLabel* m_pName = nullptr;
    QLabel* m_pAcronym = nullptr;
    QLabel* m_pPrice = nullptr;
    QLabel* m_pTax = nullptr;
    QLabel* m_pDiscount = nullptr;
    QLabel* m_pStock = nullptr;
    QLabel* m_pBarcodeImg = nullptr;
    QPushButton* m_descBtn = nullptr;
    QPushButton* m_addBtn = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_subtotalLabel = nullptr;
    QLabel* m_taxLabel = nullptr;
    QLabel* m_discountLabel = nullptr;
    QLabel* m_payableLabel = nullptr;
    CameraFeedWidget* m_camera = nullptr;
};
