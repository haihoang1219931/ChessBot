#include "AssistantController.h"

AssistantController::AssistantController(QObject *parent) : QObject(parent) {
    m_audioThread = new QThread(this);
    m_audioWorker = new AudioModelWorker();
    m_audioWorker->moveToThread(m_audioThread);
    // 2. Initialize the New Dedicated LLM Text Processing Thread
    m_llmThread = new QThread(this);
    m_llmWorker = new LLMWorker();
    m_llmWorker->moveToThread(m_llmThread);
    connect(m_audioThread, &QThread::started, m_audioWorker, &AudioModelWorker::initService);

    connect(m_audioWorker, &AudioModelWorker::isListeningChanged, this, [this](bool listening) {
        m_isListening = listening; Q_EMIT isListeningChanged();
    });
    connect(m_audioWorker, &AudioModelWorker::speechFinished, this, [this](const QByteArray& pcmData) {
        qDebug("AssistantController handleSpeech");
        m_llmWorker->handleSpeech(pcmData);
    });

    connect(m_llmWorker, &LLMWorker::generationFinished, this, [this](QString text) {
        qDebug("AssistantController handleResponse");
        m_responseText = text;
        Q_EMIT responseTextChanged(m_responseText);
    });

    connect(m_llmThread, &QThread::started, m_llmWorker, &LLMWorker::doWork);

    connect(m_audioThread, &QThread::finished, m_audioWorker, &QObject::deleteLater);
}

AssistantController::~AssistantController() {
    stopService();
}

void AssistantController::startService() {
    if (!m_audioThread->isRunning()) {
        m_audioThread->start();
    }
    if (!m_llmThread->isRunning()) {
        m_llmThread->start();
    }
}

void AssistantController::stopService() {
    if (m_audioThread && m_audioThread->isRunning()) {
        m_audioThread->quit();
        m_audioThread->wait();
    }
    if (m_llmThread && m_llmThread->isRunning()) {
        m_llmThread->quit();
        m_llmThread->wait();
    }
}

void AssistantController::generateResponse(const QString &prompt) {
    if (m_isThinking || prompt.isEmpty()) return;

    m_isThinking = true;
    Q_EMIT isThinkingChanged();

    QMetaObject::invokeMethod(m_audioWorker, [this, prompt]() {
        m_audioWorker->handleManualPrompt(prompt);
    }, Qt::QueuedConnection);
}

