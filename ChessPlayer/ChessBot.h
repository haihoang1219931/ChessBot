#ifndef CHESSBOT_H
#define CHESSBOT_H

#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
class Game;
typedef enum{
    DETECT_MOVE,
    CALCULATE_NEXT_MOVE,
    EXECUTE_NEXT_MOVE,
    INFORM_RESULT,
    PROCESS_DONE,
} PROCESSING_PHASE;
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

Q_SIGNALS:
    void progressChanged(int value);

private:
    void detectMove();
    void calculateNextMove();
    void executeNextMove();
    void informResult();

private:
    bool m_stopped = false;
    QMutex *m_mutex;
    QWaitCondition* m_pauseCond;
    Game* m_game;
    bool m_pause = false;
    int m_progress;
    int m_state;
};

#endif // CHESSBOT_H
