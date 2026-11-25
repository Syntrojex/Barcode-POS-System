#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <memory>

#ifdef WITH_SCAN_AUDIO_QT
class QMediaDevices;
class QSoundEffect;
#elif defined(WITH_SCAN_AUDIO_WINMM)
class QTemporaryFile;
#endif

/**
 * @class ScanAudio
 * @brief Reusable low-latency POS confirmation audio service for successful cashier barcode scans.
 *
 * Independent of order storage, billing, and UI layout. Reads the existing
 * AppState::instance().settings().beepOnScan flag, suppresses unintended duplicate triggers
 * within a short cooldown interval, and degrades gracefully when audio hardware or Qt Multimedia
 * is unavailable.
 */
class ScanAudio : public QObject
{
    Q_OBJECT
public:
    static constexpr qint64 kDefaultMinIntervalMs = 150;

    static ScanAudio &instance();

    /** @brief Pre-initializes the audio backend and asset so the first scan has minimal latency. */
    void warmUp();

    /** @brief Plays one scan confirmation beep if Settings::beepOnScan is enabled and not debounced. */
    bool playSuccess();

    /** @brief Plays one scan confirmation beep if @p enabled is true and not debounced. */
    bool playIfEnabled(bool enabled);

    /** @brief Pure trigger-gating rule for setting state and duplicate-suppression interval. */
    static bool shouldPlay(bool beepEnabled, qint64 elapsedSinceLastMs, qint64 minIntervalMs = kDefaultMinIntervalMs);

    /** @brief Synthesizes a clean 110 ms, 44.1 kHz 16-bit mono PCM WAV retail scanner beep. */
    static QByteArray buildSuccessToneWav();

    bool isAvailable() const;
    QString backendName() const;
    void setMinIntervalMs(qint64 ms);
    qint64 minIntervalMs() const { return m_minIntervalMs; }
    void resetDebounce();

signals:
    /** @brief Emitted whenever confirmation feedback passes setting and debounce checks. */
    void feedbackTriggered();

private:
    ScanAudio();
    ~ScanAudio() override;
    ScanAudio(const ScanAudio &) = delete;
    ScanAudio &operator=(const ScanAudio &) = delete;

    void ensureInitialized();
    void shutdown();
    void refreshDeviceAvailability();
    bool triggerBackendPlayback();

    qint64 m_minIntervalMs = kDefaultMinIntervalMs;
    QElapsedTimer m_lastPlayTimer;
    bool m_initialized = false;
    bool m_shuttingDown = false;
    bool m_available = false;
    bool m_warnedUnavailable = false;
    QByteArray m_wavBuffer;
#ifdef WITH_SCAN_AUDIO_QT
    std::unique_ptr<QMediaDevices> m_mediaDevices;
    std::unique_ptr<QSoundEffect> m_effect;
#elif defined(WITH_SCAN_AUDIO_WINMM)
    std::unique_ptr<QTemporaryFile> m_temporaryWav;
    QString m_temporaryWavPath;
#endif
};
