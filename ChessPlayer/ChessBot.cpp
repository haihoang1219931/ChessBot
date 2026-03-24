#include <QThread>
#include "ChessBot.h"
#include "SimpleChess.h"

ChessBot::ChessBot(QThread *parent) :
  QThread(parent)
{
    m_mutex = new QMutex;
    m_pauseCond = new QWaitCondition;
    m_game = new Game();
}

void ChessBot::run()
{
    printf("Dowork\r\n");
    m_stopped = false; // Reset flags
    int i = 0;
    m_state = DETECT_MOVE;
    while(!m_stopped){
        // Check for Stop
        m_mutex->lock();
        if(m_pause)
            m_pauseCond->wait(m_mutex); // in this place, your thread will stop to execute until someone calls resume
        m_mutex->unlock();
        switch (m_state) {
        case DETECT_MOVE: {
            detectMove();
        }
            break;
        case CALCULATE_NEXT_MOVE: {
            calculateNextMove();
        }
            break;
        case EXECUTE_NEXT_MOVE: {
            executeNextMove();
        }
            break;
        case INFORM_RESULT: {
            informResult();
        }
            break;
        case PROCESS_DONE: {
            m_stopped = true;
        }
            break;
        }
        // Simulate work
        QThread::msleep(1);
        printf("process %d\r\n",i);
        i++;
        m_progress = i;
        Q_EMIT progressChanged(i%101);
    }

    printf("Dowork finished\r\n");
}

void ChessBot::detectMove()
{
    // TODO: Update current game
    m_state = CALCULATE_NEXT_MOVE;
}

int random(int a, int b) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(a, b);
    return dis(gen);
}
void ChessBot::calculateNextMove()
{
    // get all pieces that are allowed to move
    std::vector<Piece*> moveable_pieces;
    pieceColor player = m_game->currentPlayer();
    for (unsigned i=0; i < 8; ++i) {
        for (unsigned j=0; j < 8; ++j) {
            Piece* p = m_game->getPiece(i, j);
            if (p->color == player && !p->legalMoves.empty())
                moveable_pieces.push_back(p);
        }
    }
    // pick one and make a random legal move with it
    Piece* p = moveable_pieces.at( random(0, moveable_pieces.size()-1) );
    m_game->move(p->legalMoves.at( random(0, p->legalMoves.size()-1) ));
    m_state = EXECUTE_NEXT_MOVE;
}

void ChessBot::executeNextMove()
{
    // TODO: Send command to robot and wait until execution is done
//    m_
}

void ChessBot::informResult()
{
    // TODO: Signal GUI that robot execution is done
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
