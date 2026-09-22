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
#include <QProcess>
#include <QByteArray>
// Strategy B: High-Performance Local Piper TTS Engine
class PiperTTSEngine: public QObject {
private:
    QTextToSpeech *m_tts;
public:
    PiperTTSEngine(QObject *parent = nullptr) {
        m_tts = new QTextToSpeech(this);
    }
    ~PiperTTSEngine() { /* Release Piper resources */ }

    bool isPCMGenerator()  { return true; } // Piper returns raw audio byte frames

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

    QByteArray generatePCM(const QString &text,
                           const QString& piperExePath,
                           const QString& modelPath) {
        QProcess piperProcess;

        // Set up arguments to bypass the WAV header and stream raw data
        QStringList arguments;
        arguments << "--model" << modelPath
                  << "--output-raw"; // Emits uncompressed raw PCM samples

        piperProcess.start(piperExePath, arguments);

        if (!piperProcess.waitForStarted()) {
            return QByteArray();
        }

        // Write the phrase to standard input and close stdin to signal EOF
        piperProcess.write(text.toUtf8());
        piperProcess.closeWriteChannel();

        // Block and wait for processing to complete
        if (!piperProcess.waitForFinished()) {
            return QByteArray();
        }
        return piperProcess.readAllStandardOutput(); // Returns data frames to your QAudioOutput loop
    }
    void stop()  { /* Cancel active Piper processing steps */ }
};

#endif // TTSENGINE_H
