// "New Product" / "Edit Product" dialog (manager): a simple form on the left, the barcode on the right.
// The barcode is generated automatically from the chosen category; saving goes through AppState::addProduct /
// updateProduct, which write Product.txt (with the Category Id) and the barcode PNG.
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

        Ui::Modal d(parent, isNew ? "plus" : "edit", isNew ? "New Product" : "Edit Product",
                    isNew ? "Choose a category and fill in the details - the barcode is created for you."
                          : "Barcode and category stay the same.",
                    QString(), 780);

        // small helpers: a caption above a field, and a money box without spin arrows
        auto field = [](const QString &caption, QWidget *w)
        {
            auto *box = new QVBoxLayout;
            box->setSpacing(5);
            box->addWidget(W::label(caption, "caption"));
            box->addWidget(w);
            return box;
        };
        auto money = [](double value)
        {
            auto *s = new QDoubleSpinBox;
            s->setDecimals(2);
            s->setRange(0, 10000000);
            s->setValue(value);
            s->setButtonSymbols(QAbstractSpinBox::NoButtons);
            s->setMinimumHeight(38);
            return s;
        };

        auto *cols = new QHBoxLayout;
        cols->setSpacing(18);

        // ------------------------------------------------ form (left)
        auto *form = new QVBoxLayout;
        form->setSpacing(14);

        auto *catBox = new QComboBox;
        catBox->setMinimumHeight(38);
        const QString curCat = cur.categoryId.isEmpty() ? cur.ean.left(2) : cur.categoryId;
        for (const auto &c : store.categories())
            catBox->addItem(c.id + "  ·  " + c.name, c.id); // the Category Id becomes the barcode prefix
        if (!isNew && catBox->findData(curCat) < 0 && curCat.size() == 2)
            catBox->addItem(curCat + "  ·  (not in PCategory.txt)", curCat);
        if (!isNew)
        {
            catBox->setCurrentIndex(qMax(0, catBox->findData(curCat)));
            catBox->setEnabled(false); // the category is part of the barcode, which is fixed
        }
        form->addLayout(field("Category", catBox));

        auto *name = new QLineEdit(cur.name);
        name->setPlaceholderText("e.g. Olpers Full Cream Milk 1.5L");
        name->setMinimumHeight(38);
        form->addLayout(field("Product name", name));

        auto *row1 = new QHBoxLayout;
        row1->setSpacing(12);
        auto *acr = new QLineEdit(cur.sku);
        acr->setMaxLength(8);
        acr->setPlaceholderText("Up to 8 letters");
        acr->setMinimumHeight(38);
        row1->addLayout(field("Acronym", acr), 1);
        auto *unitRow = new QHBoxLayout;
        unitRow->setSpacing(6);
        auto *countBtn = W::button("Count", "tab");
        auto *weightBtn = W::button("Weight", "tab");
        auto *unitGroup = new QButtonGroup(&d);
        for (auto *b : {countBtn, weightBtn})
        {
            b->setCheckable(true);
            b->setMinimumHeight(38);
            unitGroup->addButton(b);
            unitRow->addWidget(b, 1);
        }
        (cur.unit == "weight" ? weightBtn : countBtn)->setChecked(true);
        auto *unitW = new QWidget;
        unitW->setLayout(unitRow);
        unitRow->setContentsMargins(0, 0, 0, 0);
        row1->addLayout(field("Sold by", unitW), 1);
        form->addLayout(row1);

        auto *row2 = new QHBoxLayout;
        row2->setSpacing(12);
        auto *price = money(cur.price);
        auto *tax = money(cur.tax);
        auto *disc = money(cur.discount);
        row2->addLayout(field("Price (PKR, no tax)", price), 1);
        row2->addLayout(field("Tax (PKR)", tax), 1);
        row2->addLayout(field("Discount (PKR)", disc), 1);
        form->addLayout(row2);

        auto *row3 = new QHBoxLayout;
        row3->setSpacing(12);
        auto *stock = new QSpinBox;
        stock->setRange(0, 1000000);
        stock->setValue(cur.stock);
        stock->setButtonSymbols(QAbstractSpinBox::NoButtons);
        stock->setMinimumHeight(38);
        row3->addLayout(field(isNew ? "Opening stock" : "Stock", stock), 1);
        row3->addStretch(2);
        form->addLayout(row3);
        form->addStretch();
        cols->addLayout(form, 3);

        // ------------------------------------------------ barcode (right)
        auto *card = W::card(nullptr, "subCard");
        auto *cv = new QVBoxLayout(card);
        cv->setContentsMargins(16, 14, 16, 14);
        cv->setSpacing(8);
        cv->addWidget(W::label("Barcode", "caption"));
        auto *preview = new QLabel;
        preview->setAlignment(Qt::AlignCenter);
        preview->setMinimumHeight(92);
        preview->setStyleSheet("background:white; border-radius:8px;");
        cv->addWidget(preview);
        auto *codeLbl = W::label("", "mono");
        codeLbl->setAlignment(Qt::AlignCenter);
        codeLbl->setStyleSheet("font-size:20px; font-weight:800; letter-spacing:3px;");
        cv->addWidget(codeLbl);
        auto *catLbl = W::label("", "muted");
        catLbl->setAlignment(Qt::AlignCenter);
        catLbl->setWordWrap(true);
        cv->addWidget(catLbl);
        cv->addStretch();
        auto *sheet = W::button("Print labels", "flat", "print");
        cv->addWidget(sheet, 0, Qt::AlignHCenter);
        card->setMinimumWidth(250);
        cols->addWidget(card, 2);
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
            codeLbl->setText(have ? code : "--------");
            const Category *c = store.category(have ? code.left(2) : catBox->currentData().toString());
            catLbl->setText(have ? "Starts with category " + code.left(2) + (c ? " (" + c->name + ")" : QString()) : QString());
            sheet->setEnabled(have);
            if (have)
                preview->setPixmap(QPixmap::fromImage(BarcodeArt::render(code, 2, 70, false)));
            else
                preview->setText("Pick a category");
        };
        auto generate = [&] // new products: a fresh barcode for the chosen category, no button needed
        {
            if (!isNew)
                return;
            code = app.nextBarcode(catBox->currentData().toString());
            errLbl->setVisible(code.isEmpty());
            if (code.isEmpty())
                errLbl->setText("This category has no free barcodes left.");
            showCode();
        };
        auto suggestAcronym = [&]
        {
            QString s = name->text().toUpper();
            s.remove(QRegularExpression("[^A-Z0-9]"));
            return s.left(8);
        };
        bool acrTouched = !isNew;
        QObject::connect(acr, &QLineEdit::textEdited, &d, [&](const QString &) { acrTouched = true; });
        QObject::connect(name, &QLineEdit::textChanged, &d, [&](const QString &)
                         {
            if (!acrTouched)
                acr->setText(suggestAcronym()); });
        QObject::connect(catBox, qOverload<int>(&QComboBox::currentIndexChanged), &d, [&](int) { generate(); });
        QObject::connect(sheet, &QPushButton::clicked, &d, [&]
                         {
            if (code.size() == 8)
                saveLabelSheet(&d, code, name->text().trimmed().isEmpty() ? acr->text() : name->text().trimmed()); });

        auto *cancel = W::button("Cancel", "flat");
        auto *save = W::button(isNew ? "Add Product" : "Save Changes", "primary", "check");
        save->setMinimumHeight(40);
        save->setDefault(true); // Enter saves
        d.footer()->addStretch();
        d.footer()->addWidget(cancel);
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

        if (isNew)
            generate();
        else
            showCode();
        name->setFocus();
        return d.exec() == QDialog::Accepted;
    }
}
