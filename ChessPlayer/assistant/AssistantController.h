#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <QVector>
#include <vector>
#include <string>

#include "LLMWorker.h"
#include "AudioModelWorker.h"

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
    AudioModelWorker* m_audioWorker = nullptr;
    QThread* m_audioThread = nullptr;
    LLMWorker* m_llmWorker;
    QThread* m_llmThread= nullptr;

    bool m_isListening = false;
    bool m_isThinking = false;
    QString m_responseText;
};
