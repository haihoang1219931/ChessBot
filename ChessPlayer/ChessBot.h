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

class ChessController;
#ifdef IMAGE_PROCESS_MOVE
class ChessImageProcessing;
#endif
typedef enum{
    STATE_INIT,
    STATE_PENDING,
    STATE_DONE
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
    PLAY_DETECT_MOVE,
    PLAY_CALCULATE_NEXT_MOVE,
    PLAY_EXECUTE_NEXT_MOVE,
    PLAY_INFORM_RESULT,
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
    INIT_CHECK_CALIB_FILE,
    INIT_REQUEST_CALIB_CHESSBOARD,
    INIT_REQUEST_CALIB_RIGHT_DROPZONE,
    INIT_REQUEST_CALIB_LEFT_DROPZONE,
    INIT_DONE
} STATE_INIT_PHASE;

typedef enum{
    TEST_ROBOT,
    TEST_INFORM_RESULT,
    TEST_DONE
} STATE_TEST_PHASE;
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
    void updateCorners(QPoint c1, QPoint c2,QPoint c3, QPoint c4);
    QObject* chessControllerObject() const;
    ChessController* chessController();
public Q_SLOTS:
    void run() override;
    void startService();
    void stopService();
    void togglePause(bool paused);
    void sendTestCommand(QString command);
    void initRobotCommunication();
    void processNextMove();
    void setLevel(int level);
    void setSide(int side);
    void randomMove();
    void resetGame();
    void loadCorners(QString file);
    void connectCamera();
    void disconnectCamera();

Q_SIGNALS:
    void gameEnded(int endState);
    void sideChanged(int side);
    void levelTypeChanged(int type);
    void levelScoreChanged(int score);

private:
    void playLoop();
    void configureLoop();
    void testLoop();
    uint8_t playDetectMove();
    uint8_t playRandomMove();
    uint8_t playCalculateNextMove();
    uint8_t playExecuteNextMove();
    uint8_t playInformResult();
    uint8_t configureChessBoardCalib();
    uint8_t configureSide();
    uint8_t configureLevel();
    uint8_t testRobot();
    void initRobot();
    bool detectArduinoPort(int baudRate = 38400);
    bool getArduinoVersion();
    QPoint readCalibrationPoint(const QString &command);
    bool saveCalibrationData(const QString &fileName);

private:
    bool m_stopped = false;
    QMutex *m_mutex;
    QWaitCondition* m_pauseCond;
    ChessController* m_chessController;
#ifdef IMAGE_PROCESS_MOVE
    ChessImageProcessing* m_moveDetector;
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
    QString m_arduinoVersion;
    QVector<QVector<QPoint>> m_chessboardCalib;    // 8x8 chessboard
    QVector<QVector<QPoint>> m_dropzoneRightCalib; // 8x2 right
    QVector<QVector<QPoint>> m_dropzoneLeftCalib;  // 8x2 left
    int m_calibRow;
    int m_calibCol;
};

#endif // CHESSBOT_H
