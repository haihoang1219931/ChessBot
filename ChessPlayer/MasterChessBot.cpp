#include "MasterChessBot.h"
#include <QDebug>
#include "ChessController.h"
MasterChessBot::MasterChessBot(QObject *parent) : QObject(parent)
{
    m_workerChessbot = new ChessBot();
    m_workerAssistant = new AssistantController();
    m_speech = new VoiceStreamer();
    handleNewComment("I'm chess robot. Nice to play");
//    connect(m_workerChessbot, &ChessBot::newCommentAdded,
//            this, &MasterChessBot::handleNewComment);
//    connect(m_workerAssistant, &AssistantController::generationFinished,
//            this, &MasterChessBot::handleNewComment);

    connect(m_workerChessbot, &ChessBot::boardChanged,
            this, &MasterChessBot::boardChanged);
    connect(m_workerChessbot, &ChessBot::detectFailed,
            this, &MasterChessBot::detectFailed);
    connect(m_workerChessbot, &ChessBot::playTurnChanged,
            this, &MasterChessBot::playTurnChanged);
    connect(m_workerChessbot, &ChessBot::gameEnded,
            this, &MasterChessBot::gameEnded);
    connect(m_workerChessbot, &ChessBot::calibrationUploadProgress,
            this, &MasterChessBot::calibrationUploadProgress);
    connect(m_workerChessbot, &ChessBot::calibrationUploadComplete,
            this, &MasterChessBot::calibrationUploadComplete);
    connect(m_workerChessbot, &ChessBot::showPromotionPieces,
            this, &MasterChessBot::showPromotionPieces);
    connect(m_workerChessbot, &ChessBot::foundLastFEN,
            this, &MasterChessBot::foundLastFEN);
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

void MasterChessBot::setEngineElo(QString level, int score)
{
    m_workerChessbot->setEngineElo(level, score);
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
//    m_workerAssistant->startService();
}

void MasterChessBot::stopService()
{
    m_workerChessbot->stopService();
//    m_workerAssistant->stopService();
}

void MasterChessBot::sendTestCommand(QString command)
{
    m_workerChessbot->sendTestCommand(command);
}

int MasterChessBot::playerColor()
{
    return m_workerChessbot->chessController()->playerColor();
}

QString MasterChessBot::getCalibrationJson() const
{
    return m_workerChessbot->getCalibrationJson();
}

QVariantList MasterChessBot::chessboardCorners() const
{
    return m_workerChessbot->chessboardCorners();
}

void MasterChessBot::stopGame(QString comment)
{
    m_workerChessbot->stopGame(comment);
}

void MasterChessBot::handleNewComment(const QString &text)
{
    m_speech->speak(text);
}
