#include "Dialogs.h"
#include "Theme.h"
#include "Widgets.h"
#include <QButtonGroup>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>

namespace Dialogs
{
    QString receiptText(const OrderRecord &o)
    {
        const int w = 36;
        auto center = [&](const QString &s)
        { return QString((w - s.size()) / 2, ' ') + s; };
        auto two = [&](const QString &l, const QString &r)
        { return l + QString(qMax(1, w - l.size() - r.size()), ' ') + r; };
        const QString rule(w, '-');
        QString t;
        t += center("SMARTPOS RETAIL") + "\n" + center("Store #102 - Register 02") + "\n" + rule + "\n";
        t += two("Order", o.id) + "\n";
        t += two("Date", o.time.toString("yyyy-MM-dd HH:mm")) + "\n";
        t += two("Cashier", "Alex Rivera") + "\n";
        t += two("Customer", o.customer) + "\n" + rule + "\n";
        double gross = 0;
        for (const auto &l : o.lines)
        {
            const double sub = l.price * l.qty;
            gross += sub;
            QString name = l.name;
            if (name.size() > w - 12)
                name = name.left(w - 13) + "…";
            t += two(QString("%1 x %2").arg(l.qty).arg(name), W::money(sub)) + "\n";
        }
        t += rule + "\n";
        t += two("Gross subtotal", W::money(gross)) + "\n";
        if (o.discount > 0)
            t += two("Loyalty discount", W::money(-o.discount)) + "\n";
        t += two("GST included", W::money(o.total * 8.0 / 108.0)) + "\n";
        t += two("TOTAL", W::money(o.total)) + "\n" + rule + "\n";
        t += two("Payment", o.payment) + "\n";
        if (o.tendered > 0)
        {
            t += two("Tendered", W::money(o.tendered)) + "\n";
            t += two("Change", W::money(o.tendered - o.total)) + "\n";
        }
        t += "\n" + center("Thank you for shopping!") + "\n";
        return t;
    }

    void receipt(QWidget *parent, const OrderRecord &o)
    {
        QDialog d(parent);
        d.setWindowTitle("Bill - " + o.id);
        auto *v = new QVBoxLayout(&d);
        v->setContentsMargins(18, 18, 18, 14);
        auto *te = new QPlainTextEdit(receiptText(o));
        te->setReadOnly(true);
        QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        f.setPointSize(10);
        te->setFont(f);
        te->setStyleSheet("QPlainTextEdit{background:white;border:1px solid #e2e7f0;border-radius:10px;padding:14px;}");
        te->setMinimumSize(420, 440);
        v->addWidget(te);
        auto *h = new QHBoxLayout;
        h->addStretch();
        auto *close = W::button("Close", "primary");
        h->addWidget(close);
        v->addLayout(h);
        QObject::connect(close, &QPushButton::clicked, &d, &QDialog::accept);
        d.exec();
    }

    void orderDetails(QWidget *parent, const OrderRecord &o)
    {
        QDialog d(parent);
        d.setWindowTitle("Order " + o.id);
        auto *v = new QVBoxLayout(&d);
        v->setContentsMargins(20, 18, 20, 16);
        v->setSpacing(10);
        auto *top = new QHBoxLayout;
        top->addWidget(W::label(o.id, "h1"));
        top->addWidget(W::badge(o.status, W::statusBadgeKind(o.status)));
        top->addStretch();
        v->addLayout(top);
        v->addWidget(W::label(QString("%1  •  %2  •  %3")
                                  .arg(o.time.toString("yyyy-MM-dd HH:mm"), o.customer, o.shift),
                              "muted"));
        auto *t = new QTableWidget(o.lines.size(), 4);
        t->setHorizontalHeaderLabels({"ITEM", "QTY", "PRICE", "SUBTOTAL"});
        t->verticalHeader()->hide();
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setSelectionMode(QAbstractItemView::NoSelection);
        t->setShowGrid(false);
        t->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        for (int i = 0; i < o.lines.size(); ++i)
        {
            const auto &l = o.lines[i];
            t->setItem(i, 0, new QTableWidgetItem(l.name));
            const QStringList nums = {QString::number(l.qty), W::money(l.price), W::money(l.price * l.qty)};
            for (int c = 0; c < 3; ++c)
            {
                auto *it = new QTableWidgetItem(nums[c]);
                it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                t->setItem(i, c + 1, it);
            }
            t->setRowHeight(i, 38);
        }
        t->setMinimumSize(560, 200);
        t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        t->setStyleSheet("QTableWidget{background:white;border:1px solid #e2e7f0;border-radius:10px;}");
        W::alignHeaders(t, {1, 2, 3}, Qt::AlignRight);
        v->addWidget(t);
        v->addWidget(W::label(QString("Payment: %1      Total: %2").arg(o.payment, W::money(o.total)), "h2"));
        auto *h = new QHBoxLayout;
        h->addStretch();
        auto *bill = W::button("View Bill", "", "bills");
        auto *close = W::button("Close", "primary");
        h->addWidget(bill);
        h->addWidget(close);
        v->addLayout(h);
        QObject::connect(close, &QPushButton::clicked, &d, &QDialog::accept);
        QObject::connect(bill, &QPushButton::clicked, &d, [&]
                         { receipt(&d, o); });
        d.exec();
    }

    bool payment(QWidget *parent, double total, QString &mode, double &tendered)
    {
        QDialog d(parent);
        d.setWindowTitle("Instant Payment");
        d.setMinimumWidth(400);
        auto *v = new QVBoxLayout(&d);
        v->setContentsMargins(22, 20, 22, 18);
        v->setSpacing(12);
        v->addWidget(W::label("TOTAL PAYABLE", "caption"));
        auto *tot = W::label(W::money(total), "bigTotal");
        v->addWidget(tot);

        auto *modes = new QHBoxLayout;
        auto *group = new QButtonGroup(&d);
        const QStringList names = {"Cash", "Card", "Wallet"};
        const QStringList icons = {"cash", "card", "tag"};
        for (int i = 0; i < 3; ++i)
        {
            auto *b = W::button(names[i], "tab", icons[i]);
            b->setCheckable(true);
            b->setMinimumHeight(40);
            group->addButton(b, i);
            modes->addWidget(b);
        }
        group->button(0)->setChecked(true);
        v->addLayout(modes);

        auto *tenderRow = new QWidget;
        auto *th = new QHBoxLayout(tenderRow);
        th->setContentsMargins(0, 0, 0, 0);
        th->addWidget(W::label("Cash tendered", "muted"));
        auto *spin = new QDoubleSpinBox;
        spin->setPrefix("$ ");
        spin->setDecimals(2);
        spin->setRange(0, 100000);
        spin->setValue(qCeil(total));
        spin->setMinimumWidth(130);
        th->addStretch();
        th->addWidget(spin);
        v->addWidget(tenderRow);
        auto *change = W::label("", "h2");
        v->addWidget(change);

        auto *h = new QHBoxLayout;
        h->addStretch();
        auto *cancel = W::button("Cancel");
        auto *ok = W::button("Confirm Payment", "primary", "check");
        h->addWidget(cancel);
        h->addWidget(ok);
        v->addLayout(h);

        auto refresh = [&]
        {
            const bool cash = group->checkedId() == 0;
            tenderRow->setVisible(cash);
            change->setVisible(cash);
            const double c = spin->value() - total;
            change->setText(c >= -0.005 ? "Change due: " + W::money(c) : "Short by " + W::money(-c));
            change->setStyleSheet(c >= -0.005 ? "color:#006329;" : "color:#ba1a1a;");
            ok->setEnabled(!cash || c >= -0.005);
        };
        QObject::connect(group, &QButtonGroup::idClicked, &d, [&](int)
                         { refresh(); });
        QObject::connect(spin, qOverload<double>(&QDoubleSpinBox::valueChanged), &d, [&](double)
                         { refresh(); });
        QObject::connect(cancel, &QPushButton::clicked, &d, &QDialog::reject);
        QObject::connect(ok, &QPushButton::clicked, &d, &QDialog::accept);
        refresh();
        if (d.exec() != QDialog::Accepted)
            return false;
        const int id = group->checkedId();
        mode = names[id];
        tendered = id == 0 ? spin->value() : 0;
        return true;
    }
}
