// "Complete Order Payment" (redesigned): billing breakdown with item / seasonal / special discount and tax on the
// left; tender mode, cash numpad shortcuts and balance to return on the right.
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
        const QString who = req.customer.trimmed().isEmpty() ? QString("Walk-in Customer") : req.customer.trimmed();
        QString sub = QString("Order #%1  •  %2").arg(req.orderId, who);
        const QString phone = req.contact.trimmed();
        if (!phone.isEmpty() && phone != "-")
            sub += QString("  (%1)").arg(phone);
        Ui::Modal d(parent, "cash", "Complete Order Payment", sub, "POS DESK", 880);

        Pricing::Quote q = Pricing::quote(req.lines, 0);

        auto *cols = new QHBoxLayout;
        cols->setSpacing(14);

        // ============================================================ left: billing breakdown
        auto *bill = W::card(nullptr, "subCard");
        auto *bv = new QVBoxLayout(bill);
        bv->setContentsMargins(16, 14, 16, 14);
        bv->setSpacing(8);
        auto *itemsBadge = W::badge("", "blue");
        W::headerRow(bv, "bills", "Billing Breakdown", itemsBadge);

        QLabel *gross = nullptr, *itemDisc = nullptr, *seasonal = nullptr, *special = nullptr, *tax = nullptr;
        QLabel *seasonalName = nullptr;
        auto row = [&](const QString &k, QLabel *&val, QLabel **keyOut = nullptr)
        {
            auto *r = new QHBoxLayout;
            auto *kl = W::label(k, "muted");
            kl->setWordWrap(true);
            r->addWidget(kl, 1);
            val = W::label("PKR 0.00");
            val->setStyleSheet("font-weight:800; font-size:14px;");
            r->addWidget(val);
            bv->addLayout(r);
            if (keyOut)
                *keyOut = kl;
        };
        row("Gross Subtotal", gross);
        row("Item Discount", itemDisc);
        row("Seasonal Discount", seasonal, &seasonalName);

        // special discount: typed percent, at most 8 percent (requirement 8)
        auto *spBox = W::card(nullptr, "field");
        auto *sh = new QHBoxLayout(spBox);
        sh->setContentsMargins(10, 8, 10, 8);
        sh->setSpacing(8);
        sh->addWidget(W::label("Special Discount", "muted"));
        auto *pct = new QDoubleSpinBox;
        pct->setRange(0, UiStore::kSpecialDiscountCapPercent);
        pct->setDecimals(1);
        pct->setSingleStep(0.5);
        pct->setSuffix(" %");
        pct->setToolTip("Special discount, at most 8 percent");
        pct->setMinimumWidth(84);
        sh->addWidget(pct);
        sh->addWidget(W::badge(QString("CAPPED AT %1% MAX").arg(UiStore::kSpecialDiscountCapPercent, 0, 'f', 0), "grey"));
        sh->addStretch();
        special = W::label("PKR 0.00");
        special->setStyleSheet("font-weight:800; font-size:14px;");
        sh->addWidget(special);
        bv->addWidget(spBox);

        row("Sales Tax", tax);
        bv->addStretch();

        auto *totalBox = W::card(nullptr, "totalBox");
        auto *tv = new QVBoxLayout(totalBox);
        tv->setContentsMargins(16, 12, 16, 14);
        tv->setSpacing(2);
        tv->addWidget(W::label("NET PAYABLE AMOUNT", "caption"));
        auto *payable = W::label("PKR 0.00");
        payable->setStyleSheet("font-size:32px; font-weight:800; color:white;");
        tv->addWidget(payable);
        bv->addWidget(totalBox);
        cols->addWidget(bill, 5);

        // ============================================================ right: tender
        auto *right = new QVBoxLayout;
        right->setSpacing(8);
        right->addWidget(W::label("SELECT TENDER MODE", "caption"));
        auto *modeRow = new QHBoxLayout;
        modeRow->setSpacing(8);
        auto *modes = new QButtonGroup(&d);
        const QStringList modeNames = {"Cash", "Card", "Wallet"};
        const QStringList modeSub = {"CASH IN HAND", "SWIPE / TAP", "QR CODE"};
        const QStringList modeIcons = {"cash", "card", "wallet"};
        for (int i = 0; i < 3; ++i)
        {
            auto *b = W::button(modeNames[i] + "\n" + modeSub[i], "tab", modeIcons[i]);
            b->setCheckable(true);
            b->setMinimumHeight(58);
            modes->addButton(b, i);
            modeRow->addWidget(b, 1);
        }
        modes->button(0)->setChecked(true);
        right->addLayout(modeRow);

        // page 0: cash, page 1: card / wallet gateway
        auto *pages = new QStackedWidget;
        auto *cashPage = new QWidget;
        auto *cv = new QVBoxLayout(cashPage);
        cv->setContentsMargins(0, 0, 0, 0);
        cv->setSpacing(8);
        cv->addWidget(W::label("TENDER AMOUNT RECEIVED", "caption"));
        auto *tender = new QDoubleSpinBox;
        tender->setPrefix("PKR  ");
        tender->setDecimals(2);
        tender->setRange(0, 100000000);
        tender->setGroupSeparatorShown(true);
        tender->setAlignment(Qt::AlignRight);
        tender->setMinimumHeight(54);
        tender->setStyleSheet("font-size:24px; font-weight:800;");
        tender->setButtonSymbols(QAbstractSpinBox::NoButtons);
        cv->addWidget(tender);
        cv->addWidget(W::label("FAST CASH TENDER SHORTCUTS", "caption"));
        auto *quick = new QHBoxLayout;
        quick->setSpacing(8);
        QPushButton *exact = nullptr;
        bool replaceOnFirstKey = true; // the pre-filled exact amount is replaced by the first note the cashier adds
        bool updating = false;
        for (int add : {500, 1000, 5000})
        {
            auto *b = W::button(QString("+%L1").arg(add), "soft");
            b->setMinimumHeight(40);
            b->setToolTip("Adds to the amount received (the first key replaces the pre-filled exact amount)");
            QObject::connect(b, &QPushButton::clicked, &d, [&, add]
                             {
                const double base = replaceOnFirstKey ? 0.0 : tender->value();
                replaceOnFirstKey = false;
                tender->setValue(base + add); });
            quick->addWidget(b, 1);
        }
        exact = W::button("Exact", "primary");
        exact->setMinimumHeight(40);
        quick->addWidget(exact, 1);
        cv->addLayout(quick);
        pages->addWidget(cashPage);

        auto *gatePage = new QWidget;
        auto *gv = new QVBoxLayout(gatePage);
        gv->setContentsMargins(0, 0, 0, 0);
        gv->setSpacing(8);
        auto *gateCap = W::label("CARD / DIGITAL GATEWAY ROUTE", "caption");
        gv->addWidget(gateCap);
        auto *gate = new QComboBox;
        gate->setEditable(true);
        gate->setMinimumHeight(40);
        gv->addWidget(gate);
        auto *gateHint = W::label("Pick the bank or wallet, or type another name.", "muted");
        gv->addWidget(gateHint);
        gv->addStretch();
        pages->addWidget(gatePage);
        right->addWidget(pages);

        // balance to return (green) / short by (red)
        auto *balBox = new QFrame;
        auto *bh = new QHBoxLayout(balBox);
        bh->setContentsMargins(16, 10, 16, 10);
        auto *balCap = W::label("BALANCE CHANGE RETURN", "caption");
        auto *balVal = W::label("PKR 0.00");
        auto *balCol = new QVBoxLayout;
        balCol->setSpacing(0);
        balCol->addWidget(balCap);
        balCol->addWidget(balVal);
        bh->addLayout(balCol, 1);
        right->addStretch();
        right->addWidget(balBox);
        cols->addLayout(right, 6);
        d.body()->addLayout(cols);

        // ============================================================ footer
        auto *cancel = W::button("Cancel  [Esc]", "", "close");
        auto *ok = W::button("Confirm Payment && Print  [Enter]", "primary", "check");
        ok->setMinimumHeight(42);
        ok->setDefault(true);
        d.footer()->addWidget(W::label("Bill is saved as PDF from the receipt screen", "muted"));
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
                gateCap->setText("WALLET");
            }
            else
            {
                gate->addItems({"HBL - POS Terminal #04", "Meezan Bank - POS", "UBL - POS Terminal", "Allied Bank - POS"});
                gateCap->setText("CARD / DIGITAL GATEWAY ROUTE");
            }
            gate->setCurrentIndex(0);
        };

        auto refresh = [&]
        {
            q = Pricing::quote(req.lines, pct->value());
            W::setBadge(itemsBadge, QString("%1 ITEM%2").arg(q.units).arg(q.units == 1 ? "" : "S"), "blue");
            gross->setText(W::money(q.gross));
            itemDisc->setText(W::money(-q.itemDiscount));
            seasonalName->setText(q.seasonName.isEmpty()
                                      ? QString("Seasonal Discount (none active)")
                                      : QString("Seasonal %1% (%2)").arg(q.seasonalPercent, 0, 'g', 4).arg(q.seasonName));
            seasonal->setText(W::money(-q.seasonal));
            special->setText(W::money(-q.special));
            tax->setText(W::money(q.tax));
            payable->setText(W::money(q.payable));
            exact->setText(QString("Exact (%1)").arg(W::money(q.payable)));

            const bool cash = modes->checkedId() == 0;
            pages->setCurrentIndex(cash ? 0 : 1);
            bool enough = true;
            if (cash)
            {
                const double diff = tender->value() - q.payable;
                enough = diff >= -0.005;
                balCap->setText(enough ? "BALANCE CHANGE RETURN" : "SHORT BY");
                balVal->setText(W::money(enough ? qMax(0.0, diff) : -diff));
            }
            else
            {
                balCap->setText(modes->checkedId() == 1 ? "CHARGE TO CARD" : "CHARGE TO WALLET");
                balVal->setText(W::money(q.payable));
            }
            const QString bg = enough ? "#0b7a4a" : "#ba1a1a";
            balBox->setStyleSheet(QString("QFrame{background:%1; border-radius:12px;} QLabel{background:transparent; color:white;}").arg(bg));
            balVal->setStyleSheet("font-size:26px; font-weight:800; color:white; background:transparent;");
            balCap->setStyleSheet("font-size:10.5px; font-weight:800; letter-spacing:1px; color:#d7f8e2; background:transparent;");
            ok->setEnabled(cash ? enough : !gate->currentText().trimmed().isEmpty());
        };

        QObject::connect(pct, qOverload<double>(&QDoubleSpinBox::valueChanged), &d, [&](double)
                         {
            // while the amount is still the pre-filled exact amount it follows the new payable
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
        tender->setFocus();
        tender->selectAll();
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
