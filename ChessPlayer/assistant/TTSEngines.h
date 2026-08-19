#ifndef TTSENGINES_H
#define TTSENGINES_H
#pragma once
#include <QString>
#include <QByteArray>
#include <QList>
#include <QTextToSpeech>
#include <QEventLoop>
#include <QDebug>

// Strategy B: High-Performance Local Piper TTS Engine
class PiperTTSEngine: public QObject {
private:
    QTextToSpeech *m_tts;
public:
    PiperTTSEngine(QObject *parent = nullptr) { m_tts = new QTextToSpeech(this); }
    ~PiperTTSEngine() { /* Release Piper resources */ }

    bool isPCMGenerator()  { return false; } // Piper returns raw audio byte frames

    void speakDirect(const QString &text)  {
        if (text.isEmpty()) return;

        // 1. Trigger the speech engine asynchronously
        m_tts->say(text);

        // 2. Create a local event loop on the stack
        QEventLoop loop;

        // 3. Connect the state change signal to exit the loop when Ready
        QObject::connect(m_tts, &QTextToSpeech::stateChanged, [&loop](QTextToSpeech::State state) {
            if (state == QTextToSpeech::Ready) {
                loop.quit(); // Exit the local loop safely
            }
        });

        // 4. Block here until loop.quit() is called
        loop.exec();
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
