#include <QThread>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>
#include <QFile>
#include <QSerialPortInfo>
#include <QTime>
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
#ifdef IMAGE_PROCESS_MOVE
    std::vector<cv::Rect> moves;
    cv::Mat src1 = cv::imread("1.jpg");
    cv::Mat src2 = cv::imread("2.jpg");
    if(!src1.empty() && !src2.empty()) {
        m_moveDetector->extractMove(src1, src2, moves);
        std::vector<cv::Point> chessMoves;
        m_moveDetector->convertChessMove(moves, chessMoves);
        for(int i = 0; i< chessMoves.size(); i++) {
            qDebug("Move(%d,%d)\r\n",chessMoves[i].x,chessMoves[i].y);
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
        qDebug("Error: Could not open camera.\r\n");
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
    qDebug("Dowork\r\n");
    m_stopped = false; // Reset flags
    
    // Create QSerialPort in worker thread to avoid threading issues
    robotController = new QSerialPort();
    
    while(!m_stopped){
        // Check for Stop
        m_mutex->lock();
        if(m_pause)
            m_pauseCond->wait(m_mutex); // in this place, your thread will stop to execute until someone calls resume
        m_mutex->unlock();
        switch (m_state) {
        case STATE_INIT_COM: {
            initRobot();
        }
            break;
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
    
    // Cleanup serial port before exiting thread
    if (robotController->isOpen()) {
        robotController->close();
    }
    delete robotController;
    robotController = nullptr;

    qDebug("Dowork finished\r\n");
}

void ChessBot::playLoop()
{
    switch (m_statePlay) {
    case PLAY_SETUP: {
#ifdef IMAGE_PROCESS_MOVE
        readFrame(imageBefore);
        qDebug("First image [%d,%d]\r\n",
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
    qDebug("playDetectMove\r\n");
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
            qDebug("(%d,%d) name[%d] color[%d]\r\n",
                   chessMoves[i].x,chessMoves[i].y,
                   m_game->getPiece(chessMoves[i].x,chessMoves[i].y)->name,
                   m_game->getPiece(chessMoves[i].x,chessMoves[i].y)->color);
            if(m_game->getPiece(chessMoves[i].x,chessMoves[i].y)->name != pieceName::EMPTY &&
               (int)m_game->getPiece(chessMoves[i].x,chessMoves[i].y)->color == (int)pieceColor::EMPTY+m_side+1){
                startMove.x=chessMoves[i].x;
                startMove.y=chessMoves[i].y;
                qDebug("startMove (%d,%d)\r\n",startMove.x,startMove.y);
            } else {
                stopMove.x=chessMoves[i].x;
                stopMove.y=chessMoves[i].y;
                qDebug("stopMove (%d,%d)\r\n",stopMove.x,stopMove.y);
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

QPoint ChessBot::readCalibrationPoint(const QString &command)
{
    QPoint point(-1, -1);
    
    if (!robotController->isOpen()) {
        qDebug("Serial port is not open.\r\n");
        return point;
    }
    
    // Send calibration request command
    qDebug("Sending calibration command: %s\r\n", command.toStdString().c_str());
    robotController->write(command.toLatin1());
    robotController->waitForBytesWritten(500);
    
    // Determine expected response prefix based on command
    QString expectedPrefix;
    if (command.startsWith("lccbr")) {
        expectedPrefix = "CB";
    } else if (command.startsWith("lcdpr")) {
        expectedPrefix = "DP";
    } else if (command.startsWith("lcdbr")) {
        expectedPrefix = "DB";
    } else {
        qDebug("Unknown command type: %s\r\n", command.toStdString().c_str());
        return point;
    }
    
    // Wait for response and handle multiple responses
    if (robotController->waitForReadyRead(2000)) {
        QByteArray combinedResponse = robotController->readAll();
        qDebug("Raw response received: %s\r\n", combinedResponse.constData());
        
        // Split response into lines/messages (handle multiple responses)
        QString responseStr = QString::fromLatin1(combinedResponse);
        QStringList responses = responseStr.split(QRegExp("[\\r\\n]+"), QString::SkipEmptyParts);
        
        // Find the first valid response
        for (const QString &response : responses) {
            QString trimmedResponse = response.trimmed();
            qDebug("Processing response line: %s\r\n", trimmedResponse.toStdString().c_str());
            
            // Check if response starts with expected prefix
            if (!trimmedResponse.startsWith(expectedPrefix)) {
                qDebug("Skipping invalid response (wrong prefix): %s\r\n", trimmedResponse.toStdString().c_str());
                continue;
            }
            
            // Extract x and y values from response
            // Expected format: "CB r[0] c[1] x[123] y[456]" (or DP/DB instead of CB)
            QRegExp xPattern("x\\[(\\d+)\\]");
            QRegExp yPattern("y\\[(\\d+)\\]");
            
            int xPos = xPattern.indexIn(trimmedResponse);
            int yPos = yPattern.indexIn(trimmedResponse);
            
            if (xPos != -1 && yPos != -1) {
                bool okX, okY;
                int x = xPattern.cap(1).toInt(&okX);
                int y = yPattern.cap(1).toInt(&okY);
                
                if (okX && okY) {
                    point = QPoint(x, y);
                    qDebug("Valid calibration point received: (%d, %d)\r\n", x, y);
                    QThread::msleep(100); // Small delay between requests
                    return point;
                }
            }
            
            qDebug("Failed to parse coordinates from response: %s\r\n", trimmedResponse.toStdString().c_str());
        }
        
        qDebug("No valid response found with expected format (prefix: %s)\r\n", expectedPrefix.toStdString().c_str());
    } else {
        qDebug("No response to calibration command: %s\r\n", command.toStdString().c_str());
    }
    
    QThread::msleep(100); // Small delay between requests
    return point;
}

void ChessBot::initRobot()
{
    switch (m_stateInit) {
    case INIT_DETECT_PORT: {
        qDebug("[Step 1] Detecting Arduino port...\r\n");
        if (detectArduinoPort()) {
            m_stateInit = INIT_GET_VERSION;
        } else {
            qDebug("Failed to detect Arduino port. Initialization aborted.\r\n");
            m_state = STATE_EXIT;
            togglePause(true);
        }
    }
        break;
        
    case INIT_GET_VERSION: {
        qDebug("[Step 2] Getting Arduino version...\r\n");
        if (getArduinoVersion()) {
            m_stateInit = INIT_CHECK_CALIB_FILE;
        } else {
            qDebug("Failed to get Arduino version. Initialization aborted.\r\n");
            m_state = STATE_EXIT;
            togglePause(true);
        }
    }
        break;
        
    case INIT_CHECK_CALIB_FILE: {
        qDebug("[Step 3] Checking for calibration file...\r\n");
        QFile calibFile("calib.json");
        
        if (!calibFile.exists()) {
            qDebug("Calibration file not found. Requesting calibration data from Arduino...\r\n");
            m_chessboardCalib = QVector<QVector<QPoint>>(8, QVector<QPoint>(8));
            m_dropzoneRightCalib = QVector<QVector<QPoint>>(8, QVector<QPoint>(2));
            m_dropzoneLeftCalib = QVector<QVector<QPoint>>(8, QVector<QPoint>(2));
            m_calibRow = 0;
            m_calibCol = 0;
            m_stateInit = INIT_REQUEST_CALIB_CHESSBOARD;
        } else {
            qDebug("Calibration file found. Skipping calibration request.\r\n");
            m_stateInit = INIT_DONE;
        }
    }
        break;
        
    case INIT_REQUEST_CALIB_CHESSBOARD: {
        if (m_calibRow < 8) {
            if (m_calibCol < 8) {
                QString command = QString::asprintf("lccbr%c%d", m_calibRow, m_calibCol);
                QPoint point = readCalibrationPoint(command);
                m_chessboardCalib[m_calibRow][m_calibCol] = point;
                m_calibCol++;
            } else {
                m_calibCol = 0;
                m_calibRow++;
            }
        } else {
            qDebug("Chessboard calibration complete.\r\n");
            m_calibRow = 0;
            m_calibCol = 0;
            m_stateInit = INIT_REQUEST_CALIB_RIGHT_DROPZONE;
        }
    }
        break;
        
    case INIT_REQUEST_CALIB_RIGHT_DROPZONE: {
        if (m_calibRow < 8) {
            if (m_calibCol < 2) {
                QString command = QString::asprintf("lcdpr%dc%d", m_calibRow, m_calibCol);
                QPoint point = readCalibrationPoint(command);
                m_dropzoneRightCalib[m_calibRow][m_calibCol] = point;
                m_calibCol++;
            } else {
                m_calibCol = 0;
                m_calibRow++;
            }
        } else {
            qDebug("Right dropzone calibration complete.\r\n");
            m_calibRow = 0;
            m_calibCol = 0;
            m_stateInit = INIT_REQUEST_CALIB_LEFT_DROPZONE;
        }
    }
        break;
        
    case INIT_REQUEST_CALIB_LEFT_DROPZONE: {
        if (m_calibRow < 8) {
            if (m_calibCol < 2) {
                QString command = QString::asprintf("lcdbr%dc%d", m_calibRow, m_calibCol);
                QPoint point = readCalibrationPoint(command);
                m_dropzoneLeftCalib[m_calibRow][m_calibCol] = point;
                m_calibCol++;
            } else {
                m_calibCol = 0;
                m_calibRow++;
            }
        } else {
            qDebug("Left dropzone calibration complete.\r\n");
            qDebug("All calibration data collected successfully.\r\n");
            m_stateInit = INIT_DONE;
        }
    }
        break;
        
    case INIT_DONE: {
        qDebug("=== Robot Initialization Complete ===\r\n");
        m_state = STATE_CONFIGURE;
        togglePause(true);
    }
        break;
    }
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

void ChessBot::initRobotCommunication() {
    qDebug("ChessBot::initRobotCommunication\r\n");
    m_state = STATE_INIT_COM;
    m_stateInit = INIT_DETECT_PORT;
    m_calibRow = 0;
    m_calibCol = 0;
    togglePause(false);
    startService();
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
    qDebug("Set level: %d\r\n",level);
    m_levelScore = level;
    m_levelType = level/400+1;
    m_chessController->setEngineLevel(m_levelType);
}

void ChessBot::setSide(int side)
{
    qDebug("Set side: %d\r\n",side);
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
        qDebug("Could not open file for reading: %s\r\n",fileName.toStdString().c_str());
        return;
    }

    // Read all data and parse into a JSON document
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);

    // Ensure the root of the JSON is an array
    if (!doc.isArray()) {
        qDebug("JSON format error: Root is not an array.\r\n");
        return;
    }

    QJsonArray jsonArray = doc.array();
    int cornerID = 0;
    for (const QJsonValue &value : jsonArray) {
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            // Extract x and y, then append as a QPoint
            points.append(QPoint(obj["x"].toInt(), obj["y"].toInt()));
            qDebug("corner[%d] (%d,%d)\r\n",cornerID,
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
    qDebug("Reset game side[%d]\r\n",m_side);
    m_chessController->newGame();
}

bool ChessBot::detectArduinoPort(int baudRate)
{
    qDebug("Detecting Arduino port at %d baudrate...\r\n", baudRate);
    
    // Get all available serial ports
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    
    if (ports.isEmpty()) {
        qDebug("No COM ports found.\r\n");
        return false;
    }
    
    qDebug("Found %d available COM port(s):\r\n", ports.size());
    
    // Try each port
    for (const QSerialPortInfo &portInfo : ports) {
        qDebug("Trying port: %s (%s)\r\n",
               portInfo.portName().toStdString().c_str(),
               portInfo.description().toStdString().c_str());
        
        // Configure and open the port
        robotController->setPortName(portInfo.portName());
        robotController->setBaudRate(baudRate);
        robotController->setDataBits(QSerialPort::Data8);
        robotController->setParity(QSerialPort::NoParity);
        robotController->setStopBits(QSerialPort::OneStop);
        robotController->setFlowControl(QSerialPort::NoFlowControl);
        
        if (robotController->open(QIODevice::ReadWrite)) {
            qDebug("Opened port: %s\r\n", portInfo.portName().toStdString().c_str());
            
            for(int i=0; i< 10; i++) {
                // Send version request
                robotController->write("v");
                robotController->waitForBytesWritten(500);

                // Wait for response with timeout
                if (robotController->waitForReadyRead(1000)) {
                    QByteArray response = robotController->readAll();
                    qDebug("Response received: %s\r\n", response.constData());

                    // Check if response contains "[v]"
                    if (response.contains("[v]")) {
                        qDebug("Arduino detected on port: %s\r\n",
                               portInfo.portName().toStdString().c_str());
                        m_arduinoVersion = QString::fromLatin1(response);
                        return true;
                    }
                } else {
                    qDebug("No response from port: %s\r\n", portInfo.portName().toStdString().c_str());
                }
            }
            robotController->close();
        } else {
            qDebug("Failed to open port: %s\r\n", portInfo.portName().toStdString().c_str());
        }
    }
    
    qDebug("Arduino not detected on any port.\r\n");
    return false;
}

bool ChessBot::getArduinoVersion()
{
    if (!robotController->isOpen()) {
        qDebug("Serial port is not open.\r\n");
        return false;
    }
    
    // Send version request
    qDebug("Sending version request...\r\n");
    robotController->write("v");
    robotController->waitForBytesWritten(500);
    
    // Collect all responses for 2 seconds
    QByteArray allResponses;
    QTime timer;
    timer.start();
    
    qDebug("Collecting responses for 2 seconds...\r\n");
    while (timer.elapsed() < 2000) {
        if (robotController->waitForReadyRead(100)) {
            QByteArray chunk = robotController->readAll();
            allResponses.append(chunk);
            qDebug("Received chunk: %s\r\n", chunk.constData());
        }
    }
    
    if (allResponses.isEmpty()) {
        qDebug("No response from Arduino.\r\n");
        return false;
    }
    
    qDebug("Total responses: %s\r\n", allResponses.constData());
    
    // Split responses into lines and find the valid one with "[v]"
    QString responseStr = QString::fromLatin1(allResponses);
    QStringList responses = responseStr.split(QRegExp("[\\r\\n]+"), QString::SkipEmptyParts);
    
    for (const QString &response : responses) {
        QString trimmedResponse = response.trimmed();
        qDebug("Processing response: %s\r\n", trimmedResponse.toStdString().c_str());
        
        // Check if response contains "[v]"
        if (trimmedResponse.contains("[v]")) {
            // Extract version from response
            // Assuming format like: "[v]version_number"
            int startPos = trimmedResponse.indexOf("[v]");
            if (startPos != -1) {
                startPos += 3; // Move past "[v]"
                int endPos = trimmedResponse.indexOf("]", startPos);
                if (endPos == -1) {
                    endPos = trimmedResponse.length();
                }
                
                QString version = QString::fromLatin1(
                    trimmedResponse.mid(startPos, endPos - startPos).toLatin1()
                );
                m_arduinoVersion = version;
                qDebug("Valid Arduino Version found: %s\r\n", version.toStdString().c_str());
                return true;
            }
        }
    }
    
    qDebug("No valid response found with '[v]' pattern\r\n");
    return false;
}
