#ifndef CHESSBOT_H
#define CHESSBOT_H

#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QVariant>
#include <QVariantList>
#include <QPoint>
#include <QVector>

#define CONFIGURE_CHESSBOARD_CALIB_FILE "calib_data.json"
typedef enum {
    ZONE_BOT,
    ZONE_PLAYER
}ZONE_TYPE;

class ChessController;
class MoveDetectParams;
class Move;
#ifdef IMAGE_PROCESS_MOVE
    #include "ChessImageProcessing.h"
#endif

typedef enum{
    INIT_COMMUNICATION,
    CALIB_UPLOAD_TO_ROBOT,
    CALIB_REQUEST_FROM_ROBOT,
    ENABLE_ROBOT,
    HOMING_ROBOT,
} INIT_DIRECTION;

typedef enum{
    STATE_INIT,
    STATE_PENDING,
    STATE_DONE_FAIL,
    STATE_DONE_SUCCESS
} STATE_ACTION;

typedef enum {
    STATE_INIT_COM,
    STATE_PLAY,
    STATE_CONFIGURE,
    STATE_TEST,
    STATE_EXIT,
} STATE_CHESBOT;

typedef enum{
    PLAY_CHECK_LOG,
    PLAY_CALCULATE_NEXT_MOVE_RESET,
    PLAY_SETUP,
    PLAY_INIT,
    PLAY_CHECK_CURRENT_MOVE,
    PLAY_DETECT_MOVE,
    PLAY_CALCULATE_NEXT_MOVE,
    PLAY_EXECUTE_NEXT_MOVE,
    PLAY_INFORM_RESULT,
    PLAY_INFORM_ERROR,
    PLAY_INFORM_ERROR_CALCULATE_NEXT_MOVE,
    PLAY_INFORM_ERROR_EXECUTE_NEXT_MOVE,
    PLAY_REQUEST_PROMOTE_PIECE,
    PLAY_ENDGAME_TIMEOUT,
    PLAY_PROCESS_DONE,
} STATE_PLAY_PHASE;

typedef enum{
    CONFIGURE_CHESSBOARD,
    CONFIGURE_SIDE,
    CONFIGURE_LEVEL,
    CONFIGURE_INFORM_RESULT,
    CONFIGURE_DONE
} STATE_CONFIGURE_PHASE;

typedef enum{
    INIT_DETECT_PORT,
    INIT_SEND_CALIBRATION,
    INIT_REQUEST_CALIB_CHESSBOARD,
    INIT_REQUEST_CALIB_RIGHT_DROPZONE,
    INIT_REQUEST_CALIB_LEFT_DROPZONE,
    INIT_ENABLE_ROBOT,
    INIT_GO_HOME,
    INIT_DONE
} STATE_INIT_PHASE;

typedef enum{
    TEST_ROBOT,
    TEST_CHECK_RESULT,
    TEST_DONE
} STATE_TEST_PHASE;

typedef enum{
    PIECE_MOVE_NORMAL,
    PIECE_MOVE_CAPTURE,
    PIECE_MOVE_ENPASSANT
} PIECE_MOVE_TYPE;

struct GameInfo {
    std::string timestamp = "";
    std::string fen = "";
    std::string turn = "";
    bool success = false;
};

typedef struct{
    int rowID;
    int colID;
    ZONE_TYPE zoneType;
}DropPoint;

class ChessBot : public QThread {
    Q_OBJECT
    Q_PROPERTY(QObject* chessController READ chessControllerObject CONSTANT)
public:
    explicit ChessBot(QThread *parent = nullptr);
    virtual ~ChessBot();
    QObject* chessControllerObject() const;
    ChessController* chessController();
    QVariantList chessboardCorners() const;
    QString getCalibrationJson() const;
    bool saveCalibrationData(QString fileName = CONFIGURE_CHESSBOARD_CALIB_FILE);
    bool loadCalibrationData(QString fileName = CONFIGURE_CHESSBOARD_CALIB_FILE);
    void updateCorners(QVariantList corners);
    void updateCalibrationData(int type, int row, int col, int x, int y);
    void acceptPlayFENFromHistory(bool accept);

    void run() override;
    void startService();
    void stopService();
    void togglePause(bool paused);
    void sendTestCommand(QString command);
    int executeCommand(QString command);
    void homingRobot();
    void initRobotCommunication();
    void processNextMove();
    void undoMove();
    void resetGame();
    void setEngineElo(QString level, int score);
    void setPlayerColor(int color);
    void playInputMove(int startIndex, int stopIndex, int promotePiece = -1);
    void playInputCancelPromotion();
    void stopGame(QString comment);

Q_SIGNALS:
    void boardChanged(QStringList boardModel);
    void detectFailed();
    void playTurnChanged(int nextTurn);
    void gameEnded(int endState);
    void calibrationUploadProgress(int direction, int progress);
    void calibrationUploadComplete(int direction, bool success);
    void showPromotionPieces();
    void foundLastFEN();
    void newCommentAdded(QString text);
    void newMoveAdded(QString fen, QString playColor, QString move);

private:
    void playLoop();
    void configureLoop();
    void testLoop();
    bool playCheckEndGame();
    bool playCheckDoubleMove();
    bool canMoveStraight(int startRow, int startCol, int stopRow, int stopCol, PIECE_MOVE_TYPE moveType = PIECE_MOVE_NORMAL);
    uint8_t playDetectMove();
    uint8_t playRandomMove();
    uint8_t playCalculateNextMove();
    uint8_t playExecuteNextMove();
    uint8_t playInformResult();
    uint8_t configureChessBoardCalib();
    uint8_t configureSide();
    uint8_t configureLevel();
    uint8_t testRobot();
    uint8_t testCheckResult();
    void initRobot();
    bool detectArduinoPort(int baudRate = 38400);
    bool readCalibrationPoint(const QString &command, QPoint& point);
    bool sendCalibrationCells();
    void abortCalibrationUpload();
    bool waitForCalibrationProgress();
    uint8_t enableRobot();
    uint8_t goHome();
    bool isCalibDataLoaded();
    QPoint notationToCoord(const std::string& notation, const std::string& playerSide);
    void logWithTimestampQt(QString data);
    GameInfo getLastChessState(const std::string& filepath);
    QString getLatestLogFile(const QString& folderPath);
    bool findLastFENInLog();
    bool sendRobotCommand(const char* cmd, int waitTime = 200);
    QString readRobotResponse(int waitTime = 500);
    bool getFreeDropPoint(DropPoint& result, uint8_t promotePiece = 0);
    void resetDropZoneMap(int playerColor);
    void updateDropZone(uint8_t piece, int row, int col, ZONE_TYPE zone);
    char pieceName(int piece, int color);
#ifdef IMAGE_PROCESS_MOVE
    void processAndSaveFailures(const cv::Mat& imageBefore, const cv::Mat& imageAfter);
    int getNextFileCounter(const std::string& folderPath);
    void makeDirectory(const std::string& path);
    std::string formatFilename(const std::string& folder, int number);
#endif

private:
    bool m_stopped = false;
    QMutex *m_mutex;
    QWaitCondition* m_pauseCond;
    ChessController* m_chessController;
#ifdef IMAGE_PROCESS_MOVE
    ChessImageProcessing* m_moveDetector;
    cv::VideoCapture cap;
    cv::Mat imageBefore,imageAfter;
    bool readFrame(cv::Mat& outImg);
#endif
    QSerialPort *robotController;
    QString m_commandTest;
    bool m_pause = false;
    int m_state;
    int m_statePlay;
    int m_stateConfigure;
    int m_stateTest;
    int m_stateInit;
    int m_width;
    int m_height;
#ifdef IMAGE_PROCESS_MOVE
    MoveDetectParams* m_detectParams;
#endif
    QString m_arduinoVersion;
    QVector<QPoint> m_chessboardConners; // 4 corners
    QVector<QVector<QPoint>> m_chessboardCalib;    // 8x8 chessboard
    QVector<QVector<QPoint>> m_dropzoneRightCalib; // 8x2 right
    QVector<QVector<QPoint>> m_dropzoneLeftCalib;  // 8x2 left
    uint8_t m_dropZoneMapPlayer[8][2];
    uint8_t m_dropZoneMapBot[8][2];
    int m_calibRow;
    int m_calibCol;
    int m_calibCellCount;
    QString m_cmdId;
    bool m_validCalibFileFound;
    char m_robotCommand[32];
    GameInfo m_lastGame;
    QString m_timeoutComment;
};

#endif // CHESSBOT_H
