#include "AppState.h"
#include "DarkModeManager.h"
#include "DashboardPage.h"
#include "Dialogs.h"
#include "MainWindow.h"
#include "SettingsPage.h"
#include "Theme.h"
#include "Widgets.h"
#include "billing/BillRecordCodec.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QLineEdit>
#include <QSettings>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

namespace
{
    QPushButton *button(QWidget &parent, const QString &text)
    {
        for (auto *control : parent.findChildren<QPushButton *>())
            if (control->text() == text)
                return control;
        return nullptr;
    }

    template<typename T>
    T *setting(QWidget &page, const QString &name)
    {
        for (auto *control : page.findChildren<T *>())
            if (control->accessibleName() == name)
                return control;
        return nullptr;
    }

    bool writeFile(const QDir &dir, const QString &name, const QByteArray &contents)
    {
        QFile file(dir.filePath(name));
        return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
    }
}

class TerminalUiTests : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void revenueUsesPaymentTimeForBothRoles();
    void preferencesReachCheckoutAndReceipt();
private:
    QTemporaryDir m_fixture;
};

void TerminalUiTests::initTestCase()
{
    QVERIFY(m_fixture.isValid());
    // The app uses explicit organization/application names and the default settings format.
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_fixture.filePath("preferences"));
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, m_fixture.filePath("system"));
    qputenv("BARCODEHUB_DATA", m_fixture.path().toUtf8());
    const QDir dir(m_fixture.path());
    QVERIFY(writeFile(dir, "Product.txt",
                      "1 01001020 OLPMILK Olpers Milk 1L count 340.00 0.00 10.00 42 data/barcodes/01001020.png#\n"));
    QVERIFY(writeFile(dir, "Seasons.txt", {}));
    const auto today = QDate::currentDate();
    QByteArray orders, details, bills, billDetails;
    auto add = [&](int index, QDate paid, const char *time, const char *status, int amount)
    {
        const auto orderId = QByteArray::number(26110100 + index);
        const auto billId = QByteArray::number(261101000 + index);
        const auto created = paid.addDays(-2).toString(Qt::ISODate).toUtf8();
        orders += orderId + " " + created + " 12:00:00 QA - " + status + " " + QByteArray::number(amount) + " cash - #\n";
        details += orderId + " 1 01001020 1 #\n";
        barcodehub::billing::Bill bill;
        bill.id = billId.toStdString();
        bill.orderId = orderId.toStdString();
        bill.customerName = "QA";
        bill.paymentStatus = status;
        bill.date = paid.toString(Qt::ISODate).toStdString();
        bill.time = time;
        bill.amountReceived = amount;
        bill.totals.withoutTax = bill.totals.payable = amount;
        bills += QByteArray::fromStdString(barcodehub::billing::BillRecordCodec::format(bill)) + '\n';
        billDetails += QByteArray::fromStdString(barcodehub::billing::BillRecordCodec::formatDetail(
            bill.id, {"OLPMILK", double(amount), 0, 0, 1})) + '\n';
    };
    add(1, today, "00:05:00", "paid", 330);
    add(2, today, "23:59:59", "paid", 650);
    add(3, today.addDays(-3), "10:00:00", "paid", 510);
    add(4, today, "11:10:00", "archived", 2000);
    add(5, today.addDays(-7), "18:30:00", "paid", 1500);
    QVERIFY(writeFile(dir, "Order.txt", orders));
    QVERIFY(writeFile(dir, "OrderDetail.txt", details));
    QVERIFY(writeFile(dir, "Bill.txt", bills));
    QVERIFY(writeFile(dir, "BillDetail.txt", billDetails));
    qApp->setStyleSheet(Theme::styleSheet());
}

void TerminalUiTests::revenueUsesPaymentTimeForBothRoles()
{
    for (const auto role : {UserRole::Manager, UserRole::Cashier})
    {
        MainWindow window(role, "Dashboard QA");
        window.resize(1100, 700);
        window.show();
        QVERIFY(window.goTo(MainWindow::Dashboard));
        auto *chart = window.findChild<RevenueChart *>("dashboardRevenue");
        QVERIFY(chart);
        QVERIFY(chart->isVisible());
        QCOMPARE(chart->accessibleName(), "Hourly revenue");
        const auto hourly = chart->accessibleDescription();
        QVERIFY(hourly.contains("00:00  PKR 330.00"));
        QVERIFY(hourly.contains("23:00  PKR 650.00"));
        QVERIFY(!hourly.contains("2,000") && !hourly.contains("510"));
        auto *week = button(window, "Last 7 days");
        QVERIFY(week);
        week->click();
        QCOMPARE(chart->accessibleName(), "Daily revenue");
        const auto daily = chart->accessibleDescription();
        QVERIFY(daily.contains("PKR 980.00") && daily.contains("PKR 510.00"));
        QVERIFY(!daily.contains("1,500") && !daily.contains("2,000"));
        if (qEnvironmentVariableIsSet("SYNTRO_QA_SCREENSHOTS"))
        {
            for (const bool dark : {false, true})
            {
                barcodehub::ui::DarkModeManager::instance().setDarkMode(dark);
                for (const QSize size : {QSize(1100, 700), QSize(1440, 900)})
                {
                    window.resize(size);
                    QApplication::processEvents();
                    const auto path = QDir(qEnvironmentVariable("SYNTRO_QA_SCREENSHOTS")).filePath(
                        QString("%1-%2-%3-dashboard.png").arg(RolePolicy::name(role)).arg(size.width())
                            .arg(dark ? "dark" : "light"));
                    QVERIFY(window.grab().save(path));
                }
            }
        }
    }

    // Empty and fully discounted periods still provide meaningful chart feedback.
    RevenueChart empty;
    empty.setData({"00:00"}, {0.0}, false);
    QVERIFY(empty.accessibleDescription().contains("No paid orders"));
    empty.setData({"00:00"}, {0.0}, true);
    QVERIFY(empty.accessibleDescription().contains("PKR 0.00"));

    DashboardPage live;
    auto *chart = live.findChild<RevenueChart *>("dashboardRevenue");
    QVERIFY(chart);
    const auto beforeSale = chart->accessibleDescription();
    OrderRecord sale;
    sale.customer = "Dashboard test sale";
    sale.payment = "Cash";
    sale.tendered = 500;
    sale.lines.push_back({"Milk", "OLPMILK", 1, 340, 0, 10, 1});
    QString error;
    QVERIFY2(AppState::instance().addOrder(sale, &error), qPrintable(error));
    QVERIFY(chart->accessibleDescription() != beforeSale);
    auto *week = button(live, "Last 7 days");
    QVERIFY(week);
    week->click();
    QVERIFY(chart->accessibleDescription().contains("PKR 1,310.00"));
}

void TerminalUiTests::preferencesReachCheckoutAndReceipt()
{
    MainWindow window(UserRole::Manager, "Settings QA");
    window.resize(1100, 700);
    window.show();
    QVERIFY(window.goTo(MainWindow::Settings));
    auto *page = window.findChild<SettingsPage *>();
    auto *tabs = page->findChild<QTabWidget *>();
    QCOMPARE(tabs->count(), 6);
    auto *payment = setting<QComboBox>(*page, "Default payment method");
    auto *autoAdd = setting<QCheckBox>(*page, "Add scans directly to the cart");
    auto *beep = setting<QCheckBox>(*page, "Beep on successful scan");
    auto *low = setting<QSpinBox>(*page, "Low-stock limit");
    auto *theme = setting<QComboBox>(*page, "Color theme");
    auto *paper = setting<QComboBox>(*page, "PDF paper size");
    auto *folder = setting<QLineEdit>(*page, "Receipt export folder");
    QVERIFY(payment && autoAdd && beep && low && theme && paper && folder);
    auto *discard = button(*page, "Discard changes");
    auto *apply = button(*page, "Apply");
    QVERIFY(discard && apply);
    QVERIFY(!apply->isEnabled());

    payment->setCurrentText("Card");
    QVERIFY(apply->isEnabled());
    discard->click();
    QCOMPARE(payment->currentText(), "Cash");

    payment->setCurrentText("Wallet");
    autoAdd->setChecked(true);
    beep->setChecked(false);
    low->setValue(45);
    theme->setCurrentText("Dark mode");
    paper->setCurrentText("A4");
    folder->setText(m_fixture.filePath("does-not-exist"));
    apply->click();
    QCOMPARE(tabs->tabText(tabs->currentIndex()), "Receipts");
    QCOMPARE(AppState::instance().settings().lowStockThreshold, 10);

    folder->setText(m_fixture.path());
    apply->click();
    QVERIFY(!apply->isEnabled());
    const auto saved = Settings::load();
    QCOMPARE(saved.lowStockThreshold, 45);
    QCOMPARE(AppState::instance().lowStock().size(), 1);
    QVERIFY(saved.autoAddScans && !saved.beepOnScan && saved.receiptA4);
    QCOMPARE(saved.receiptFolder, m_fixture.path());
    QVERIFY(saved.defaultPayment == barcodehub::order::PaymentMode::Wallet);
    QVERIFY(Theme::isDark());
    auto *topTheme = window.findChild<QPushButton *>("darkModeToggle");
    QVERIFY(topTheme);
    topTheme->click();
    QCOMPARE(theme->currentText(), "Light mode");

    Dialogs::PaymentResult result;
    Dialogs::PaymentRequest request;
    request.lines.push_back({"Milk", "OLPMILK", 1, 340, 0, 10, 1});
    bool correctWallet = false;
    QTimer::singleShot(0, &window, [&]
    {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog)
            return;
        auto *wallet = button(*dialog, "Wallet");
        correctWallet = wallet && wallet->isChecked();
        dialog->accept();
    });
    QVERIFY(Dialogs::payment(&window, request, result));
    QVERIFY(correctWallet);
    QCOMPARE(result.mode, "Wallet");
    QCOMPARE(result.merchant, "JazzCash");

    const QString pdf = m_fixture.filePath("test-receipt.pdf");
    QVERIFY(Dialogs::saveReceiptPdf(AppState::instance().orders().first(), pdf));
    QFile receipt(pdf);
    QVERIFY(receipt.open(QIODevice::ReadOnly));
    const auto contents = receipt.readAll();
    QVERIFY(contents.startsWith("%PDF"));
    QVERIFY(contents.contains("/MediaBox [0 0 595"));

    const bool nonNativeDialogs = QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs);
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    bool usedSavedFolder = false;
    QTimer::singleShot(0, &window, [&]
    {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        auto *exportButton = dialog ? button(*dialog, "Save as PDF") : nullptr;
        if (!exportButton)
        {
            if (dialog)
                dialog->reject();
            return;
        }
        QTimer::singleShot(0, dialog, [&]
        {
            auto *file = qobject_cast<QFileDialog *>(QApplication::activeModalWidget());
            if (!file)
                return;
            usedSavedFolder = file->directory() == QDir(m_fixture.path());
            file->reject();
        });
        exportButton->click();
        dialog->reject();
    });
    Dialogs::receipt(&window, AppState::instance().orders().first());
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs, nonNativeDialogs);
    QVERIFY(usedSavedFolder);
    QSignalSpy preview(page, &SettingsPage::toast);
    auto *sound = button(*page, "Play test sound");
    QVERIFY(sound);
    sound->click();
    QCOMPARE(preview.size(), 1);

    if (qEnvironmentVariableIsSet("SYNTRO_QA_SCREENSHOTS"))
        for (const bool dark : {false, true})
        {
            barcodehub::ui::DarkModeManager::instance().setDarkMode(dark);
            for (int i = 0; i < tabs->count(); ++i)
            {
                tabs->setCurrentIndex(i);
                QApplication::processEvents();
                const auto path = QDir(qEnvironmentVariable("SYNTRO_QA_SCREENSHOTS")).filePath(
                    QString("Settings-%1-%2.png").arg(tabs->tabText(i), dark ? "dark" : "light"));
                QVERIFY(window.grab().save(path));
            }
        }

    SettingsPage cashier(nullptr, UserRole::Cashier);
    auto *cashierTabs = cashier.findChild<QTabWidget *>();
    QCOMPARE(cashierTabs->count(), 5);
    QCOMPARE(setting<QComboBox>(cashier, "Default payment method")->currentText(), "Wallet");
}

QTEST_MAIN(TerminalUiTests)
#include "terminal_ui_tests.moc"
