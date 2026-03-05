#ifndef CHESSBOT_H
#define CHESSBOT_H

#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>

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
    bool m_stopped = false;
    QMutex *m_mutex;
    QWaitCondition* m_pauseCond;
    bool m_pause = false;
    int m_progress;
};

#endif // CHESSBOT_H
