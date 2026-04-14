#ifndef CHESSBOT_H
#define CHESSBOT_H

#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QSerialPort>

class Game;

typedef enum{
    STATE_INIT,
    STATE_PENDING,
    STATE_DONE
} STATE_ACTION;

typedef enum {
    STATE_PLAY,
    STATE_CONFIGURE,
    STATE_TEST
} STATE_CHESBOT;

typedef enum{
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
    TEST_ROBOT,
    TEST_INFORM_RESULT,
    TEST_DONE
} STATE_TEST_PHASE;
class ChessBot : public QThread {
    Q_OBJECT
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
public:
    explicit ChessBot(QThread *parent = nullptr);
    int progress();
public Q_SLOTS:
    void run() override;
    void startService();
    void stopService();
    void togglePause(bool paused);
    void sendTestCommand(QString command);
    void processNextMove();

Q_SIGNALS:
    void progressChanged(int value);

private:
    void playLoop();
    void configureLoop();
    void testLoop();
    uint8_t playDetectMove();
    uint8_t playCalculateNextMove();
    uint8_t playExecuteNextMove();
    uint8_t playInformResult();
    uint8_t configureChessBoardCalib();
    uint8_t configureSide();
    uint8_t configureLevel();
    uint8_t testRobot();

private:
    bool m_stopped = false;
    QMutex *m_mutex;
    QWaitCondition* m_pauseCond;
    Game* m_game;
    QSerialPort *robotController;
    QString m_commandTest;
    bool m_pause = false;
    int m_progress;
    int m_state;
    int m_statePlay;
    int m_stateConfigure;
    int m_stateTest;
};

#endif // CHESSBOT_H
