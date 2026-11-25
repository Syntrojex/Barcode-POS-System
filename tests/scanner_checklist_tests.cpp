#include "AppState.h"
#include "MainWindow.h"
#include "RecordOrderPage.h"
#include "ScanAudio.h"
#include "ScannerPage.h"
#include "Theme.h"
#include "Widgets.h"

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QDialog>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

namespace
{
    bool writeSampleData(const QString &dirPath)
    {
        QDir dir(dirPath);
        if (!dir.exists() && !dir.mkpath("."))
            return false;

        QFile products(dir.filePath("Product.txt"));
        if (!products.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        products.write(
            "1 01001020 OLPMILK Olpers Full Cream Milk 1L count 340.00 0.00 10.00 42 data/barcodes/01001020.png#\n"
            "7 01007350 EGGS12 Farm Fresh Eggs Dozen count 390.00 0.00 0.00 4 data/barcodes/01007350.png#\n"
            "8 01008418 NESTWATR Nestle Pure Life Water 1.5L count 110.00 8.00 0.00 150 data/barcodes/01008418.png#\n"
            "11 01011722 COLA15 Coca Cola 1.5L count 230.00 18.00 0.00 0 data/barcodes/01011722.png#\n");
        products.close();

        for (const char *emptyName : {"Order.txt", "OrderDetail.txt", "Bill.txt", "Seasons.txt"})
        {
            QFile f(dir.filePath(QString::fromLatin1(emptyName)));
            if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
                return false;
            f.close();
        }
        return true;
    }
}

class ScannerChecklistTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void groupedScanChecklistAndQuantitySteppers();
    void sendCheckedBatchToCollectiveCheckout();
    void stockClampingAndOutOfStockHandling();
    void mainWindowRoutesBatchToRecordOrder();
    void partialTransfersRemainAvailableForNextOrder();
    void scannerAudioForAcceptedScansOnly();
    void twoOrdersFromOneChecklist();

private:
    QTemporaryDir m_tempDataDir;
};

void ScannerChecklistTests::initTestCase()
{
    QVERIFY(m_tempDataDir.isValid());
    QVERIFY(writeSampleData(m_tempDataDir.path()));
    qputenv("BARCODEHUB_DATA", m_tempDataDir.path().toUtf8());

    auto &state = AppState::instance();
    QCOMPARE(state.products().size(), 4);
}

void ScannerChecklistTests::groupedScanChecklistAndQuantitySteppers()
{
    ScannerPage scanner;
    auto *feed = scanner.findChild<CameraFeedWidget *>();
    auto *manual = scanner.findChild<QLineEdit *>("scannerManualInput");
    auto *table = scanner.findChild<QTableWidget *>("scannerChecklist");
    auto *sendBtn = scanner.findChild<QPushButton *>("scannerAddChecked");
    QVERIFY(feed != nullptr);
    QVERIFY(manual != nullptr);
    QVERIFY(table != nullptr);
    QVERIFY(sendBtn != nullptr);
    QVERIFY(!sendBtn->isEnabled());

    // Scan Water 3 times (matching duplicate camera/manual scans) -> grouped into 1 row with qty 3
    emit feed->barcodeDetected("01008418");
    emit feed->barcodeDetected("01008418");
    manual->setText("01008418");
    QTest::keyClick(manual, Qt::Key_Return);
    QVERIFY(manual->text().isEmpty());

    QCOMPARE(scanner.entries().size(), 1);
    QCOMPARE(scanner.entries().at(0).product.ean, QString("01008418"));
    QCOMPARE(scanner.entries().at(0).qty, 3);
    QVERIFY(scanner.entries().at(0).checked);
    QCOMPARE(table->rowCount(), 1);

    // Scan Eggs (1) and Milk (1) -> 3 grouped rows, 5 total units
    emit feed->barcodeDetected("01007350");
    emit feed->barcodeDetected("01001020");
    QCOMPARE(scanner.entries().size(), 3);
    QCOMPARE(table->rowCount(), 3);
    QCOMPARE(sendBtn->text(), QString("Add checked to checkout (5)"));
    QVERIFY(sendBtn->isEnabled());

    // Adjust Milk qty (row 0) from 1 -> 2 using the row's QtyStepper '+' button
    auto *stepper = qobject_cast<QtyStepper *>(table->cellWidget(0, 2));
    QVERIFY(stepper != nullptr);
    emit stepper->plus();
    QCOMPARE(scanner.entries().at(0).qty, 2);
    QCOMPARE(sendBtn->text(), QString("Add checked to checkout (6)"));

    // Uncheck row 1 (Eggs) -> only Milk (2) + Water (3) remain checked (5 units)
    auto *row1Cell = table->cellWidget(1, 0);
    QVERIFY(row1Cell != nullptr);
    auto *cb1 = row1Cell->findChild<QCheckBox *>();
    QVERIFY(cb1 != nullptr);
    cb1->setChecked(false);
    QCOMPARE(sendBtn->text(), QString("Add checked to checkout (5)"));

    // Select all restores Eggs -> 6 units
    scanner.selectAll();
    QCOMPARE(sendBtn->text(), QString("Add checked to checkout (6)"));

    // Deselect all -> disabled
    scanner.deselectAll();
    QVERIFY(!sendBtn->isEnabled());
}

void ScannerChecklistTests::sendCheckedBatchToCollectiveCheckout()
{
    ScannerPage scanner;
    RecordOrderPage recordOrder;
    AppState::instance().settings().autoAddScans = false; // collective batch must add regardless!

    connect(&scanner, &ScannerPage::sendBatchToOrder, &recordOrder,
            [&](const QVector<QPair<QString, int>> &items)
            { scanner.removeTransferredItems(recordOrder.addScannedBatch(items)); });

    const auto water = AppState::instance().findByCode("01008418");
    const auto eggs = AppState::instance().findByCode("01007350");
    const auto milk = AppState::instance().findByCode("01001020");
    QVERIFY(water && eggs && milk);

    scanner.recordScan(*water);
    scanner.recordScan(*water);
    scanner.recordScan(*water); // qty 3 @ (110 + 8) = 354
    scanner.recordScan(*eggs);  // qty 1 @ 390 = 390
    scanner.recordScan(*milk);  // qty 1 @ (340 - 10) = 330

    // Uncheck Eggs (row 1) so only Milk (1) and Water (3) are sent
    auto *table = scanner.findChild<QTableWidget *>("scannerChecklist");
    QVERIFY(table != nullptr);
    auto *eggsCb = table->cellWidget(1, 0)->findChild<QCheckBox *>();
    QVERIFY(eggsCb != nullptr);
    eggsCb->setChecked(false);

    QSignalSpy batchSpy(&scanner, &ScannerPage::sendBatchToOrder);
    scanner.sendCheckedToOrder();
    QCOMPARE(batchSpy.count(), 1);

    // Checked rows are cleared from staging; unchecked Eggs remains in checklist
    QCOMPARE(scanner.entries().size(), 1);
    QCOMPARE(scanner.entries().at(0).product.ean, QString("01007350"));
    QVERIFY(!scanner.entries().at(0).checked);

    // RecordOrderPage cart now contains Milk (1) + Water (3) = 4 units, collective net payable = 684.00
    auto *itemsBadge = recordOrder.findChild<QTableWidget *>();
    QVERIFY(itemsBadge != nullptr);
    QCOMPARE(itemsBadge->rowCount(), 2);

    auto *payable = recordOrder.findChild<QLabel *>("checkoutPayable");
    QVERIFY(payable != nullptr);
    QCOMPARE(payable->text(), QString("PKR 684.00"));
}

void ScannerChecklistTests::stockClampingAndOutOfStockHandling()
{
    ScannerPage scanner;
    const auto outOfStock = AppState::instance().findByCode("01011722");
    const auto eggs = AppState::instance().findByCode("01007350");
    QVERIFY(outOfStock && eggs);

    scanner.recordScan(*outOfStock);
    QCOMPARE(scanner.entries().size(), 1);
    QVERIFY(!scanner.entries().at(0).checked);

    auto *table = scanner.findChild<QTableWidget *>("scannerChecklist");
    QVERIFY(table != nullptr);
    auto *cb = table->cellWidget(0, 0)->findChild<QCheckBox *>();
    QVERIFY(cb != nullptr);
    QVERIFY(!cb->isEnabled());

    // Scanning Eggs 5 times clamps at stock 4
    for (int i = 0; i < 5; ++i)
        scanner.recordScan(*eggs);
    QCOMPARE(scanner.entries().at(0).qty, 4);

    // If cart already holds 2 Eggs and batch requests 4, RecordOrderPage clamps addition to 2
    RecordOrderPage recordOrder;
    recordOrder.addScannedBatch({{"01007350", 2}});
    recordOrder.addScannedBatch({{"01007350", 4}});
    auto *payable = recordOrder.findChild<QLabel *>("checkoutPayable");
    QVERIFY(payable != nullptr);
    QCOMPARE(payable->text(), QString("PKR 1,560.00")); // 4 * 390
}

void ScannerChecklistTests::mainWindowRoutesBatchToRecordOrder()
{
    const bool originalDark = Theme::isDark();
    Theme::setDark(true);
    MainWindow win(UserRole::Cashier, "Cashier Test");
    auto *scanner = win.findChild<ScannerPage *>();
    QVERIFY(scanner != nullptr);

    const auto milk = AppState::instance().findByCode("01001020");
    const auto water = AppState::instance().findByCode("01008418");
    QVERIFY(milk && water);

    scanner->recordScan(*milk);
    scanner->recordScan(*water);
    scanner->sendCheckedToOrder();

    QCOMPARE(win.recordPage()->findChild<QLabel *>("checkoutPayable")->text(), QString("PKR 448.00"));
    Theme::setDark(originalDark);
}

void ScannerChecklistTests::partialTransfersRemainAvailableForNextOrder()
{
    MainWindow window(UserRole::Cashier, "Cashier Test");
    auto *scanner = window.findChild<ScannerPage *>();
    auto *order = window.recordPage();
    const auto eggs = AppState::instance().findByCode("01007350");
    QVERIFY(scanner && order && eggs);

    order->addScannedBatch({{eggs->ean, 3}});
    for (int i = 0; i < 4; ++i)
        scanner->recordScan(*eggs);
    scanner->sendCheckedToOrder();
    QCOMPARE(scanner->entries().size(), 1);
    QCOMPARE(scanner->entries().first().qty, 3);
    QCOMPARE(order->findChild<QLabel *>("checkoutPayable")->text(), QString("PKR 1,560.00"));

    scanner->sendCheckedToOrder();
    QCOMPARE(scanner->entries().first().qty, 3);
    order->holdOrder();
    scanner->sendCheckedToOrder();
    QVERIFY(scanner->entries().isEmpty());
    QCOMPARE(order->findChild<QLabel *>("checkoutPayable")->text(), QString("PKR 1,170.00"));
    QVERIFY(!AppState::instance().heldOrders().isEmpty());
}

void ScannerChecklistTests::scannerAudioForAcceptedScansOnly()
{
    ScannerPage scanner;
    auto *feed = scanner.findChild<CameraFeedWidget *>();
    auto *manual = scanner.findChild<QLineEdit *>("scannerManualInput");
    auto &audio = ScanAudio::instance();
    auto &settings = AppState::instance().settings();
    const bool beep = settings.beepOnScan;
    QSignalSpy feedback(&audio, &ScanAudio::feedbackTriggered);
    settings.beepOnScan = true;
    audio.resetDebounce();
    emit feed->barcodeDetected("99999999");
    emit feed->barcodeDetected("01011722");
    QCOMPARE(feedback.count(), 0);

    emit feed->barcodeDetected("01001020");
    QCOMPARE(feedback.count(), 1);
    audio.resetDebounce();
    manual->setText("01008418");
    QTest::keyClick(manual, Qt::Key_Return);
    QCOMPARE(feedback.count(), 2);

    settings.beepOnScan = false;
    audio.resetDebounce();
    emit feed->barcodeDetected("01008418");
    QCOMPARE(feedback.count(), 2);
    settings.beepOnScan = beep;
}

void ScannerChecklistTests::twoOrdersFromOneChecklist()
{
    auto &state = AppState::instance();
    const auto milk = state.findByCode("01001020");
    const auto water = state.findByCode("01008418");
    QVERIFY(milk && water);
    MainWindow window(UserRole::Cashier, "Cashier Test");
    auto *scanner = window.findChild<ScannerPage *>();
    auto *order = window.recordPage();
    auto *table = scanner->findChild<QTableWidget *>("scannerChecklist");
    scanner->recordScan(*milk);
    scanner->recordScan(*water);
    scanner->recordScan(*water);
    table->cellWidget(1, 0)->findChild<QCheckBox *>()->setChecked(false);
    scanner->sendCheckedToOrder();
    QCOMPARE(scanner->entries().size(), 1);
    QCOMPARE(scanner->entries().first().product.ean, milk->ean);

    auto settleCash = [order] {
        int ticks = 0;
        bool receipt = false;
        QTimer timer;
        QObject::connect(&timer, &QTimer::timeout, order, [&] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            if (dialog->windowTitle().startsWith("Bill - "))
            {
                receipt = true;
                dialog->accept();
                return;
            }
            for (auto *button : dialog->findChildren<QPushButton *>())
                if (button->isEnabled() && button->text().startsWith("Confirm Payment"))
                {
                    button->click();
                    return;
                }
            if (++ticks > 20)
                dialog->reject();
        });
        timer.start(50);
        order->checkout();
        return receipt;
    };

    const auto before = state.orders().size();
    QVERIFY(settleCash());
    QCOMPARE(state.orders().size(), before + 1);
    QString firstId, firstBill;
    for (const auto &saved : state.orders())
        if (saved.status == "Paid")
        {
            firstId = saved.id;
            firstBill = saved.billId;
            QCOMPARE(saved.itemCount(), 2);
        }
    QVERIFY(!firstId.isEmpty() && !firstBill.isEmpty());
    QCOMPARE(scanner->entries().size(), 1);
    scanner->selectAll();
    scanner->sendCheckedToOrder();
    QVERIFY(scanner->entries().isEmpty());
    QVERIFY(settleCash());
    state.reloadAll();
    QCOMPARE(state.orders().size(), before + 2);
    QVERIFY(!firstBill.isEmpty());
    bool secondOrderFound = false;
    for (const auto &saved : state.orders())
        if (saved.status == "Paid" && saved.id != firstId)
        {
            QVERIFY(!saved.billId.isEmpty() && saved.billId != firstBill);
            QCOMPARE(saved.itemCount(), 1);
            secondOrderFound = true;
        }
    QVERIFY(secondOrderFound);
    QCOMPARE(state.findByCode(milk->ean)->stock, milk->stock - 1);
    QCOMPARE(state.findByCode(water->ean)->stock, water->stock - 2);
}

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    ScannerChecklistTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "scanner_checklist_tests.moc"
