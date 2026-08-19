#include "AssistantController.h"

AssistantController::AssistantController(QObject *parent) : QObject(parent) {
    // 1. Initialize the Audio input processing thread
    m_audioThread = new QThread(this);
    m_audioWorker = new AudioModelWorker();
    m_audioWorker->moveToThread(m_audioThread);
    // 2. Initialize the New Dedicated LLM Text Processing Thread
    m_llmThread = new QThread(this);
    m_llmWorker = new LLMWorker();
    m_llmWorker->moveToThread(m_llmThread);
    // 3. Initialize the Audio output processing thread
    m_voiceWorker = new AudioOutputWorker();
    m_voiceThread = new QThread(this);
    m_voiceWorker->moveToThread(m_voiceThread);

    connect(m_audioThread, &QThread::started, m_audioWorker, &AudioModelWorker::initService);

    connect(m_audioWorker, &AudioModelWorker::isListeningChanged, this, [this](bool listening) {
        m_isListening = listening; Q_EMIT isListeningChanged();
    });
    connect(m_audioWorker, &AudioModelWorker::speechFinished, this, [this](const QByteArray& pcmData) {
        qDebug("AssistantController handleSpeech");
//        m_llmWorker->handleSpeech(pcmData);
    });

    connect(m_llmWorker, &LLMWorker::tokenGenerated, this, [this](QString text) {
        m_responseText += text;
        Q_EMIT responseTextChanged(m_responseText);
        m_voiceWorker->handleToken(text);
    });
    connect(m_llmWorker, &LLMWorker::generationFinished, this, [this](QString text) {
        qDebug("AssistantController handleResponse");
        m_responseText = text;
        m_isThinking = false;
        Q_EMIT isThinkingChanged();
        Q_EMIT responseTextChanged(m_responseText);
        m_responseText = "";
    });

    connect(m_llmThread, &QThread::started, m_llmWorker, &LLMWorker::doWork);
    connect(m_audioThread, &QThread::finished, m_audioWorker, &QObject::deleteLater);
    connect(m_voiceThread, &QThread::started, m_voiceWorker, &AudioOutputWorker::doWork);
}

AssistantController::~AssistantController() {
    stopService();
}

void AssistantController::testVoice(QString text) {
    m_voiceWorker->clearQueue();
    QStringList tokens = text.split(" ");
    for(QString tmpToken: tokens)
    m_voiceWorker->handleToken(tmpToken+" ");
}

void AssistantController::startService() {
    if (!m_audioThread->isRunning()) {
        m_audioThread->start();
    }
    if (!m_llmThread->isRunning()) {
        m_llmThread->start();
    }
    if (!m_voiceThread->isRunning()) {
        m_voiceThread->start();
    }
}

void AssistantController::stopService() {
    if (m_audioThread && m_audioThread->isRunning()) {
        m_audioThread->quit();
        m_audioThread->wait();
    }
    m_llmWorker->stop();
    if (m_llmThread && m_llmThread->isRunning()) {
        m_llmThread->quit();
        m_llmThread->wait();
    }
    m_voiceWorker->stop();
    if (m_voiceThread && m_voiceThread->isRunning()) {
        m_voiceThread->quit();
        m_voiceThread->wait();
    }
}

void AssistantController::generateResponse(const QString &prompt) {
    if (m_isThinking || prompt.isEmpty()) return;
    m_isThinking = true;
    Q_EMIT isThinkingChanged();
    m_llmWorker->handlePrompt(prompt);
}

