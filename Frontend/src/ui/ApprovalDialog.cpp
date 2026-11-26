// "Request edit" (cashier): asks a manager for permission to edit a paid order (requirement 3).
// The request is saved by UiStore::requestEdit; a manager approves or rejects it on the Orders page.
#include "Dialogs.h"
#include "Theme.h"
#include "UiKit.h"
#include "UiStore.h"
#include "Widgets.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace Dialogs
{
    bool requestEditReason(QWidget *parent, const OrderRecord &order, QString *reason)
    {
        Ui::Modal d(parent, "unlock", "Request edit", "A manager has to approve before a paid order can be changed.",
                    QString(), 500);

        auto *summary = W::card(nullptr, "subCard");
        auto *sv = new QVBoxLayout(summary);
        sv->setContentsMargins(14, 10, 14, 10);
        sv->setSpacing(2);
        auto *title = W::label(QString("Order #%1  ·  %2").arg(order.id, W::money(order.total)), "h2");
        sv->addWidget(title);
        sv->addWidget(W::label((order.customer.isEmpty() ? QString("Walk-in Customer") : order.customer) + "  ·  " +
                                   order.paymentTime().toString("d MMM yyyy, h:mm AP"),
                               "muted"));
        d.body()->addWidget(summary);

        d.body()->addSpacing(4);
        d.body()->addWidget(W::label("Reason", "caption"));
        auto *why = new QComboBox;
        why->setEditable(true);
        why->setMinimumHeight(40);
        why->addItems({"Wrong quantity entered", "Wrong item scanned", "Price or discount correction",
                       "Customer returned an item", "Customer request"});
        d.body()->addWidget(why);

        auto *cancel = W::button("Cancel", "flat");
        auto *send = W::button("Send to manager", "primary", "check");
        send->setMinimumHeight(40);
        send->setDefault(true);
        d.footer()->addStretch();
        d.footer()->addWidget(cancel);
        d.footer()->addWidget(send);
        QObject::connect(cancel, &QPushButton::clicked, &d, &QDialog::reject);
        QObject::connect(send, &QPushButton::clicked, &d, &QDialog::accept);

        why->setFocus();
        if (d.exec() != QDialog::Accepted)
            return false;
        if (reason)
            *reason = why->currentText().trimmed();
        return true;
    }
}
