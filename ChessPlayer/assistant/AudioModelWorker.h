#ifndef AUDIOMODELWORKER_H
#define AUDIOMODELWORKER_H

#include <QObject>
#include <QAudioInput>
#include <QIODevice>
#include <QByteArray>
#include <QDebug>

class AudioModelWorker : public QObject {
    Q_OBJECT
public:
    AudioModelWorker(QObject *parent = nullptr);
    ~AudioModelWorker();

public Q_SLOTS:
    void initService();
    void handleManualPrompt(const QString &prompt);

private Q_SLOTS:
    void processIncomingAudio();

Q_SIGNALS:
    void isListeningChanged(bool listening);
//    void isThinkingChanged(bool thinking);
//    void tokenGenerated(const QString &text);
//    void generationFinished(const QString &finalText);
    void speechFinished(const QByteArray& pcmData);

private:
    void initializeAudio();

    QAudioInput* m_audioInput = nullptr;
    QIODevice* m_audioIOStream = nullptr;
    QByteArray m_accumulatedPcmData;
    bool m_isSpeaking = false;
    int m_consecutiveSilenceSamples = 0;
};

#endif // AUDIOMODELWORKER_H
