#ifndef LLMWORKER_H
#define LLMWORKER_H

#include <QThread>
#include <QObject>
#include <QString>
#include <QMutex>
#include <QWaitCondition>

#include "llama.h"
#include "whisper.h"

struct ChatMessage {
    std::string role;
    std::string content;
};

enum LLM_STATE {
    LLM_INIT,
    LLM_TRANSCRIBE,
    LLM_PROCESSING,
    LLM_PROCESSING_DONE,
};

enum LLM_RESULT {
    LLM_PENDING,
    LLM_DONE_SUCCESS,
    LLM_DONE_FAILED,
};

class LLMWorker : public QObject {
    Q_OBJECT

public:
    explicit LLMWorker(QObject *parent = nullptr);
    ~LLMWorker();
    void togglePause(bool paused);

public Q_SLOTS:
    void doWork();
    void handleSpeech(const QByteArray& pcmData);

Q_SIGNALS:
    void tokenGenerated(const QString &token);
    void generationFinished(const QString &completeResponse);

private:
    bool m_stopped = false;
    bool m_pause = false;
    QMutex *m_mutex;
    QWaitCondition* m_pauseCond;
    void initializeLlama();
    void initializeWhisper();
    int runLlamaInference();
    int transcribeAudio();
    llama_model* m_model = nullptr;
    llama_context* m_ctx = nullptr;
    struct whisper_context* m_whisperCtx = nullptr;
    whisper_full_params m_whisperParams;
    std::vector<ChatMessage> m_conversationHistory;
    QByteArray m_pcmData;
    QString m_prompt;
    int m_pastTokensCount = 0;

    int m_state;
};

#endif // LLMWORKER_H
