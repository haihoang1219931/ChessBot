#include "AudioModelWorker.h"
#include <QDebug>
#include <cmath>


AudioModelWorker::AudioModelWorker(QObject *parent) :
    QObject(parent)
{

}

AudioModelWorker::~AudioModelWorker() {

}

void AudioModelWorker::initService() {
    initializeAudio();
}

void AudioModelWorker::handleManualPrompt(const QString &prompt) {
    if (m_audioInput) m_audioInput->suspend();
//    runLlamaInference(prompt);
}


void AudioModelWorker::initializeAudio() {
    QAudioFormat format;
    format.setSampleRate(16000); format.setChannelCount(1); format.setSampleSize(16);
    format.setCodec("audio/pcm"); format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    m_audioInput = new QAudioInput(QAudioDeviceInfo::defaultInputDevice(), format, this);
    m_audioIOStream = m_audioInput->start();

    if (m_audioIOStream) {
        connect(m_audioIOStream, &QIODevice::readyRead, this, &AudioModelWorker::processIncomingAudio);
        Q_EMIT isListeningChanged(true);
        qDebug() << "Microphone auto-monitoring is active.";
    } else {
        qWarning() << "Audio hardware input stream failed to open.";
    }
}

void AudioModelWorker::processIncomingAudio() {
    QByteArray freshBytes = m_audioIOStream->readAll();
    if (freshBytes.isEmpty()) return;

    const int16_t* samples = reinterpret_cast<const int16_t*>(freshBytes.constData());
    int sampleCount = freshBytes.size() / sizeof(int16_t);

    float sumSquares = 0.0f;
    for (int i = 0; i < sampleCount; ++i) {
        float normalized = samples[i] / 32768.0f;
        sumSquares += normalized * normalized;
    }
    float currentVolume = std::sqrt(sumSquares / sampleCount);

    if (currentVolume > 0.015f) { // Active Speech Threshold
        if (!m_isSpeaking) {
            qDebug() << "Speech detected. Recording...";
            m_isSpeaking = true;
            m_accumulatedPcmData.clear();
        }
        m_accumulatedPcmData.append(freshBytes);
        m_consecutiveSilenceSamples = 0;
    } else {
        if (m_isSpeaking) {
            m_accumulatedPcmData.append(freshBytes);
            m_consecutiveSilenceSamples += sampleCount;

            // 1.5 Seconds of Silence Cutoff (16 samples per millisecond)
            if ((m_consecutiveSilenceSamples / 16) >= 1500) {
                qDebug() << "Speech finished. Starting processing pipeline.";
                m_isSpeaking = false;
                m_consecutiveSilenceSamples = 0;
                Q_EMIT speechFinished(m_accumulatedPcmData);
            }
        }
    }
}
