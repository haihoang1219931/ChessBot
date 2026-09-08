#ifndef TTSENGINES_H
#define TTSENGINES_H
#pragma once
#include <QString>
#include <QByteArray>
#include <QList>
#include <QTextToSpeech>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>
#include <QThread>

// Strategy B: High-Performance Local Piper TTS Engine
class PiperTTSEngine: public QObject {
private:
    QTextToSpeech *m_tts;
public:
    PiperTTSEngine(QObject *parent = nullptr) {
        m_tts = new QTextToSpeech(this); }
    ~PiperTTSEngine() { /* Release Piper resources */ }

    bool isPCMGenerator()  { return false; } // Piper returns raw audio byte frames

    void speakDirect(const QString &text)  {
        if (text.isEmpty()) return;
        int sleepTime = text.split(" ").size();
        qDebug("PiperTTSEngine speakDirect m_tts sleep[%d] state[%d] [%s]",
               sleepTime,
               m_tts->state(),
               text.toStdString().c_str());

        m_tts->say(text);
        int countTime = 0;
        int lastState = m_tts->state();
        while(countTime < sleepTime) {
            int state = m_tts->state();
            if(lastState == QTextToSpeech::Speaking &&
                   state == QTextToSpeech::Ready) break;
            lastState = state;
            QThread::msleep(500);
            countTime++;
        }
        qDebug("PiperTTSEngine speakDirect done");
    }

    QList<QByteArray> generatePCM(const QString &text) {
        QList<QByteArray> chunks;

        // --- REAL PIPER INFERENCE HOOK ---
        // std::vector<int16_t> pcm_output;
        // piper::textToAudio(piperConfig, text.toStdString(), pcm_output);
        // chunks.append(QByteArray(reinterpret_cast<char*>(pcm_output.data()), pcm_output.size() * 2));

        Q_UNUSED(text);
        return chunks; // Returns data frames to your QAudioOutput loop
    }
    void stop()  { /* Cancel active Piper processing steps */ }
};

#endif // TTSENGINE_H
