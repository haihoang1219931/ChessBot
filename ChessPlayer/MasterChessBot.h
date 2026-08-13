#ifndef MASTERCHESSBOT_H
#define MASTERCHESSBOT_H

#include <QObject>
#include <ChessBot.h>
#include <assistant/AssistantController.h>
#define USE_SYSTEM_VOICE
#if defined(USE_SYSTEM_VOICE)
#include <QTextToSpeech>
#else
#include "voice/PiperStreamer.h"
#endif

class MasterChessBot : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ChessBot* chessbot READ chessbot CONSTANT)

public:
    explicit MasterChessBot(QObject *parent = nullptr);
    ChessBot* chessbot();
    AssistantController* assistant();
    Q_INVOKABLE bool saveCalibrationData(QString fileName = CONFIGURE_CHESSBOARD_CALIB_FILE);
    Q_INVOKABLE bool loadCalibrationData(QString fileName = CONFIGURE_CHESSBOARD_CALIB_FILE);
    Q_INVOKABLE void updateCorners(QVariantList corners);
    Q_INVOKABLE void updateCalibrationData(int type, int row, int col, int x, int y);
    Q_INVOKABLE void acceptPlayFENFromHistory(bool accept);
    Q_INVOKABLE void homingRobot();
    Q_INVOKABLE void initRobotCommunication();
    Q_INVOKABLE void processNextMove();
    Q_INVOKABLE void undoMove();
    Q_INVOKABLE void setEngineElo(int score);
    Q_INVOKABLE void setPlayerColor(int color);
    Q_INVOKABLE void resetGame();
    Q_INVOKABLE void playInputMove(int startIndex, int stopIndex, int promotePiece = -1);
    Q_INVOKABLE void playInputCancelPromotion();
    Q_INVOKABLE void startService();
    Q_INVOKABLE void stopService();
    Q_INVOKABLE void sendTestCommand(QString command);
    Q_INVOKABLE int playerColor();

public Q_SLOTS:
    void handleNewComment(const QString &text);

Q_SIGNALS:
    void boardChanged(QStringList boardModel);
    void detectFailed();
    void playTurnChanged(int nextTurn);
    void gameEnded(int endState);
    void calibrationUploadProgress(int direction, int progress);
    void calibrationUploadComplete(int direction, bool success);
    void showPromotionPieces();
    void foundLastFEN();
private:
    ChessBot* m_workerChessbot;
    AssistantController* m_workerAssistant;
#if defined(USE_SYSTEM_VOICE)
    QTextToSpeech *m_speech;
#else
    PiperStreamer *m_speech;
#endif
};

#endif // MASTERCHESSBOT_H
