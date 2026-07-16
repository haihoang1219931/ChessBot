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
#if defined(_WIN32)
#include <QTextToSpeech>
#else
#include "voice/PiperStreamer.h"
#endif

#define CONFIGURE_CHESSBOARD_CALIB_FILE "calib_data.json"

class ChessController;
class MoveDetectParams;
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
    PLAY_SETUP,
    PLAY_INIT,
    PLAY_CHECK_CURRENT_MOVE,
    PLAY_DETECT_MOVE,
    PLAY_CALCULATE_NEXT_MOVE,
    PLAY_EXECUTE_NEXT_MOVE,
    PLAY_INFORM_RESULT,
    PLAY_INFORM_ERROR,
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
    INIT_GET_VERSION,
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
class ChessBot : public QThread {
    Q_OBJECT
    Q_PROPERTY(int levelType READ levelType NOTIFY levelTypeChanged)
    Q_PROPERTY(int levelScore READ levelScore NOTIFY levelScoreChanged)
    Q_PROPERTY(int side READ side NOTIFY sideChanged)
    Q_PROPERTY(QObject* chessController READ chessControllerObject CONSTANT)
public:
    explicit ChessBot(QThread *parent = nullptr);
    virtual ~ChessBot();
    int levelType();
    int levelScore();
    int side();
    QObject* chessControllerObject() const;
    ChessController* chessController();
    Q_INVOKABLE QVariantList chessboardCorners() const;
    Q_INVOKABLE QString getCalibrationJson() const;
    Q_INVOKABLE bool saveCalibrationData(QString fileName = CONFIGURE_CHESSBOARD_CALIB_FILE);
    Q_INVOKABLE bool loadCalibrationData(QString fileName = CONFIGURE_CHESSBOARD_CALIB_FILE);
    Q_INVOKABLE void updateCorners(QVariantList corners);
    Q_INVOKABLE void updateCalibrationData(int type, int row, int col, int x, int y);

public Q_SLOTS:
    void run() override;
    void startService();
    void stopService();
    void togglePause(bool paused);
    void sendTestCommand(QString command);
    void initRobotCommunication();
    void processNextMove();
    void undoMove();
    void setLevel(int level);
    void setSide(int side);
    void resetGame();
    void connectCamera();
    void disconnectCamera();
    void speakText(const QString &text);
    void speakMove(const QString &piece, const QString &move);
    void playInputMove(int startIndex, int stopIndex, int promotePiece = -1);
    void playInputCancelPromotion();

Q_SIGNALS:
    void detectFailed();
    void playTurnChanged(int nextTurn);
    void gameEnded(int endState);
    void sideChanged(int side);
    void levelTypeChanged(int type);
    void levelScoreChanged(int score);
    void calibrationUploadProgress(int direction, int progress);
    void calibrationUploadComplete(int direction, bool success);
    void showPromotionPieces();

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
    bool getArduinoVersion();
    bool readCalibrationPoint(const QString &command, QPoint& point);
    bool sendCalibrationCells();
    void abortCalibrationUpload();
    bool waitForCalibrationProgress();
    uint8_t enableRobot();
    uint8_t goHome();
    bool isCalibDataLoaded();
    QPoint notationToCoord(const std::string& notation, const std::string& playerSide);

private:
    bool m_stopped = false;
    QMutex *m_mutex;
    QWaitCondition* m_pauseCond;
    ChessController* m_chessController;
#if defined(_WIN32)
    QTextToSpeech *m_speech;
#else
    PiperStreamer *m_speech;
#endif
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
    int m_levelType;
    int m_levelScore;
    int m_side;
    int m_width;
    int m_height;
    MoveDetectParams* m_detectParams;
    QString m_arduinoVersion;
    QVector<QPoint> m_chessboardConners; // 4 corners
    QVector<QVector<QPoint>> m_chessboardCalib;    // 8x8 chessboard
    QVector<QVector<QPoint>> m_dropzoneRightCalib; // 8x2 right
    QVector<QVector<QPoint>> m_dropzoneLeftCalib;  // 8x2 left
    int m_calibRow;
    int m_calibCol;
    int m_calibCellCount;
    QString m_cmdId;
    bool m_validCalibFileFound;
    QString m_robotCommand;
};

#endif // CHESSBOT_H
