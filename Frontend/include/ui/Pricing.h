#pragma once
// Seasonal + special discount arithmetic for the checkout screens.
//
// The Increment 2 backend stores one "discount per unit" for every bill line. The seasonal discount (active season
// from the Discounts page) and the special discount (cashier, at most 8 percent) are therefore *folded into that
// per-unit discount* before the order is handed to the backend, so Order.txt / Bill.txt totals always equal what the
// customer paid. The breakdown (item / seasonal / special) is kept on the OrderRecord for the receipt of this session.
#include "AppState.h"
#include <QDate>

namespace Pricing
{
    struct Quote
    {
        double gross = 0;        // sum of unit price * qty (no tax)
        double itemDiscount = 0; // product discounts from Product.txt
        double seasonal = 0;     // seasonal discount (active season, restricted to its categories)
        double special = 0;      // special discount (cashier typed percent, capped at 8)
        double tax = 0;
        double payable = 0;      // gross - itemDiscount - seasonal - special + tax
        double seasonalPercent = 0;
        double specialPercent = 0; // after capping
        QString seasonName;
        int units = 0, lines = 0;
        double totalDiscount() const { return itemDiscount + seasonal + special; }
    };

    // Cap a typed special-discount percent to 0..8 (UiStore::kSpecialDiscountCapPercent).
    double clampSpecial(double percent);

    // `lines` carry only the product's own discount per unit. Returns the quote for `on` (default today).
    Quote quote(const QVector<ReceiptLine> &lines, double specialPercent = 0, const QDate &on = QDate::currentDate());

    // Same lines with the seasonal and special amounts folded into each line's per-unit discount (2 decimals).
    QVector<ReceiptLine> fold(const QVector<ReceiptLine> &lines, double specialPercent = 0,
                              const QDate &on = QDate::currentDate());

    // Copy the quote's breakdown into an OrderRecord (subtotal/discount/tax/total + seasonal/special parts).
    void apply(OrderRecord &order, const Quote &q);
}
