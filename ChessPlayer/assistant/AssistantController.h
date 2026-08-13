#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <QAudioInput>
#include <QIODevice>
#include <QByteArray>
#include <QVector>
#include <vector>
#include <string>

#include "llama.h"
#include "whisper.h"

// Private data structure for message history
struct ChatMessage {
    std::string role;
    std::string content;
};

// ============================================================================
// 1. BACKGROUND AUDIO & MODEL WORKER
// ============================================================================
class PrivateAudioModelWorker : public QObject {
    Q_OBJECT
public:
    PrivateAudioModelWorker() = default;
    ~PrivateAudioModelWorker();

public slots:
    void initService();
    void handleManualPrompt(const QString &prompt);

private slots:
    void processIncomingAudio();

signals:
    void isListeningChanged(bool listening);
    void isThinkingChanged(bool thinking);
    void tokenGenerated(const QString &text);
    void generationFinished(const QString &finalText);

private:
    void initializeLlama();
    void initializeWhisper();
    void initializeAudio();
    void handleSpeechFinished();
    void runLlamaInference(const QString &prompt);

    QAudioInput* m_audioInput = nullptr;
    QIODevice* m_audioIOStream = nullptr;
    QByteArray m_accumulatedPcmData;
    bool m_isSpeaking = false;
    int m_consecutiveSilenceSamples = 0;

    llama_model* m_model = nullptr;
    llama_context* m_ctx = nullptr;
    struct whisper_context* m_whisperCtx = nullptr;
    whisper_full_params m_whisperParams;
    std::vector<ChatMessage> m_conversationHistory;
    int m_pastTokensCount = 0;
};

// ============================================================================
// 2. MAIN FRONT-FACING QML CONTROLLER
// ============================================================================
class AssistantController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isListening READ isListening NOTIFY isListeningChanged)
    Q_PROPERTY(QString responseText READ responseText NOTIFY responseTextChanged)
    Q_PROPERTY(bool isThinking READ isThinking NOTIFY isThinkingChanged)

public:
    explicit AssistantController(QObject *parent = nullptr);
    ~AssistantController();

    Q_INVOKABLE void startService();
    Q_INVOKABLE void stopService();
    Q_INVOKABLE void generateResponse(const QString &prompt);

    bool isListening() const { return m_isListening; }
    QString responseText() const { return m_responseText; }
    bool isThinking() const { return m_isThinking; }

Q_SIGNALS:
    void isListeningChanged();
    void responseTextChanged(QString text);
    void isThinkingChanged();
    void generationFinished(const QString &finalText);

private:
    PrivateAudioModelWorker* d_worker = nullptr;
    QThread* m_workerThread = nullptr;

    bool m_isListening = false;
    bool m_isThinking = false;
    QString m_responseText;
};
