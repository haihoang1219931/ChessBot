#ifndef MASTERCHESSBOT_H
#define MASTERCHESSBOT_H

#include <QObject>
#include "ChessBot.h"
#if defined(USE_AI_ASSISTANT)
#include "assistant/AssistantController.h"
#endif

class MasterChessBot : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ChessBot* chessbot READ chessbot CONSTANT)

public:
    explicit MasterChessBot(QObject *parent = nullptr);
    ChessBot* chessbot();
#if defined(USE_AI_ASSISTANT)
    AssistantController* assistant();
#endif
    Q_INVOKABLE bool saveCalibrationData(QString fileName = CONFIGURE_CHESSBOARD_CALIB_FILE);
    Q_INVOKABLE bool loadCalibrationData(QString fileName = CONFIGURE_CHESSBOARD_CALIB_FILE);
    Q_INVOKABLE void updateCorners(QVariantList corners);
    Q_INVOKABLE void updateCalibrationData(int type, int row, int col, int x, int y);
    Q_INVOKABLE void acceptPlayFENFromHistory(bool accept);
    Q_INVOKABLE void homingRobot();
    Q_INVOKABLE void initRobotCommunication();
    Q_INVOKABLE void processNextMove();
    Q_INVOKABLE void undoMove();
    Q_INVOKABLE void setEngineElo(QString level, int score);
    Q_INVOKABLE void setPlayerColor(int color);
    Q_INVOKABLE void resetGame();
    Q_INVOKABLE void playInputMove(int startIndex, int stopIndex, int promotePiece = -1);
    Q_INVOKABLE void playInputCancelPromotion();
    Q_INVOKABLE void startService();
    Q_INVOKABLE void stopService();
    Q_INVOKABLE void sendTestCommand(QString command);
    Q_INVOKABLE int playerColor();
    Q_INVOKABLE void classifyImage();
    Q_INVOKABLE bool isClassificationDone();
    Q_INVOKABLE QString getCalibrationJson() const;
    Q_INVOKABLE QSize getImageSize() const;
    Q_INVOKABLE QVariantList chessboardCorners() const;
    Q_INVOKABLE void stopGame(QString comment);
    Q_INVOKABLE int timerLimit() const;
    Q_INVOKABLE void setTimeLimit(int timeout) const;
    Q_INVOKABLE QString botName() const;
    Q_INVOKABLE QString playerName() const;

Q_SIGNALS:
    void boardChanged(QStringList boardModel);
    void detectFailed();
    void playTurnChanged(int nextTurn);
    void gameEnded(int endState);
    void calibrationUploadProgress(int direction, int progress);
    void calibrationUploadComplete(int direction, bool success);
    void showPromotionPieces();
    void foundLastFEN();
    void preprocessDone(QString imagePath);
    void classificationDone(QStringList boardModel,
                            QStringList boardModelReverted);


private:
    ChessBot* m_workerChessbot;
#if defined(USE_AI_ASSISTANT)
    AssistantController* m_workerAssistant;
#endif
};

#endif // MASTERCHESSBOT_H
