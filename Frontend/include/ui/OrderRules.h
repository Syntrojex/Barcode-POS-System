#pragma once
// Order-editing rules (Increment 3, requirement 3). One place, no widgets (SRP):
//  - a PAID order may be edited within 7 days of its payment, and only after a manager approves it;
//  - an ON HOLD / IN PROGRESS order is simply resumed (no approval);
//  - an ARCHIVED order is never opened again.
// Front-end helper only: it works on the OrderRecord that AppState already loads from Order.txt.
#include "AppState.h"
#include <QDateTime>
#include <QString>

namespace OrderRules
{
    constexpr int kEditWindowDays = 7;

    // Moment the edit window closes (payment time + 7 days).
    inline QDateTime editDeadline(const OrderRecord &o) { return o.time.addDays(kEditWindowDays); }

    inline bool isPaid(const OrderRecord &o) { return o.status == "Paid"; }
    inline bool isResumable(const OrderRecord &o) { return o.status == "On Hold" || o.status == "In Progress"; }

    // Whole days still left to edit a paid order (rounded up: 6 days 3 h left -> 7). 0 = window closed.
    inline int editDaysLeft(const OrderRecord &o, const QDateTime &now = QDateTime::currentDateTime())
    {
        if (!isPaid(o))
            return 0;
        const qint64 secs = now.secsTo(editDeadline(o));
        if (secs <= 0)
            return 0;
        return int((secs + 86399) / 86400);
    }

    inline bool paidEditable(const OrderRecord &o, const QDateTime &now = QDateTime::currentDateTime())
    {
        return editDaysLeft(o, now) > 0;
    }

    // "Editable for 5 days" / "Editable for 1 day"
    inline QString editBadgeText(int daysLeft)
    {
        return QString("Editable for %1 day%2").arg(daysLeft).arg(daysLeft == 1 ? "" : "s");
    }

    // Badge colour: green while there is plenty of time, amber for the last three days (W::badge kinds).
    inline QString editBadgeKind(int daysLeft) { return daysLeft <= 3 ? "amber" : "green"; }

    // Whole days since the order was paid (for the "window closed" message).
    inline int daysSincePaid(const OrderRecord &o, const QDateTime &now = QDateTime::currentDateTime())
    {
        return int(o.time.daysTo(now));
    }
}
