#ifndef AUDIOOUTPUTWORKER_H
#define AUDIOOUTPUTWORKER_H

#pragma once
#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QIODevice>
#include <QBuffer>
#include <QMutex>
#include <QWaitCondition>
#include "TTSEngines.h" // Include your new interface header

enum AUDIOOUTPUT_STATE {
    AUDIOOUTPUT_INIT,
    AUDIOOUTPUT_WAITING,
    AUDIOOUTPUT_PROCESSING,
    AUDIOOUTPUT_PROCESSING_EXIT,
};

enum AUDIOOUTPUT_RESULT {
    AUDIOOUTPUT_PENDING,
    AUDIOOUTPUT_DONE_INTERRUPT,
    AUDIOOUTPUT_DONE_SUCCESS,
    AUDIOOUTPUT_DONE_FAILED,
};

class AudioOutputWorker : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutputWorker(QObject *parent = nullptr);
    ~AudioOutputWorker();

    void stop();
    void togglePause(bool paused);
    void requestInterruption();
    void setVoiceModel(const QString& botName,
                       const QString& piperExePath,
                       const QString& modelPath);
public Q_SLOTS:
    void handleToken(const QString &token);
    void startWorker();
    void stopWorker();
    void clearQueue();
    void doWork();

Q_SIGNALS:
    void voiceStarted();
    void voiceFinished();

private:
    int processAudioLoop();
    void appendAndPlayPCM(const QByteArray &newPcmData);

    QQueue<QString> m_textQueue;

    QString m_previousText;
    QString m_sentenceBuffer;

    // Active Engine Strategy Pointers
    PiperTTSEngine *m_currentEngine;

    // Hardware Audio Objects
    QAudioOutput *m_audioOutput;
    QIODevice *m_audioDevice;
    QBuffer m_buffer;
    QByteArray m_audioData;
    qint64 m_readPosition;
    bool m_stopped = false;
    bool m_pause = false;
    QMutex *m_mutex;
    QWaitCondition* m_pauseCond;
    QAtomicInt m_interrupted; // Thread-safe atomic flag
    int m_state;
    int m_nextState;
    bool m_emitVoiceStop;
    QString m_botName;
    QString m_piperExePath;
    QString m_modelPath;
};

#endif // AUDIOOUTPUTWORKER_H
