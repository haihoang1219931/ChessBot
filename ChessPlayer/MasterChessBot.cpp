#include "MasterChessBot.h"
#include <QDebug>
#include "ChessController.h"
MasterChessBot::MasterChessBot(QObject *parent) : QObject(parent)
{
    m_workerChessbot = new ChessBot();
    m_workerAssistant = new AssistantController();
#if defined(USE_SYSTEM_VOICE)
    // 1. Check if engines exist on your OS
    qDebug() << "Available TTS Engines:" << QTextToSpeech::availableEngines();

    m_speech = new QTextToSpeech();
    // Explicitly enforce the system language to kickstart SAPI
    m_speech->setLocale(QLocale::system());
    // 2. Print current engine state (Should be Ready)
    qDebug() << "Current TTS Engine:" << m_speech->availableEngines();
    qDebug() << "Initial State:" << m_speech->state();

    // 3. Optional: Connect a debug log to trace status changes
    connect(m_speech, &QTextToSpeech::stateChanged, [](QTextToSpeech::State state) {
        qDebug() << "TTS State Changed to:" << state;
    });
#else
    m_speech = new PiperStreamer();
#endif
    handleNewComment("I'm chess robot. Nice to play");
    connect(m_workerChessbot, &ChessBot::newCommentAdded,
            this, &MasterChessBot::handleNewComment);
    connect(m_workerAssistant, &AssistantController::responseTextChanged,
            this, &MasterChessBot::handleNewComment);
}

ChessBot* MasterChessBot::chessbot()
{
    return m_workerChessbot;
}

AssistantController* MasterChessBot::assistant()
{
    return m_workerAssistant;
}

bool MasterChessBot::saveCalibrationData(QString fileName)
{
    return m_workerChessbot->saveCalibrationData(fileName);
}

bool MasterChessBot::loadCalibrationData(QString fileName)
{
    return m_workerChessbot->loadCalibrationData(fileName);
}

void MasterChessBot::updateCorners(QVariantList corners)
{
    m_workerChessbot->updateCorners(corners);
}

void MasterChessBot::updateCalibrationData(int type, int row, int col, int x, int y)
{
    m_workerChessbot->updateCalibrationData(type,row,col,x,y);
}

void MasterChessBot::acceptPlayFENFromHistory(bool accept)
{
    m_workerChessbot->acceptPlayFENFromHistory(accept);
}

void MasterChessBot::homingRobot()
{
    m_workerChessbot->homingRobot();
}

void MasterChessBot::initRobotCommunication()
{
    m_workerChessbot->initRobotCommunication();
}

void MasterChessBot::processNextMove()
{
    m_workerChessbot->processNextMove();
}

void MasterChessBot::undoMove()
{
    m_workerChessbot->undoMove();
}

void MasterChessBot::setEngineElo(int score)
{
    m_workerChessbot->setEngineElo(score);
}
void MasterChessBot::setPlayerColor(int color)
{
    m_workerChessbot->setPlayerColor(color);
}
void MasterChessBot::resetGame()
{
    m_workerChessbot->resetGame();
}

void MasterChessBot::playInputMove(int startIndex, int stopIndex, int promotePiece)
{
    m_workerChessbot->playInputMove(startIndex,stopIndex,promotePiece);
}

void MasterChessBot::playInputCancelPromotion()
{
    m_workerChessbot->playInputCancelPromotion();
}

void MasterChessBot::startService()
{
    m_workerChessbot->startService();
    m_workerAssistant->startService();
}

void MasterChessBot::stopService()
{
    m_workerChessbot->stopService();
    m_workerAssistant->stopService();
}

void MasterChessBot::sendTestCommand(QString command)
{
    m_workerChessbot->sendTestCommand(command);
}

int MasterChessBot::playerColor()
{
    return m_workerChessbot->chessController()->playerColor();
}

void MasterChessBot::handleNewComment(const QString &text)
{
#if defined(USE_SYSTEM_VOICE)
    m_speech->say(text.trimmed());
#else
    m_speech->speak(text);
#endif
}
