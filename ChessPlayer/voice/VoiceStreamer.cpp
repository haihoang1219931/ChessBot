#include "VoiceStreamer.h"
#include <QThread>
#include <QDebug>
#include <QAudioDeviceInfo> // Qt5 Specific Audio Device Engine

VoiceStreamer::VoiceStreamer(QObject *parent)
    : QObject(parent), m_audioOutput(nullptr) {
#if defined(USE_SYSTEM_VOICE)
    m_voice = new QAxObject("SAPI.SpVoice");
    m_stream = new QAxObject("SAPI.SpMemoryStream");
    m_voice->setProperty("AudioOutputStream", m_stream->asVariant());
#else
    m_piperProcess = new QProcess(this);
#endif
    // 1. Check for Default Output Audio Hardware in Qt5
    QAudioDeviceInfo defaultDevice = QAudioDeviceInfo::defaultOutputDevice();
    if (defaultDevice.isNull()) {
        qCritical() << "❌ ERROR: No default Qt5 audio speaker hardware found!";
        return;
    }
    qDebug() << "ℹ️ Qt5 Audio Output Found:" << defaultDevice.deviceName();

    // 2. Configure Format matching Piper's "High" quality specifications
    QAudioFormat format;
    format.setSampleRate(22050);
    format.setChannelCount(1);
    format.setSampleSize(16); // Qt5 uses setSampleSize instead of setSampleFormat
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec("audio/pcm"); // Crucial explicitly forced fallback for Qt5

    if (!defaultDevice.isFormatSupported(format)) {
        qWarning() << "⚠️ 22050Hz raw PCM not supported natively. Attempting nearest matching fallback format...";
        format = defaultDevice.nearestFormat(format);
    }

    // 3. Initialize Audio Output Engine
    m_audioOutput = new QAudioOutput(defaultDevice, format, this);
    connect(m_audioOutput, &QAudioOutput::stateChanged, this, &VoiceStreamer::handleAudioStateChanged);

    // 4. Bind our continuous byte memory device arrays
    m_audioBufferDevice.setBuffer(&m_rawAudioData);
    m_audioBufferDevice.open(QIODevice::ReadWrite);
#if defined(USE_SYSTEM_VOICE)

#else
    // 5. Setup Piper Standalone System Process Engine
    // NOTE: If you are running on Windows, change "./piper" to "piper.exe"
    QString program = "piper";
    QStringList arguments;
    arguments << "--model" << "./voice/model/en_US-hfc_female-medium.onnx"
              << "--output-raw"; // Instant zero-lag stream emissions

    m_piperProcess->setProgram(program);
    m_piperProcess->setArguments(arguments);
    m_piperProcess->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_piperProcess, &QProcess::readyReadStandardOutput,
            this, &VoiceStreamer::handleReadyRead);
    connect(m_piperProcess, &QProcess::readyReadStandardError,
            this, [this]() {
        qWarning() << "⚠️ Piper Process Output Log:" << m_piperProcess->readAllStandardError();
    });
    connect(m_piperProcess, &QProcess::errorOccurred,
            this, &VoiceStreamer::handleProcessError);

    m_piperProcess->start();
    if (!m_piperProcess->waitForStarted()) {
        qCritical() << "❌ CRITICAL: Could not execute Piper. Confirm directory setup permissions.";
        return;
    }
#endif
    // 6. Connect the continuous buffer stream output up to your speaker channels
    m_audioOutput->start(&m_audioBufferDevice);
    qDebug() << "🚀 Qt5 Piper Worker Engine initialized and waiting.";
}

VoiceStreamer::~VoiceStreamer() {
#if defined(USE_SYSTEM_VOICE)
#else
    if (m_piperProcess->state() == QProcess::Running) {
        m_piperProcess->kill();
        m_piperProcess->waitForFinished();
    }
#endif
    if (m_audioOutput) {
        m_audioOutput->stop();
    }
}

// 1. Rename or update your existing speak function to wrap safely
void VoiceStreamer::speak(const QString &text) {
    // This check determines if the caller is on a different thread than this object
    if (thread() != QThread::currentThread()) {
        // Safe cross-thread invocation via Qt MetaObject System
        QMetaObject::invokeMethod(this, "requestSpeech",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, text));
    } else {
        // Direct call if we are already safely on the same thread
        requestSpeech(text);
    }
}

// 2. Add the actual execution slot that interacts with the QProcess socket
void VoiceStreamer::requestSpeech(const QString &text) {
#if defined(USE_SYSTEM_VOICE)
#else
    if (m_piperProcess->state() != QProcess::Running) {
        qWarning() << "❌ Speech Aborted: Engine background thread is unresponsive.";
        return;
    }

#endif
    QString cleanText = text.trimmed();
    if (!cleanText.isEmpty()) {
        if (m_audioOutput->state() == QAudio::IdleState) {
            m_audioOutput->stop();
            m_rawAudioData.clear();
            m_audioBufferDevice.seek(0);
            m_audioOutput->start(&m_audioBufferDevice);
        }
#if defined(USE_SYSTEM_VOICE)
        // Speak asynchronously into the memory stream
        // 1 = SPF_ASYNC flag
        m_voice->dynamicCall("Speak(QString, uint)", text, 1);
        m_voice->dynamicCall("WaitUntilDone(int)", -1); // Wait indefinitely until complete

        // Pull raw data out of the stream buffer
        QVariant streamData = m_stream->dynamicCall("GetData()");

        m_pcmChunk = streamData.toByteArray();
        startSpeech();
#else
        qDebug() << "🗣️ Thread-Safe write to Piper process socket:" << cleanText;
        m_piperProcess->write((cleanText + "\n").toUtf8());
#endif
    }
}
void VoiceStreamer::handleReadyRead() {
#if defined(USE_SYSTEM_VOICE)

#else
    m_pcmChunk = m_piperProcess->readAllStandardOutput();
    if (pcmChunk.isEmpty()) return;
#endif
    startSpeech();
}

void VoiceStreamer::startSpeech() {
    qDebug() << "🎵 Appending bytes to Qt5 Buffer Array. Volume Size:" << m_pcmChunk.size();

    // Preserve the hardware's active playback coordinate position
    qint64 currentPlayPos = m_audioBufferDevice.pos();

    // Seek to the tail of the data vector, append, and restore play head pointer
    m_audioBufferDevice.seek(m_rawAudioData.size());
    m_audioBufferDevice.write(m_pcmChunk);
    m_audioBufferDevice.seek(currentPlayPos);

    // If Qt5 sound went idle due to buffer starving under slow CPU load, force wake up channel paths
    if (m_audioOutput->state() == QAudio::IdleState) {
        // Simple trick to force Qt5 stream device loops to re-evaluate their buffer size
        m_audioOutput->suspend();
        m_audioOutput->resume();
    }
}

void VoiceStreamer::handleAudioStateChanged(QAudio::State newState) {
    // Tracking this allows you to debug internal channel disruptions easily
    qDebug() << "🎛️ Audio Hardware Engine State change notification:" << newState;
}

void VoiceStreamer::handleProcessError(QProcess::ProcessError error) {
    qCritical() << "❌ Backend process execution fault thrown:" << error;
}
