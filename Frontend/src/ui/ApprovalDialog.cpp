// "Manager Approval Required": a paid order can be edited (within 7 days of payment) only after a manager signs it off.
// Frontend only: the check goes through UiStore::verifyManager (demo staff id / PIN until the backend has accounts).
#include "Dialogs.h"
#include "Theme.h"
#include "UiKit.h"
#include "UiStore.h"
#include "Widgets.h"
#include <QAction>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace Dialogs
{
    bool managerApproval(QWidget *parent, const OrderRecord &order, QString *managerName, QString *reason)
    {
        auto &store = UiStore::instance();
        Ui::Modal d(parent, "shield", "Manager Approval Required", "SECURITY GATE  •  AUDIT TRAIL", QString(), 560);

        // info banner
        auto *info = W::card(nullptr, "subCard");
        auto *ih = new QHBoxLayout(info);
        ih->setContentsMargins(12, 10, 12, 10);
        ih->setSpacing(10);
        ih->addWidget(W::iconLabel("info", Theme::primary, 20), 0, Qt::AlignTop);
        auto *msg = W::label(QString("This paid order (<b>#%1 – %2 • %3</b>) can only be edited with an authorized manager's "
                                     "approval. Modifying finalized sales alters fiscal records and inventory ledgers.")
                                 .arg(order.id.toHtmlEscaped(),
                                      (order.customer.isEmpty() ? QString("Walk-in Customer") : order.customer).toHtmlEscaped(),
                                      W::money(order.total)));
        msg->setTextFormat(Qt::RichText);
        msg->setWordWrap(true);
        ih->addWidget(msg, 1);
        d.body()->addWidget(info);

        // staff id
        d.body()->addWidget(W::label("MANAGER USERNAME / STAFF ID", "caption"));
        auto *staff = new QLineEdit(UiStore::demoManagerId());
        staff->setMinimumHeight(40);
        staff->addAction(Theme::icon("user", Theme::outline, 16), QLineEdit::LeadingPosition);
        auto *staffOk = staff->addAction(Theme::icon("check", Theme::tertiary, 18), QLineEdit::TrailingPosition);
        d.body()->addWidget(staff);

        // PIN with show / hide
        d.body()->addWidget(W::label("SECURITY PASSWORD / PIN", "caption"));
        auto *pin = new QLineEdit;
        pin->setEchoMode(QLineEdit::Password);
        pin->setPlaceholderText("Manager PIN");
        pin->setMinimumHeight(40);
        pin->addAction(Theme::icon("lock", Theme::outline, 16), QLineEdit::LeadingPosition);
        auto *eye = pin->addAction(Theme::icon("eye", Theme::outline, 18), QLineEdit::TrailingPosition);
        eye->setCheckable(true);
        eye->setToolTip("Show / hide the PIN");
        QObject::connect(eye, &QAction::toggled, &d, [pin](bool on)
                         { pin->setEchoMode(on ? QLineEdit::Normal : QLineEdit::Password); });
        d.body()->addWidget(pin);
        d.body()->addWidget(W::label(QString("Demo account: %1 / PIN %2").arg(UiStore::demoManagerId(), UiStore::demoManagerPin()), "muted"));

        // reason (goes to the audit log)
        d.body()->addWidget(W::label("REASON FOR EDIT / AUDIT LOG ENTRY", "caption"));
        auto *why = new QComboBox;
        why->setEditable(true);
        why->setMinimumHeight(40);
        why->addItems({"Customer item exchange / barcode error", "Wrong quantity entered", "Price or discount correction",
                       "Customer request", "Other"});
        d.body()->addWidget(why);

        auto *err = W::label("", "muted");
        err->setWordWrap(true);
        err->setStyleSheet("color:#ba1a1a; font-weight:700; background:#ffdad6; border-radius:8px; padding:8px 10px;");
        err->hide();
        d.body()->addWidget(err);

        // staff id check mark: only shown while the id is a known manager
        auto idKnown = [&]
        { return staff->text().trimmed().compare(UiStore::demoManagerId(), Qt::CaseInsensitive) == 0; };
        staffOk->setVisible(idKnown());
        QObject::connect(staff, &QLineEdit::textChanged, &d, [&](const QString &)
                         { staffOk->setVisible(idKnown()); err->hide(); });
        QObject::connect(pin, &QLineEdit::textChanged, &d, [&](const QString &)
                         { err->hide(); });

        auto *cancel = W::button("Cancel  [Esc]");
        auto *ok = W::button("Approve && Unlock Order  [Enter]", "primary", "unlock");
        ok->setMinimumHeight(40);
        ok->setDefault(true);
        d.footer()->addWidget(W::label("Approval is stored with the order", "muted"));
        d.footer()->addStretch();
        d.footer()->addWidget(cancel);
        d.footer()->addWidget(ok);
        QObject::connect(cancel, &QPushButton::clicked, &d, &QDialog::reject);
        QObject::connect(ok, &QPushButton::clicked, &d, [&]
                         {
            QString who;
            if (!store.verifyManager(staff->text(), pin->text(), &who))
            {
                err->setText("Staff ID or PIN is not correct. Ask a manager to sign in.");
                err->show();
                pin->selectAll();
                pin->setFocus();
                return;
            }
            if (managerName)
                *managerName = who;
            if (reason)
                *reason = why->currentText().trimmed().isEmpty() ? QString("Customer request") : why->currentText().trimmed();
            d.accept(); });

        pin->setFocus();
        return d.exec() == QDialog::Accepted;
    }
}
