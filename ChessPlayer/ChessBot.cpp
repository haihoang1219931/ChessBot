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
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include "ChessBot.h"
#include "chessAlgo/ChessController.h"
// Platform-specific headers for directory scanning and creation
#if defined(_WIN32)
    #include <windows.h>
    #include <direct.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

ChessBot::ChessBot(QThread *parent) :
    QThread(parent),
    m_validCalibFileFound(false)
{
    m_width = 1920;
    m_height = 1080;
    m_side = 0;
    m_mutex = new QMutex;
    m_pauseCond = new QWaitCondition;
    m_chessController = new ChessController();
#ifdef IMAGE_PROCESS_MOVE
    m_detectParams = new MoveDetectParams();
    m_detectParams->roi_percent = 50;
    m_detectParams->diff_thresh = 30;
    m_detectParams->canny_low = 14;
    m_detectParams->pieceMinPoints = 400;
    m_detectParams->pieceRoiPercent = 100;
    m_detectParams->playerSide = m_side == 0?
                "white":"black";
#endif
#if defined(_WIN32)
    // 1. Check if engines exist on your OS
    qDebug() << "Available TTS Engines:" << QTextToSpeech::availableEngines();

    m_speech = new QTextToSpeech();
    // Explicitly enforce the system language to kickstart SAPI
    m_speech->setLocale(QLocale::system());
    // 2. Print current engine state (Should be Ready)
    qDebug() << "Current TTS Engine:" << m_speech->availableEngines();
    qDebug() << "Initial State:" << m_speech->state();

    // 3. Optional: Connect a debug log to trace status changes
    connect(m_speech, &QTextToSpeech::stateChanged, [](QTextToSpeech::State state) {
        qDebug() << "TTS State Changed to:" << state;
    });
#else
    m_speech = new PiperStreamer();
#endif
    speakText("I'm chess robot. Nice to play");
    m_chessboardCalib = QVector<QVector<QPoint>>(8, QVector<QPoint>(8));
    m_dropzoneRightCalib = QVector<QVector<QPoint>>(8, QVector<QPoint>(2));
    m_dropzoneLeftCalib = QVector<QVector<QPoint>>(8, QVector<QPoint>(2));
#ifdef IMAGE_PROCESS_MOVE
    m_moveDetector = new ChessImageProcessing();
#endif
#ifdef IMAGE_PROCESS_MOVE
#endif
    m_validCalibFileFound = loadCalibrationData();
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
void ChessBot::connectCamera()
{
#ifdef IMAGE_PROCESS_MOVE
    cap.open(0);
    if (!cap.isOpened()) {
        qDebug("Error: Could not open camera.");
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
    qDebug("Dowork");
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
        QString pieceTargetNotation = m_chessController->uiIndexToSquareNotation(stopUiIndex);
        Move chosenMove;
        if(m_chessController->moveByUiSquares(startUiIndex,stopUiIndex,chosenMove)) {
            speakMove(fenBeforeMove,m_side,pieceType,pieceTargetNotation,chosenMove);
            m_state = STATE_PLAY;
            m_statePlay = PLAY_CALCULATE_NEXT_MOVE;
            Q_EMIT playTurnChanged(m_side == Color::WHITE ? 1-m_side:m_side);
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
        m_chessController->choosePromotion(m_side == 0 ?listPromotions[promotePiece]:
                                                        listPromotions[promotePiece].toUpper());
        m_state = STATE_PLAY;
        m_statePlay = PLAY_CALCULATE_NEXT_MOVE;
        togglePause(false);
        startService();
    }
}

void ChessBot::playInputCancelPromotion()
{
    m_chessController->cancelPromotion();
}

void ChessBot::playLoop()
{
    switch (m_statePlay) {
    case PLAY_SETUP: {
        qDebug("PLAY_SETUP");
        logWithTimestampQt("===New game===");
        qDebug("Init FEN: %s",m_chessController->extractFEN().toStdString().c_str());
        logWithTimestampQt(m_chessController->extractFEN());
        qDebug("Init FEN done");
#ifdef IMAGE_PROCESS_MOVE
        sendTestCommand("rs");
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
#ifdef IMAGE_PROCESS_MOVE
        sendTestCommand("rs");
#endif
        m_statePlay = PLAY_CALCULATE_NEXT_MOVE;
    }
        break;
    case PLAY_CALCULATE_NEXT_MOVE: {
        qDebug("PLAY_CALCULATE_NEXT_MOVE");
        if(playCalculateNextMove() == STATE_DONE_SUCCESS){
            m_statePlay = PLAY_EXECUTE_NEXT_MOVE;
        }
    }
        break;
    case PLAY_EXECUTE_NEXT_MOVE: {
        qDebug("PLAY_EXECUTE_NEXT_MOVE");
        if(playExecuteNextMove()== STATE_DONE_SUCCESS){
            m_statePlay = PLAY_INFORM_RESULT;
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
        speakText("Choose your promotion piece");
        m_statePlay = PLAY_PROCESS_DONE;
    }
        break;
    case PLAY_INFORM_ERROR:{
        printf("Can not detect move\r\n");
        speakText("Can not detect move");
        Q_EMIT detectFailed();
        m_statePlay = PLAY_PROCESS_DONE;
    }
        break;

    case PLAY_PROCESS_DONE: {
        qDebug("PLAY_PROCESS_DONE");
        Q_EMIT playTurnChanged(m_side == Color::WHITE ? m_side:1-m_side);
        playCheckEndGame();
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
            m_stateTest = TEST_DONE;
        }
    }
    case TEST_CHECK_RESULT: {
        if(testCheckResult() == STATE_DONE_SUCCESS){
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
bool ChessBot::playCheckDoubleMove()
{
    QString gameState = m_chessController->buildResultText();
    if(gameState == "BLACK_CHECK") {
        return m_side == 1;
    } else if(gameState == "WHITE_CHECK") {
        return m_side == 0;
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
    int minRow = std::min(startRow,stopRow);
    int maxRow = std::max(startRow,stopRow);
    int minCol = std::min(startCol,stopCol);
    int maxCol = std::max(startCol,stopCol);
    for(int row = minRow; row<= maxRow; row++) {
        for(int col = minCol; col <= maxCol; col++) {
            printf("row[%d] col[%d] %s\r\n",row,col,board[row*8+col].toStdString().c_str());
            if((row == minRow && col == minCol) ||
                (row == maxRow && col == maxCol) ||
                (moveType == PIECE_MOVE_CAPTURE && row == stopCol && col == stopCol) ||
                (moveType == PIECE_MOVE_ENPASSANT && row == startRow))
                continue;
            if(board[row*8+col] != "") {
                foundBlockingPiece = true;
            }
        }
    }
    return !foundBlockingPiece;
}

bool ChessBot::playCheckEndGame()
{
    QString gameState = m_chessController->buildResultText();
    printf("gameState[%s]\r\n",gameState.toStdString().c_str());
    if(gameState != "") {
        if(gameState == "DRAW_STALEMATE" ||
                gameState == "DRAW_PIECE") {
            speakText("Game draw");
            Q_EMIT gameEnded(0);
            return false;
        } else if(gameState == "WHITE_WIN") {
            speakText(m_side == 0?"Check mate. You win":
                                  "Check mate. You lost");
            Q_EMIT gameEnded(m_side == 0?1:2);
            return false;
        } else if(gameState == "BLACK_WIN") {
            speakText(m_side == 1?"Check mate. You win":
                                  "Check mate. You lost");
            Q_EMIT gameEnded(m_side == 1?1:2);
            return false;
        } else if(gameState == "BLACK_CHECK") {
            speakText(m_side == 0?"Check mate":"Good checkmate");
            return true;
        } else if(gameState == "WHITE_CHECK") {
            speakText(m_side == 1?"Check mate":"Good checkmate");
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
        for(int i = 0; i< chessMoves.size(); i++) {
            qDebug("Possible Move %s",chessMoves[i].c_str());
            QString from = QString::fromStdString(chessMoves[i]).left(2);  // Result: "e2"
            QString to = QString::fromStdString(chessMoves[i]).right(2);   // Result: "e4"
            choosenPiece = m_chessController->pieceType(from);
            choosenPieceMoveNotation = to;
            if(m_chessController->moveByCoordinates(from,to,choosenMove)) {
                detectState = STATE_DONE_SUCCESS;
                break;
            } else {
                if(m_chessController->status() == "CHOOSE_PROMOTION_PIECE") {
                    Q_EMIT showPromotionPieces();
                    detectState = STATE_PENDING;
                    break;
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
                break;
            } else {
                if(m_chessController->status() == "CHOOSE_PROMOTION_PIECE") {
                    Q_EMIT showPromotionPieces();
                    detectState = STATE_PENDING;
                    break;
                }
            }
        } else {
            speakText("No invalid move found\r\n");
        }

#endif
    if(detectState == STATE_DONE_SUCCESS) {
        speakMove(fenBeforeMove,m_side,choosenPiece, choosenPieceMoveNotation, choosenMove);
    }
    return detectState;
}

uint8_t ChessBot::playRandomMove()
{
    QStringList randomMoves = m_chessController->findBestMoveCoordinates();
    Move choosenMove;
    printf("=== Player move %s->%s\r\n",
            randomMoves[0].toStdString().c_str(),
            randomMoves[1].toStdString().c_str());
    if(randomMoves[0] != randomMoves[1]) {        
        m_chessController->moveByCoordinates(randomMoves[0],randomMoves[1],choosenMove);
        return STATE_DONE_SUCCESS;
    } else {
        speakText("No invalid move found\r\n");
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
    qDebug("playCalculateNextMove FEN: %s",m_chessController->extractFEN().toStdString().c_str());
    logWithTimestampQt(m_chessController->extractFEN());
    // play engine move
    m_chessController->playEngineMove();
    QString lastMove = QString::fromStdString(m_chessController->moveHistory()[m_chessController->moveHistory().size()-1].toShortString());
    printf("lastMove %s\r\n", lastMove.toStdString().c_str());
    QString from = lastMove.left(2);  // Result: "e2"
    QString to = lastMove.right(2);   // Result: "e4"
    QPoint fromCoord, toCoord;
    fromCoord = notationToCoord(from.toStdString(),
                                                m_side != 0?"white":"black");
    toCoord = notationToCoord(to.toStdString(),
                                                m_side != 0?"white":"black");
    printf("Bot move %s->%s\r\n",
           from.toStdString().c_str(),
           to.toStdString().c_str());
    if(m_chessController->botMove().isQuiet())
    {
        printf("Move quited\r\n");
    }
    else //Castling or Promotion or Capture
    {
        if(m_chessController->botMove().isCastling())
        {

            //move King
            printf("Castling move king\r\n");
            Square rookOrigin = SQ_NONE;
            Square rookDestination = SQ_NONE;

            if(m_chessController->botMove().isKingSideCastling())
            {
                if(m_side == WHITE)
                {
                    rookOrigin = SQ_H1;
                    rookDestination = SQ_F1;
                    printf("Castling H1->F1\r\n");
                }
                else
                {
                    rookOrigin = SQ_H8;
                    rookDestination = SQ_F8;
                    printf("Castling H8->F8\r\n");
                }
            }
            else // QueenSideCastling
            {
                if(m_side == WHITE)
                {
                    rookOrigin = SQ_A1;
                    rookDestination = SQ_D1;
                    printf("Castling A1->D1\r\n");
                }
                else
                {
                    rookOrigin = SQ_A8;
                    rookDestination = SQ_D8;
                    printf("Castling A8->D8\r\n");
                }
            }

            //move rook
            printf("Castling move rook\r\n");
        }
        else if (m_chessController->botMove().isPromotion())
        {
            unsigned int promotedType = m_chessController->botMove().getPromotedPieceType();

            if(m_chessController->botMove().isCapture())
            {
                //remove the captured piece
                unsigned int capturedPieceType = m_chessController->botMove().getCapturedPieceType();
                printf("Capture remove piece %d \r\n",capturedPieceType);
            }
            printf("Capture remove pawn color[%d] \r\n",m_side == 0?"White":"Black");
            printf("Capture Add piece[%d] color[%d] \r\n",promotedType,m_side == 0?"White":"Black");
        }
        else
        {
            if (m_chessController->botMove().isEnPassant()) // watch out ep capture is a capture
            {
                printf("Capture remove pawn color[%d] \r\n",m_side != 0?"White":"Black");
            }
            else //Move is capture
            {
                //remove the captured piece
                unsigned int type(m_chessController->botMove().getCapturedPieceType());
                printf("Capture remove captured piece[%d] color[%s] \r\n",type,m_side != 0?"White":"Black");

            }

            printf("Move piece from %s to %s\r\n",from.toStdString().c_str(),
                   to.toStdString().c_str());
        }
    }


    char robotCommand[32];
    if(m_chessController->botMove().isCapture()) {
        sprintf(robotCommand,"a%d%d%d%d%c%c",fromCoord.y(),fromCoord.x(),toCoord.y(),toCoord.x(),
                'p',
                canMoveStraight(fromCoord.y(),fromCoord.x(),toCoord.y(),toCoord.x(),PIECE_MOVE_CAPTURE)?'-':'n');
    } else if(m_chessController->botMove().isCastling()) {
        sprintf(robotCommand,"CST%d%d%d%d%c",fromCoord.y(),fromCoord.x(),
                toCoord.y(),toCoord.x() > fromCoord.x()?7:0,
                '-');
    } else if(m_chessController->botMove().isEnPassant()) {
        sprintf(robotCommand,"pp%d%d%d%d%c%c%c",fromCoord.y(),fromCoord.x(),toCoord.y(),toCoord.x(),
                'p','0',
                canMoveStraight(fromCoord.y(),fromCoord.x(),toCoord.y(),toCoord.x(),PIECE_MOVE_ENPASSANT)?'-':'n');
    } else if(m_chessController->botMove().isPromotion()) {
        /**
         * @brief promoChar
         * 1	0	0	0	knight-promotion
         * 1	0	0	1	bishop-promotion
         * 1	0	1	0	rook-promotion
         * 1	0	1	1	queen-promotion
         * 1	1	0	0	knight-promo capture
         * 1	1	0	1	bishop-promo capture
         * 1	1	1	0	rook-promo capture
         * 1	1	1	1	queen-promo capture
         */
        char capturedPieceChar = '0';
        char promotePieceChar = 'q';
        if(m_chessController->botMove().isCapture())
        {
            capturedPieceChar = m_chessController->botMove().getCapturedPieceType();
        }
        promotePieceChar = m_chessController->botMove().getPromotedPieceType();

        sprintf(robotCommand,"pm%d%d%d%d%c%c",fromCoord.y(),fromCoord.x(),toCoord.y(),toCoord.x(),
                capturedPieceChar,promotePieceChar);
    } else {
        sprintf(robotCommand,"c%d%d%d%d%c",fromCoord.y(),fromCoord.x(),toCoord.y(),toCoord.x(),
                canMoveStraight(fromCoord.y(),fromCoord.x(),toCoord.y(),toCoord.x())?'-':'n');
    }
    m_robotCommand = QString(robotCommand);
    qDebug("playCalculateNextMove %s to cmd[%s]\r\n",
           lastMove.toStdString().c_str(),
           robotCommand);
    return STATE_DONE_SUCCESS;
}

uint8_t ChessBot::playExecuteNextMove()
{
    // TODO: Send command to robot and wait until execution is done
#ifdef IMAGE_PROCESS_MOVE
    qDebug("Request Robot playExecuteNextMove");
    if (!robotController->isOpen()) {
        qDebug("Serial port is not open for abort.");
        return STATE_DONE_SUCCESS;
    } else {
        robotController->write(m_robotCommand.toUtf8());
        robotController->waitForBytesWritten(1000);
        sleep(1);
        if (robotController->waitForReadyRead(500)) {
            QByteArray chunk = robotController->readAll();
            qDebug("Received progress chunk: %s", chunk.constData());
        }
        QString moveCmdID = "";
        QString moveCmdRequest = "";
        QString moveCmdState = "";
        int retry = 0;
        do {
            robotController->write("cmd");
            robotController->waitForBytesWritten(200);
            if (robotController->waitForReadyRead(500)) {
                QByteArray chunk = robotController->readAll();
                qDebug("Received progress chunk: %s", chunk.constData());
                QString moveCmdIDStr = QString::fromLatin1(chunk);
                if(moveCmdIDStr.contains("[cmd]")) {
                    moveCmdID = moveCmdIDStr.section(']', 1);
                    qDebug("move: %s", moveCmdID.toStdString().c_str());
                    break;
                }
            }
            retry++;
        } while(retry < 5);

        if(moveCmdID != "") {
            moveCmdRequest = "_"+moveCmdID;
            retry = 0;
            do {
                robotController->write(moveCmdRequest.toStdString().c_str());
                robotController->waitForBytesWritten(200);
                if (robotController->waitForReadyRead(200)) {
                    QByteArray chunk = robotController->readAll();
                    qDebug("Received progress chunk: %s", chunk.constData());
                    QString moveCmdStateStr = QString::fromLatin1(chunk);
                    if(moveCmdStateStr.contains("]DONE")) {
                        moveCmdState = moveCmdStateStr.section(']', 1);
                        qDebug("move State: %s", moveCmdState.toStdString().c_str());
                        break;
                    }
                }
                sleep(1);
                retry++;
            } while(retry < 25);
        }
        return STATE_DONE_SUCCESS;
    }
#else
    return STATE_DONE_SUCCESS;
#endif
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
    robotController->write(m_commandTest.toStdString().c_str());
    robotController->waitForBytesWritten(200);
    if (robotController->waitForReadyRead(500)) {
        QByteArray chunk = robotController->readAll();
        qDebug("Received progress chunk: %s", chunk.constData());
        testState = STATE_DONE_SUCCESS;
    }
//    sleep(1);
//    robotController->write("cmd");
//    robotController->waitForBytesWritten(200);
//    if (robotController->waitForReadyRead(500)) {
//        QByteArray chunk = robotController->readAll();
//        qDebug("Received progress chunk: %s", chunk.constData());
//        QString positionCmdIDStr = QString::fromLatin1(chunk);
//        if(positionCmdIDStr.contains("[cmd]")) {
//            m_cmdId = positionCmdIDStr.section(']', 1);
//            qDebug("position Cmd ID: %s", m_cmdId.toStdString().c_str());
//            testState = STATE_DONE_SUCCESS;
//        }
//    }
    return testState;
}

uint8_t ChessBot::testCheckResult()
{
//    uint8_t testState = STATE_PENDING;
//    robotController->write(("_"+m_cmdId).toStdString().c_str());
//    robotController->waitForBytesWritten(200);
//    if (robotController->waitForReadyRead(500)) {
//        QByteArray chunk = robotController->readAll();
//        qDebug("Received progress chunk: %s", chunk.constData());
//        QString positionCmdStateStr = QString::fromLatin1(chunk);
//        if(positionCmdStateStr.contains("]DONE")) {
//            qDebug("position command %s Done", m_cmdId.toStdString().c_str());
//            testState = STATE_DONE_SUCCESS;
//        }
//    }
//    sleep(1);
    return STATE_DONE_SUCCESS;
}

bool ChessBot::readCalibrationPoint(const QString &command,QPoint& point)
{

    if (!robotController->isOpen()) {
        qDebug("Serial port is not open.");
        return false;
    }

    // Send calibration request command
    qDebug("Sending calibration command: %s", command.toStdString().c_str());
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
    if (!robotController->isOpen()) {
        qDebug("Serial port is not open.");
         Q_EMIT calibrationUploadComplete(CALIB_UPLOAD_TO_ROBOT, false);
        return false;
    }

    qDebug("Sending calibration data to RobotController...");

    // Send start marker and wait for acknowledgment
    robotController->write("CALIB_START\n");
    robotController->waitForBytesWritten(200);

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
            robotController->write(cmd.toLatin1());
            robotController->waitForBytesWritten(20);

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
            robotController->write(cmd.toLatin1());
            robotController->waitForBytesWritten(20);

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
            robotController->write(cmd.toLatin1());
            robotController->waitForBytesWritten(20);

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
    robotController->write("CALIB_END\n");
    robotController->waitForBytesWritten(200);

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
    if (!robotController->isOpen()) {
        qDebug("Serial port is not open for abort.");
        return;
    }

    qDebug("Aborting calibration upload...");
    robotController->write("CALIB_ABORT\n");
    robotController->waitForBytesWritten(100);
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
        if (robotController->waitForReadyRead(50)) {
            QByteArray chunk = robotController->readAll();
            allResponses.append(chunk);
            qDebug("Received progress chunk: %s", chunk.constData());
        }
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
            m_stateInit = INIT_GET_VERSION;
        } else {
            qDebug("Failed to detect Arduino port. Initialization aborted.");
            Q_EMIT calibrationUploadComplete(INIT_COMMUNICATION, false);
            m_state = STATE_EXIT;
            togglePause(true);
        }
    }
        break;

    case INIT_GET_VERSION: {
        qDebug("[Step 2] Getting Arduino version...");
        if (getArduinoVersion()) {
#ifdef INIT_ROBOT_CALIBRATION
            m_stateInit = INIT_SEND_CALIBRATION;
#else
            m_stateInit = INIT_ENABLE_ROBOT;
#endif
        } else {
            qDebug("Failed to get Arduino version. Initialization aborted.");
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
        m_state = STATE_CONFIGURE;
        togglePause(true);
    }
        break;
    }
}

uint8_t ChessBot::enableRobot()
{
    qDebug("Enable Robot");
    if (!robotController->isOpen()) {
        qDebug("Serial port is not open for abort.");
        return STATE_DONE_SUCCESS;
    } else {
        robotController->write("ee");
        robotController->waitForBytesWritten(1000);
        bool robotEnabled = false;
        int retry = 0;
        do {
            robotController->write("es");
            robotController->waitForBytesWritten(200);
            if (robotController->waitForReadyRead(500)) {
                QByteArray chunk = robotController->readAll();
                qDebug("Received progress chunk: %s", chunk.constData());
                QString responseStr = QString::fromLatin1(chunk);
                if(responseStr.contains("[es] Enabled")) {
                    robotEnabled = true;
                }
            }
        } while(!robotEnabled && retry < 5);
        sleep(1);
        return STATE_DONE_SUCCESS;
    }
}

uint8_t ChessBot::goHome()
{
    qDebug("Request Robot to go home");
    if (!robotController->isOpen()) {
        qDebug("Serial port is not open for abort.");
        return STATE_DONE_SUCCESS;
    } else {
        robotController->write("ha");
        robotController->waitForBytesWritten(1000);
        sleep(1);
        if (robotController->waitForReadyRead(500)) {
            QByteArray chunk = robotController->readAll();
            qDebug("CMD[ha] Received progress chunk: %s", chunk.constData());
        }
        QString homeCmdID = "";
        QString homeCmdRequest = "";
        QString homeCmdState = "";
        int retry = 0;
        do {
            robotController->write("cmd");
            robotController->waitForBytesWritten(200);
            if (robotController->waitForReadyRead(500)) {
                QByteArray chunk = robotController->readAll();
                qDebug("CMD[cmd] Received progress chunk: %s", chunk.constData());
                QString homeCmdIDStr = QString::fromLatin1(chunk);
                if(homeCmdIDStr.contains("[cmd]")) {
                    QRegularExpression re("\\d+");
                    QRegularExpressionMatch match = re.match(homeCmdIDStr);
                    if (match.hasMatch()) {
                        homeCmdID = match.captured(0);
                        qDebug() << "Extracted numbers:" << homeCmdID; // Outputs: "0001"
                        break;
                    }
                }
            }
            retry++;
        } while(retry < 5);

        if(homeCmdID != "") {
            homeCmdRequest = "_"+homeCmdID;
            retry = 0;
            do {
                robotController->write(homeCmdRequest.toStdString().c_str());
                robotController->waitForBytesWritten(200);
                if (robotController->waitForReadyRead(200)) {
                    QByteArray chunk = robotController->readAll();
                    qDebug("CMD[%s] Received progress chunk: %s",
                           homeCmdRequest.toStdString().c_str(),
                           chunk.constData());
                    QString homeCmdStateStr = QString::fromLatin1(chunk);
                    if(homeCmdStateStr.contains("]DONE")) {
                        homeCmdState = homeCmdStateStr.section(']', 1);
                        qDebug("homeCmdState: %s", homeCmdState.toStdString().c_str());
                        break;
                    }
                }
                sleep(1);
                retry++;
            } while(retry < 25);
        }
        return STATE_DONE_SUCCESS;
    }
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
#if defined(IMAGE_PROCESS_MOVE) && defined(DEBUG_SIMPLE_MOVE)
    if(m_chessboardConners.size() == 4)
    {
        m_moveDetector->setCorners(m_chessboardConners[0].x(),m_chessboardConners[0].y(),
                m_chessboardConners[1].x(),m_chessboardConners[1].y(),
                m_chessboardConners[2].x(),m_chessboardConners[2].y(),
                m_chessboardConners[3].x(),m_chessboardConners[3].y());
        cv::Mat src1 = cv::imread("/home/hainh/Desktop/Project/ChessBot/ChessPlayer/build/failcases/f0049.jpg");
        cv::Mat src2 = cv::imread("/home/hainh/Desktop/Project/ChessBot/ChessPlayer/build/failcases/f0050.jpg");
        if(!src1.empty() && !src2.empty()) {
            std::vector<std::string> chessMoves = m_moveDetector->findPossibleMoves(src1, src2, *m_detectParams);
            for(int i = 0; i< chessMoves.size(); i++) {
                qDebug("Possible Move %s",chessMoves[i].c_str());
            }
        }
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
    m_state = STATE_TEST;
    m_stateTest = TEST_ROBOT;
    m_commandTest = command;
    togglePause(false);
}

void ChessBot::homingRobot()
{
    sendTestCommand("ha");
}

void ChessBot::initRobotCommunication() {
    qDebug("ChessBot::initRobotCommunication");
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
//    m_statePlay = PLAY_INFORM_ERROR;
    Q_EMIT playTurnChanged(m_side == Color::WHITE ? 1-m_side : m_side);
    togglePause(false);
    startService();
}

void ChessBot::undoMove()
{
    m_chessController->undoMove();
}

void ChessBot::setLevel(int level)
{
    qDebug("Set level: %d",level);
    m_levelScore = level;
    m_levelType = level/400+1;
    m_chessController->setEngineLevel(m_levelType);
}

void ChessBot::setSide(int side)
{
    qDebug("Set side: %d",side);
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
    qDebug("Reset game side[%d]",m_side);
    m_chessController->newGame();    
    if(m_side == 1) {
        m_state = STATE_PLAY;
        m_statePlay = PLAY_CALCULATE_NEXT_MOVE_RESET;
        togglePause(false);
        startService();
    }
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

        // Configure and open the port
        robotController->setPortName(portInfo.portName());
        robotController->setBaudRate(baudRate);
        robotController->setDataBits(QSerialPort::Data8);
        robotController->setParity(QSerialPort::NoParity);
        robotController->setStopBits(QSerialPort::OneStop);
        robotController->setFlowControl(QSerialPort::NoFlowControl);

        if (robotController->open(QIODevice::ReadWrite)) {
            qDebug("Opened port: %s", portInfo.portName().toStdString().c_str());
            for(int i=0; i< 2; i++) {
                if (robotController->waitForReadyRead(1000)) {
                    QByteArray response = robotController->readAll();
                    qDebug("First connect: %s", response.constData());
                }
            }
            for(int i=0; i< 2; i++) {
                // Send version request
                robotController->write("v");
                robotController->waitForBytesWritten(500);

                // Wait for response with timeout
                if (robotController->waitForReadyRead(1000)) {
                    QByteArray response = robotController->readAll();
                    qDebug("Response received: %s", response.constData());

                    // Check if response contains "[v]"
                    if (response.contains("[v]")) {
                        qDebug("Arduino detected on port: %s",
                               portInfo.portName().toStdString().c_str());
                        m_arduinoVersion = QString::fromLatin1(response);
                        return true;
                    }
                } else {
                    qDebug("No response from port: %s", portInfo.portName().toStdString().c_str());
                }
            }
            robotController->close();
        } else {
            qDebug("Failed to open port: %s", portInfo.portName().toStdString().c_str());
        }
    }

    qDebug("Arduino not detected on any port.");
    return false;
}

bool ChessBot::getArduinoVersion()
{
    if (!robotController->isOpen()) {
        qDebug("Serial port is not open.");
        return false;
    }

    // Send version request
    qDebug("Sending version request...");
    robotController->write("v");
    robotController->waitForBytesWritten(500);

    // Collect all responses for 2 seconds
    QByteArray allResponses;
    QTime timer;
    timer.start();

    qDebug("Collecting responses for 2 seconds...");
    while (timer.elapsed() < 2000) {
        if (robotController->waitForReadyRead(100)) {
            QByteArray chunk = robotController->readAll();
            allResponses.append(chunk);
            qDebug("Received chunk: %s", chunk.constData());
        }
    }

    if (allResponses.isEmpty()) {
        qDebug("No response from Arduino.");
        return false;
    }

    qDebug("Total responses: %s", allResponses.constData());

    // Split responses into lines and find the valid one with "[v]"
    QString responseStr = QString::fromLatin1(allResponses);
    QStringList responses = responseStr.split(QRegExp("[\\r\\n]+"), QString::SkipEmptyParts);

    for (const QString &response : responses) {
        QString trimmedResponse = response.trimmed();
        qDebug("Processing response: %s", trimmedResponse.toStdString().c_str());

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
                qDebug("Valid Arduino Version found: %s", version.toStdString().c_str());
                return true;
            }
        }
    }

    qDebug("No valid response found with '[v]' pattern");
    return false;
}

void ChessBot::speakText(const QString &text)
{
#if defined(_WIN32)
    m_speech->say(text.trimmed());
#else
    m_speech->speak(text);
#endif
}

void ChessBot::speakMove(const QString fen, const int color,
                         QString pieceType, const QString pieceNotation, const Move &move)
{
    QString formattedMove = m_chessController->processRobotCommentary(fen,color, pieceType, pieceNotation, move);
    speakText(formattedMove);
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
