// "Add New Product to Catalog" / "Edit Product" (manager).
// Layout follows the Stitch design: 1 Category selection, 2 Product specifications, EAN-8 barcode engine on the right.
// Frontend only: everything is saved through the existing AppState::addProduct / updateProduct (Increment 2 backend).
#include "BarcodeArt.h"
#include "Dialogs.h"
#include "Theme.h"
#include "UiKit.h"
#include "UiStore.h"
#include "Widgets.h"
#include <QButtonGroup>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMarginsF>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QVBoxLayout>

namespace
{
    // Round numbered badge "1" / "2" in front of a section title.
    QLabel *stepDot(int n)
    {
        auto *l = new QLabel(QString::number(n));
        l->setFixedSize(22, 22);
        l->setAlignment(Qt::AlignCenter);
        l->setStyleSheet("background:#2554d8; color:white; border-radius:11px; font-weight:800; font-size:12px;");
        return l;
    }

    // Saves a one-page A4 sheet (3 x 8 labels) with this barcode; returns false when the user cancels or it fails.
    bool saveLabelSheet(QWidget *parent, const QString &code, const QString &name)
    {
        const QString suggested = QDir(AppState::instance().dataDir()).filePath("labels-" + code + ".pdf");
        const QString path = QFileDialog::getSaveFileName(parent, "Save barcode label sheet", suggested, "PDF (*.pdf)");
        if (path.isEmpty())
            return false;
        QPdfWriter pdf(path);
        pdf.setPageSize(QPageSize(QPageSize::A4));
        pdf.setPageMargins(QMarginsF(10, 10, 10, 10), QPageLayout::Millimeter);
        pdf.setResolution(300);
        pdf.setCreator("Syntro-Check");
        QPainter p(&pdf);
        if (!p.isActive())
            return false;
        const QImage img = BarcodeArt::render(code, 4, 120, true);
        QFont f = p.font();
        f.setPixelSize(34);
        f.setBold(true);
        p.setFont(f);
        const int cols = 3, rows = 8;
        const QRect page = p.viewport();
        const int cw = page.width() / cols, ch = page.height() / rows;
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
            {
                const QRect cell = QRect(c * cw, r * ch, cw, ch).adjusted(12, 12, -12, -12);
                p.drawText(QRect(cell.left(), cell.top(), cell.width(), 44), Qt::AlignCenter, name.left(26));
                const QRect box = cell.adjusted(0, 48, 0, 0);
                const QSize s = img.size().scaled(box.size(), Qt::KeepAspectRatio);
                p.drawImage(QRect(box.center().x() - s.width() / 2, box.top(), s.width(), s.height()), img);
            }
        p.end();
        return true;
    }
}

namespace Dialogs
{
    bool editProduct(QWidget *parent, const ProductInfo *existing)
    {
        auto &app = AppState::instance();
        auto &store = UiStore::instance();
        const bool isNew = (existing == nullptr);
        const ProductInfo cur = existing ? *existing : ProductInfo();

        Ui::Modal d(parent, isNew ? "plus" : "edit",
                    isNew ? "Add New Product to Catalog" : "Edit Product",
                    isNew ? "Configure SKU specs, retail pricing and auto-generate the category-linked EAN-8 barcode"
                          : "Change the product details. The barcode stays the same.",
                    isNew ? "SKU ENTRY" : "EDIT", 900);

        auto *cols = new QHBoxLayout;
        cols->setSpacing(14);
        auto *left = new QVBoxLayout;
        left->setSpacing(12);

        // ------------------------------------------------ 1. category
        auto *c1 = W::card(nullptr, "subCard");
        auto *v1 = new QVBoxLayout(c1);
        v1->setContentsMargins(14, 12, 14, 12);
        v1->setSpacing(8);
        auto *h1 = new QHBoxLayout;
        h1->setSpacing(8);
        h1->addWidget(stepDot(1));
        h1->addWidget(W::label("Category Selection *", "h2"));
        h1->addStretch();
        h1->addWidget(W::label("DEFINES BARCODE PREFIX", "caption"));
        v1->addLayout(h1);

        auto *catBox = new QComboBox;
        const QString curCat = cur.ean.left(2);
        bool curListed = false;
        for (const auto &c : store.categories())
        {
            catBox->addItem(QString("%1 - %2").arg(c.id, c.name), c.id); // the Category Id becomes the barcode prefix
            if (c.id == curCat)
                curListed = true;
        }
        if (!isNew && !curListed && curCat.size() == 2)
            catBox->addItem(curCat + " - (category not configured)", curCat);
        if (isNew)
        {
            for (int i = 0; i < catBox->count(); ++i)
            {
                auto *m = qobject_cast<QStandardItemModel *>(catBox->model());
                if (!m || m->item(i)->isEnabled())
                {
                    catBox->setCurrentIndex(i);
                    break;
                }
            }
        }
        else
        {
            const int idx = catBox->findData(curCat);
            if (idx >= 0)
                catBox->setCurrentIndex(idx);
            catBox->setEnabled(false); // the category is part of the barcode, which is fixed
        }
        catBox->setMinimumHeight(38);
        v1->addWidget(catBox);
        auto *note = W::label("The first two digits of every barcode come automatically from the Category ID "
                              "(e.g. 10 for Dairy). Categories are managed on the Categories page.", "muted");
        note->setWordWrap(true);
        v1->addWidget(note);
        left->addWidget(c1);

        // ------------------------------------------------ 2. specifications
        auto *c2 = W::card(nullptr, "subCard");
        auto *v2 = new QVBoxLayout(c2);
        v2->setContentsMargins(14, 12, 14, 12);
        v2->setSpacing(8);
        auto *h2 = new QHBoxLayout;
        h2->setSpacing(8);
        h2->addWidget(stepDot(2));
        h2->addWidget(W::label("Product Specifications", "h2"));
        h2->addStretch();
        v2->addLayout(h2);

        auto *g = new QGridLayout;
        g->setHorizontalSpacing(10);
        g->setVerticalSpacing(4);
        g->setColumnStretch(0, 3);
        g->setColumnStretch(1, 3);
        g->setColumnStretch(2, 3);

        auto *idField = new QLineEdit(QString("PRD-%1").arg(isNew ? app.nextProductId() : cur.id, 4, 10, QChar('0')));
        idField->setReadOnly(true);
        idField->setMinimumHeight(36);
        auto *name = new QLineEdit(cur.name);
        name->setPlaceholderText("e.g. Olpers Full Cream Milk 1.5L");
        name->setMinimumHeight(36);
        g->addWidget(W::label("PRODUCT ID", "caption"), 0, 0);
        g->addWidget(W::label("PRODUCT NAME *", "caption"), 0, 1, 1, 2);
        g->addWidget(idField, 1, 0);
        g->addWidget(name, 1, 1, 1, 2);

        auto *acr = new QLineEdit(cur.sku);
        acr->setMaxLength(8);
        acr->setPlaceholderText("e.g. OLPMILK");
        acr->setMinimumHeight(36);
        auto *acrCount = W::label("", "caption");
        auto *acrHead = new QHBoxLayout;
        acrHead->setContentsMargins(0, 0, 0, 0);
        acrHead->addWidget(W::label("ACRONYM *", "caption"));
        acrHead->addStretch();
        acrHead->addWidget(acrCount);
        auto *acrHeadW = new QWidget;
        acrHeadW->setLayout(acrHead);
        g->addWidget(acrHeadW, 2, 0);
        g->addWidget(W::label("UNIT TYPE", "caption"), 2, 1, 1, 2);
        g->addWidget(acr, 3, 0);

        auto *unitRow = new QHBoxLayout;
        unitRow->setSpacing(6);
        auto *countBtn = W::button("Count / Unit", "tab");
        auto *weightBtn = W::button("Weight / Kg", "tab");
        countBtn->setCheckable(true);
        weightBtn->setCheckable(true);
        countBtn->setMinimumHeight(36);
        weightBtn->setMinimumHeight(36);
        auto *unitGroup = new QButtonGroup(&d);
        unitGroup->setExclusive(true);
        unitGroup->addButton(countBtn);
        unitGroup->addButton(weightBtn);
        (cur.unit == "weight" ? weightBtn : countBtn)->setChecked(true);
        unitRow->addWidget(countBtn, 1);
        unitRow->addWidget(weightBtn, 1);
        auto *unitW = new QWidget;
        unitW->setLayout(unitRow);
        unitRow->setContentsMargins(0, 0, 0, 0);
        g->addWidget(unitW, 3, 1, 1, 2);

        auto money = [&](double value)
        {
            auto *s = new QDoubleSpinBox;
            s->setPrefix("PKR ");
            s->setDecimals(2);
            s->setRange(0, 10000000);
            s->setValue(value);
            s->setMinimumHeight(36);
            return s;
        };
        auto *price = money(cur.price);
        auto *tax = money(cur.tax);
        auto *disc = money(cur.discount);
        g->addWidget(W::label("RETAIL PRICE (NO TAX)", "caption"), 4, 0);
        g->addWidget(W::label("TAX PER UNIT", "caption"), 4, 1);
        g->addWidget(W::label("DISCOUNT / UNIT", "caption"), 4, 2);
        g->addWidget(price, 5, 0);
        g->addWidget(tax, 5, 1);
        g->addWidget(disc, 5, 2);
        v2->addLayout(g);

        auto *gst = W::button("Set tax to 5% of price", "mini");
        QObject::connect(gst, &QPushButton::clicked, &d, [=] { tax->setValue(qRound64(price->value() * 5.0) / 100.0); });
        v2->addWidget(gst, 0, Qt::AlignLeft);

        // stock stepper
        auto *stockRow = new QHBoxLayout;
        stockRow->setSpacing(8);
        auto *stockTxt = new QVBoxLayout;
        stockTxt->setSpacing(0);
        stockTxt->addWidget(W::label(isNew ? "Initial Stock Volume" : "Stock on hand", "h2"));
        stockTxt->addWidget(W::label(isNew ? "Register the starting quantity" : "Available quantity", "muted"));
        stockRow->addLayout(stockTxt, 1);
        auto *minus = W::button("−", "step");
        auto *plus = W::button("+", "step");
        auto *stock = new QSpinBox;
        stock->setRange(0, 1000000);
        stock->setValue(cur.stock);
        stock->setButtonSymbols(QAbstractSpinBox::NoButtons);
        stock->setAlignment(Qt::AlignCenter);
        stock->setMinimumWidth(80);
        stock->setMinimumHeight(36);
        QObject::connect(minus, &QPushButton::clicked, &d, [=] { stock->setValue(stock->value() - 1); });
        QObject::connect(plus, &QPushButton::clicked, &d, [=] { stock->setValue(stock->value() + 1); });
        stockRow->addWidget(minus);
        stockRow->addWidget(stock);
        stockRow->addWidget(plus);
        v2->addLayout(stockRow);
        left->addWidget(c2);
        left->addStretch();
        cols->addLayout(left, 6);

        // ------------------------------------------------ barcode engine (right)
        auto *c3 = W::card(nullptr, "subCard");
        auto *v3 = new QVBoxLayout(c3);
        v3->setContentsMargins(14, 12, 14, 12);
        v3->setSpacing(8);
        auto *h3 = new QHBoxLayout;
        h3->setSpacing(8);
        h3->addWidget(W::label("EAN-8 Barcode Engine", "h2"));
        h3->addStretch();
        auto *engineBadge = W::badge("Ready", "green");
        h3->addWidget(engineBadge);
        v3->addLayout(h3);

        auto *gen = W::button(isNew ? "Generate Barcode" : "Barcode locked", "primary", "bolt");
        gen->setMinimumHeight(40);
        gen->setEnabled(isNew);
        v3->addWidget(gen);

        auto *preview = new QLabel;
        preview->setAlignment(Qt::AlignCenter);
        preview->setMinimumHeight(96);
        preview->setStyleSheet("background:white; border:1px solid #dbe4f7; border-radius:10px;");
        v3->addWidget(preview);

        auto *digits = new QHBoxLayout;
        digits->setSpacing(6);
        digits->addStretch();
        auto *dPre = W::label("", "mono");
        auto *dSer = W::label("", "mono");
        auto *dChk = W::label("", "mono");
        dPre->setStyleSheet("background:#dce1ff; color:#1d46b4; font-weight:800; font-size:17px; padding:3px 8px; border-radius:6px;");
        dSer->setStyleSheet("font-weight:800; font-size:17px; padding:3px 4px;");
        dChk->setStyleSheet("background:#85f8c4; color:#005137; font-weight:800; font-size:17px; padding:3px 8px; border-radius:6px;");
        digits->addWidget(dPre);
        digits->addWidget(dSer);
        digits->addWidget(dChk);
        digits->addStretch();
        v3->addLayout(digits);

        auto kv = [&](const QString &k, QLabel *&val)
        {
            auto *r = new QHBoxLayout;
            r->addWidget(W::label(k, "muted"));
            r->addStretch();
            val = W::label("—", "mono");
            val->setStyleSheet("font-weight:700;");
            r->addWidget(val);
            v3->addLayout(r);
        };
        QLabel *kPrefix = nullptr, *kSerial = nullptr, *kCheck = nullptr;
        kv("Category prefix", kPrefix);
        kv("Item serial", kSerial);
        kv("Mod-10 check digit", kCheck);
        auto *pathLbl = W::label("", "muted");
        pathLbl->setWordWrap(true);
        v3->addWidget(pathLbl);

        auto *sheet = W::button("Print Barcode Label Sheet", "soft", "print");
        sheet->setMinimumHeight(38);
        v3->addWidget(sheet);
        v3->addStretch();
        cols->addWidget(c3, 4);
        d.body()->addLayout(cols);

        auto *errLbl = W::label("", "muted");
        errLbl->setWordWrap(true);
        errLbl->setStyleSheet("color:#ba1a1a; font-weight:700; background:#ffdad6; border-radius:8px; padding:8px 10px;");
        errLbl->hide();
        d.body()->addWidget(errLbl);

        // ------------------------------------------------ behaviour
        QString code = isNew ? QString() : cur.ean;
        auto showCode = [&]
        {
            const bool have = code.size() == 8;
            dPre->setText(have ? code.left(2) : "--");
            dSer->setText(have ? code.mid(2, 5) : "-----");
            dChk->setText(have ? code.right(1) : "-");
            kPrefix->setText(have ? code.left(2) : "—");
            kSerial->setText(have ? code.mid(2, 5) : "—");
            kCheck->setText(have ? code.right(1) + "  (valid)" : "—");
            pathLbl->setText(have ? "Saved as data/barcodes/" + code + ".png" : "No barcode yet - pick a category.");
            sheet->setEnabled(have);
            if (have)
                preview->setPixmap(QPixmap::fromImage(BarcodeArt::render(code, 2, 70, false)));
            else
                preview->setText("—");
            W::setBadge(engineBadge, have ? "Ready" : "Waiting", have ? "green" : "grey");
        };
        auto generate = [&]
        {
            if (!isNew)
                return;
            code = app.nextBarcode(catBox->currentData().toString());
            if (code.isEmpty())
            {
                errLbl->setText("No free barcode left in this category (serial 00001 - 99999 is used up).");
                errLbl->show();
            }
            else
                errLbl->hide();
            showCode();
        };
        auto suggestAcronym = [&]
        {
            QString s = name->text().toUpper();
            s.remove(QRegularExpression("[^A-Z0-9]"));
            return s.left(8);
        };
        bool acrTouched = !isNew;
        auto updateCount = [&] { acrCount->setText(QString("[ %1 / 8 max ]").arg(acr->text().size())); };
        QObject::connect(acr, &QLineEdit::textEdited, &d, [&](const QString &) { acrTouched = true; });
        QObject::connect(acr, &QLineEdit::textChanged, &d, [&](const QString &) { updateCount(); });
        QObject::connect(name, &QLineEdit::textChanged, &d, [&](const QString &)
                         {
            if (!acrTouched)
                acr->setText(suggestAcronym()); });
        QObject::connect(catBox, qOverload<int>(&QComboBox::currentIndexChanged), &d, [&](int) { generate(); });
        QObject::connect(gen, &QPushButton::clicked, &d, [&] { generate(); });
        QObject::connect(sheet, &QPushButton::clicked, &d, [&]
                         {
            if (code.size() == 8 && saveLabelSheet(&d, code, name->text().trimmed().isEmpty() ? acr->text() : name->text().trimmed()))
                errLbl->hide(); });

        // footer: Cancel [Esc] ... hint ... Save [Enter]
        auto *cancel = W::button("Cancel  [Esc]", "", "close");
        auto *hint = W::label("Press Enter to save instantly", "muted");
        auto *save = W::button(isNew ? "Save Product to Inventory" : "Save Changes", "primary", "check");
        save->setMinimumHeight(40);
        save->setDefault(true);
        d.footer()->addWidget(cancel);
        d.footer()->addStretch();
        d.footer()->addWidget(hint);
        d.footer()->addWidget(save);
        QObject::connect(cancel, &QPushButton::clicked, &d, &QDialog::reject);
        QObject::connect(save, &QPushButton::clicked, &d, [&]
                         {
            ProductInfo p = cur;
            p.name = name->text().trimmed();
            p.sku = acr->text().trimmed().toUpper();
            p.unit = weightBtn->isChecked() ? "weight" : "count";
            p.price = price->value();
            p.tax = tax->value();
            p.discount = disc->value();
            p.stock = stock->value();
            p.ean = code;
            p.categoryId = code.left(2); // requirement 2: the Category Id is stored in Product.txt
            QString err;
            const bool ok = isNew ? app.addProduct(p, &err) : app.updateProduct(p, &err);
            if (!ok)
            {
                errLbl->setText(err.isEmpty() ? QString("The product could not be saved.") : err);
                errLbl->show();
                return;
            }
            d.accept(); });

        updateCount();
        if (isNew)
            generate();
        else
            showCode();
        name->setFocus();
        return d.exec() == QDialog::Accepted;
    }
}
