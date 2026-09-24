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
        m_llmWorker->handleSpeech(pcmData);
    });

    connect(m_llmWorker, &LLMWorker::tokenGenerated, this, [this](QString text) {
        m_responseText += text;
//        m_voiceWorker->handleToken(text);
        Q_EMIT responseTextChanged(m_responseText);
    });
    connect(m_llmWorker, &LLMWorker::generationFinished, this, [this](QString text) {
        qDebug("AssistantController handle llmFinish");
        m_responseText = text;
        m_isThinking = false;
        Q_EMIT isThinkingChanged();
        m_voiceWorker->handleToken(m_responseText);
        Q_EMIT responseTextChanged(m_responseText);
        m_responseText = "";
    });

    connect(m_voiceWorker, &AudioOutputWorker::voiceStarted, this, [this]() {
        qDebug("AssistantController pause listening");
        m_audioWorker->togglePause(true);
    });

    connect(m_voiceWorker, &AudioOutputWorker::voiceFinished, this, [this]() {
        qDebug("AssistantController return listening");
        m_audioWorker->togglePause(false);
    });

//    connect(m_audioThread, &QThread::finished, m_audioWorker, &QObject::deleteLater);
//    connect(m_llmThread, &QThread::started, m_llmWorker, &LLMWorker::doWork);
    connect(m_voiceThread, &QThread::started, m_voiceWorker, &AudioOutputWorker::doWork);
    m_playerName = "Player";
}

AssistantController::~AssistantController() {
    stopService();
}

void AssistantController::startService() {
//    if (!m_audioThread->isRunning()) {
//        m_audioThread->start();
//    }
//    if (!m_llmThread->isRunning()) {
//        m_llmThread->start();
//    }
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

void AssistantController::singleVoice(QString text) {
    qDebug("AssistantController singleVoice [%s]",text.toStdString().c_str());
//    m_audioWorker->togglePause(true);
//    m_voiceWorker->clearQueue();
//    QStringList tokens = text.split(" ");
//    for(QString tmpToken: tokens)
//        m_voiceWorker->handleToken(tmpToken+" ");
//    m_voiceWorker->handleToken(".");
    m_voiceWorker->handleToken(text+".");
}

void AssistantController::analyzeChessMove(QString fen, QString playColor, QString move) {
    m_audioWorker->togglePause(true);
    if (m_isThinking || fen.isEmpty() || playColor.isEmpty() || move.isEmpty()) return;
    m_isThinking = true;
    Q_EMIT isThinkingChanged();
    m_llmWorker->analyzeChessMove(fen,playColor,move);
}

void AssistantController::generateResponse(const QString &prompt) {
    if (m_isThinking || prompt.isEmpty()) return;
    m_isThinking = true;
    Q_EMIT isThinkingChanged();
    m_llmWorker->handlePrompt(prompt);
}

void AssistantController::setAIModel(const QString& botName,
                    const QString& playerName,
                    const QString& whisperModelPath,
                    const QString& llmModelPath,
                    const QString& piperExePath,
                    const QString& piperModelPath) {
    m_llmWorker->setModel(botName,whisperModelPath, llmModelPath);
    m_llmWorker->initializeLlama();
    m_llmWorker->initializeWhisper();
    m_voiceWorker->setVoiceModel(botName,piperExePath,piperModelPath);
    m_playerName = playerName;
}

QString AssistantController::playerName()
{
    return m_playerName;
}
