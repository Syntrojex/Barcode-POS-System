#include "Dialogs.h"
#include "Theme.h"
#include "Widgets.h"
#include <QLineEdit>
#include <QButtonGroup>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QLabel>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QPageSize>
#include <QPdfWriter>
#include <QStandardPaths>
#include <QTextBrowser>
#include <QTextDocument>

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
        t += center("SYNTRO-CHECK") + "\n" + center("Retail receipt") + "\n" + rule + "\n";
        t += two("Order", o.id) + "\n";
        if (!o.billId.isEmpty()) t += two("Bill", o.billId) + "\n";
        t += two("Date", o.time.toString("yyyy-MM-dd HH:mm")) + "\n";
        t += two("Customer", o.customer) + "\n" + rule + "\n";
        for (const auto &l : o.lines)
        {
            const double sub = l.price * l.qty;
            QString name = l.name;
            if (name.size() > w - 12)
                name = name.left(w - 13) + "…";
            t += two(QString("%1 x %2").arg(l.qty).arg(name), W::money(sub)) + "\n";
        }
        t += rule + "\n";
        t += two("Subtotal", W::money(o.subtotal)) + "\n";
        if (o.discount > 0)
            t += two("Discount", W::money(-o.discount)) + "\n";
        t += two("Tax", W::money(o.tax)) + "\n";
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

    // Bill as a small HTML page: header, details, item table, totals, payment.
    // The same HTML is shown on screen and saved as the PDF, so both always match.
    QString receiptHtml(const OrderRecord &o)
    {
        auto esc = [](const QString &x) { return x.toHtmlEscaped(); };
        auto row = [&](const QString &l, const QString &r, bool strong = false)
        {
            const QString w = strong ? "font-weight:700;" : "";
            return QString("<tr><td style='padding:2px 0;color:#5b6478;%3'>%1</td>"
                           "<td align='right' style='padding:2px 0;%3'>%2</td></tr>").arg(l, r, w);
        };
        QString h;
        h += "<div style='font-family:Segoe UI,Arial,sans-serif;font-size:10pt;color:#1b2b55;'>";
        h += "<p align='center' style='margin:0;font-size:18pt;font-weight:800;color:#2554d8;'>Syntro-Check</p>";
        h += "<p align='center' style='margin:0 0 10px 0;color:#5b6478;'>Sales receipt</p>";

        // order details
        h += "<table width='100%' cellspacing='0'>";
        h += row("Bill no.", o.billId.isEmpty() ? QString("-") : esc(o.billId));
        h += row("Order no.", esc(o.id));
        h += row("Date", o.time.toString("dd MMM yyyy, hh:mm AP"));
        h += row("Customer", esc(o.customer.isEmpty() ? QString("Walk-in") : o.customer));
        if (!o.contact.isEmpty())
            h += row("Contact", esc(o.contact));
        h += "</table><hr style='color:#dce4f5;'/>";

        // items
        h += "<table width='100%' cellspacing='0' cellpadding='3'>"
             "<tr style='background:#eef3ff;color:#2554d8;font-weight:700;'>"
             "<td>Item</td><td align='center'>Qty</td><td align='right'>Price</td>"
             "<td align='right'>Disc.</td><td align='right'>Amount</td></tr>";
        for (const auto &l : o.lines)
        {
            const double amount = (l.price - l.discount) * l.qty;
            h += QString("<tr><td>%1<br/><span style='color:#8a94ab;font-size:8pt;'>%2</span></td>"
                         "<td align='center'>%3</td><td align='right'>%4</td><td align='right'>%5</td>"
                         "<td align='right'>%6</td></tr>")
                     .arg(esc(l.name), esc(l.acronym)).arg(l.qty)
                     .arg(QLocale(QLocale::English).toString(l.price, 'f', 2),
                          l.discount > 0 ? QLocale(QLocale::English).toString(l.discount * l.qty, 'f', 2) : QString("-"),
                          QLocale(QLocale::English).toString(amount, 'f', 2));
        }
        h += "</table><hr style='color:#dce4f5;'/>";

        // totals
        h += "<table width='100%' cellspacing='0'>";
        h += row("Subtotal", W::money(o.subtotal));
        h += row("Discount", o.discount > 0 ? W::money(-o.discount) : W::money(0));
        h += row("Tax", W::money(o.tax));
        h += "</table>";
        h += QString("<table width='100%' cellspacing='0' cellpadding='6' style='margin-top:6px;'>"
                     "<tr style='background:#2554d8;color:white;font-weight:800;font-size:13pt;'>"
                     "<td>TOTAL</td><td align='right'>%1</td></tr></table>").arg(W::money(o.total));

        // payment
        h += "<table width='100%' cellspacing='0' style='margin-top:8px;'>";
        QString mode = o.payment.isEmpty() ? QString("-") : esc(o.payment);
        if (!o.merchant.isEmpty() && o.merchant != "-")
            mode += " (" + esc(o.merchant) + ")";
        h += row("Payment", mode);
        if (o.tendered > 0)
        {
            h += row("Amount received", W::money(o.tendered));
            h += row("Balance returned", W::money(o.tendered - o.total));
        }
        if (!o.status.isEmpty())
            h += row("Status", esc(o.status));
        h += "</table>";
        h += "<p align='center' style='margin-top:14px;color:#5b6478;'>Thank you for shopping with us!</p></div>";
        return h;
    }

    // Saves the bill as a PDF (A5 page) using Qt's built-in PDF writer.
    bool saveReceiptPdf(const OrderRecord &o, const QString &path)
    {
        QPdfWriter pdf(path);
        pdf.setPageSize(QPageSize(QPageSize::A5));
        pdf.setPageMargins(QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);
        pdf.setTitle("Bill " + (o.billId.isEmpty() ? o.id : o.billId));
        pdf.setCreator("Syntro-Check");
        QTextDocument doc;
        doc.setHtml(receiptHtml(o));
        doc.print(&pdf);
        return QFileInfo(path).exists();
    }

    void receipt(QWidget *parent, const OrderRecord &o)
    {
        QDialog d(parent);
        d.setWindowTitle("Bill - " + (o.billId.isEmpty() ? o.id : o.billId));
        auto *v = new QVBoxLayout(&d);
        v->setContentsMargins(18, 18, 18, 14);
        v->setSpacing(12);
        auto *view = new QTextBrowser;
        view->setHtml(receiptHtml(o));
        view->setStyleSheet("QTextBrowser{background:white;border:1px solid #dce9ff;border-radius:10px;padding:14px;}");
        view->setMinimumSize(470, 600);
        v->addWidget(view);

        auto *h = new QHBoxLayout;
        auto *pdfBtn = W::button("Save as PDF", "soft", "print");
        auto *close = W::button("Close", "primary");
        h->addWidget(pdfBtn);
        h->addStretch();
        h->addWidget(close);
        v->addLayout(h);
        QObject::connect(close, &QPushButton::clicked, &d, &QDialog::accept);
        QObject::connect(pdfBtn, &QPushButton::clicked, &d, [&d, &o]
        {
            const QString name = QString("Bill_%1.pdf").arg(o.billId.isEmpty() ? o.id : o.billId);
            const QString path = QFileDialog::getSaveFileName(&d, "Save bill as PDF",
                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + name, "PDF files (*.pdf)");
            if (path.isEmpty())
                return;
            if (saveReceiptPdf(o, path))
                QMessageBox::information(&d, "Bill saved", "Saved to:\n" + QDir::toNativeSeparators(path));
            else
                QMessageBox::warning(&d, "Bill not saved", "Could not write the PDF file.");
        });
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
        top->addWidget(W::label("Order " + o.id, "h1"));
        top->addWidget(W::badge(o.status, W::statusBadgeKind(o.status)));
        top->addStretch();
        v->addLayout(top);
        // date • customer • contact (empty parts are skipped, so no stray dots)
        QStringList meta{o.time.toString("dd MMM yyyy, hh:mm AP"), o.customer.isEmpty() ? QString("Walk-in") : o.customer};
        if (!o.contact.isEmpty()) meta << o.contact;
        v->addWidget(W::label(meta.join("  •  "), "muted"));
        auto *t = new QTableWidget(o.lines.size(), 5);
        t->setHorizontalHeaderLabels({"ITEM", "QTY", "PRICE", "DISCOUNT", "AMOUNT"});
        t->verticalHeader()->hide();
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setSelectionMode(QAbstractItemView::NoSelection);
        t->setShowGrid(false);
        t->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        for (int i = 0; i < o.lines.size(); ++i)
        {
            const auto &l = o.lines[i];
            auto cell = [&](int col, const QString &text, Qt::Alignment a)
            {
                auto *it = new QTableWidgetItem(text);
                it->setTextAlignment(a | Qt::AlignVCenter);
                t->setItem(i, col, it);
            };
            cell(0, l.name, Qt::AlignLeft);
            cell(1, QString::number(l.qty), Qt::AlignCenter);
            cell(2, W::money(l.price), Qt::AlignRight);
            cell(3, l.discount > 0 ? W::money(-l.discount * l.qty) : QString("-"), Qt::AlignRight);
            cell(4, W::money((l.price - l.discount) * l.qty), Qt::AlignRight);
        }
        t->setMinimumSize(660, 200);
        t->setStyleSheet("QTableWidget{border:1px solid #dce9ff;border-radius:8px;}");
        v->addWidget(t);
        // totals breakdown
        auto *sum = new QFormLayout;
        sum->setLabelAlignment(Qt::AlignLeft);
        auto right = [](const QString &x, bool big = false)
        {
            auto *l = new QLabel(x);
            l->setAlignment(Qt::AlignRight);
            if (big) l->setStyleSheet("font-weight:800; font-size:15px;");
            return l;
        };
        sum->addRow("Subtotal", right(W::money(o.subtotal)));
        sum->addRow("Discount", right(o.discount > 0 ? W::money(-o.discount) : W::money(0)));
        sum->addRow("Tax", right(W::money(o.tax)));
        sum->addRow("Payment", right(o.payment.isEmpty() ? QString("-") : o.payment));
        auto *totalLbl = new QLabel("Total");
        totalLbl->setStyleSheet("font-weight:800; font-size:15px;");
        sum->addRow(totalLbl, right(W::money(o.total), true));
        v->addLayout(sum);
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

    void productDescription(QWidget *parent, int productId)
    {
        const ProductDescriptionView pd = AppState::instance().describe(productId);
        if (pd.title.isEmpty())
            return;
        QDialog d(parent);
        d.setWindowTitle("Product description");
        auto *v = new QVBoxLayout(&d);
        v->setContentsMargins(22, 18, 22, 16);
        v->setSpacing(10);
        auto *top = new QHBoxLayout;
        top->addWidget(W::label(pd.title, "h1"));
        const bool out = pd.level == "out", low = pd.level == "low";
        top->addWidget(W::badge(out ? "Out of stock" : low ? "Low stock" : "In stock", out ? "red" : low ? "amber" : "green"));
        top->addStretch();
        v->addLayout(top);
        auto *body = new QHBoxLayout;
        auto *form = new QFormLayout;
        form->setHorizontalSpacing(24);
        form->setVerticalSpacing(8);
        for (const auto &f : pd.fields)
            form->addRow(W::label(f.first.toUpper(), "caption"), W::label(f.second, "h2"));
        body->addLayout(form, 1);
        QPixmap px(pd.imagePath);
        if (!px.isNull())
        {
            auto *img = new QLabel;
            img->setPixmap(px.scaledToWidth(200, Qt::SmoothTransformation));
            body->addWidget(img, 0, Qt::AlignTop);
        }
        v->addLayout(body);
        auto *h = new QHBoxLayout;
        h->addStretch();
        auto *close = W::button("Close", "primary");
        h->addWidget(close);
        v->addLayout(h);
        QObject::connect(close, &QPushButton::clicked, &d, &QDialog::accept);
        d.exec();
    }

    void loadIssues(QWidget *parent)
    {
        auto &st = AppState::instance();
        QDialog d(parent);
        d.setWindowTitle("Product.txt - data check");
        auto *v = new QVBoxLayout(&d);
        v->setContentsMargins(20, 16, 20, 14);
        v->setSpacing(10);
        v->addWidget(W::label("Product data check", "h1"));
        v->addWidget(W::label(st.productFile().isEmpty() ? "Product.txt was not found." : st.productFile(), "muted"));
        auto *list = new QListWidget;
        const QStringList issues = st.productIssues();
        list->addItems(issues.isEmpty() ? QStringList{"No problems found."} : issues);
        list->setMinimumSize(560, 260);
        v->addWidget(list);
        auto *close = W::button("Close", "primary");
        auto *h = new QHBoxLayout;
        h->addStretch();
        h->addWidget(close);
        v->addLayout(h);
        QObject::connect(close, &QPushButton::clicked, &d, &QDialog::accept);
        d.exec();
    }

    bool payment(QWidget *parent, double total, QString &mode, double &tendered, QString &merchant)
    {
        QDialog d(parent);
        d.setWindowTitle("Instant Payment");
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
        spin->setPrefix("PKR ");
        spin->setDecimals(2);
        spin->setRange(0, 10000000);
        spin->setValue(qCeil(total / 10.0) * 10.0);
        spin->setMinimumWidth(130);
        th->addStretch();
        th->addWidget(spin);
        v->addWidget(tenderRow);
        auto *change = W::label("", "h2");
        v->addWidget(change);
        auto *merchantRow = new QWidget;
        auto *mh = new QHBoxLayout(merchantRow);
        mh->setContentsMargins(0, 0, 0, 0);
        auto *merchantName = W::label("Bank / wallet name", "muted");
        auto *merchantInput = new QLineEdit;
        merchantInput->setPlaceholderText("e.g. HBL, JazzCash");
        mh->addWidget(merchantName);
        mh->addWidget(merchantInput, 1);
        v->addWidget(merchantRow);

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
            merchantRow->setVisible(!cash);
            const double c = spin->value() - total;
            change->setText(c >= -0.005 ? "Change due: " + W::money(c) : "Short by " + W::money(-c));
            change->setStyleSheet(c >= -0.005 ? "color:#006329;" : "color:#ba1a1a;");
            ok->setEnabled(cash ? c >= -0.005 : !merchantInput->text().trimmed().isEmpty());
        };
        QObject::connect(group, &QButtonGroup::idClicked, &d, [&](int)
                         { refresh(); });
        QObject::connect(spin, qOverload<double>(&QDoubleSpinBox::valueChanged), &d, [&](double)
                         { refresh(); });
        QObject::connect(merchantInput, &QLineEdit::textChanged, &d, [&](const QString&)
                         { refresh(); });
        QObject::connect(cancel, &QPushButton::clicked, &d, &QDialog::reject);
        QObject::connect(ok, &QPushButton::clicked, &d, &QDialog::accept);
        refresh();
        if (d.exec() != QDialog::Accepted)
            return false;
        const int id = group->checkedId();
        mode = names[id];
        tendered = id == 0 ? spin->value() : 0;
        merchant = id == 0 ? QString() : merchantInput->text().trimmed();
        return true;
    }

    bool login(QWidget *parent, UserRole &role, QString &name)
    {
        QDialog d(parent);
        d.setWindowTitle("Syntro-Check - Sign in");
        d.setMinimumWidth(420);
        auto *v = new QVBoxLayout(&d);
        v->setContentsMargins(28, 24, 28, 22);
        v->setSpacing(12);

        auto *title = new QLabel("Sign in to Syntro-Check");
        title->setStyleSheet("font-size:20px; font-weight:800; color:#1b2b55;");
        auto *sub = new QLabel("Enter your name and choose your role.");
        sub->setStyleSheet("color:#5b6478;");
        v->addWidget(title);
        v->addWidget(sub);

        auto *nameEdit = new QLineEdit;
        nameEdit->setPlaceholderText("Your name");
        nameEdit->setMinimumHeight(38);
        v->addWidget(nameEdit);

        // One button per role; the button pressed decides the role.
        auto *cashier = W::button("Cashier", "primary", "cart");
        auto *manager = W::button("Manager", "soft", "products");
        for (auto *b : {cashier, manager})
            b->setMinimumHeight(46);
        auto *row = new QHBoxLayout;
        row->setSpacing(10);
        row->addWidget(cashier);
        row->addWidget(manager);
        v->addLayout(row);

        auto *hint = new QLabel("Cashier: record orders and generate bills.\nManager: view products only.");
        hint->setStyleSheet("color:#8a94ab; font-size:11px;");
        v->addWidget(hint);

        QObject::connect(cashier, &QPushButton::clicked, &d, [&] { role = UserRole::Cashier; d.accept(); });
        QObject::connect(manager, &QPushButton::clicked, &d, [&] { role = UserRole::Manager; d.accept(); });
        QObject::connect(nameEdit, &QLineEdit::returnPressed, cashier, &QPushButton::click);

        if (d.exec() != QDialog::Accepted)
            return false;
        name = nameEdit->text().trimmed();
        if (name.isEmpty())
            name = RolePolicy::name(role);
        return true;
    }
}
