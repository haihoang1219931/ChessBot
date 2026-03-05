#include "ChessBot.h"
#include <QThread>
ChessBot::ChessBot(QThread *parent) :
  QThread(parent)
{
    m_mutex = new QMutex;
        m_pauseCond = new QWaitCondition;
}

void ChessBot::run()
{
    printf("Dowork\r\n");
    m_stopped = false; // Reset flags
    int i = 0;
    while(!m_stopped){
        // Check for Stop
        m_mutex->lock();
                if(m_pause)
                    m_pauseCond->wait(m_mutex); // in this place, your thread will stop to execute until someone calls resume
                m_mutex->unlock();

        // Simulate work
        QThread::msleep(1);
        printf("process %d\r\n",i);
        i++;
        m_progress = i;
        Q_EMIT progressChanged(i%101);
    }

    printf("Dowork finished\r\n");
}

int ChessBot::progress()
{
    return m_progress;
}

void ChessBot::startService() {
    start();
}

void ChessBot::stopService() {

    if (this->isRunning()) {
        m_stopped = true;         // Signal the loop to break
        this->quit();      // Tell the event loop to exit
        this->wait();      // BLOCK until the thread is fully dead
    }
}

void ChessBot::togglePause(bool paused)
{
    if(paused == true){
        m_mutex->lock();
        m_pause = true;
        m_mutex->unlock();
    }else{
        m_mutex->lock();
        m_pause = false;
        m_mutex->unlock();
        m_pauseCond->wakeAll();
    }
}
