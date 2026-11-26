// "Payment" dialog: the bill on the left (only the discounts that apply are shown), how the customer pays on the
// right (cash with quick amounts, or card / wallet), and the change to return.
// Frontend only: the numbers come from Pricing::quote (the same function the order is saved with), so what the
// cashier sees here is exactly what lands in Order.txt / Bill.txt.
#include "Dialogs.h"
#include "Pricing.h"
#include "Theme.h"
#include "UiKit.h"
#include "UiStore.h"
#include "Widgets.h"
#include <QButtonGroup>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QtMath>

namespace Dialogs
{
    bool payment(QWidget *parent, const PaymentRequest &req, PaymentResult &res)
    {
        Pricing::Quote q = Pricing::quote(req.lines, 0);
        const QString who = req.customer.trimmed().isEmpty() ? QString("Walk-in Customer") : req.customer.trimmed();
        Ui::Modal d(parent, "cash", "Payment",
                    QString("Order #%1  ·  %2  ·  %3 item%4").arg(req.orderId, who).arg(q.units).arg(q.units == 1 ? "" : "s"),
                    QString(), 820);

        auto *cols = new QHBoxLayout;
        cols->setSpacing(18);

        // ============================================================ left: the bill
        auto *bill = W::card(nullptr, "subCard");
        auto *bv = new QVBoxLayout(bill);
        bv->setContentsMargins(18, 16, 18, 16);
        bv->setSpacing(10);
        bv->addWidget(W::label("Bill", "caption"));

        // one "label ......... amount" line; returns the row so it can be hidden when it is zero
        auto line = [&](const QString &k, QLabel *&key, QLabel *&val)
        {
            auto *row = new QWidget;
            auto *h = new QHBoxLayout(row);
            h->setContentsMargins(0, 0, 0, 0);
            key = W::label(k, "muted");
            val = W::label("PKR 0.00");
            val->setStyleSheet("font-weight:700;");
            h->addWidget(key, 1);
            h->addWidget(val);
            bv->addWidget(row);
            return row;
        };
        QLabel *grossK, *gross, *itemK, *itemDisc, *seasonK, *seasonal, *taxK, *tax;
        line("Subtotal", grossK, gross);
        auto *itemRow = line("Item discount", itemK, itemDisc);
        auto *seasonRow = line("Seasonal discount", seasonK, seasonal);

        // special discount: typed percent, at most 8 percent (requirement 8)
        auto *spRow = new QHBoxLayout;
        spRow->setSpacing(8);
        spRow->addWidget(W::label("Special discount", "muted"));
        auto *pct = new QDoubleSpinBox;
        pct->setRange(0, UiStore::kSpecialDiscountCapPercent);
        pct->setDecimals(1);
        pct->setSingleStep(0.5);
        pct->setSuffix(" %");
        pct->setButtonSymbols(QAbstractSpinBox::NoButtons);
        pct->setAlignment(Qt::AlignRight);
        pct->setFixedWidth(70);
        pct->setMinimumHeight(32);
        pct->setToolTip(QString("At most %1%").arg(UiStore::kSpecialDiscountCapPercent, 0, 'f', 0));
        spRow->addWidget(pct);
        spRow->addWidget(W::label(QString("max %1%").arg(UiStore::kSpecialDiscountCapPercent, 0, 'f', 0), "muted"));
        spRow->addStretch();
        auto *special = W::label("PKR 0.00");
        special->setStyleSheet("font-weight:700;");
        spRow->addWidget(special);
        bv->addLayout(spRow);

        line("Tax", taxK, tax);
        bv->addStretch();

        auto *totalBox = W::card(nullptr, "totalBox");
        auto *th = new QHBoxLayout(totalBox);
        th->setContentsMargins(16, 12, 16, 12);
        auto *totalK = W::label("Total");
        totalK->setStyleSheet("font-size:15px; font-weight:700; color:white; background:transparent;");
        auto *payable = W::label("PKR 0.00");
        payable->setStyleSheet("font-size:28px; font-weight:800; color:white; background:transparent;");
        th->addWidget(totalK);
        th->addStretch();
        th->addWidget(payable);
        bv->addWidget(totalBox);
        cols->addWidget(bill, 1);

        // ============================================================ right: how the customer pays
        auto *right = new QVBoxLayout;
        right->setSpacing(10);
        right->addWidget(W::label("Pay with", "caption"));
        auto *modeRow = new QHBoxLayout;
        modeRow->setSpacing(6);
        auto *modes = new QButtonGroup(&d);
        const QStringList modeNames = {"Cash", "Card", "Wallet"};
        for (int i = 0; i < 3; ++i)
        {
            auto *b = W::button(modeNames[i], "tab"); // text only: an icon would vanish on the selected (blue) button
            b->setCheckable(true);
            b->setMinimumHeight(42);
            modes->addButton(b, i);
            modeRow->addWidget(b, 1);
        }
        using Payment = barcodehub::order::PaymentMode;
        const auto preferredPayment = AppState::instance().settings().defaultPayment;
        const int initialMode = preferredPayment == Payment::Card ? 1 : preferredPayment == Payment::Wallet ? 2 : 0;
        modes->button(initialMode)->setChecked(true);
        right->addLayout(modeRow);
        right->addSpacing(4);

        // page 0: cash received, page 1: bank / wallet
        auto *pages = new QStackedWidget;
        auto *cashPage = new QWidget;
        auto *cv = new QVBoxLayout(cashPage);
        cv->setContentsMargins(0, 0, 0, 0);
        cv->setSpacing(8);
        cv->addWidget(W::label("Amount received", "caption"));
        auto *tender = new QDoubleSpinBox;
        tender->setPrefix("PKR ");
        tender->setDecimals(2);
        tender->setRange(0, 100000000);
        tender->setGroupSeparatorShown(true);
        tender->setAlignment(Qt::AlignRight);
        tender->setMinimumHeight(52);
        tender->setStyleSheet("font-size:22px; font-weight:800;");
        tender->setButtonSymbols(QAbstractSpinBox::NoButtons);
        cv->addWidget(tender);
        auto *quick = new QHBoxLayout;
        quick->setSpacing(6);
        bool replaceOnFirstKey = true; // the pre-filled exact amount is replaced by the first note the cashier adds
        bool updating = false;
        auto *exact = W::button("Exact", "soft");
        exact->setMinimumHeight(36);
        quick->addWidget(exact, 1);
        for (int add : {500, 1000, 5000})
        {
            auto *b = W::button(QString("+%L1").arg(add), "soft");
            b->setMinimumHeight(36);
            b->setToolTip("Adds to the amount received");
            QObject::connect(b, &QPushButton::clicked, &d, [&, add]
                             {
                const double base = replaceOnFirstKey ? 0.0 : tender->value();
                replaceOnFirstKey = false;
                tender->setValue(base + add); });
            quick->addWidget(b, 1);
        }
        cv->addLayout(quick);
        cv->addStretch();
        pages->addWidget(cashPage);

        auto *gatePage = new QWidget;
        auto *gv = new QVBoxLayout(gatePage);
        gv->setContentsMargins(0, 0, 0, 0);
        gv->setSpacing(8);
        auto *gateCap = W::label("Bank / terminal", "caption");
        gv->addWidget(gateCap);
        auto *gate = new QComboBox;
        gate->setEditable(true);
        gate->setMinimumHeight(42);
        gv->addWidget(gate);
        gv->addStretch();
        pages->addWidget(gatePage);
        right->addWidget(pages, 1);

        // change to return (green) / still short (red) / charged to card or wallet (blue)
        auto *balBox = new QFrame;
        auto *bh = new QHBoxLayout(balBox);
        bh->setContentsMargins(16, 12, 16, 12);
        auto *balCap = W::label("Change");
        auto *balVal = W::label("PKR 0.00");
        bh->addWidget(balCap);
        bh->addStretch();
        bh->addWidget(balVal);
        right->addWidget(balBox);
        cols->addLayout(right, 1);
        d.body()->addLayout(cols);

        // ============================================================ footer
        auto *cancel = W::button("Cancel", "flat");
        auto *ok = W::button("Pay", "primary", "check");
        ok->setMinimumHeight(42);
        ok->setMinimumWidth(190);
        ok->setDefault(true); // Enter pays
        d.footer()->addStretch();
        d.footer()->addWidget(cancel);
        d.footer()->addWidget(ok);
        QObject::connect(cancel, &QPushButton::clicked, &d, &QDialog::reject);

        // ============================================================ behaviour
        auto fillGateways = [&]
        {
            gate->clear();
            if (modes->checkedId() == 2)
            {
                gate->addItems({"JazzCash", "Easypaisa", "SadaPay", "NayaPay"});
                gateCap->setText("Wallet");
            }
            else
            {
                gate->addItems({"HBL - POS Terminal 04", "Meezan Bank - POS", "UBL - POS Terminal", "Allied Bank - POS"});
                gateCap->setText("Bank / terminal");
            }
            gate->setCurrentIndex(0);
        };

        auto refresh = [&]
        {
            q = Pricing::quote(req.lines, pct->value());
            gross->setText(W::money(q.gross));
            itemDisc->setText(W::money(-q.itemDiscount));
            itemRow->setVisible(q.itemDiscount > 0.005);   // only lines that apply are shown
            seasonK->setText(QString("Seasonal discount (%1%)").arg(q.seasonalPercent, 0, 'g', 4));
            seasonal->setText(W::money(-q.seasonal));
            seasonRow->setVisible(!q.seasonName.isEmpty());
            seasonRow->setToolTip(q.seasonName);
            special->setText(W::money(-q.special));
            tax->setText(W::money(q.tax));
            payable->setText(W::money(q.payable));

            const int mode = modes->checkedId();
            pages->setCurrentIndex(mode == 0 ? 0 : 1);
            QString bg = "#e3f6ec", fg = "#0b6b3f";
            bool enough = true;
            if (mode == 0)
            {
                const double diff = tender->value() - q.payable;
                enough = diff >= -0.005;
                balCap->setText(enough ? "Change to return" : "Still short");
                balVal->setText(W::money(enough ? qMax(0.0, diff) : -diff));
                if (!enough)
                {
                    bg = "#fde7e6";
                    fg = "#ba1a1a";
                }
            }
            else
            {
                balCap->setText(mode == 1 ? "Charge to card" : "Charge to wallet");
                balVal->setText(W::money(q.payable));
                bg = "#e6edff";
                fg = "#1d46b4";
            }
            balBox->setStyleSheet(QString("QFrame{background:%1; border-radius:10px;} QLabel{background:transparent; color:%2;}").arg(bg, fg));
            balCap->setStyleSheet(QString("font-size:14px; font-weight:700; color:%1;").arg(fg));
            balVal->setStyleSheet(QString("font-size:22px; font-weight:800; color:%1;").arg(fg));
            ok->setText("Pay " + W::money(q.payable));
            ok->setEnabled(mode == 0 ? enough : !gate->currentText().trimmed().isEmpty());
        };

        QObject::connect(pct, qOverload<double>(&QDoubleSpinBox::valueChanged), &d, [&](double)
                         {
            // while the amount is still the pre-filled exact amount it follows the new total
            const double before = q.payable;
            const Pricing::Quote nq = Pricing::quote(req.lines, pct->value());
            if (replaceOnFirstKey && qAbs(tender->value() - before) < 0.005)
            {
                updating = true;
                tender->setValue(nq.payable);
                updating = false;
            }
            refresh(); });
        QObject::connect(tender, qOverload<double>(&QDoubleSpinBox::valueChanged), &d, [&](double)
                         {
            if (!updating)
                replaceOnFirstKey = false;
            refresh(); });
        QObject::connect(exact, &QPushButton::clicked, &d, [&]
                         {
            replaceOnFirstKey = true;
            updating = true;
            tender->setValue(q.payable);
            updating = false;
            refresh(); });
        QObject::connect(modes, &QButtonGroup::idClicked, &d, [&](int)
                         {
            fillGateways();
            refresh(); });
        QObject::connect(gate, &QComboBox::currentTextChanged, &d, [&](const QString &) { refresh(); });
        QObject::connect(ok, &QPushButton::clicked, &d, &QDialog::accept);

        fillGateways();
        updating = true;
        tender->setValue(q.payable); // exact amount pre-filled: pressing Enter completes a cash sale at once
        updating = false;
        refresh();
        if (initialMode == 0)
        {
            tender->setFocus();
            tender->selectAll();
        }
        else
            gate->setFocus();
        if (d.exec() != QDialog::Accepted)
            return false;

        const int id = modes->checkedId();
        res.mode = modeNames[id];
        res.merchant = id == 0 ? QString() : gate->currentText().trimmed();
        res.tendered = id == 0 ? tender->value() : 0;
        res.specialPercent = Pricing::clampSpecial(pct->value());
        return true;
    }
}
