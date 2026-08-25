#include <QElapsedTimer>
#include <QThread>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>
#include <QFile>
#include <QSerialPortInfo>
#include <QTime>
#include <QDebug>
#include <QTextCodec>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QString>
#include <QDebug>
#include "ChessBot.h"
#include "chessAlgo/ChessController.h"
#include "Move.hpp"

// Platform-specific headers for directory scanning and creation
#if defined(_WIN32)
    #include <windows.h>
    #include <direct.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <sys/types.h>
#endif
#include <fstream>

ChessBot::ChessBot(QThread *parent) :
    QThread(parent),
    m_validCalibFileFound(false)
{
    m_width = 1920;
    m_height = 1080;
    m_mutex = new QMutex;
    m_pauseCond = new QWaitCondition;
    m_chessController = new ChessController();
    m_state = STATE_EXIT;
#ifdef IMAGE_PROCESS_MOVE
    m_detectParams = new MoveDetectParams();
    m_detectParams->roi_percent = 50;
    m_detectParams->diff_thresh = 30;
    m_detectParams->canny_low = 14;
    m_detectParams->pieceMinPoints = 400;
    m_detectParams->pieceRoiPercent = 100;
    m_detectParams->playerSide = m_chessController->playerColor() == 0?
                "white":"black";
#endif
    m_chessboardCalib = QVector<QVector<QPoint>>(8, QVector<QPoint>(8));
    m_dropzoneRightCalib = QVector<QVector<QPoint>>(8, QVector<QPoint>(2));
    m_dropzoneLeftCalib = QVector<QVector<QPoint>>(8, QVector<QPoint>(2));
#ifdef IMAGE_PROCESS_MOVE
    m_moveDetector = new ChessImageProcessing();
#endif
#ifdef IMAGE_PROCESS_MOVE
#endif
    m_validCalibFileFound = loadCalibrationData();
    connect(m_chessController,&ChessController::boardChanged,
            this,&ChessBot::boardChanged);
}

ChessBot::~ChessBot()
{
    stopService();
}
#ifdef IMAGE_PROCESS_MOVE
bool openFirstTime = false;
bool ChessBot::readFrame(cv::Mat& outImg)
{
    QElapsedTimer timer;
    timer.start();
    bool readResult = false;
    if (!cap.isOpened()) {
        QElapsedTimer timer;
        timer.start();
        cap.open(0);
        qint64 milliSeconds = timer.elapsed();

        qDebug() << "Open took" << milliSeconds << "milliseconds.";
//        if(!openFirstTime)
        {
            QElapsedTimer timer;
            timer.start();
            cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));

            // Set your target resolution
            cap.set(cv::CAP_PROP_FRAME_WIDTH, m_width);
            cap.set(cv::CAP_PROP_FRAME_HEIGHT, m_height);

            // Set your target frame rate
            cap.set(cv::CAP_PROP_FPS, 30);

            // Verify what the hardware actually set (some cameras fallback if unsupported)
            double actual_width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
            double actual_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
            double actual_fps = cap.get(cv::CAP_PROP_FPS);

            std::cout << "Capture initialized: " << actual_width << "x" << actual_height
                      << " @ " << actual_fps << " FPS" << std::endl;

            qint64 milliSeconds = timer.elapsed();

            qDebug() << "Set property took" << milliSeconds << "milliseconds.";
            openFirstTime = true;
            timer.start();
            for(int i=0; i< 0;i++) {
//                cap.read(outImg);
                cap.grab();
//                QThread::msleep(30);
                printf(".");
            }
            milliSeconds = timer.elapsed();

            qDebug() << "grap 2 images took" << milliSeconds << "milliseconds.";
        }
    }
    if (cap.isOpened()) {
        QElapsedTimer timer;
        timer.start();
        readResult = cap.read(outImg);
        // 3. Get the elapsed time
        qint64 milliSeconds = timer.elapsed();

        qDebug() << "Single capture took" << milliSeconds << "milliseconds.";
        cap.release();
    }
    // 3. Get the elapsed time
    qint64 milliSeconds = timer.elapsed();

    qDebug() << "The read frame took" << milliSeconds << "milliseconds.";
    return readResult;
}
#endif

void ChessBot::updateCorners(QVariantList corners)
{
    m_chessboardConners.clear();
    for (const QVariant &val : corners) {
        QVariantMap map = val.toMap();
        int x = map["x"].toInt()*m_width/640;
        int y = map["y"].toInt()*m_height/360;

        m_chessboardConners.append(QPoint(map["x"].toInt(),map["y"].toInt()));
    }

    if(m_chessboardConners.size() == 4) {
#ifdef IMAGE_PROCESS_MOVE
    m_moveDetector->setCorners(m_chessboardConners[0].x(),m_chessboardConners[0].y(),
            m_chessboardConners[1].x(),m_chessboardConners[1].y(),
            m_chessboardConners[2].x(),m_chessboardConners[2].y(),
            m_chessboardConners[3].x(),m_chessboardConners[3].y());
#endif
    }
    saveCalibrationData();
}

void ChessBot::updateCalibrationData(int type, int row, int col, int x, int y)
{
    qDebug("ChessBot::updateCalibrationData");
    if(type == 0) {
        m_chessboardCalib[row][col].setX(x);
        m_chessboardCalib[row][col].setY(y);
    } else if(type == 1) {
        m_dropzoneRightCalib[row][col].setX(x);
        m_dropzoneRightCalib[row][col].setY(y);
    } else {
        m_dropzoneLeftCalib[row][col].setX(x);
        m_dropzoneLeftCalib[row][col].setY(y);
    }
    sendTestCommand("tx"+QString::number(x)+"y"+QString::number(y));
}

void ChessBot::run()
{
    qDebug("ChessBot Dowork");
    m_stopped = false; // Reset flags
    m_state = STATE_EXIT;
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
    }

    // Cleanup serial port before exiting thread
    if (robotController->isOpen()) {
        robotController->close();
    }
    delete robotController;
    robotController = nullptr;

    qDebug("Dowork finished");
}

void ChessBot::playInputMove(int startUiIndex, int stopUiIndex, int promotePiece) {

    qDebug("======== playInputMove %d->%d",startUiIndex,stopUiIndex);
    for(int row = 0; row < 8; row ++) {
        for(int col = 0; col < 8 ; col++) {
            printf("%s ",m_chessController->uiIndexToSquareNotation(row*8+col).toStdString().c_str());
        }
        printf("\r\n");
    }
    if(promotePiece < 0) {
        QString fenBeforeMove = m_chessController->extractFEN();
        QString pieceType = m_chessController->uiIndexToPieceType(startUiIndex);
        QString pieceCurrentNotation = m_chessController->uiIndexToSquareNotation(startUiIndex);
        QString pieceTargetNotation = m_chessController->uiIndexToSquareNotation(stopUiIndex);
        Move chosenMove;
        if(m_chessController->moveByUiSquares(startUiIndex,stopUiIndex,chosenMove)) {
            QString formattedMove = pieceType + " "+
                    pieceCurrentNotation + " to "+ pieceTargetNotation;
            Q_EMIT newMoveAdded(fenBeforeMove,
                                   m_chessController->playerColor() == Color::WHITE?"White":"Black",
                                   formattedMove);
            m_mutex->lock();
            m_state = STATE_PLAY;
            m_statePlay = PLAY_CALCULATE_NEXT_MOVE;
            m_mutex->unlock();
            Q_EMIT playTurnChanged(m_chessController->playerColor() == Color::WHITE ? 1-m_chessController->playerColor():m_chessController->playerColor());
            togglePause(false);
            startService();
        } else {
            if(m_chessController->status() == "CHOOSE_PROMOTION_PIECE") {
                Q_EMIT showPromotionPieces();
            } else {
                qDebug("Invalid input move");
            }
        }
    } else {
        QStringList listPromotions = {"q","r","n","b"};
        m_chessController->choosePromotion(m_chessController->playerColor() == 0 ?listPromotions[promotePiece]:
                                                        listPromotions[promotePiece].toUpper());
        m_mutex->lock();
        m_state = STATE_PLAY;
        m_statePlay = PLAY_CALCULATE_NEXT_MOVE;
        m_mutex->unlock();
        togglePause(false);
        startService();
    }

}

void ChessBot::playInputCancelPromotion()
{
    m_chessController->cancelPromotion();
}

void ChessBot::stopGame(QString comment)
{
    togglePause(true);
    m_mutex->lock();
    m_state = STATE_PLAY;
    m_statePlay = PLAY_ENDGAME_TIMEOUT;
    m_timeoutComment = comment;
    m_mutex->unlock();
    togglePause(false);
    startService();
}

void ChessBot::playLoop()
{
    switch (m_statePlay) {
    case PLAY_CHECK_LOG: {
        qDebug("PLAY_CHECK_LOG");
        if(findLastFENInLog()) {
            Q_EMIT foundLastFEN();
            togglePause(true);
        } else {
            if(m_chessController->playerColor() == 1) {
                m_statePlay = PLAY_CALCULATE_NEXT_MOVE_RESET;
            } else {
                m_statePlay = PLAY_SETUP;
            }
        }
    }
        break;
    case PLAY_SETUP: {
        qDebug("PLAY_SETUP");
        logWithTimestampQt("===New game===");
        qDebug("Init FEN: %s",m_chessController->extractFEN().toStdString().c_str());
        logWithTimestampQt(m_chessController->extractFEN());
        qDebug("Init FEN done");
#ifdef IMAGE_PROCESS_MOVE
        executeCommand("rs");
        readFrame(imageBefore);
        qDebug("First image [%d,%d]",
               imageBefore.rows,imageBefore.cols);
#endif
        m_statePlay = PLAY_PROCESS_DONE;
    }
        break;
    case PLAY_INIT: {
        qDebug("PLAY_INIT");
        if(playCheckEndGame() == true)
            m_statePlay = PLAY_CHECK_CURRENT_MOVE;
        else
            m_statePlay = PLAY_PROCESS_DONE;
    }
        break;
    case PLAY_CHECK_CURRENT_MOVE:{
        qDebug("PLAY_CHECK_CURRENT_MOVE");
        if(!playCheckDoubleMove())
            m_statePlay = PLAY_DETECT_MOVE;
        else
            m_statePlay = PLAY_PROCESS_DONE;
    }
        break;
    case PLAY_DETECT_MOVE: {
        qDebug("PLAY_DETECT_MOVE");
        int detectMoveState = playDetectMove();
        if(detectMoveState == STATE_DONE_SUCCESS){
            m_statePlay = PLAY_CALCULATE_NEXT_MOVE;
        } else if (detectMoveState == STATE_PENDING) {
            m_statePlay = PLAY_REQUEST_PROMOTE_PIECE;
        } else {
            m_statePlay = PLAY_INFORM_ERROR;
#ifdef IMAGE_PROCESS_MOVE
            processAndSaveFailures(imageBefore,imageAfter);
#endif
        }
    }
        break;
    case PLAY_CALCULATE_NEXT_MOVE_RESET: {
        qDebug("PLAY_CALCULATE_NEXT_MOVE_RESET");
        resetDropZoneMap(m_chessController->playerColor());
        m_statePlay = PLAY_CALCULATE_NEXT_MOVE;
    }
        break;
    case PLAY_CALCULATE_NEXT_MOVE: {
        qDebug("PLAY_CALCULATE_NEXT_MOVE");
        if(playCalculateNextMove() == STATE_DONE_SUCCESS){
            m_statePlay = PLAY_EXECUTE_NEXT_MOVE;
        } else {
            m_statePlay = PLAY_INFORM_ERROR_CALCULATE_NEXT_MOVE;
        }
    }
        break;
    case PLAY_EXECUTE_NEXT_MOVE: {
        qDebug("PLAY_EXECUTE_NEXT_MOVE");
        if(playExecuteNextMove()== STATE_DONE_SUCCESS){
            m_statePlay = PLAY_INFORM_RESULT;
        } else {
            m_statePlay = PLAY_INFORM_ERROR_EXECUTE_NEXT_MOVE;
        }
    }
        break;
    case PLAY_INFORM_RESULT: {
        qDebug("PLAY_INFORM_RESULT");
        if(playInformResult()== STATE_DONE_SUCCESS){
            m_statePlay = PLAY_PROCESS_DONE;
        }
    }
        break;
    case PLAY_REQUEST_PROMOTE_PIECE: {
        qDebug("PLAY_REQUEST_PROMOTE_PIECE");
        Q_EMIT newCommentAdded("Choose your promotion piece");
        m_statePlay = PLAY_PROCESS_DONE;
    }
        break;
    case PLAY_ENDGAME_TIMEOUT: {
        qDebug("PLAY_ENDGAME_TIMEOUT");
        logWithTimestampQt(m_timeoutComment);
        m_timeoutComment = "";
        m_state = STATE_EXIT;
        togglePause(true);
    }
        break;
    case PLAY_INFORM_ERROR:{
        qDebug("Can not detect move");
        Q_EMIT newCommentAdded("Can not detect move");
        Q_EMIT detectFailed();
        m_statePlay = PLAY_PROCESS_DONE;
    }
        break;
    case PLAY_INFORM_ERROR_CALCULATE_NEXT_MOVE: {
        qDebug("Can not calculate best move");
        Q_EMIT newCommentAdded("Can not calculate best move");
        m_statePlay = PLAY_PROCESS_DONE;
    }
        break;
    case PLAY_INFORM_ERROR_EXECUTE_NEXT_MOVE: {
        qDebug("Execute move error");
        Q_EMIT newCommentAdded("Execute move error");
        m_statePlay = PLAY_PROCESS_DONE;
    }
        break;
    case PLAY_PROCESS_DONE: {
        qDebug("PLAY_PROCESS_DONE");
        Q_EMIT playTurnChanged(m_chessController->playerColor() == Color::WHITE ? m_chessController->playerColor():1-m_chessController->playerColor());
        playCheckEndGame();
        m_state = STATE_EXIT;
        togglePause(true);
    }
        break;
    }
}

void ChessBot::configureLoop()
{
    switch (m_stateConfigure) {
    case CONFIGURE_CHESSBOARD: {
        if(configureChessBoardCalib() == STATE_DONE_SUCCESS){
            m_stateConfigure = CONFIGURE_DONE;
        }
    }
        break;
    case CONFIGURE_LEVEL: {
        if(configureLevel() == STATE_DONE_SUCCESS){
            m_stateConfigure = CONFIGURE_DONE;
        }
    }
        break;
    case CONFIGURE_SIDE: {
        if(configureSide() == STATE_DONE_SUCCESS){
            m_stateConfigure = CONFIGURE_DONE;
        }
    }
        break;
    case CONFIGURE_DONE: {
        m_state = STATE_EXIT;
        togglePause(true);
    }
        break;
    }
}

void ChessBot::testLoop()
{
    switch (m_stateTest) {
    case TEST_ROBOT: {
        if(testRobot() == STATE_DONE_SUCCESS){
            m_stateTest = TEST_CHECK_RESULT;
        }
    }
    case TEST_CHECK_RESULT: {
        if(testCheckResult() != STATE_PENDING){
            m_stateTest = TEST_DONE;
        }
    }
        break;
    case TEST_DONE: {
        m_state = STATE_EXIT;
        togglePause(true);
    }
        break;
    }
}
bool ChessBot::playCheckDoubleMove()
{
    QString gameState = m_chessController->buildResultText();
    if(gameState == "BLACK_CHECK") {
        return m_chessController->playerColor() == 1;
    } else if(gameState == "WHITE_CHECK") {
        return m_chessController->playerColor() == 0;
    }
    return false;
}

bool ChessBot::canMoveStraight(int startRow, int startCol, int stopRow, int stopCol, PIECE_MOVE_TYPE moveType)
{
    bool foundBlockingPiece = false;
    // reject move outside 3x3 block
    if(abs(startCol - stopCol) > 2 || abs(startRow - stopRow) > 2) return false;
    // check pieces inside 3x3 block
    QStringList board = m_chessController->board();
    for(int row = 0; row < 8; row ++) {
        for(int col = 0 ; col < 8; col ++) {
            printf("%s ",board[row*8+col] != "" ? board[row*8+col].toStdString().c_str():
                    "__");
        }
        printf("\r\n");
    }
    int minRow = std::min(startRow,stopRow);
    int maxRow = std::max(startRow,stopRow);
    int minCol = std::min(startCol,stopCol);
    int maxCol = std::max(startCol,stopCol);
    printf("Check from rc[%d,%d] to rc[%d,%d]\r\n",
           minRow,minCol,maxRow,maxCol);
    for(int row = minRow; row<= maxRow; row++) {
        for(int col = minCol; col <= maxCol; col++) {
            QString pieceType = (m_chessController->playerColor() == Color::WHITE ? board[row*8+col]:board[(7-row)*8+(7-col)]);
            printf("%s ",pieceType != "" ? pieceType.toStdString().c_str():
                    "__");
            if((row == startRow && col == startCol) ||
                (row == stopRow && col == stopCol) ||
                (moveType == PIECE_MOVE_ENPASSANT && row == startRow))
                continue;
            if(pieceType != "") {
                foundBlockingPiece = true;
            }
        }
        printf("\r\n");
    }
    return !foundBlockingPiece;
}

bool ChessBot::playCheckEndGame()
{
    QString gameState = m_chessController->buildResultText();
    qDebug("gameState[%s]",gameState.toStdString().c_str());
    if(gameState != "") {
        if(gameState == "DRAW_STALEMATE" ||
                gameState == "DRAW_PIECE") {
            Q_EMIT newCommentAdded("Game draw");
            Q_EMIT gameEnded(0);
            return false;
        } else if(gameState == "WHITE_WIN") {
            Q_EMIT newCommentAdded(m_chessController->playerColor() == 0?"Check mate. You win":
                                  "Check mate. You lost");
            Q_EMIT gameEnded(m_chessController->playerColor() == 0?1:2);
            return false;
        } else if(gameState == "BLACK_WIN") {
            Q_EMIT newCommentAdded(m_chessController->playerColor() == 1?"Check mate. You win":
                                  "Check mate. You lost");
            Q_EMIT gameEnded(m_chessController->playerColor() == 1?1:2);
            return false;
        } else if(gameState == "BLACK_CHECK") {
            Q_EMIT newCommentAdded(m_chessController->playerColor() == 0?"Check mate":"Good checkmate");
            return true;
        } else if(gameState == "WHITE_CHECK") {
            Q_EMIT newCommentAdded(m_chessController->playerColor() == 1?"Check mate":"Good checkmate");
            return true;
        }
    } else {
        return true;
    }
}
uint8_t ChessBot::playDetectMove()
{
    qDebug("playDetectMove");
    int detectState = STATE_DONE_FAIL;
    Move choosenMove;
    QString choosenPiece;
    QString choosenPieceMoveNotation;
    QString fenBeforeMove = m_chessController->extractFEN();
#ifdef IMAGE_PROCESS_MOVE
    if(!readFrame(imageAfter)){
        return STATE_DONE_FAIL;
    }
    if(!imageBefore.empty() && !imageAfter.empty()) {
        cv::imwrite("imageBefore.jpg",imageBefore);
        cv::imwrite("imageAfter.jpg",imageAfter);
        std::vector<std::string> chessMoves = m_moveDetector->findPossibleMoves(imageBefore, imageAfter,
            *m_detectParams);
        std::string possibleMove = "";
        int numPossibleMove = 0;
        for(int i = 0; i< chessMoves.size(); i++) {
            qDebug("Checking Move %s",chessMoves[i].c_str());
            QString from = QString::fromStdString(chessMoves[i]).left(2);  // Result: "e2"
            QString to = QString::fromStdString(chessMoves[i]).right(2);   // Result: "e4"
            if(m_chessController->isValidMoveByCoordinates(from,to,choosenMove)) {
                numPossibleMove++;
                possibleMove = chessMoves[i];
            }
        }
        if(numPossibleMove == 1) {
            qDebug("Found Move %s",possibleMove.c_str());
            QString from = QString::fromStdString(possibleMove).left(2);  // Result: "e2"
            QString to = QString::fromStdString(possibleMove).right(2);   // Result: "e4"
            choosenPiece = m_chessController->pieceType(from);
            choosenPieceMoveNotation = to;
            if(m_chessController->moveByCoordinates(from,to,choosenMove)) {
                detectState = STATE_DONE_SUCCESS;
            } else {
                if(m_chessController->status() == "CHOOSE_PROMOTION_PIECE") {
                    Q_EMIT showPromotionPieces();
                    detectState = STATE_PENDING;
                }
            }
        }
    }
#elif defined(TEST_RANDOM_MOVE)
        QStringList randomMoves = m_chessController->findBestMoveCoordinates();
        printf("=== Player move %s->%s\r\n",
                randomMoves[0].toStdString().c_str(),
                randomMoves[1].toStdString().c_str());
        if(randomMoves[0] != randomMoves[1]) {
            QString from = randomMoves[0].left(2);  // Result: "e2"
            QString to = randomMoves[1].right(2);   // Result: "e4"
            choosenPiece = m_chessController->pieceType(from);
            choosenPieceMoveNotation = to;
            if(m_chessController->moveByCoordinates(randomMoves[0],randomMoves[1],choosenMove)) {
                detectState = STATE_DONE_SUCCESS;
                QString formattedMove = choosenPiece + " "+
                        from + " to "+ to;
                Q_EMIT newMoveAdded(fenBeforeMove,
                                    m_chessController->playerColor() == Color::WHITE?"White":"Black",
                                    formattedMove);
                break;
            } else {
                if(m_chessController->status() == "CHOOSE_PROMOTION_PIECE") {
                    Q_EMIT showPromotionPieces();
                    detectState = STATE_PENDING;
                    break;
                }
            }
        } else {
            Q_EMIT newCommentAdded("No invalid move found\r\n");
        }

#endif
    return detectState;
}

uint8_t ChessBot::playRandomMove()
{
    QStringList randomMoves = m_chessController->findBestMoveCoordinates();
    Move choosenMove;
    qDebug("=== Player move %s->%s",
            randomMoves[0].toStdString().c_str(),
            randomMoves[1].toStdString().c_str());
    if(randomMoves[0] != randomMoves[1]) {        
        m_chessController->moveByCoordinates(randomMoves[0],randomMoves[1],choosenMove);
        return STATE_DONE_SUCCESS;
    } else {
        Q_EMIT newCommentAdded("No invalid move found\r\n");
        return STATE_DONE_FAIL;
    }
}

QPoint ChessBot::notationToCoord(const std::string& notation, const std::string& playerSide)
{
    // Validate input length
    if (notation.length() < 2) return QPoint(-1,-1);

    char file = notation[0];
    char rank = notation[1];

    // Validate chess boundaries
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return QPoint(-1,-1);

    int x, y;

    if (playerSide == "black") {
        x = file - 'a';
        y = '8' - rank;
    } else { // white at bottom / black at top
        x = 'h' - file;
        y = rank - '1';
    }

    return QPoint(x,y);
}
uint8_t ChessBot::playCalculateNextMove()
{
    QString currentFen = m_chessController->extractFEN();
    qDebug("playCalculateNextMove FEN: %s",currentFen.toStdString().c_str());
    logWithTimestampQt(currentFen);
    qDebug("playCalculateNextMove log FEN done");
    // play engine move
    m_chessController->playEngineMove();
    qDebug("check move history");
    if(m_chessController->status() == "NO_LEGAL_ENGINE_MOVE_FOUND") {
        return STATE_DONE_FAIL;
    }
    qDebug("get last move");
    Move lastMove = m_chessController->botMove();
    qDebug("get last move string");
    QString lastMoveStr = QString::fromStdString(lastMove.toShortString());
    qDebug("parsing move");
    if(lastMoveStr.length() < 4) {
        qDebug("invalid move");
        return STATE_DONE_FAIL;
    }
    qDebug("lastMove %s", lastMoveStr.toStdString().c_str());
    QString from = lastMoveStr.left(2);  // Result: "e2"
    QString to = lastMoveStr.mid(2, 2);   // Result: "e4"

    QPoint fromCoord, toCoord;    
    fromCoord = notationToCoord(from.toStdString(),
                                                m_chessController->playerColor() != 0?"white":"black");
    toCoord = notationToCoord(to.toStdString(),
                                                m_chessController->playerColor() != 0?"white":"black");
    qDebug("Bot move %s->%s",
           from.toStdString().c_str(),
           to.toStdString().c_str());
    if(fromCoord.x() < 0 || fromCoord.x() > 7 || fromCoord.y() < 0 || fromCoord.y() > 7 ||
        toCoord.x() < 0 || toCoord.x() > 7 || toCoord.y() < 0 || toCoord.y() > 7)
        return STATE_DONE_FAIL;
    if(m_chessController->botMove().isQuiet())
    {
        qDebug("Move quited");
        sprintf(m_robotCommand,"c%d%d%d%d%c",fromCoord.y(),fromCoord.x(),toCoord.y(),toCoord.x(),
                canMoveStraight(fromCoord.y(),fromCoord.x(),toCoord.y(),toCoord.x())?'-':'n');
    }
    else //Castling or Promotion or Capture
    {
        if(m_chessController->botMove().isCastling())
        {

            //move King
            qDebug("Castling move king");
            Square rookOrigin = SQ_NONE;
            Square rookDestination = SQ_NONE;

            if(m_chessController->botMove().isKingSideCastling())
            {
                if(m_chessController->playerColor() == WHITE)
                {
                    rookOrigin = SQ_H1;
                    rookDestination = SQ_F1;
                    qDebug("Castling H1->F1");
                }
                else
                {
                    rookOrigin = SQ_H8;
                    rookDestination = SQ_F8;
                    qDebug("Castling H8->F8");
                }
            }
            else // QueenSideCastling
            {
                if(m_chessController->playerColor() == WHITE)
                {
                    rookOrigin = SQ_A1;
                    rookDestination = SQ_D1;
                    qDebug("Castling A1->D1");
                }
                else
                {
                    rookOrigin = SQ_A8;
                    rookDestination = SQ_D8;
                    qDebug("Castling A8->D8");
                }
            }

            //move rook
            qDebug("Castling move rook");
            sprintf(m_robotCommand,"CST%d%d%d%d%c",fromCoord.y(),fromCoord.x(),
                    toCoord.y(),toCoord.x() > fromCoord.x()?7:0,
                    '-');
        }
        else if (m_chessController->botMove().isPromotion())
        {
            char pawnPromoteChar = m_chessController->playerColor() == Color::WHITE?'p':'P';
            char promotePieceChar = pieceName(m_chessController->botMove().getPromotedPieceType(),
                                              1 - m_chessController->playerColor());
            qDebug("Capture remove pawn color[%s]",m_chessController->playerColor() == 0?"White":"Black");
            qDebug("Capture Add piece[%c] color[%s]",promotePieceChar,m_chessController->playerColor() == 0?"White":"Black");
            DropPoint dropCapturePoint, dropPawnPromotePoint, piecePromotePoint;
            dropCapturePoint.rowID = 0;
            dropCapturePoint.colID = 0;
            dropCapturePoint.zoneType = ZONE_BOT;
            bool isPromotionWithCapture = fromCoord.x() != toCoord.x();
            if(isPromotionWithCapture) {
                // Capture
                char capturedPieceChar = pieceName(m_chessController->botMove().getCapturedPieceType(),
                                                   m_chessController->playerColor());
                if(!getFreeDropPoint(dropCapturePoint)) {
                    qDebug("No space to drop captured piece");
                    return STATE_DONE_FAIL;
                } else {
                    updateDropZone(capturedPieceChar,
                                   dropCapturePoint.rowID,
                                   dropCapturePoint.colID,
                                   dropCapturePoint.zoneType);
                }
            }
            if(!getFreeDropPoint(piecePromotePoint,promotePieceChar)) {
                qDebug("Not found promote piece");
                return STATE_DONE_FAIL;
            } else {
                updateDropZone(0,
                               piecePromotePoint.rowID,
                               piecePromotePoint.colID,
                               piecePromotePoint.zoneType);
            }
            if(!getFreeDropPoint(dropPawnPromotePoint)) {
                qDebug("No space to drop pawn");
                return STATE_DONE_FAIL;
            } else {
                updateDropZone(pawnPromoteChar,
                               dropPawnPromotePoint.rowID,
                               dropPawnPromotePoint.colID,
                               dropPawnPromotePoint.zoneType);
            }
            sprintf(m_robotCommand,"pm%d%d"
                                   "%d%d"
                                   "%c%d%d"
                                   "%c%d%d"
                                   "%c%d%d",
                    fromCoord.y(),fromCoord.x(),
                    toCoord.y(),toCoord.x(),
                    dropCapturePoint.zoneType==ZONE_BOT?'b':'p',dropCapturePoint.rowID,dropCapturePoint.colID,
                    piecePromotePoint.zoneType==ZONE_BOT?'b':'p',piecePromotePoint.rowID,piecePromotePoint.colID,
                    dropPawnPromotePoint.zoneType==ZONE_BOT?'b':'p',dropPawnPromotePoint.rowID,dropPawnPromotePoint.colID);
        }
        else
        {
            if (m_chessController->botMove().isEnPassant()) // watch out ep capture is a capture
            {
                qDebug("Capture remove pawn color[%d]",m_chessController->playerColor() != 0?"White":"Black");
                DropPoint dropPoint;
                char capturePiece = m_chessController->playerColor() == Color::WHITE ? 'P':'p';
                if(getFreeDropPoint(dropPoint)) {
                    sprintf(m_robotCommand,"pp%d%d"
                                           "%d%d%c"
                                           "%c%d%d",
                            fromCoord.y(),fromCoord.x(),
                            toCoord.y(),toCoord.x(),
                            canMoveStraight(fromCoord.y(),fromCoord.x(),toCoord.y(),toCoord.x(),PIECE_MOVE_CAPTURE)?'-':'n',
                            dropPoint.zoneType == 0 ? 'b':'p',
                            dropPoint.rowID,dropPoint.colID
                        );
                    updateDropZone(capturePiece,
                                   dropPoint.rowID,
                                   dropPoint.colID,
                                   dropPoint.zoneType);
                } else {
                    qDebug("No space to drop piece");
                    return STATE_DONE_FAIL;
                }
            }
            else //Move is capture
            {
                //remove the captured piece
                char capturePiece = pieceName(m_chessController->botMove().getCapturedPieceType(),
                                              m_chessController->playerColor());
                qDebug("Capture remove captured piece[%c] color[%s]",
                       capturePiece,m_chessController->playerColor() != 0?"White":"Black");
                DropPoint dropPoint;
                if(getFreeDropPoint(dropPoint)) {
                    sprintf(m_robotCommand,"a%d%d"
                                           "%d%d%c"
                                           "%c%d%d",
                            fromCoord.y(),fromCoord.x(),
                            toCoord.y(),toCoord.x(),
                            canMoveStraight(fromCoord.y(),fromCoord.x(),toCoord.y(),toCoord.x(),PIECE_MOVE_CAPTURE)?'-':'n',
                            dropPoint.zoneType == ZONE_BOT ? 'b':'p',
                            dropPoint.rowID,dropPoint.colID
                        );
                    updateDropZone(capturePiece,
                                   dropPoint.rowID,
                                   dropPoint.colID,
                                   dropPoint.zoneType);
                } else {
                    qDebug("No space to drop piece");
                    return STATE_DONE_FAIL;
                }
            }
        }
    }
    qDebug("playCalculateNextMove %s to cmd[%s]\r\n",
           lastMoveStr.toStdString().c_str(),
           m_robotCommand);
    return STATE_DONE_SUCCESS;
}

bool ChessBot::sendRobotCommand(const char* cmd, int waitTime)
{
    qDebug("sendRobotCommand %d [%s]",strlen(cmd),cmd);
    if (!robotController->isOpen()) {
        qDebug("Serial port is not open to write");
        return false;
    }
    robotController->write(cmd);
    return robotController->waitForBytesWritten(waitTime);
}

QString ChessBot::readRobotResponse(int waitTime)
{
    QByteArray chunk;
    int totalTime = 0;
    if(!robotController->isOpen()) {
        qDebug("Serial port is not open to read");
        return QString::fromUtf8(chunk);
    }
    if(robotController->waitForReadyRead(waitTime)) {
        chunk = robotController->readAll();
        while (robotController->waitForReadyRead(10)) {
            chunk += robotController->readAll();
        }
    }
    qDebug("Robot rep [%d]:%s",chunk.size(),chunk.data());
    return QString::fromUtf8(chunk);
}
uint8_t ChessBot::playExecuteNextMove()
{
    // TODO: Send command to robot and wait until execution is done
    return executeCommand(m_robotCommand);
}

uint8_t ChessBot::playInformResult()
{
    // TODO: Signal GUI that robot execution is done
#ifdef IMAGE_PROCESS_MOVE
    readFrame(imageBefore);
#endif
    return STATE_DONE_SUCCESS;
}

uint8_t ChessBot::configureChessBoardCalib()
{
    return STATE_DONE_SUCCESS;
}

uint8_t ChessBot::configureSide()
{
    return STATE_DONE_SUCCESS;
}

uint8_t ChessBot::configureLevel()
{
    return STATE_DONE_SUCCESS;
}

uint8_t ChessBot::testRobot()
{
    uint8_t testState = STATE_PENDING;
    if(!sendRobotCommand(m_commandTest.toStdString().c_str())) {
        return STATE_DONE_FAIL;
    }
    QString chunk = readRobotResponse();
    if(chunk.size() > 0) testState = STATE_DONE_SUCCESS;
    return testState;
}

uint8_t ChessBot::testCheckResult()
{
    QString cmdID = "";
    QString cmdRequest = "";
    QString cmdState = "";
    uint8_t cmdResult = STATE_DONE_FAIL;
    int retry = 0;
    do {
        retry++;
        if(!sendRobotCommand("cmd")) {
            continue;
        }
        QString cmdIDStr = readRobotResponse();
        if(cmdIDStr.contains("[cmd]")) {
            QRegularExpression re("\\d+");
            QRegularExpressionMatch match = re.match(cmdIDStr);
            if (match.hasMatch()) {
                cmdID = match.captured(0);
                qDebug() << "Extracted numbers:" << cmdID; // Outputs: "0001"
                break;
            }
        }
    } while(retry < 5);

    if(cmdID != "") {
        cmdRequest = "_"+cmdID;
        retry = 0;
        do {
            if(!sendRobotCommand(cmdRequest.toStdString().c_str())) continue;
            QString cmdStateStr = readRobotResponse();
            if(cmdStateStr.contains("]DONE")) {
                cmdState = cmdStateStr.section(']', 1);
                cmdResult = STATE_DONE_SUCCESS;
                qDebug("cmdState: %s", cmdState.toStdString().c_str());
                break;
            }
            sleep(1);
            retry++;
        } while(retry < 25);
    }
    return cmdResult;
}

bool ChessBot::readCalibrationPoint(const QString &command,QPoint& point)
{
    // Send calibration request command
    qDebug("Sending calibration command: %s", command.toStdString().c_str());
    if(!sendRobotCommand(command.toLatin1(),500))
    return false;
    // Determine expected response prefix based on command
    QString expectedPrefix;
    if (command.startsWith("lccbr")) {
        expectedPrefix = "CB";
    } else if (command.startsWith("lcdpr")) {
        expectedPrefix = "DP";
    } else if (command.startsWith("lcdbr")) {
        expectedPrefix = "DB";
    } else {
        qDebug("Unknown command type: %s", command.toStdString().c_str());
        return false;
    }

    // Wait for response and handle multiple responses
    if (robotController->waitForReadyRead(2000)) {
        QByteArray combinedResponse = robotController->readAll();
        qDebug("Raw response received: %s", combinedResponse.constData());

        // Split response into lines/messages (handle multiple responses)
        QString responseStr = QString::fromLatin1(combinedResponse);
        QStringList responses = responseStr.split(QRegExp("[\\r\\n]+"), QString::SkipEmptyParts);

        // Find the first valid response
        for (const QString &response : responses) {
            QString trimmedResponse = response.trimmed();
            qDebug("Processing response line: %s", trimmedResponse.toStdString().c_str());

            // Check if response starts with expected prefix
            if (!trimmedResponse.startsWith(expectedPrefix)) {
                qDebug("Skipping invalid response (wrong prefix): %s", trimmedResponse.toStdString().c_str());
                continue;
            }

            // Extract x and y values from response
            // Expected format: "CB r[0] c[1] x[123] y[456]" (or DP/DB instead of CB)
            QRegExp xPattern("x\\[(-?\\d+)\\]");
            QRegExp yPattern("y\\[(-?\\d+)\\]");

            int xPos = xPattern.indexIn(trimmedResponse);
            int yPos = yPattern.indexIn(trimmedResponse);

            if (xPos != -1 && yPos != -1) {
                bool okX, okY;
                int x = xPattern.cap(1).toInt(&okX);
                int y = yPattern.cap(1).toInt(&okY);

                if (okX && okY) {
                    point = QPoint(x, y);
                    qDebug("Valid calibration point received: (%d, %d)", x, y);
                    QThread::msleep(100); // Small delay between requests
                    return true;
                }
            }

            qDebug("Failed to parse coordinates from response: %s", trimmedResponse.toStdString().c_str());
        }

        qDebug("No valid response found with expected format (prefix: %s)", expectedPrefix.toStdString().c_str());
    } else {
        qDebug("No response to calibration command: %s", command.toStdString().c_str());
    }

    QThread::msleep(100); // Small delay between requests
    return false;
}

bool ChessBot::sendCalibrationCells()
{
    qDebug("Sending calibration data to RobotController...");

    // Send start marker and wait for acknowledgment
    if(!sendRobotCommand("CALIB_START\n"))
        return false;

    // Wait for CALIB_START_ACK response within 2 seconds
    if (!robotController->waitForReadyRead(2000)) {
        qDebug("No CALIB_START_ACK response received within timeout");
        Q_EMIT calibrationUploadComplete(CALIB_UPLOAD_TO_ROBOT, false);
        return false;
    }

    QByteArray startResponse = robotController->readAll();
    QString startResponseStr = QString::fromLatin1(startResponse).trimmed();
    qDebug("CALIB_START response: %s", startResponseStr.toStdString().c_str());

    if (!startResponseStr.contains("CALIB_START_ACK")) {
        qDebug("Invalid CALIB_START response: %s", startResponseStr.toStdString().c_str());
        Q_EMIT calibrationUploadComplete(CALIB_UPLOAD_TO_ROBOT, false);
        return false;
    }

    qDebug("CALIB_START acknowledged, proceeding with calibration upload...");

    // Send chessboard calibration (8x8 = 64 cells)
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            QPoint p = m_chessboardCalib[r][c];
            QString cmd = QString("SC r%1 c%2 x%3 y%4\n")
                .arg(r).arg(c).arg(p.x()).arg(p.y());
            if(!sendRobotCommand(cmd.toLatin1(),20)){
                qDebug("Failed to send calibration for chessboard cell [%d,%d]", r, c);
                Q_EMIT calibrationUploadComplete(CALIB_UPLOAD_TO_ROBOT, false);
                return false;
            }

            // Wait for progress response
            if (!waitForCalibrationProgress()) {
                qDebug("Failed to receive calibration progress response for chessboard cell [%d,%d]", r, c);
                Q_EMIT calibrationUploadComplete(CALIB_UPLOAD_TO_ROBOT, false);
                return false;
            }

            // Allow UI to process events and check for abort
            QThread::msleep(10);
        }
    }

    // Send right dropzone calibration (8x2 = 16 cells)
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 2; c++) {
            QPoint p = m_dropzoneRightCalib[r][c];
            QString cmd = QString("SR r%1 c%2 x%3 y%4\n")
                .arg(r).arg(c).arg(p.x()).arg(p.y());
            if(!sendRobotCommand(cmd.toLatin1(),20)) {
                qDebug("Failed to send calibration for right dropzone cell [%d,%d]", r, c);
                Q_EMIT calibrationUploadComplete(CALIB_UPLOAD_TO_ROBOT, false);
                return false;
            }

            // Wait for progress response
            if (!waitForCalibrationProgress()) {
                qDebug("Failed to receive calibration progress response for right dropzone cell [%d,%d]", r, c);
                Q_EMIT calibrationUploadComplete(CALIB_UPLOAD_TO_ROBOT, false);
                return false;
            }

            // Allow UI to process events and check for abort
            QThread::msleep(10);
        }
    }

    // Send left dropzone calibration (8x2 = 16 cells)
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 2; c++) {
            QPoint p = m_dropzoneLeftCalib[r][c];
            QString cmd = QString("SL r%1 c%2 x%3 y%4\n")
                .arg(r).arg(c).arg(p.x()).arg(p.y());
            if(!sendRobotCommand(cmd.toLatin1(),20)) {
                qDebug("Failed to send calibration for left dropzone cell [%d,%d]", r, c);
                Q_EMIT calibrationUploadComplete(CALIB_UPLOAD_TO_ROBOT, false);
                return false;
            }

            // Wait for progress response
            if (!waitForCalibrationProgress()) {
                qDebug("Failed to receive calibration progress response for left dropzone cell [%d,%d]", r, c);
                Q_EMIT calibrationUploadComplete(CALIB_UPLOAD_TO_ROBOT, false);
                return false;
            }

            // Allow UI to process events and check for abort
            QThread::msleep(10);
        }
    }

    // Send end marker
    if(!sendRobotCommand("CALIB_END\n")) {
        qDebug("Failed to send calibration end");
        Q_EMIT calibrationUploadComplete(CALIB_UPLOAD_TO_ROBOT, false);
        return false;
    }

    // Wait for final response
    if (robotController->waitForReadyRead(3000)) {
        QByteArray response = robotController->readAll();
        QString responseStr = QString::fromLatin1(response).trimmed();
        qDebug("Calibration end response: %s", responseStr.toStdString().c_str());

        if (responseStr.contains("CALIB_OK")) {
            qDebug("RobotController accepted all calibration data");
             Q_EMIT calibrationUploadComplete(CALIB_UPLOAD_TO_ROBOT, true);
            return true;
        } else if (responseStr.contains("CALIB_ERROR")) {
            qDebug("RobotController rejected calibration data");
             Q_EMIT calibrationUploadComplete(CALIB_UPLOAD_TO_ROBOT, false);
            return false;
        } else {
            qDebug("Unexpected response from RobotController");
             Q_EMIT calibrationUploadComplete(CALIB_UPLOAD_TO_ROBOT, false);
            return false;
        }
    } else {
        qDebug("No response from RobotController to calibration end");
         Q_EMIT calibrationUploadComplete(CALIB_UPLOAD_TO_ROBOT, false);
        return false;
    }
}

void ChessBot::abortCalibrationUpload()
{
    qDebug("Aborting calibration upload...");
    sendRobotCommand("CALIB_ABORT\n",100);
    Q_EMIT calibrationUploadComplete(CALIB_UPLOAD_TO_ROBOT, false);
}

bool ChessBot::waitForCalibrationProgress()
{
    // Collect all responses for 2 seconds
    QByteArray allResponses;
    QTime timer;
    timer.start();

    qDebug("Collecting calibration progress responses for 2 seconds...");
    while (timer.elapsed() < 100) {
        QString chunk = readRobotResponse(50);
        if(chunk.size() > 0) allResponses.append(chunk);
    }

    if (allResponses.isEmpty()) {
        qDebug("No progress responses received within 2 seconds");
        return false;
    }

    qDebug("Total progress responses: %s", allResponses.constData());

    // Split responses into lines and find the valid progress response
    QString responseStr = QString::fromLatin1(allResponses);
    QStringList responses = responseStr.split(QRegExp("[\\r\\n]+"), QString::SkipEmptyParts);

    // Parse the progress format: "R1/16 L12/16 C23/64"
    for (const QString &response : responses) {
        QString trimmedResponse = response.trimmed();
        qDebug("Processing progress response: %s", trimmedResponse.toStdString().c_str());

        QRegExp progressRegex("R(\\d+)/(\\d+) L(\\d+)/(\\d+) C(\\d+)/(\\d+)");
        if (progressRegex.indexIn(trimmedResponse) != -1) {
            int rightReceived = progressRegex.cap(1).toInt();
            int rightTotal = progressRegex.cap(2).toInt();
            int leftReceived = progressRegex.cap(3).toInt();
            int leftTotal = progressRegex.cap(4).toInt();
            int chessReceived = progressRegex.cap(5).toInt();
            int chessTotal = progressRegex.cap(6).toInt();

            // Calculate total progress
            int totalReceived = rightReceived + leftReceived + chessReceived;
            int totalExpected = rightTotal + leftTotal + chessTotal;

            if (totalExpected > 0) {
                int progressPercent = (totalReceived * 100) / totalExpected;
                qDebug("Calibration progress: R%d/%d L%d/%d C%d/%d = %d%%",
                       rightReceived, rightTotal, leftReceived, leftTotal,
                       chessReceived, chessTotal, progressPercent);
                Q_EMIT calibrationUploadProgress(CALIB_UPLOAD_TO_ROBOT, progressPercent);
                return true;
            }
        }
    }

    qDebug("No valid progress response found in collected data");
    return false;
}

void ChessBot::initRobot()
{
    switch (m_stateInit) {
    case INIT_DETECT_PORT: {
        qDebug("[Step 1] Detecting Arduino port...");
        if (detectArduinoPort()) {
            m_stateInit = INIT_ENABLE_ROBOT;
        } else {
            qDebug("Failed to detect Arduino port. Initialization aborted.");
            Q_EMIT calibrationUploadComplete(INIT_COMMUNICATION, false);
            m_state = STATE_EXIT;
            togglePause(true);
        }
    }
        break;

    case INIT_SEND_CALIBRATION: {
        qDebug("[Step 3] Checking for calibration file...");
        if (isCalibDataLoaded()) {
            qDebug("Calibration data valid found. Loading and uploading to RobotController...");
            if (sendCalibrationCells()) {
                qDebug("Calibration data uploaded to RobotController.");
                m_stateInit = INIT_ENABLE_ROBOT;
            } else {
                qDebug("Failed to upload calibration data to RobotController. Initialization aborted.");
                m_state = STATE_EXIT;
                togglePause(true);
            }
        } else {
            qDebug("Calibration data is invalid. Requesting calibration data from RobotController...");
            m_calibRow = 0;
            m_calibCol = 0;
            m_calibCellCount = 0;
            m_stateInit = INIT_REQUEST_CALIB_CHESSBOARD;
        }
    }
        break;

    case INIT_REQUEST_CALIB_CHESSBOARD: {
        if (m_calibRow < 8) {
            if (m_calibCol < 8) {
                QString command = QString::asprintf("lccbr%dc%d", m_calibRow, m_calibCol);
                QPoint point;
                if(!readCalibrationPoint(command,point)){
                    Q_EMIT calibrationUploadComplete(CALIB_REQUEST_FROM_ROBOT,false);
                    m_stateInit = INIT_ENABLE_ROBOT;
                    break;
                }
                m_chessboardCalib[m_calibRow][m_calibCol] = point;
                m_calibCellCount ++;
                calibrationUploadProgress(CALIB_REQUEST_FROM_ROBOT,m_calibCellCount*100/98);
                m_calibCol++;
            } else {
                m_calibCol = 0;
                m_calibRow++;
            }
        } else {
            qDebug("Chessboard calibration complete.");
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
                QPoint point;
                if(!readCalibrationPoint(command,point)){
                    Q_EMIT calibrationUploadComplete(CALIB_REQUEST_FROM_ROBOT,false);
                    m_stateInit = INIT_ENABLE_ROBOT;
                    break;
                }
                m_dropzoneRightCalib[m_calibRow][m_calibCol] = point;
                m_calibCellCount ++;
                calibrationUploadProgress(CALIB_REQUEST_FROM_ROBOT,m_calibCellCount*100/98);
                m_calibCol++;
            } else {
                m_calibCol = 0;
                m_calibRow++;
            }
        } else {
            qDebug("Right dropzone calibration complete.");
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
                QPoint point;
                if(!readCalibrationPoint(command,point)){
                    Q_EMIT calibrationUploadComplete(CALIB_REQUEST_FROM_ROBOT,false);
                    m_stateInit = INIT_ENABLE_ROBOT;
                    break;
                }
                m_dropzoneLeftCalib[m_calibRow][m_calibCol] = point;
                m_calibCellCount ++;
                calibrationUploadProgress(CALIB_REQUEST_FROM_ROBOT,m_calibCellCount*100/98);
                m_calibCol++;
            } else {
                m_calibCol = 0;
                m_calibRow++;
            }
        } else {
            if(m_calibCellCount == 64+16+16) {
                Q_EMIT calibrationUploadComplete(CALIB_REQUEST_FROM_ROBOT,true);
                qDebug("Left dropzone calibration complete.");
                qDebug("All calibration data collected successfully.");
                saveCalibrationData();
            } else {
                Q_EMIT calibrationUploadComplete(CALIB_REQUEST_FROM_ROBOT,false);
            }

            m_stateInit = INIT_ENABLE_ROBOT;
        }
    }
        break;
    case INIT_ENABLE_ROBOT: {
        if(enableRobot() == STATE_DONE_SUCCESS){
            Q_EMIT calibrationUploadComplete(ENABLE_ROBOT,true);
            m_stateInit = INIT_GO_HOME;
        }
    }
        break;
    case INIT_GO_HOME: {
        if(goHome() == STATE_DONE_SUCCESS){
            Q_EMIT calibrationUploadComplete(HOMING_ROBOT,true);
            m_stateInit = INIT_DONE;
        }
    }
        break;
    case INIT_DONE: {
        qDebug("=== Robot Initialization Complete ===");
        Q_EMIT calibrationUploadComplete(HOMING_ROBOT,true);
        m_state = STATE_EXIT;
        togglePause(true);
    }
        break;
    }
}

uint8_t ChessBot::enableRobot()
{
    qDebug("Enable Robot");
    if(!sendRobotCommand("ee",1000))
        return STATE_DONE_FAIL;
    QString responseStr = readRobotResponse();
    if(responseStr.contains("[es] Enabled")) {
    }
    sleep(1);
    return STATE_DONE_SUCCESS;
}

uint8_t ChessBot::goHome()
{
    if(!sendRobotCommand("ha",1000))
        return STATE_DONE_FAIL;
    sleep(1);
    readRobotResponse();

    return STATE_DONE_SUCCESS;
}

bool ChessBot::saveCalibrationData(QString fileName)
{
    qDebug("Saving calibration data to: %s", fileName.toStdString().c_str());

    QJsonObject root;

    // Save chessboard calibration (8x8)
    QJsonArray cameraCorners;
    for (int cornerID = 0; cornerID < m_chessboardConners.size(); cornerID++) {
        QJsonObject pointObj;
        pointObj["x"] = m_chessboardConners[cornerID].x();
        pointObj["y"] = m_chessboardConners[cornerID].y();
        cameraCorners.append(pointObj);
    }
    root["camera_calibration"] = cameraCorners;

    // Save chessboard calibration (8x8)
    QJsonArray chessboardArray;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            QJsonObject pointObj;
            pointObj["row"] = row;
            pointObj["col"] = col;
            pointObj["x"] = m_chessboardCalib[row][col].x();
            pointObj["y"] = m_chessboardCalib[row][col].y();
            chessboardArray.append(pointObj);
        }
    }
    root["chessboard"] = chessboardArray;

    // Save right dropzone calibration (8x2)
    QJsonArray rightDropzoneArray;
    for (int row = 0; row < m_dropzoneRightCalib.size(); row++) {
        for (int col = 0; col < m_dropzoneRightCalib[row].size(); col++) {
            QJsonObject pointObj;
            pointObj["row"] = row;
            pointObj["col"] = col;
            pointObj["x"] = m_dropzoneRightCalib[row][col].x();
            pointObj["y"] = m_dropzoneRightCalib[row][col].y();
            rightDropzoneArray.append(pointObj);
        }
    }
    root["dropzone_right"] = rightDropzoneArray;

    // Save left dropzone calibration (8x2)
    QJsonArray leftDropzoneArray;
    for (int row = 0; row < m_dropzoneLeftCalib.size(); row++) {
        for (int col = 0; col < m_dropzoneLeftCalib[row].size(); col++) {
            QJsonObject pointObj;
            pointObj["row"] = row;
            pointObj["col"] = col;
            pointObj["x"] = m_dropzoneLeftCalib[row][col].x();
            pointObj["y"] = m_dropzoneLeftCalib[row][col].y();
            leftDropzoneArray.append(pointObj);
        }
    }
    root["dropzone_left"] = leftDropzoneArray;

    // Create JSON document and write to file
    QJsonDocument doc(root);
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug("Failed to open file for writing: %s", fileName.toStdString().c_str());
        return false;
    }

    file.write(doc.toJson());
    file.close();

    qDebug("Calibration data saved successfully to: %s", fileName.toStdString().c_str());
    return true;
}

bool ChessBot::loadCalibrationData(QString fileName)
{
    QJsonDocument doc;
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) return false;
        QByteArray data = file.readAll();
        doc = QJsonDocument::fromJson(data);
    } else {
        QString data = getCalibrationJson();
        if (data.isEmpty()) return false;
        doc = QJsonDocument::fromJson(data.toUtf8());
    }

    if (!doc.isObject()) return false;

    QJsonObject root = doc.object();

    // 1. Helper lambda to parse grids (8x8 or 8x2)
    auto parseGrid = [&](const QString &key, int rows, int cols) {
        QVector<QVector<QPoint>> grid(rows, QVector<QPoint>(cols));
        QJsonArray arr = root.value(key).toArray();

        for (const QJsonValue &val : arr) {
            QJsonObject obj = val.toObject();
            int r = obj.value("row").toInt();
            int c = obj.value("col").toInt();
            if (r < rows && c < cols) {
                grid[r][c] = QPoint(obj.value("x").toInt(), obj.value("y").toInt());
            }
        }
        return grid;
    };

    // 2. Parse the specific variables
    m_chessboardCalib    = parseGrid("chessboard", 8, 8);
    m_dropzoneRightCalib = parseGrid("dropzone_right", 8, 2);
    m_dropzoneLeftCalib  = parseGrid("dropzone_left", 8, 2);

    // 3. Handle camera_calibration (corners)
    // Since it's a list of 4 points, we'll treat it as a 1x4 or 4x1 2D vector
    m_chessboardConners.clear();
    QJsonArray calibArr = root.value("camera_calibration").toArray();
    for (const QJsonValue &val : calibArr) {
        QJsonObject obj = val.toObject();
        m_chessboardConners.append(QPoint(obj.value("x").toInt()*m_width/640,
                                          obj.value("y").toInt()*m_height/360));
    }
    qDebug("m_chessboardConners.size() %d",m_chessboardConners.size());
#if defined(IMAGE_PROCESS_MOVE)
    if(m_chessboardConners.size() == 4)
    {
        m_moveDetector->setCorners(m_chessboardConners[0].x(),m_chessboardConners[0].y(),
                m_chessboardConners[1].x(),m_chessboardConners[1].y(),
                m_chessboardConners[2].x(),m_chessboardConners[2].y(),
                m_chessboardConners[3].x(),m_chessboardConners[3].y());
#ifdef defined(DEBUG_SIMPLE_MOVE)
        cv::Mat src1 = cv::imread("/home/hainh/Desktop/Project/ChessBot/ChessPlayer/build/failcases/f0109.jpg");
        cv::Mat src2 = cv::imread("/home/hainh/Desktop/Project/ChessBot/ChessPlayer/build/failcases/f0110.jpg");
        if(!src1.empty() && !src2.empty()) {
            std::vector<std::string> chessMoves = m_moveDetector->findPossibleMoves(src1, src2, *m_detectParams);
            for(int i = 0; i< chessMoves.size(); i++) {
                qDebug("Possible Move %s",chessMoves[i].c_str());
            }
        }
#endif
    }
#endif

    return true;
}

QString ChessBot::getCalibrationJson() const
{
    QJsonObject root;

    QJsonArray chessboardArray;
    for (int row = 0; row < m_chessboardCalib.size(); ++row) {
        QJsonArray rowArray;
        for (int col = 0; col < m_chessboardCalib[row].size(); ++col) {
            QJsonObject pointObj;
            pointObj["x"] = m_chessboardCalib[row][col].x();
            pointObj["y"] = m_chessboardCalib[row][col].y();
            rowArray.append(pointObj);
        }
        chessboardArray.append(rowArray);
    }
    root["chessboard"] = chessboardArray;

    QJsonArray rightArray;
    for (int row = 0; row < m_dropzoneRightCalib.size(); ++row) {
        QJsonArray rowArray;
        for (int col = 0; col < m_dropzoneRightCalib[row].size(); ++col) {
            QJsonObject pointObj;
            pointObj["x"] = m_dropzoneRightCalib[row][col].x();
            pointObj["y"] = m_dropzoneRightCalib[row][col].y();
            rowArray.append(pointObj);
        }
        rightArray.append(rowArray);
    }
    root["dropzone_right"] = rightArray;

    QJsonArray leftArray;
    for (int row = 0; row < m_dropzoneLeftCalib.size(); ++row) {
        QJsonArray rowArray;
        for (int col = 0; col < m_dropzoneLeftCalib[row].size(); ++col) {
            QJsonObject pointObj;
            pointObj["x"] = m_dropzoneLeftCalib[row][col].x();
            pointObj["y"] = m_dropzoneLeftCalib[row][col].y();
            rowArray.append(pointObj);
        }
        leftArray.append(rowArray);
    }
    root["dropzone_left"] = leftArray;

    if (!m_chessboardConners.isEmpty()) {
        QJsonArray cornersArray;
        for (const QPoint &pt : m_chessboardConners) {
            QJsonObject ptObj;
            ptObj["x"] = pt.x();
            ptObj["y"] = pt.y();
            cornersArray.append(ptObj);
        }
        root["camera_calibration"] = cornersArray;
    }

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

bool ChessBot::isCalibDataLoaded() {
    if(!m_validCalibFileFound) return false;
    // 1. Check Chessboard (8 rows, 8 columns)
    if (m_chessboardCalib.size() != 8) return false;
    for (const auto& row : m_chessboardCalib) {
        if (row.size() != 8) return false;
    }

    // 2. Check Dropzone Right (8 rows, 2 columns)
    if (m_dropzoneRightCalib.size() != 8) return false;
    for (const auto& row : m_dropzoneRightCalib) {
        if (row.size() != 2) return false;
    }

    // 3. Check Dropzone Left (8 rows, 2 columns)
    if (m_dropzoneLeftCalib.size() != 8) return false;
    for (const auto& row : m_dropzoneLeftCalib) {
        if (row.size() != 2) return false;
    }

    return true;
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
    if(m_state != STATE_EXIT) {
        qDebug("Previous move is not finished");
        return;
    }
    m_state = STATE_TEST;
    m_stateTest = TEST_ROBOT;
    m_commandTest = command;
    togglePause(false);
}

int ChessBot::executeCommand(QString command)
{
    m_commandTest = command;
    if(testRobot() == STATE_DONE_FAIL){
        return STATE_DONE_FAIL;
    }
    if(testCheckResult() == STATE_DONE_FAIL){
        return STATE_DONE_FAIL;
    }
    return STATE_DONE_SUCCESS;
}

void ChessBot::homingRobot()
{
    sendTestCommand("ha");
}

void ChessBot::initRobotCommunication() {
    qDebug("ChessBot::initRobotCommunication");
    if(m_state != STATE_EXIT) {
        qDebug("Previous move is not finished");
        return;
    }
    m_mutex->lock();
    m_state = STATE_INIT_COM;
    m_stateInit = INIT_DETECT_PORT;
    m_mutex->unlock();
    m_calibRow = 0;
    m_calibCol = 0;
    togglePause(false);
    startService();
}
void ChessBot::processNextMove()
{
    if(m_state != STATE_EXIT) {
        qDebug("Previous move is not finished %d",m_state);
        return;
    }
    m_mutex->lock();
    m_state = STATE_PLAY;
    m_statePlay = PLAY_INIT;
    m_mutex->unlock();
//    m_statePlay = PLAY_INFORM_ERROR;
    Q_EMIT playTurnChanged(m_chessController->playerColor() == Color::WHITE ? 1-m_chessController->playerColor() : m_chessController->playerColor());
    togglePause(false);
    startService();
}

void ChessBot::undoMove()
{
    m_chessController->undoMove();
}

QVariantList ChessBot::chessboardCorners() const {
    QVariantList rootList;
    qDebug("Number of m_chessboardConners %d",m_chessboardConners.size());
    for (const QPoint &corner : m_chessboardConners) {
        QVariantMap pointMap;
        pointMap["x"] = corner.x()*640/m_width;
        pointMap["y"] = corner.y()*360/m_height;
        rootList.append(pointMap);
    }
    return rootList;
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
    qDebug("Reset game side[%d] m_state[%d]",m_chessController->playerColor(),m_state);
    m_chessController->newGame();
    resetDropZoneMap(m_chessController->playerColor());
    m_mutex->lock();
    m_state = STATE_PLAY;
    m_statePlay = PLAY_CHECK_LOG;
    m_mutex->unlock();
    togglePause(false);
    startService();
}

void ChessBot::setEngineElo(QString level, int score)
{
    m_chessController->setEngineElo(level, score);
}

void ChessBot::setPlayerColor(int color)
{
    m_chessController->setPlayerColor(color);
#if defined(IMAGE_PROCESS_MOVE)
    m_detectParams->playerSide = color == 0?
                "white":"black";
#endif
}

bool ChessBot::detectArduinoPort(int baudRate)
{
    qDebug("Detecting Arduino port at %d baudrate...", baudRate);

    // Get all available serial ports
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    if (ports.isEmpty()) {
        qDebug("No COM ports found.");
        return false;
    }

    qDebug("Found %d available COM port(s):", ports.size());

    // Try each port
    for (const QSerialPortInfo &portInfo : ports) {
        if(!portInfo.portName().toLower().contains("usb")) continue;
        qDebug("Trying port: %s (%s)",
               portInfo.portName().toStdString().c_str(),
               portInfo.description().toStdString().c_str());
        robotController->setPortName("/dev/"+portInfo.portName());
        if (robotController->open(QIODevice::ReadWrite)) {
            qDebug("Opened port: %s", portInfo.portName().toStdString().c_str());
            // Configure and open the port
            robotController->setBaudRate(38400);
            QString response = readRobotResponse(1000);
            if(sendRobotCommand("v",1000)) {
                QString response = readRobotResponse(1000);
                if (response.contains("[v]")) {
                    qDebug("Arduino detected on port: %s",
                           portInfo.portName().toStdString().c_str());
                    return true;
                } else {
                    qDebug("No response from port: %s", portInfo.portName().toStdString().c_str());
                }
            } else {
                qDebug("Wait write response timeout");
            }
        } else {
            qDebug("Failed to open port: %s", portInfo.portName().toStdString().c_str());
        }
    }

    qDebug("Arduino not detected on any port.");
    return false;
}

void ChessBot::acceptPlayFENFromHistory(bool accept)
{
    qDebug("Reset game side[%d]",m_chessController->playerColor());
    if(accept) {
        qDebug("Play last fen [%s]\r\n",m_lastGame.fen.c_str());
        m_chessController->newGame(QString::fromStdString(m_lastGame.fen));
        m_chessController->setPlayerColor(m_lastGame.turn == "White"?Color::BLACK:Color::WHITE);
#ifdef IMAGE_PROCESS_MOVE
        m_detectParams->playerSide = m_chessController->playerColor() == 0?
                    "white":"black";
#endif
        m_chessController->setEngineElo("Advanced",700);
        if(m_chessController->playerColor() == 0) {
            m_mutex->lock();
            m_state = STATE_PLAY;
            m_statePlay = PLAY_CALCULATE_NEXT_MOVE_RESET;
            m_mutex->unlock();
            togglePause(false);
            startService();
        } else {
            m_mutex->lock();
            m_state = STATE_PLAY;
            m_statePlay = PLAY_SETUP;
            m_mutex->unlock();
            togglePause(false);
            startService();
        }
    } else {
        if(m_chessController->playerColor() == 1) {
            m_mutex->lock();
            m_state = STATE_PLAY;
            m_statePlay = PLAY_CALCULATE_NEXT_MOVE_RESET;
            m_mutex->unlock();
            togglePause(false);
            startService();
        } else {
            m_mutex->lock();
            m_state = STATE_PLAY;
            m_statePlay = PLAY_SETUP;
            m_mutex->unlock();
            togglePause(false);
            startService();
        }
    }
}

bool ChessBot::findLastFENInLog()
{
    // Look in the current app deployment directory (or specify an absolute path)
    QString latestFilePath = getLatestLogFile(".");

    if (!latestFilePath.isEmpty()) {
        std::cout << "Targeting file: " << latestFilePath.toStdString() << std::endl;

        // Pass it right along to your C++11 function from earlier
        m_lastGame = getLastChessState(latestFilePath.toStdString());

        if (m_lastGame.success) {
            std::cout << "Timestamp: " << m_lastGame.timestamp << std::endl;
            std::cout << "FEN:       " << m_lastGame.fen << std::endl;
            std::cout << "Turn:      " << m_lastGame.turn << std::endl;
            if(m_chessController->isFENValid(m_lastGame.fen) &&
                    !m_chessController->areFENPositionsEqualDefault(m_lastGame.fen)) {
                return true;
            }
        }
    } else {
        std::cout << "No valid chess log files found in the directory." << std::endl;
    }
    return false;
}
QString ChessBot::getLatestLogFile(const QString& folderPath) {
    QDir directory(folderPath);

    // 1. Safety check to make sure the folder path exists
    if (!directory.exists()) {
        qWarning() << "Directory does not exist:" << folderPath;
        return QString();
    }

    // 2. Set up wildcard filters to only look for files matching your pattern
    QStringList nameFilters;
    nameFilters << "play_history_*.txt";

    // 3. Scan directory.
    // QDir::Name sorts alphabetically. Because your files use "yyyy-MM-dd",
    // alphabetical sorting naturally puts the oldest first and latest last.
    QFileInfoList fileList = directory.entryInfoList(
        nameFilters,
        QDir::Files,
        QDir::Name
    );

    // 4. If the list isn't empty, the very last element is our latest file
    if (!fileList.isEmpty()) {
        return fileList.last().absoluteFilePath();
    }

    return QString(); // Return empty string if no matching files found
}

GameInfo ChessBot::getLastChessState(const std::string& filepath) {
    std::ifstream file(filepath);
    GameInfo info;

    if (!file.is_open()) {
        return info; // success defaults to false
    }

    std::string line;
    std::vector<std::string> gameLines;
    bool activeGame = false;

    // Phase 1: Collect lines belonging ONLY to the most recent game
    while (std::getline(file, line)) {
        if (line.find("===New game===") != std::string::npos) {
            gameLines.clear(); // Wipe older games
            activeGame = true;
            continue;
        }
        if (activeGame && !line.empty()) {
            gameLines.push_back(line);
        }
    }
    file.close();

    // Phase 2: Read backwards from the end to find the last valid log entry
    for (auto it = gameLines.rbegin(); it != gameLines.rend(); ++it) {
        std::string currentLine = *it;

        size_t openBracket = currentLine.find('[');
        size_t closeBracket = currentLine.find(']');

        // Ensure the line contains a timestamp and data after it
        if (openBracket != std::string::npos && closeBracket != std::string::npos && closeBracket + 1 < currentLine.length()) {

            // 1. Extract Timestamp
            info.timestamp = currentLine.substr(openBracket + 1, closeBracket - openBracket - 1);

            // 2. Extract raw FEN string
            size_t fenStart = currentLine.find_first_not_of(" \t", closeBracket + 1);
            if (fenStart == std::string::npos) continue;
            info.fen = currentLine.substr(fenStart);

            // 3. Extract Turn to Play
            std::stringstream ss(info.fen);
            std::string board, color;
            if (ss >> board >> color) {
                if (color == "w") info.turn = "White";
                else if (color == "b") info.turn = "Black";
                else info.turn = "Unknown";
            }

            info.success = true;
            break; // Found the last update, exit
        }
    }

    return info;
}

void ChessBot::logWithTimestampQt(QString data) {
    // 1. Get current date and format as yyyy-MM-dd
    QString dateString = QDate::currentDate().toString("yyyy-MM-dd");
    QString filename = "play_history_"+dateString + ".txt";

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);

        // Write the log line
        out << "[" << timestamp << "] " << data << "\n";

        file.close();
    } else {
        qWarning() << "Error: Could not open file" << filename << "for writing:" << file.errorString();
    }
}
#ifdef IMAGE_PROCESS_MOVE
// Custom padding helper to replace std::setw/std::setfill
std::string ChessBot::formatFilename(const std::string& folder, int number) {
    std::string numStr = std::to_string(number);
    // Pad with leading zeros until the number length is 4 digits
    while (numStr.length() < 4) {
        numStr = "0" + numStr;
    }
    return folder + "/f" + numStr + ".jpg";
}
// Platform-independent directory creation for C++11
void ChessBot::makeDirectory(const std::string& path) {
#if defined(_WIN32)
    _mkdir(path.c_str());
#else
    mkdir(path.c_str(), 0777);
#endif
}

// C++11 compliant directory scanner
int ChessBot::getNextFileCounter(const std::string& folderPath) {
    int maxIndex = 0;

#if defined(_WIN32)
    // Windows implementation using FindFirstFile / FindNextFile
    std::string searchPath = folderPath + "/*.*";
    WIN32_FIND_DATAA fileData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &fileData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::string filename(fileData.cFileName);
                // Strip extension if present to get the stem
                size_t lastDot = filename.find_last_of(".");
                if (lastDot != std::string::npos) {
                    filename = filename.substr(0, lastDot);
                }
                
                if (filename.size() > 1 && filename[0] == 'f') {
                    try {
                        int num = std::stoi(filename.substr(1));
                        if (num > maxIndex) maxIndex = num;
                    } catch (...) {}
                }
            }
        } while (FindNextFileA(hFind, &fileData));
        FindClose(hFind);
    }
#else
    // Linux / macOS implementation using dirent.h
    DIR* dir = opendir(folderPath.c_str());
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) { // Regular file
                std::string filename(entry->d_name);
                size_t lastDot = filename.find_last_of(".");
                if (lastDot != std::string::npos) {
                    filename = filename.substr(0, lastDot);
                }

                if (filename.size() > 1 && filename[0] == 'f') {
                    try {
                        int num = std::stoi(filename.substr(1));
                        if (num > maxIndex) maxIndex = num;
                    } catch (...) {}
                }
            }
        }
        closedir(dir);
    }
#endif

    return maxIndex + 1;
}

void ChessBot::processAndSaveFailures(const cv::Mat& imageBefore, const cv::Mat& imageAfter) {
    if(imageBefore.cols > 0 && imageBefore.rows > 0 && imageAfter.cols > 0 && imageAfter.rows > 0) {
        qDebug("Processing and saving failure images.");
    } else {
        qDebug("Invalid images provided for saving.");
        return;
    }
    std::string dirName = "failcases";
    makeDirectory(dirName); // Uses our C++11 fallback folder creator

    static int fileCounter = getNextFileCounter(dirName);

    std::string pathBefore = formatFilename(dirName, fileCounter++);
    std::string pathAfter  = formatFilename(dirName, fileCounter++);
    
    cv::imwrite(pathBefore, imageBefore);
    cv::imwrite(pathAfter, imageAfter);

    qDebug("Saved:%s and %s", pathBefore.c_str(),pathAfter.c_str());
}
#endif

bool ChessBot::getFreeDropPoint(DropPoint& result, uint8_t promotePiece)
{
    bool foundDropPoint = false;
    for(int zone = 0; zone <2; zone++){
        for(int rowId = 0; rowId < 8; rowId ++){
            for(int colId = 0; colId < 2; colId ++){
                if(zone == ZONE_PLAYER?
                        m_dropZoneMapPlayer[rowId][colId] == promotePiece:
                        m_dropZoneMapBot[rowId][colId] == 0) {
                    result.rowID = rowId;
                    result.colID = colId;
                    result.zoneType = (ZONE_TYPE)zone;
                    foundDropPoint = true;
                    break;
                }
            }
            if(foundDropPoint) break;
        }
    }
    return foundDropPoint;
}

void ChessBot::resetDropZoneMap(int playerColor)
{
    for(int rowId = 0; rowId < 8; rowId++) {
        for(int colId = 0; colId < 2; colId++) {
            m_dropZoneMapPlayer[rowId][colId] = 0;
            m_dropZoneMapBot[rowId][colId] = 0;
        }
    }
    m_dropZoneMapPlayer[0][0] = playerColor == Color::WHITE?'Q':'q';
    m_dropZoneMapPlayer[1][0] = playerColor == Color::WHITE?'R':'r';
    m_dropZoneMapPlayer[2][0] = playerColor == Color::WHITE?'N':'n';
    m_dropZoneMapPlayer[3][0] = playerColor == Color::WHITE?'B':'b';
}

void ChessBot::updateDropZone(uint8_t piece, int row, int col, ZONE_TYPE zone)
{
    if(zone == ZONE_PLAYER) {
        m_dropZoneMapPlayer[row][col] = piece;
    } else {
        m_dropZoneMapBot[row][col] = piece;
    }
}
char ChessBot::pieceName(int piece, int color)
{
    char pieceChar = '0';
    switch (piece) {
        case 0: pieceChar = color == Color::WHITE?'P':'p';
            break;
        case 1: pieceChar = color == Color::WHITE?'N':'n';
            break;
        case 2: pieceChar = color == Color::WHITE?'B':'b';
            break;
        case 3: pieceChar = color == Color::WHITE?'R':'r';
            break;
        case 4: pieceChar = color == Color::WHITE?'Q':'q';
            break;
    }
    return pieceChar;
}
