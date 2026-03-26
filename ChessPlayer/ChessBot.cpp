#include <QThread>
#include "ChessBot.h"
#include "SimpleChess.h"

ChessBot::ChessBot(QThread *parent) :
    QThread(parent)
{
    m_mutex = new QMutex;
    m_pauseCond = new QWaitCondition;
    m_game = new Game();
    robotController = new QSerialPort();
}

ChessBot::~ChessBot()
{
    stopService();
}
void ChessBot::run()
{
    printf("Dowork\r\n");
    m_stopped = false; // Reset flags
    while(!m_stopped){
        // Check for Stop
        m_mutex->lock();
        if(m_pause)
            m_pauseCond->wait(m_mutex); // in this place, your thread will stop to execute until someone calls resume
        m_mutex->unlock();
        switch (m_state) {
        case STATE_PLAY :{
            playLoop();
        }
            break;
        case STATE_CONFIGURE :{
            configureLoop();
        }
            break;
        case STATE_TEST :{
            testLoop();
        }
            break;
        }
        if(m_state == STATE_EXIT) {
            break;
        }
    }

    printf("Dowork finished\r\n");
}

void ChessBot::playLoop()
{
    switch (m_statePlay) {
    case PLAY_INIT: {
        m_statePlay = PLAY_DETECT_MOVE;
    }
        break;
    case PLAY_DETECT_MOVE: {
        if(playDetectMove() == STATE_DONE){
            m_statePlay = PLAY_CALCULATE_NEXT_MOVE;
        }
    }
        break;
    case PLAY_CALCULATE_NEXT_MOVE: {
        if(playCalculateNextMove() == STATE_DONE){
            m_statePlay = PLAY_EXECUTE_NEXT_MOVE;
        }
    }
        break;
    case PLAY_EXECUTE_NEXT_MOVE: {
        if(playExecuteNextMove()== STATE_DONE){
            m_statePlay = PLAY_INFORM_RESULT;
        }
    }
        break;
    case PLAY_INFORM_RESULT: {
        if(playInformResult()== STATE_DONE){
            Q_EMIT gameUpdated(getModelFromGame());
            m_statePlay = PLAY_PROCESS_DONE;
        }
    }
        break;
    case PLAY_PROCESS_DONE: {
        togglePause(true);
    }
        break;
    }
}

void ChessBot::configureLoop()
{
    switch (m_stateConfigure) {
    case CONFIGURE_CHESSBOARD: {
        if(configureChessBoardCalib() == STATE_DONE){
            m_stateConfigure = CONFIGURE_DONE;
        }
    }
        break;
    case CONFIGURE_LEVEL: {
        if(configureLevel() == STATE_DONE){
            m_stateConfigure = CONFIGURE_DONE;
        }
    }
        break;
    case CONFIGURE_SIDE: {
        if(configureSide() == STATE_DONE){
            m_stateConfigure = CONFIGURE_DONE;
        }
    }
        break;
    case CONFIGURE_DONE: {
        togglePause(true);
    }
        break;
    }
}

void ChessBot::testLoop()
{
    switch (m_stateTest) {
    case TEST_ROBOT: {
        if(testRobot() == STATE_DONE){
            m_stateTest = TEST_DONE;
        }
    }
        break;
    case TEST_DONE: {
        togglePause(true);
    }
        break;
    }
}

uint8_t ChessBot::playDetectMove()
{
    playCalculateNextMove();
    Q_EMIT gameUpdated(getModelFromGame());
    msleep(1000);
    return STATE_DONE;
}

int random(int a, int b) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(a, b);
    return dis(gen);
}
uint8_t ChessBot::playCalculateNextMove()
{
    if(m_game->state != gameState::PLAYING) {
        return STATE_DONE;
    }
    // get all pieces that are allowed to move
    std::vector<Piece*> moveable_pieces;
    pieceColor player = m_game->currentPlayer();
    m_game->calculateAllPossibleMoves(player);
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
    return STATE_DONE;
}

uint8_t ChessBot::playExecuteNextMove()
{
    // TODO: Send command to robot and wait until execution is done
    msleep(1000);
    return STATE_DONE;
}

uint8_t ChessBot::playInformResult()
{
    // TODO: Signal GUI that robot execution is done
    return STATE_DONE;
}

uint8_t ChessBot::configureChessBoardCalib()
{
    return STATE_DONE;
}

uint8_t ChessBot::configureSide()
{
    return STATE_DONE;
}

uint8_t ChessBot::configureLevel()
{
    return STATE_DONE;
}

uint8_t ChessBot::testRobot()
{
    return STATE_DONE;
}

void ChessBot::startService() {
    start();
}

void ChessBot::stopService() {

    m_state = STATE_EXIT;
    togglePause(false);
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

void ChessBot::sendTestCommand(QString command)
{
    m_state = STATE_TEST;
    m_stateTest = TEST_ROBOT;
    m_commandTest = command;
    togglePause(false);
}

void ChessBot::processNextMove()
{
    m_state = STATE_PLAY;
    m_statePlay = PLAY_INIT;
    togglePause(false);
    startService();
}

void ChessBot::setLevel(int level)
{
    printf("Set level: %d\r\n",level);
    m_levelScore = level;
    m_levelType = level/1000+1;
}

void ChessBot::setSide(int side)
{
    printf("Set side: %d\r\n",side);
    m_side = side;
    m_game->setTurn(side);
    resetGame();
}

int ChessBot::levelType()
{
    return m_levelType;
}

int ChessBot::levelScore()
{
    return m_levelScore;
}

int ChessBot::side()
{
    return m_side;
}

void ChessBot::randomMove()
{
    if(m_game->state != gameState::PLAYING) {
        switch (m_game->state) {
        case gameState::DRAW: {
            Q_EMIT gameEnded("DRAW");
        }
            break;
        case gameState::WON_BLACK: {
            Q_EMIT gameEnded("BLACK");
        }
            break;
        case gameState::WON_WHITE: {
            Q_EMIT gameEnded("WHITE");
        }
            break;
        }

    } else {
        processNextMove();
    }
}

QVariantList ChessBot::getModelFromGame()
{
    QVariantList gameModel;
    for (unsigned i=0; i < 8; ++i) {
        for (unsigned j=0; j < 8; ++j) {
            if(m_side == 0) {
                gameModel.append(QVariant::fromValue(
                    m_game->getPiece(i, j)->value()));
            } else {
                gameModel.append(QVariant::fromValue(
                    m_game->getPiece(7-i, 7-j)->value()));
            }
        }
    }
    return gameModel;
}

void ChessBot::resetGame(){
    printf("Reset game side[%d]\r\n",m_side);
    m_game->resetGame();
    Q_EMIT gameUpdated(getModelFromGame());
}
