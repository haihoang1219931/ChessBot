#ifndef PIPERSTREAMER_H
#define PIPERSTREAMER_H

#include <QObject>
#include <QProcess>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QBuffer>
#include <QByteArray>

class PiperStreamer : public QObject {
    Q_OBJECT
public:
    explicit PiperStreamer(QObject *parent = nullptr);
    ~PiperStreamer();

    void speak(const QString &text);

private Q_SLOTS:
    void handleReadyRead();
    void handleProcessError(QProcess::ProcessError error);
    void handleAudioStateChanged(QAudio::State newState);
    void requestSpeech(const QString &text);

private:
    QProcess *m_piperProcess;
    QAudioOutput *m_audioOutput;
    QBuffer m_audioBufferDevice;
    QByteArray m_rawAudioData;
};

#endif // PIPERSTREAMER_H
