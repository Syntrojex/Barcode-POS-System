#include "ScanAudio.h"
#include "AppState.h"

#include <QCoreApplication>
#include <QFile>
#include <QUrl>
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

#ifdef WITH_SCAN_AUDIO_QT
#include <QAudioDevice>
#include <QMediaDevices>
#include <QSoundEffect>
#elif defined(WITH_SCAN_AUDIO_WINMM)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#endif

static void initScanAudioResources()
{
    Q_INIT_RESOURCE(scan_audio);
}

namespace
{
    constexpr int kSampleRate = 44100;
    constexpr int kDurationMs = 110;
    constexpr int kFadeInMs = 8;
    constexpr int kFadeOutMs = 24;
    constexpr double kBaseFreqHz = 1864.66;
    constexpr double kHarmonicFreqHz = 3729.32;
    constexpr double kPeakAmplitude = 0.40;
    constexpr double kPi = 3.14159265358979323846;

    void appendLe32(QByteArray &out, std::uint32_t value)
    {
        const char bytes[4] = {
            static_cast<char>(value & 0xFFu),
            static_cast<char>((value >> 8) & 0xFFu),
            static_cast<char>((value >> 16) & 0xFFu),
            static_cast<char>((value >> 24) & 0xFFu)
        };
        out.append(bytes, 4);
    }

    void appendLe16(QByteArray &out, std::uint16_t value)
    {
        const char bytes[2] = {
            static_cast<char>(value & 0xFFu),
            static_cast<char>((value >> 8) & 0xFFu)
        };
        out.append(bytes, 2);
    }
}

ScanAudio &ScanAudio::instance()
{
    static ScanAudio s;
    return s;
}

ScanAudio::ScanAudio()
{
    m_lastPlayTimer.invalidate();
}

ScanAudio::~ScanAudio()
{
    shutdown();
}

void ScanAudio::warmUp()
{
    ensureInitialized();
}

bool ScanAudio::playSuccess()
{
    return playIfEnabled(AppState::instance().settings().beepOnScan);
}

bool ScanAudio::playIfEnabled(bool enabled)
{
    const qint64 elapsed = m_lastPlayTimer.isValid() ? m_lastPlayTimer.elapsed() : -1;
    if (!shouldPlay(enabled, elapsed, m_minIntervalMs))
        return false;

    m_lastPlayTimer.restart();
    emit feedbackTriggered();
    return triggerBackendPlayback();
}

bool ScanAudio::shouldPlay(bool beepEnabled, qint64 elapsedSinceLastMs, qint64 minIntervalMs)
{
    if (!beepEnabled)
        return false;
    if (elapsedSinceLastMs >= 0 && minIntervalMs > 0 && elapsedSinceLastMs < minIntervalMs)
        return false;
    return true;
}

QByteArray ScanAudio::buildSuccessToneWav()
{
    const int numSamples = (kSampleRate * kDurationMs) / 1000;
    const int fadeInSamples = (kSampleRate * kFadeInMs) / 1000;
    const int fadeOutSamples = (kSampleRate * kFadeOutMs) / 1000;
    const std::uint32_t dataSize = static_cast<std::uint32_t>(numSamples * sizeof(std::int16_t));

    QByteArray wav;
    wav.reserve(44 + static_cast<int>(dataSize));
    wav.append("RIFF", 4);
    appendLe32(wav, 36u + dataSize);
    wav.append("WAVE", 4);
    wav.append("fmt ", 4);
    appendLe32(wav, 16u);
    appendLe16(wav, 1u); // PCM
    appendLe16(wav, 1u); // mono
    appendLe32(wav, static_cast<std::uint32_t>(kSampleRate));
    appendLe32(wav, static_cast<std::uint32_t>(kSampleRate * 2));
    appendLe16(wav, 2u);  // block align
    appendLe16(wav, 16u); // bits per sample
    wav.append("data", 4);
    appendLe32(wav, dataSize);

    for (int i = 0; i < numSamples; ++i)
    {
        const double t = static_cast<double>(i) / kSampleRate;
        double env = 1.0;
        if (i < fadeInSamples)
        {
            env = 0.5 * (1.0 - std::cos(kPi * static_cast<double>(i) / fadeInSamples));
        }
        else if (i >= numSamples - fadeOutSamples)
        {
            const int k = i - (numSamples - fadeOutSamples);
            env = 0.5 * (1.0 + std::cos(kPi * static_cast<double>(k + 1) / fadeOutSamples));
        }

        const double wave = 0.92 * std::sin(2.0 * kPi * kBaseFreqHz * t)
                          + 0.08 * std::sin(2.0 * kPi * kHarmonicFreqHz * t);
        const double scaled = std::clamp(wave * env * kPeakAmplitude * 32767.0, -32767.0, 32767.0);
        const auto sample = static_cast<std::int16_t>(std::lround(scaled));
        appendLe16(wav, static_cast<std::uint16_t>(sample));
    }

    return wav;
}

bool ScanAudio::isAvailable() const
{
    const_cast<ScanAudio *>(this)->ensureInitialized();
    const_cast<ScanAudio *>(this)->refreshDeviceAvailability();
    return m_available;
}

QString ScanAudio::backendName() const
{
#ifdef WITH_SCAN_AUDIO_QT
    return QStringLiteral("QtMultimedia (QSoundEffect)");
#elif defined(WITH_SCAN_AUDIO_WINMM)
    return QStringLiteral("Windows Multimedia (PlaySoundW)");
#else
    return QStringLiteral("Unavailable");
#endif
}

void ScanAudio::setMinIntervalMs(qint64 ms)
{
    m_minIntervalMs = std::max<qint64>(0, ms);
}

void ScanAudio::resetDebounce()
{
    m_lastPlayTimer.invalidate();
}

void ScanAudio::ensureInitialized()
{
    if (m_initialized || m_shuttingDown)
        return;
    QCoreApplication *app = QCoreApplication::instance();
    if (!app || QCoreApplication::closingDown())
        return;

    m_initialized = true;
    initScanAudioResources();

    connect(app, &QCoreApplication::aboutToQuit, this, &ScanAudio::shutdown, Qt::DirectConnection);

    QFile wavFile(QStringLiteral(":/audio/scan_beep.wav"));
    if (wavFile.open(QIODevice::ReadOnly))
        m_wavBuffer = wavFile.readAll();
    if (m_wavBuffer.isEmpty())
        m_wavBuffer = buildSuccessToneWav();

#ifdef WITH_SCAN_AUDIO_QT
    m_mediaDevices = std::make_unique<QMediaDevices>();
    connect(m_mediaDevices.get(), &QMediaDevices::audioOutputsChanged,
            this, &ScanAudio::refreshDeviceAvailability);

    m_effect = std::make_unique<QSoundEffect>();
    m_effect->setLoopCount(1);
    m_effect->setVolume(0.85f);
    connect(m_effect.get(), &QSoundEffect::statusChanged, this, [this] {
        if (m_effect && m_effect->status() == QSoundEffect::Error)
        {
            m_available = false;
            if (!m_warnedUnavailable)
            {
                m_warnedUnavailable = true;
                qWarning("ScanAudio: QSoundEffect failed to load bundled scan tone.");
            }
        }
        else
        {
            refreshDeviceAvailability();
        }
    });
    m_effect->setSource(QUrl(QStringLiteral("qrc:/audio/scan_beep.wav")));
#endif

    refreshDeviceAvailability();
}

void ScanAudio::shutdown()
{
    m_shuttingDown = true;
    m_available = false;
    m_initialized = false;
#ifdef WITH_SCAN_AUDIO_QT
    m_effect.reset();
    m_mediaDevices.reset();
#elif defined(WITH_SCAN_AUDIO_WINMM)
    PlaySoundW(nullptr, nullptr, 0);
#endif
    m_wavBuffer.clear();
}

void ScanAudio::refreshDeviceAvailability()
{
    if (!m_initialized || m_shuttingDown)
    {
        m_available = false;
        return;
    }

#ifdef WITH_SCAN_AUDIO_QT
    if (!m_effect)
    {
        m_available = false;
        return;
    }
    const auto outputs = QMediaDevices::audioOutputs();
    if (outputs.isEmpty())
    {
        m_available = false;
        return;
    }
    const QAudioDevice defaultOut = QMediaDevices::defaultAudioOutput();
    if (!defaultOut.isNull() && m_effect->audioDevice() != defaultOut)
        m_effect->setAudioDevice(defaultOut);
    m_available = (m_effect->status() != QSoundEffect::Error);
#elif defined(WITH_SCAN_AUDIO_WINMM)
    m_available = (waveOutGetNumDevs() > 0) && !m_wavBuffer.isEmpty();
#else
    m_available = false;
#endif
}

bool ScanAudio::triggerBackendPlayback()
{
    ensureInitialized();
    refreshDeviceAvailability();
    if (!m_available)
    {
        if (!m_warnedUnavailable)
        {
            m_warnedUnavailable = true;
            qWarning("ScanAudio: No audio output device available; scan feedback will remain silent.");
        }
        return false;
    }

#ifdef WITH_SCAN_AUDIO_QT
    if (!m_effect)
        return false;
    if (m_effect->isPlaying())
        m_effect->stop();
    m_effect->play();
    return true;
#elif defined(WITH_SCAN_AUDIO_WINMM)
    if (m_wavBuffer.isEmpty())
        return false;
    return PlaySoundW(reinterpret_cast<LPCWSTR>(m_wavBuffer.constData()),
                      nullptr,
                      SND_MEMORY | SND_ASYNC | SND_NODEFAULT) != FALSE;
#else
    return false;
#endif
}
