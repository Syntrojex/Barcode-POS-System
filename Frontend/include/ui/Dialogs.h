#pragma once
#include "AppState.h"
#include <QString>

class QWidget;

namespace Dialogs
{
    // Payment chooser (Cash / Card / Wallet). Returns false when cancelled.
    bool payment(QWidget *parent, double total, QString &mode, double &tendered);
    // Printable-style bill for an order.
    void receipt(QWidget *parent, const OrderRecord &order);
    // Order summary with line items.
    void orderDetails(QWidget *parent, const OrderRecord &order);
    // Text receipt body (also used for the bill preview).
    QString receiptText(const OrderRecord &order);
}
