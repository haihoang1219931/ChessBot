#ifndef LLMWORKER_H
#define LLMWORKER_H

#include <QThread>
#include <QObject>
#include <QString>
#include <QMutex>
#include <QVector>
#include <QWaitCondition>
#include <memory>

#include "llama.h"
#include "sherpa-onnx/c-api/cxx-api.h"

struct ChatMessage {
    QString role;
    QString content;
};

enum LLM_STATE {
    LLM_INIT,
    LLM_WAITING,
    LLM_TRANSCRIBE,
    LLM_PROCESSING,
    LLM_PROCESSING_EXIT,
};

enum LLM_RESULT {
    LLM_PENDING,
    LLM_DONE_INTERRUPT,
    LLM_DONE_SUCCESS,
    LLM_DONE_FAILED,
};

class LLMWorker : public QObject {
    Q_OBJECT

public:
    explicit LLMWorker(QObject *parent = nullptr);
    ~LLMWorker();
    void stop();
    void togglePause(bool paused);
    int handlePrompt(const QString& prompt);
    int analyzeChessMove(QString fen, QString playColor, QString move);
    void requestInterruption();
    void setModel(const QString& name,
                  const QString& sherpaModelPath,
                  const QString& sherpaTokensPath,
                  const QString& llmModelPath);
    void initializeLlama();
    void initializeSherpaOnnx();
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
    int runLlamaInference();
    int transcribeAudio();
    QString generatePromptChat(const QString& userPrompt);
    QString generatePromptChess(QString fen, QString playColor, QString move);

    // Core Models
    llama_model* m_model = nullptr;
    llama_context* m_ctx = nullptr;
    sherpa_onnx::cxx::OfflineRecognizer* m_recognizer = nullptr;

    QVector<ChatMessage> m_conversationHistory;
    QByteArray m_pcmData;
    QString m_userPrompt;
    QString m_fullPrompt;
    int m_pastTokensCount = 0;
    QAtomicInt m_interrupted; // Thread-safe atomic flag
    int m_state;
    int m_nextState;
    QString m_llmModelPath;
    QString m_sherpaModelPath;
    QString m_sherpaTokensPath;
    QString m_name;
};

#endif // LLMWORKER_H
