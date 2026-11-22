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
#include "UserRole.h"
#include <QVector>
#include <QWidget>
#include <optional>

class QHBoxLayout;
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
    /** @brief Shifts keyboard focus to the barcode input field (F2). */
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

    /** @brief Opens the Product Description for the product on screen (F4). */
    void describeCurrent();

    /** @brief Puts the current order on hold (parked in UiStore) and starts a fresh one (F2). One press, no dialog. */
    void holdOrder();

    /** @brief Resumes a parked (on-hold) order by its code, e.g. "HLD-108" (click on the parked strip). */
    void resumeParked(const QString& code);

    /**
     * @brief Opens an existing order in this screen for editing. Called after the manager approved the edit of a paid
     *        order (Orders page) or when resuming an in-progress order. The screen shows an "Editing order" banner.
     */
    void loadOrderForEdit(const QString& orderId);

    /** @brief Role of the signed-in user (Manager may also approve edits on the spot). */
    void setRole(UserRole role);

signals:
    /** @brief Emits a transient user notification toast message on the main window. */
    void toast(const QString& message);

    /** @brief A bill was just created for this order: the shell opens it on the Bills page ("New Order [Space]" there). */
    void billReady(const QString& orderId);

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
    QWidget* buildParkedStrip(); ///< "Parked (n)" chips of the orders on hold
    QWidget* buildLeft();
    QWidget* buildRight();
    void setPreview(const std::optional<ProductInfo>& p);
    void addToCart(const ProductInfo& p, int qty = 1);
    void changeQty(int row, int delta);
    void removeRow(int row);
    void rebuildTable();
    void updateTotals();
    void loadDemoDraft();
    void rebuildParked();        ///< redraws the on-hold chips (also keeps the "4m ago" texts fresh)
    QVector<ReceiptLine> cartLines() const;

    UserRole m_role = UserRole::Cashier;
    QString m_editingId; ///< order opened for editing from the Orders page; empty while recording a new order
    QVector<CartLine> m_cart;
    std::optional<ProductInfo> m_preview;
    int m_nextCustomLine = 1;

    QLineEdit* m_customer;
    QLineEdit* m_contact;
    QLineEdit* m_manual;
    QLabel* m_sessionId;

    CameraFeedWidget* m_feed;
    QPushButton* m_camToggle;
    QPushButton* m_addBtn;
    QPushButton* m_descBtn;
    QLabel* m_pName;
    QLabel* m_pPrice;
    QLabel* m_pMeta;
    QLabel* m_pStock;
    QLabel* m_pThumb;
    QLabel* m_pBadge;

    QTableWidget* m_table;
    QLabel* m_itemsBadge;
    QLabel* m_gross;
    QLabel* m_disc;
    QLabel* m_tax;
    QLabel* m_total;
    QPushButton* m_checkout;
    QPushButton* m_hold;
    QLabel* m_parkTitle;
    QLabel* m_parkEmpty;
    QHBoxLayout* m_parkChips;
};
