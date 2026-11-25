#include "AppState.h"
#include "RecordOrderPage.h"
#include "ScanAudio.h"
#include "Theme.h"
#include "Widgets.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace
{
    std::uint16_t readLe16(const QByteArray &data, int offset)
    {
        return static_cast<std::uint16_t>(static_cast<unsigned char>(data[offset]))
             | (static_cast<std::uint16_t>(static_cast<unsigned char>(data[offset + 1])) << 8);
    }

    std::uint32_t readLe32(const QByteArray &data, int offset)
    {
        return static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset]))
             | (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 1])) << 8)
             | (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 2])) << 16)
             | (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 3])) << 24);
    }

    bool writeSampleCatalogue(const QString &dirPath)
    {
        QDir dir(dirPath);
        if (!dir.exists() && !dir.mkpath("."))
            return false;

        QFile products(dir.filePath("Product.txt"));
        if (!products.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        products.write(
            "1 01001020 OLPMILK Olpers Full Cream Milk 1L count 340.00 0.00 10.00 42 data/barcodes/01001020.png#\n"
            "2 01002140 NBREAD National Whole Wheat Bread count 260.00 18.00 0.00 34 data/barcodes/01002140.png#\n"
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

class ScanAudioTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void wavAssetAndToneCharacteristics();
    void gatingDecisionRules();
    void settingsToggleAndDebounce();
    void cashierScanWorkflowIntegration();
    void gracefulUnavailableHardwareHandling();

private:
    QTemporaryDir m_tempDataDir;
};

void ScanAudioTests::initTestCase()
{
    QVERIFY(m_tempDataDir.isValid());
    QVERIFY(writeSampleCatalogue(m_tempDataDir.path()));
    qputenv("BARCODEHUB_DATA", m_tempDataDir.path().toUtf8());

    auto &appState = AppState::instance();
    QCOMPARE(appState.products().size(), 3);
    QVERIFY(appState.findByCode("01001020").has_value());
    QVERIFY(!appState.findByCode("96385074").has_value());
}

void ScanAudioTests::wavAssetAndToneCharacteristics()
{
    const QByteArray generated = ScanAudio::buildSuccessToneWav();
    QVERIFY(generated.size() > 44);
    QCOMPARE(generated.mid(0, 4), QByteArray("RIFF"));
    QCOMPARE(generated.mid(8, 4), QByteArray("WAVE"));
    QCOMPARE(generated.mid(12, 4), QByteArray("fmt "));
    QCOMPARE(readLe16(generated, 20), std::uint16_t(1));   // PCM
    QCOMPARE(readLe16(generated, 22), std::uint16_t(1));   // mono
    QCOMPARE(readLe32(generated, 24), std::uint32_t(44100));
    QCOMPARE(readLe16(generated, 34), std::uint16_t(16));  // 16-bit
    QCOMPARE(generated.mid(36, 4), QByteArray("data"));

    const std::uint32_t dataBytes = readLe32(generated, 40);
    QCOMPARE(static_cast<int>(dataBytes) + 44, generated.size());
    const int sampleCount = static_cast<int>(dataBytes / 2);
    const double durationMs = (static_cast<double>(sampleCount) * 1000.0) / 44100.0;
    QVERIFY(durationMs >= 80.0 && durationMs <= 160.0);

    // Smooth fade-in and fade-out endpoints (no hard DC step / click)
    const auto firstSample = static_cast<std::int16_t>(readLe16(generated, 44));
    const auto lastSample = static_cast<std::int16_t>(readLe16(generated, 44 + (sampleCount - 1) * 2));
    QVERIFY(std::abs(firstSample) <= 5);
    QVERIFY(std::abs(lastSample) <= 5);

    int peakAbs = 0;
    for (int i = 0; i < sampleCount; ++i)
    {
        const auto s = static_cast<std::int16_t>(readLe16(generated, 44 + i * 2));
        peakAbs = std::max(peakAbs, std::abs(static_cast<int>(s)));
    }
    // Moderate loudness (between ~15% and ~70% full scale)
    QVERIFY(peakAbs >= 5000 && peakAbs <= 23000);

    // Embedded resource asset exists and matches the synthesized waveform
    ScanAudio::instance().warmUp();
    QFile res(QStringLiteral(":/audio/scan_beep.wav"));
    QVERIFY(res.open(QIODevice::ReadOnly));
    const QByteArray bundled = res.readAll();
    QCOMPARE(bundled, generated);
}

void ScanAudioTests::gatingDecisionRules()
{
    QVERIFY(!ScanAudio::shouldPlay(false, -1, 150));
    QVERIFY(!ScanAudio::shouldPlay(false, 500, 150));
    QVERIFY(ScanAudio::shouldPlay(true, -1, 150));
    QVERIFY(!ScanAudio::shouldPlay(true, 0, 150));
    QVERIFY(!ScanAudio::shouldPlay(true, 149, 150));
    QVERIFY(ScanAudio::shouldPlay(true, 150, 150));
    QVERIFY(ScanAudio::shouldPlay(true, 1200, 150));
}

void ScanAudioTests::settingsToggleAndDebounce()
{
    auto &audio = ScanAudio::instance();
    auto &settings = AppState::instance().settings();
    audio.warmUp();
    audio.setMinIntervalMs(150);
    audio.resetDebounce();

    QSignalSpy spy(&audio, &ScanAudio::feedbackTriggered);

    settings.beepOnScan = true;
    audio.playSuccess();
    QCOMPARE(spy.count(), 1);

    // Immediate repeat within cooldown is suppressed
    audio.playSuccess();
    QCOMPARE(spy.count(), 1);

    // Disabling setting silences playback immediately without restart
    QTest::qWait(170);
    settings.beepOnScan = false;
    audio.playSuccess();
    QCOMPARE(spy.count(), 1);

    // Re-enabling setting restores playback immediately without restart
    settings.beepOnScan = true;
    audio.playSuccess();
    QCOMPARE(spy.count(), 2);
}

void ScanAudioTests::cashierScanWorkflowIntegration()
{
    auto &audio = ScanAudio::instance();
    auto &settings = AppState::instance().settings();
    audio.setMinIntervalMs(150);

    RecordOrderPage page;
    auto *manual = page.findChild<QLineEdit *>("checkoutBarcode");
    auto *feed = page.findChild<CameraFeedWidget *>();
    auto *payable = page.findChild<QLabel *>("checkoutPayable");
    QVERIFY(manual != nullptr);
    QVERIFY(feed != nullptr);
    QVERIFY(payable != nullptr);

    QSignalSpy audioSpy(&audio, &ScanAudio::feedbackTriggered);

    // 1. Known product scanned by camera -> 1 beep even when autoAddScans is OFF (cart unchanged)
    settings.beepOnScan = true;
    settings.autoAddScans = false;
    audio.resetDebounce();
    emit feed->barcodeDetected("01001020");
    QCOMPARE(audioSpy.count(), 1);
    QCOMPARE(payable->text(), QString("PKR 0.00"));

    // 2. Unknown EAN detected by camera -> NO beep
    QTest::qWait(170);
    emit feed->barcodeDetected("96385074");
    QCOMPARE(audioSpy.count(), 1);

    // 3. Repeated rapid barcode trigger -> duplicate suppressed (no spam)
    audio.resetDebounce();
    emit feed->barcodeDetected("01001020");
    emit feed->barcodeDetected("01001020");
    QCOMPARE(audioSpy.count(), 2);

    // 4. Next scan after cooldown -> audible again
    QTest::qWait(170);
    emit feed->barcodeDetected("01002140");
    QCOMPARE(audioSpy.count(), 3);

    // 5. USB scanner / manual barcode submission via Enter -> 1 beep and clears input
    QTest::qWait(170);
    manual->setText("01001020");
    QTest::keyClick(manual, Qt::Key_Return);
    QCOMPARE(audioSpy.count(), 4);
    QVERIFY(manual->text().isEmpty());

    // 6. Empty / invalid manual submission -> silent
    QTest::qWait(170);
    manual->setText("");
    QTest::keyClick(manual, Qt::Key_Return);
    QCOMPARE(audioSpy.count(), 4);
    manual->setText("99999999");
    QTest::keyClick(manual, Qt::Key_Return);
    QCOMPARE(audioSpy.count(), 4);

    // 7. Navigation from Products / Scanner page calling showCode(..., true) -> silent
    QTest::qWait(170);
    page.showCode("01001020", true);
    QCOMPARE(audioSpy.count(), 4);

    // 8. Auto-add enabled: out-of-stock item blocked -> silent; in-stock item added -> audible + cart updated
    settings.autoAddScans = true;
    QTest::qWait(170);
    emit feed->barcodeDetected("01011722");
    QCOMPARE(audioSpy.count(), 4);
    QCOMPARE(payable->text(), QString("PKR 0.00"));

    QTest::qWait(170);
    emit feed->barcodeDetected("01001020");
    QCOMPARE(audioSpy.count(), 5);
    QCOMPARE(payable->text(), QString("PKR 330.00"));

    // 9. Beep disabled in settings -> both camera and USB/manual scan stay silent
    settings.beepOnScan = false;
    QTest::qWait(170);
    emit feed->barcodeDetected("01002140");
    manual->setText("01002140");
    QTest::keyClick(manual, Qt::Key_Return);
    QCOMPARE(audioSpy.count(), 5);

    // 10. Camera unavailable/toggle remains stable without crash
    page.simulateScan();

    // 11. Light/dark theme switching leaves audio & scan workflow unaffected
    const bool originalDark = Theme::isDark();
    Theme::setDark(!originalDark);
    QVERIFY(!Theme::styleSheet().isEmpty());
    settings.beepOnScan = true;
    QTest::qWait(170);
    manual->setText("01001020");
    QTest::keyClick(manual, Qt::Key_Return);
    QCOMPARE(audioSpy.count(), 6);
    Theme::setDark(originalDark);

    settings.beepOnScan = true;
    settings.autoAddScans = false;
}

void ScanAudioTests::gracefulUnavailableHardwareHandling()
{
    auto &audio = ScanAudio::instance();
    audio.warmUp();
    // Should never throw or hang regardless of whether an audio output device exists
    const QString backend = audio.backendName();
    QVERIFY(!backend.isEmpty());
    audio.resetDebounce();
    const bool available = audio.isAvailable();
    const bool played = audio.playIfEnabled(true);
    QVERIFY(!audio.playIfEnabled(false));
#ifdef WITH_SCAN_AUDIO_WINMM
    if (available)
        QVERIFY2(played, "Windows rejected playback of the embedded scan tone");
    qInfo() << "Windows audio output available:" << available << "Playback accepted:" << played;
#endif
}

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    ScanAudioTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "scan_audio_tests.moc"
