#include <QThread>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>
#include <QFile>
#include "ChessBot.h"
#include "chessAlgo/ChessController.h"

#ifdef IMAGE_PROCESS_MOVE
    #include "ChessImageProcessing.h"
    static cv::VideoCapture cap;
    static cv::Mat imageBefore,imageAfter;
    static bool readFrame(cv::Mat& outImg);
#endif


ChessBot::ChessBot(QThread *parent) :
    QThread(parent)
{
    m_mutex = new QMutex;
    m_pauseCond = new QWaitCondition;
    m_chessController = new ChessController();
#ifdef IMAGE_PROCESS_MOVE
    m_moveDetector = new ChessImageProcessing();
#endif
    robotController = new QSerialPort();
    loadCorners("trapezoid_data.json");
#ifdef IMAGE_PROCESS_MOVE
    std::vector<cv::Rect> moves;
    cv::Mat src1 = cv::imread("1.jpg");
    cv::Mat src2 = cv::imread("2.jpg");
    if(!src1.empty() && !src2.empty()) {
        m_moveDetector->extractMove(src1, src2, moves);
        std::vector<cv::Point> chessMoves;
        m_moveDetector->convertChessMove(moves, chessMoves);
        for(int i = 0; i< chessMoves.size(); i++) {
            printf("Move(%d,%d)\r\n",chessMoves[i].x,chessMoves[i].y);
        }
    }
#endif
}

ChessBot::~ChessBot()
{
    stopService();
}
#ifdef IMAGE_PROCESS_MOVE
bool readFrame(cv::Mat& outImg)
{
    bool readResult = false;
    if (!cap.isOpened()) {
        cap.open(0);
    }
    if (cap.isOpened()) {
        readResult = cap.read(outImg);
        cap.release();
    }
    return readResult;
}
#endif
void ChessBot::connectCamera()
{
#ifdef IMAGE_PROCESS_MOVE
    cap.open(0);
    if (!cap.isOpened()) {
        printf("Error: Could not open camera.\r\n");
    }
#endif
}

void ChessBot::disconnectCamera()
{
#ifdef IMAGE_PROCESS_MOVE
    if(cap.isOpened()) {
        cap.release();
    }
#endif
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
    case PLAY_SETUP: {
#ifdef IMAGE_PROCESS_MOVE
        readFrame(imageBefore);
        printf("First image [%d,%d]\r\n",
               imageBefore.rows,imageBefore.cols);
#endif
        m_statePlay = PLAY_PROCESS_DONE;
    }
        break;
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
    printf("playDetectMove\r\n");
#ifndef IMAGE_PROCESS_MOVE
    playRandomMove();
#else
    if(!readFrame(imageAfter)){
        return STATE_DONE;
    }
    std::vector<cv::Rect> moves;
    m_moveDetector->extractMove(imageBefore, imageAfter, moves);
    std::vector<cv::Point> chessMoves;
    m_moveDetector->convertChessMove(moves, chessMoves);
//    chessMoves.push_back(cv::Point(0,6));
//    chessMoves.push_back(cv::Point(2,5));
    // convert
    for(int i=0; i< chessMoves.size(); i++)
    {
        int temp = chessMoves[i].x;
        chessMoves[i].x = chessMoves[i].y;
        chessMoves[i].y = temp;
    }
    if(chessMoves.size()>=2) {
        cv::Point startMove;
        cv::Point stopMove;
        for(int i=0; i< chessMoves.size(); i++) {
            if(m_side == 1) {
                chessMoves[i].x = 7 - chessMoves[i].x;
                chessMoves[i].y = 7 - chessMoves[i].y;
            }
            printf("(%d,%d) name[%d] color[%d]\r\n",
                   chessMoves[i].x,chessMoves[i].y,
                   m_game->getPiece(chessMoves[i].x,chessMoves[i].y)->name,
                   m_game->getPiece(chessMoves[i].x,chessMoves[i].y)->color);
            if(m_game->getPiece(chessMoves[i].x,chessMoves[i].y)->name != pieceName::EMPTY &&
               (int)m_game->getPiece(chessMoves[i].x,chessMoves[i].y)->color == (int)pieceColor::EMPTY+m_side+1){
                startMove.x=chessMoves[i].x;
                startMove.y=chessMoves[i].y;
                printf("startMove (%d,%d)\r\n",startMove.x,startMove.y);
            } else {
                stopMove.x=chessMoves[i].x;
                stopMove.y=chessMoves[i].y;
                printf("stopMove (%d,%d)\r\n",stopMove.x,stopMove.y);
            }
        }
        m_game->move(Move(startMove.x,startMove.y,
                        stopMove.x,stopMove.y));
        print(*m_game);
    }
#endif
    return STATE_DONE;
}

uint8_t ChessBot::playRandomMove()
{
    QStringList randomMoves = m_chessController->findBestMoveCoordinates();
    m_chessController->moveByCoordinates(randomMoves[0],randomMoves[1]);
    return STATE_DONE;
}

uint8_t ChessBot::playCalculateNextMove()
{

    return STATE_DONE;
}

uint8_t ChessBot::playExecuteNextMove()
{
    // TODO: Send command to robot and wait until execution is done
    return STATE_DONE;
}

uint8_t ChessBot::playInformResult()
{
    // TODO: Signal GUI that robot execution is done
#ifdef IMAGE_PROCESS_MOVE
    readFrame(imageBefore);
#endif
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
    m_levelType = level/400+1;
    m_chessController->setEngineLevel(m_levelType);
}

void ChessBot::setSide(int side)
{
    printf("Set side: %d\r\n",side);
    m_side = side;
    m_chessController->setPlayerColor(side);
    resetGame();
    m_state = STATE_PLAY;
    m_statePlay = PLAY_SETUP;
    togglePause(false);
    startService();
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

void ChessBot::loadCorners(QString fileName)
{
    QVector<QPoint> points;
    QFile file(fileName);

    // Open the file in read-only mode
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        printf("Could not open file for reading: %s\r\n",fileName.toStdString().c_str());
        return;
    }

    // Read all data and parse into a JSON document
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);

    // Ensure the root of the JSON is an array
    if (!doc.isArray()) {
        printf("JSON format error: Root is not an array.\r\n");
        return;
    }

    QJsonArray jsonArray = doc.array();
    int cornerID = 0;
    for (const QJsonValue &value : jsonArray) {
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            // Extract x and y, then append as a QPoint
            points.append(QPoint(obj["x"].toInt(), obj["y"].toInt()));
            printf("corner[%d] (%d,%d)\r\n",cornerID,
                   obj["x"].toInt(),obj["y"].toInt());
            cornerID++;

        }
    }
#ifdef IMAGE_PROCESS_MOVE
    if(points.size() == 4){
        m_moveDetector->corners().clear();
        for(QPoint corner: points){
            m_moveDetector->corners().push_back(
                        cv::Point(corner.x(),corner.y()));
        }
    }
    int threshold = 80;
    m_moveDetector->setThreshold(threshold);
#endif
}

void ChessBot::updateCorners(QPoint c1, QPoint c2,QPoint c3, QPoint c4)
{

}
void ChessBot::randomMove()
{
    QString gameState = m_chessController->buildResultText();
    if(gameState != "") {
        if(gameState == "DRAW_STALEMATE" ||
                gameState == "DRAW_PIECE") {
            Q_EMIT gameEnded(0);
        } else if(gameState == "WHITE_WIN") {
            Q_EMIT gameEnded(m_side == 0?1:2);
        } else if(gameState == "BLACK_WIN") {
            Q_EMIT gameEnded(m_side == 1?1:2);
        }
    } else {
        processNextMove();
    }
}

ChessController* ChessBot::chessController()
{
    return m_chessController;
}

QObject* ChessBot::chessControllerObject() const
{
    return m_chessController;
}

void ChessBot::resetGame(){
    printf("Reset game side[%d]\r\n",m_side);
    m_chessController->newGame();
}
