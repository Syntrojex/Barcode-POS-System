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

class QWidget;

namespace Dialogs
{
    /**
     * @brief Launches the payment settlement dialog allowing the cashier to complete an order.
     *
     * Provides radio options for Cash, Card, and Wallet. Dynamically adjusts input fields
     * to accept tendered cash (calculating live change) or bank/merchant provider name.
     *
     * @param[in] parent Owning parent widget for modal centering.
     * @param[in] total Total bill amount payable in PKR.
     * @param[out] mode Selected payment method string ("Cash", "Card", "Wallet").
     * @param[out] tendered Physical currency amount entered if Cash mode was selected.
     * @param[out] merchant Bank or digital wallet identifier if Card/Wallet mode was selected.
     * @return True if customer confirmed payment; false if checkout was cancelled.
     */
    bool payment(QWidget *parent, double total, QString &mode, double &tendered, QString &merchant);

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
    /**
     * @brief Sign-in screen: the user types a name and presses Cashier or Manager.
     * @param[out] role Chosen role.
     * @param[out] name Typed name (the role name if left empty).
     * @return False if the window was closed without signing in.
     */
    bool login(QWidget *parent, UserRole &role, QString &name);
}
