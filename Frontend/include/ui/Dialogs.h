#pragma once

/**
 * @file Dialogs.h
 * @brief Modal dialog windows and receipt generation utilities for the POS GUI.
 * @author Ali Abdullah (25L-3022)
 *
 * Implements the interactive dialog workflows assigned for Increment 2:
 * - Payment selection modal (supporting Cash, Card, and Mobile Wallet).
 * - Receipt viewer modal (displaying finalized bill details and printable format).
 * - Product Description modal (displaying specifications, stock, and barcode graphic).
 * - Order details inspector.
 */

#include "AppState.h"
#include "UserRole.h"
#include <QString>
#include <QVector>

class QWidget;

namespace Dialogs
{
    /** What the payment dialog needs to know about the order being settled. */
    struct PaymentRequest
    {
        QString orderId;                // e.g. "26110122"
        QString customer, contact;
        QVector<ReceiptLine> lines;     // cart lines; per-unit discount = the product's own discount only (Pricing.h adds the rest)
    };

    /** What the cashier chose in the payment dialog. */
    struct PaymentResult
    {
        QString mode;              // "Cash" | "Card" | "Wallet"
        QString merchant;          // gateway / wallet name for Card and Wallet
        double tendered = 0;       // cash received (Cash only)
        double specialPercent = 0; // special discount typed in the dialog, 0..8 (Pricing::clampSpecial)
    };

    /**
     * @brief "Complete Order Payment": billing breakdown (item / seasonal / special discount, tax), tender mode,
     *        cash numpad with quick-tender keys, balance to return. Returns false when cancelled.
     */
    bool payment(QWidget *parent, const PaymentRequest &req, PaymentResult &res);

    /**
     * @brief "Request edit" (cashier): asks for the reason to send to a manager, who approves or rejects the
     *        request on the Orders page. Returns false when the cashier cancels.
     */
    bool requestEditReason(QWidget *parent, const OrderRecord &order, QString *reason);

    /**
     * @brief "Add New Product to Catalog" (existing == nullptr) or edit an existing product: category, specifications,
     *        EAN-8 barcode generator. Saves through AppState::addProduct / updateProduct. Returns true if saved.
     */
    bool editProduct(QWidget *parent, const ProductInfo *existing);

    /**
     * @brief Displays a clean printable-style receipt preview dialog for a finalized order.
     * @param parent Owning parent widget.
     * @param order Fully recorded order with items, customer details, and payment metadata.
     */
    void receipt(QWidget *parent, const OrderRecord &order);

    /**
     * @brief Displays detailed line items, timestamps, and customer metadata for an order.
     * @param parent Owning parent widget.
     * @param order Target OrderRecord to inspect.
     */
    void orderDetails(QWidget *parent, const OrderRecord &order);

    /**
     * @brief Displays the Product Description popup dialog.
     *
     * Fulfills Increment 2 requirement: Cashier can inspect comprehensive product
     * attributes (acronym, description, pricing, tax, discount, available inventory)
     * along with rendered barcode graphic.
     *
     * @param parent Owning parent widget.
     * @param productId Numeric identifier of the product to query.
     */
    void productDescription(QWidget *parent, int productId);

    /**
     * @brief Displays any syntax or data integrity issues detected during Product.txt parsing.
     * @param parent Owning parent widget.
     */
    void loadIssues(QWidget *parent);

    /**
     * @brief Generates formatted monospace text representation of an order's finalized bill receipt.
     *
     * Contains business header, itemized quantities, unit prices, discounts, taxes,
     * total payable, amount tendered, change returned, and payment method details.
     *
     * @param order Recorded order data.
     * @return Formatted multi-line receipt text.
     */
    QString receiptText(const OrderRecord &order);

    /** @brief The bill as formatted HTML (used on screen and for the PDF). */
    QString receiptHtml(const OrderRecord &order);

    /** @brief Saves the bill as a PDF file; returns false if it could not be written. */
    bool saveReceiptPdf(const OrderRecord &order, const QString &path);
    /**
     * @brief Sign-in screen: the user types a name and presses Cashier or Manager.
     * @param[out] role Chosen role.
     * @param[out] name Typed name (the role name if left empty).
     * @return False if the window was closed without signing in.
     */
    bool login(QWidget *parent, UserRole &role, QString &name);
}
