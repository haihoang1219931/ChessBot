#ifndef VOICESTREAMER_H
#define VOICESTREAMER_H

#include <QObject>
#include <QProcess>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QBuffer>
#include <QByteArray>
#define USE_SYSTEM_VOICE
#ifdef USE_SYSTEM_VOICE
#include <QAxObject>
#endif
enum SPEAK_STATE{
    SPEAK_INIT,
    SPEAK_ONGOING,
    SPEAK_DONE,
};
class VoiceStreamer : public QObject {
    Q_OBJECT
public:
    explicit VoiceStreamer(QObject *parent = nullptr);
    ~VoiceStreamer();

    void speak(const QString &text);

private Q_SLOTS:
    void handleReadyRead();
    void handleProcessError(QProcess::ProcessError error);
    void handleAudioStateChanged(QAudio::State newState);
    void requestSpeech(const QString &text);

private:
    void startSpeech();
#ifdef USE_SYSTEM_VOICE
    QAxObject* m_voice;
    QAxObject* m_stream;
#else
    QProcess *m_piperProcess;
#endif
    QByteArray m_pcmChunk;
    QAudioOutput *m_audioOutput;
    QBuffer m_audioBufferDevice;
    QByteArray m_rawAudioData;
    int m_state;
};

#endif // VOICESTREAMER_H
