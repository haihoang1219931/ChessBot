#include "ChessController.h"

#include <QString>

#include "MagicMoves.hpp"
#include "MoveGen.hpp"
#include "Pawn.hpp"
#include "Search.hpp"
#include "TT.hpp"
#include "Tables.hpp"
#include "Utils.hpp"
#include "Piece.hpp"
#include "Eval.hpp"


// Global speech phrase configurations for kids
const QStringList BLUNDER_PHRASES = {
    "Oh no! That is a major blunder!",
    "Be careful! You left a piece hanging!",
    "Ouch! That move hurts your position."
};

const QStringList MISTAKE_PHRASES = {
    "Hmm, that's okay, but watch your defenses.",
    "An interesting choice, but you missed a safer square.",
    "Be cautious! The engine sees an opening there."
};

const QStringList BRILLIANT_PHRASES = {
    "Wow! That move is absolutely brilliant!",
    "Incredible play! Are you a grandmaster?",
    "Excellent tactical vision!"
};

const QStringList GOOD_PHRASES = {
    " is a very solid move.",
    " helps you control the board.",
    " is a nice strategic development."
};

ChessController::ChessController(QObject* parent)
    : QObject(parent)
    , m_board(std::make_shared<Board>())
    , m_selectedUiSquare(-1)
    , m_status("Ready")
    , m_promotionPending(false)
    , m_checkedKingSquare(-1)
    , m_engineDepth(3)
    , m_playerColor(0)
{
    MagicMoves::initmagicmoves();
    Tables::init();
    ZK::initZobristKeys();
    Pawn::initPawnTable();
    globalTT.init_TT_size(TT::TEST_MB_SIZE);

    refreshBoardModel();
    refreshCheckState();
}

QStringList ChessController::board() const
{
    return m_boardModel;
}

int ChessController::selectedSquare() const
{
    return m_selectedUiSquare;
}

QString ChessController::sideToMove() const
{
    return m_board->getColorToPlay() == WHITE ? "White" : "Black";
}

QString ChessController::status() const
{
    return m_status;
}

void ChessController::setStatus(QString status)
{
    qDebug("ChessController::setStatus %s",status.toStdString().c_str());
    if(m_status != status) {
        m_status = status;
        Q_EMIT statusChanged();
    }
}

std::vector<Move> ChessController::moveHistory() const
{
    return m_moveHistory;
}

bool ChessController::promotionPending() const
{
    return m_promotionPending;
}

int ChessController::checkedKingSquare() const
{
    return m_checkedKingSquare;
}

int ChessController::engineLevel() const
{
    return m_engineDepth;
}

int ChessController::playerColor() const
{
    return m_playerColor;
}

void ChessController::setEngineLevel(int level)
{
    if (level < 1) level = 1;
    if (level > 5) level = 5;
    if (m_engineDepth == level) return;
    m_engineDepth = level;
    Q_EMIT engineLevelChanged();
}

void ChessController::setPlayerColor(int color)
{
    if (color != 0 && color != 1) return;
    if (m_playerColor == color) return;
    m_playerColor = color;
    Q_EMIT playerColorChanged();
}

void ChessController::undoMove()
{

}

const std::string whiteCatsling1 = "1nbqk2r/pppp1ppp/8/2p1bn2/5N2/1B1Q4/P1rPPPPP/R3K2R w KQkq -";
const std::string whiteCatsling2 = "r3k2r/8/8/8/3B4/8/8/R3K2R w KQkq - 0 1";
const std::string whitePawnPromotion = "8/2P1k3/8/3K4/8/8/8/8 w - - 0 1";
const std::string whiteMateFen = "7k/6Q1/6K1/8/8/8/8/8 b - - 0 1";
const std::string blackMateFen = "7K/6q1/6k1/8/8/8/8/8 w - - 0 1";
const std::string staleMateFen = "7k/5Q2/7K/8/8/8/8/8 b - - 0 1";
const std::string ongoingFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const std::string whitePawnPromotionCapture = "1r6/2P1k3/8/3K4/8/8/8/8 w - - 0 1";
const std::string whitePawnDoubleCapture = "3r1r2/4P3/8/k7/8/8/8/1K6 w - - 0 1";
const std::string blackPrePromotion = "k7/8/8/8/8/8/2p5/4K3 w - - 0 1";
const std::string blackCapturePromotion = "k7/8/8/8/8/8/1p6/2R1K3 w - - 0 1";
void ChessController::newGame()
{
    m_board = std::make_shared<Board>(blackCapturePromotion);
    globalTT.clearTT();
    m_moveHistory.clear();
    Q_EMIT moveHistoryChanged();
    m_promotionPending = false;
    m_pendingPromotionMoves.clear();
    Q_EMIT promotionPendingChanged();
    clearSelection();
    refreshBoardModel();
    refreshCheckState();
    setStatus("NEW_GAME");
    Q_EMIT sideToMoveChanged();
}

void ChessController::clickSquare(int uiIndex)
{    
    if (uiIndex < 0 || uiIndex >= 64)
    {
        return;
    }

    const int square = uiIndexToSquare(uiIndex);

    if (m_promotionPending)
    {
        return;
    }

    if (m_selectedUiSquare < 0)
    {
        QString piece = pieceCodeAtSquare(square);
        QString pieceName =  convertPieceText(piece);
        printf("Click at %s\r\n",pieceName.toStdString().c_str());
        if (piece.isEmpty())
        {
            return;
        }

        const bool isWhitePiece = piece.startsWith("w");
        if ((isWhitePiece && m_board->getColorToPlay() != WHITE) || (!isWhitePiece && m_board->getColorToPlay() != BLACK))
        {
            setStatus("OWN_PIECE_SELECTED");
            return;
        }

        m_selectedUiSquare = uiIndex;
        refreshSelectionMoves();
        Q_EMIT selectedSquareChanged();
        Q_EMIT boardChanged();
        return;
    }

    Q_EMIT boardChanged();
}

bool ChessController::moveByUiSquares(int startUiIndex, int stopUiIndex, Move& chosenMove)
{
    if (startUiIndex < 0 || startUiIndex >= 64 || stopUiIndex < 0 || stopUiIndex >= 64)
    {
        return false;
    }
    if (m_playerColor == Color::BLACK) {
        startUiIndex = 63 - startUiIndex;
        stopUiIndex = 63 - stopUiIndex;
    }
    if (m_promotionPending)
    {
        setStatus("PROMOTION_PENDING");
        return false;
    }

    const int originSquare = uiIndexToSquare(startUiIndex);
    const int destinationSquare = uiIndexToSquare(stopUiIndex);

    const QString piece = pieceCodeAtSquare(originSquare);
    if (piece.isEmpty())
    {
        return false;
    }

    const bool isWhitePiece = piece.startsWith("w");
    if ((isWhitePiece && m_board->getColorToPlay() != WHITE) || (!isWhitePiece && m_board->getColorToPlay() != BLACK))
    {
        setStatus("OWN_PIECE_SELECTED");
        return false;
    }

    if (!tryFindLegalMove(originSquare, destinationSquare, chosenMove))
    {
        Q_EMIT boardChanged();
        return false;
    }
    m_board->executeMove(chosenMove);
    m_moveHistory.push_back(chosenMove);
    Q_EMIT moveHistoryChanged();
    clearSelection();
    refreshBoardModel();
    refreshCheckState();
    Q_EMIT sideToMoveChanged();

    const QString resultAfterPlayer = buildResultText();
    if (!resultAfterPlayer.isEmpty())
    {
        setStatus(resultAfterPlayer);
        Q_EMIT boardChanged();
        return true;
    }

    return true;
}

bool ChessController::moveByUiIndex(int startUiIndex,
                                    int stopUiIndex,
                                    Move& chosenMove,
                                    QChar promotionSuffix)
{
    const int originSquare = uiIndexToSquare(startUiIndex);
    const int destinationSquare = uiIndexToSquare(stopUiIndex);

    const QString piece = pieceCodeAtSquare(originSquare);
    if (piece.isEmpty())
    {
        return false;
    }

    const bool isWhitePiece = piece.startsWith("w");
    if ((isWhitePiece && m_board->getColorToPlay() != WHITE) || (!isWhitePiece && m_board->getColorToPlay() != BLACK))
    {
        setStatus("OWN_PIECE_SELECTED");
        return false;
    }

    if (!tryFindLegalMove(originSquare, destinationSquare, chosenMove, promotionSuffix))
    {
        Q_EMIT boardChanged();
        return false;
    }

    m_board->executeMove(chosenMove);
    m_moveHistory.push_back(chosenMove);
    Q_EMIT moveHistoryChanged();
    clearSelection();
    refreshBoardModel();
    refreshCheckState();
    Q_EMIT sideToMoveChanged();

    const QString resultAfterPlayer = buildResultText();
    if (!resultAfterPlayer.isEmpty())
    {
        setStatus(resultAfterPlayer);
        Q_EMIT boardChanged();
        return true;
    }
    return true;
}

bool ChessController::isValidMoveByCoordinates(const QString& startSquare,
                                        const QString& stopSquare,
                                        Move& chosenMove,
                                        QChar promotionSuffix)
{
    int startUiIndex = -1;
    if (!tryParseCoordinate(startSquare, startUiIndex))
    {
        return false;
    }

    const QString stopTrimmed = stopSquare.trimmed().toLower();
    if (stopTrimmed.size() != 2 && stopTrimmed.size() != 3)
    {
        return false;
    }

    int stopUiIndex = -1;
    if (!tryParseCoordinate(stopTrimmed.left(2), stopUiIndex))
    {
        return false;
    }

    if (startUiIndex < 0 || startUiIndex >= 64 || stopUiIndex < 0 || stopUiIndex >= 64)
    {
        return false;
    }

    if (m_promotionPending)
    {
        return false;
    }

    const int originSquare = uiIndexToSquare(startUiIndex);
    const int destinationSquare = uiIndexToSquare(stopUiIndex);

    const QString piece = pieceCodeAtSquare(originSquare);
    if (piece.isEmpty())
    {
        return false;
    }

    const bool isWhitePiece = piece.startsWith("w");
    if ((isWhitePiece && m_board->getColorToPlay() != WHITE) || (!isWhitePiece && m_board->getColorToPlay() != BLACK))
    {
        return false;
    }

    // check if the move is legal
    MoveGen moveGen(m_board);
    const auto legalMoves = moveGen.generateMoves();

    m_pendingPromotionMoves.clear();
    bool hasMatchingPromotion = false;

    for (const Move& move : legalMoves)
    {
        if (move.getOrigin() == originSquare && move.getDestination() == destinationSquare)
        {
            if (move.isPromotion())
            {
                hasMatchingPromotion = true;
                if (!promotionSuffix.isNull())
                {
                    const QString moveText = QString::fromStdString(move.toShortString());
                    if (moveText.endsWith(promotionSuffix))
                    {
                        chosenMove = move;
                        return true;
                    }
                }
                else
                {
                    m_pendingPromotionMoves.push_back(move);
                }
            }
            else
            {
                chosenMove = move;
                return true;
            }
        }
    }
    return false;
}

bool ChessController::moveByCoordinates(const QString& startSquare,
                                        const QString& stopSquare,
                                        Move& chosenMove,
                                        QChar promotionSuffix)
{
    int startUiIndex = -1;
    if (!tryParseCoordinate(startSquare, startUiIndex))
    {
        setStatus("INVALID_COORDINATE");
        return false;
    }

    const QString stopTrimmed = stopSquare.trimmed().toLower();
    if (stopTrimmed.size() != 2 && stopTrimmed.size() != 3)
    {
        setStatus("INVALID_COORDINATE");
        return false;
    }

    int stopUiIndex = -1;
    if (!tryParseCoordinate(stopTrimmed.left(2), stopUiIndex))
    {
        setStatus("INVALID_COORDINATE");
        return false;
    }

    if (startUiIndex < 0 || startUiIndex >= 64 || stopUiIndex < 0 || stopUiIndex >= 64)
    {
        return false;
    }

    if (m_promotionPending)
    {
        setStatus("PROTOMTION_PENDING");
        return false;
    }
    return moveByUiIndex(startUiIndex,stopUiIndex,chosenMove,promotionSuffix);
}

QStringList ChessController::findBestMoveCoordinates() const
{
    Search search(m_board);
    search.negaMaxRoot(m_engineDepth);

    const QString bestMoveText = QString::fromStdString(Utils::Move16ToShortString(search.myBestMove));
    if (bestMoveText.size() < 4)
    {
        return QStringList();
    }

    const QString start = bestMoveText.mid(0, 2);
    const QString stop = bestMoveText.size() >= 5 ? bestMoveText.mid(2, 3) : bestMoveText.mid(2, 2);
    return QStringList{start, stop};
}

bool ChessController::isValidDestination(int uiIndex) const
{
    return m_validDestinationUiSquares.contains(uiIndex);
}

void ChessController::choosePromotion(const QString& pieceLetter)
{
    if (!m_promotionPending)
    {
        return;
    }

    QChar suffix = pieceLetter.trimmed().toLower().isEmpty() ? QChar('q') : pieceLetter.trimmed().toLower().at(0);
    if (suffix != 'q' && suffix != 'r' && suffix != 'b' && suffix != 'n')
    {
        suffix = 'q';
    }

    Move chosenMove;
    bool found = false;

    for (const Move& move : m_pendingPromotionMoves)
    {
        const QString moveText = QString::fromStdString(move.toShortString());
        if (moveText.endsWith(suffix))
        {
            chosenMove = move;
            found = true;
            break;
        }
    }

    if (!found && !m_pendingPromotionMoves.empty())
    {
        chosenMove = m_pendingPromotionMoves.front();
        found = true;
    }

    m_promotionPending = false;
    m_pendingPromotionMoves.clear();
    Q_EMIT promotionPendingChanged();

    if (!found)
    {
        setStatus("PROMOTION_SELECTION_FAILED");
        return;
    }

    m_board->executeMove(chosenMove);
    m_moveHistory.push_back(chosenMove);
    Q_EMIT moveHistoryChanged();
    clearSelection();
    refreshBoardModel();
    refreshCheckState();
    Q_EMIT sideToMoveChanged();

    const QString resultAfterPlayer = buildResultText();
    if (!resultAfterPlayer.isEmpty())
    {
        setStatus(resultAfterPlayer);
        Q_EMIT boardChanged();
        return;
    }
    Q_EMIT boardChanged();
}

void ChessController::cancelPromotion()
{
    m_promotionPending = false;
    m_pendingPromotionMoves.clear();
}

void ChessController::refreshBoardModel()
{
    m_boardModel = QStringList();
    for (int uiIndex = 0; uiIndex < 64; ++uiIndex)
    {
        m_boardModel.append(pieceCodeAtSquare(uiIndexToSquare(uiIndex)));
    }

    Q_EMIT boardChanged();
}

void ChessController::refreshSelectionMoves()
{
    m_validDestinationUiSquares.clear();
    if (m_selectedUiSquare < 0)
    {
        return;
    }

    const int selectedSquare = uiIndexToSquare(m_selectedUiSquare);
    MoveGen moveGen(m_board);
    const auto legalMoves = moveGen.generateMoves();
    for (const Move& move : legalMoves)
    {
        if (move.getOrigin() == selectedSquare)
        {
            m_validDestinationUiSquares.insert(squareToUiIndex(move.getDestination()));
        }
    }
}

void ChessController::refreshCheckState()
{
    m_board->updateKingAttackers(m_board->getColorToPlay());
    if (m_board->isCheck())
    {
        const Square kingSquare = m_board->getColorToPlay() == WHITE ? m_board->getWhiteKingSquare() : m_board->getBlackKingSquare();
        m_checkedKingSquare = squareToUiIndex(static_cast<int>(kingSquare));
    }
    else
    {
        m_checkedKingSquare = -1;
    }

    Q_EMIT checkedKingSquareChanged();
}

void ChessController::clearSelection()
{
    m_selectedUiSquare = -1;
    m_validDestinationUiSquares.clear();
    Q_EMIT selectedSquareChanged();
}

void ChessController::playEngineMove()
{
    Search search(m_board);
    search.negaMaxRoot(m_engineDepth);
    Move bestMove = search.myBestMove;
    MoveGen moveGen(m_board);
    const auto legalMoves = moveGen.generateMoves();
    for (Move move : legalMoves)
    {
        if (QString::fromStdString(move.toShortString()) ==
                QString::fromStdString(bestMove.toShortString()))
        {
            m_botMove = move;
            m_board->executeMove(move);
            m_moveHistory.push_back(move);
            Q_EMIT moveHistoryChanged();
            refreshBoardModel();
            refreshCheckState();
            Q_EMIT sideToMoveChanged();

            const QString result = buildResultText();
            setStatus(result.isEmpty() ?
                          QString("ENGINE_PLAY_") + QString::fromStdString(bestMove.toShortString()) : result);
            return;
        }
    }
    setStatus("NO_LEGAL_ENGINE_MOVE_FOUND");
}

QString ChessController::convertPieceText(QString pieceShortName)
{
    if (pieceShortName == "wP") return "White Pawn";
    else if (pieceShortName == "wN") return "White Knight";
    else if (pieceShortName == "wB") return "White Bishop";
    else if (pieceShortName == "wR") return "White Rook";
    else if (pieceShortName == "wQ") return "White Queen";
    else if (pieceShortName == "wK") return "White King";
    else if (pieceShortName == "bP") return "Black Pawn";
    else if (pieceShortName == "bN") return "Black Knight";
    else if (pieceShortName == "bB") return "Black Bishop";
    else if (pieceShortName == "bR") return "Black Rook";
    else if (pieceShortName == "bQ") return "Black Queen";
    else if (pieceShortName == "bK") return "Black King";
    else if (pieceShortName == "") return "Empty Square";
    else return "Unknown Piece ID";
}

QString ChessController::pieceType(QString square)
{
    int uiIndex = -1;
    if(!tryParseCoordinate(square,uiIndex)) return "Unknown Piece ID";
    if (uiIndex < 0 || uiIndex >= 64)
    {
        return "";
    }
    int squareIndex = uiIndexToSquare(uiIndex);
    QString piece = pieceCodeAtSquare(squareIndex);
    QString pieceName =  convertPieceText(piece);
    return pieceName;
}

QString ChessController::extractFEN()
{
    return QString::fromStdString(m_board->extractFen());
}

bool ChessController::tryFindLegalMove(int originSquare, int destinationSquare, Move& outMove, QChar promotionSuffix)
{
    MoveGen moveGen(m_board);
    const auto legalMoves = moveGen.generateMoves();

    m_pendingPromotionMoves.clear();
    bool hasMatchingPromotion = false;

    for (const Move& move : legalMoves)
    {
        if (move.getOrigin() == originSquare && move.getDestination() == destinationSquare)
        {
            if (move.isPromotion())
            {
                hasMatchingPromotion = true;
                if (!promotionSuffix.isNull())
                {
                    const QString moveText = QString::fromStdString(move.toShortString());
                    if (moveText.endsWith(promotionSuffix))
                    {
                        outMove = move;
                        return true;
                    }
                }
                else
                {
                    m_pendingPromotionMoves.push_back(move);
                }
            }
            else
            {
                outMove = move;
                return true;
            }
        }
    }

    if (!promotionSuffix.isNull() && hasMatchingPromotion)
    {
        setStatus("INVALID_PROMOTION_PIECE_FOR_MOVE");
        return false;
    }

    if (!m_pendingPromotionMoves.empty())
    {
        m_promotionPending = true;
        Q_EMIT promotionPendingChanged();
        setStatus("CHOOSE_PROMOTION_PIECE");
        return false;
    }

    return false;
}

QString ChessController::pieceCodeAtSquare(int square) const
{
    const U64 mask = 1ULL << square;

    if (m_board->getWhitePawns() & mask) return "wP";
    if (m_board->getWhiteKnights() & mask) return "wN";
    if (m_board->getWhiteBishops() & mask) return "wB";
    if (m_board->getWhiteRooks() & mask) return "wR";
    if (m_board->getWhiteQueens() & mask) return "wQ";
    if (m_board->getWhiteKing() & mask) return "wK";

    if (m_board->getBlackPawns() & mask) return "bP";
    if (m_board->getBlackKnights() & mask) return "bN";
    if (m_board->getBlackBishops() & mask) return "bB";
    if (m_board->getBlackRooks() & mask) return "bR";
    if (m_board->getBlackQueens() & mask) return "bQ";
    if (m_board->getBlackKing() & mask) return "bK";

    return "";
}

QString ChessController::buildResultText() const
{
    MoveGen moveGen(m_board);
    const auto legalMoves = moveGen.generateMoves();

    if (legalMoves.empty())
    {
        m_board->updateKingAttackers(m_board->getColorToPlay());
        if (m_board->isCheck())
        {
            return m_board->getColorToPlay() == WHITE ? "BLACK_WIN" : "WHITE_WIN";
        }
        return "DRAW_STALEMATE";
    } else {
        m_board->updateKingAttackers();
        if (m_board->isCheck())
        {
            return m_board->getColorToPlay() == WHITE ? "BLACK_CHECK" : "WHITE_CHECK";
        }
    }

    Search search(m_board);
    if (search.isInsufficentMatingMaterial())
    {
        return "DRAW_PIECE";
    }

    return "";
}

Move ChessController::botMove() const
{
    return m_botMove;
}

int ChessController::uiIndexToSquare(int uiIndex)
{
    const int file = uiIndex % 8;
    const int rankFromTop = uiIndex / 8;
    const int rankFromBottom = 7 - rankFromTop;
    return rankFromBottom * 8 + file;
}

int ChessController::squareToUiIndex(int square)
{
    const int file = square % 8;
    const int rankFromBottom = square / 8;
    const int rankFromTop = 7 - rankFromBottom;
    return rankFromTop * 8 + file;
}

QString ChessController::uiIndexToPieceType(int uiIndex)
{
    int squareIndex = uiIndexToSquare(m_playerColor == Color::WHITE?uiIndex:63-uiIndex);
    QString piece = pieceCodeAtSquare(squareIndex);
    QString pieceName = convertPieceText(piece);
    return pieceName;
}

QString ChessController::uiIndexToSquareNotation(int uiIndex)
{
    int squareIndex = uiIndexToSquare(uiIndex);
    return QString::fromStdString(coordToNotation(squareIndex));
}

std::string ChessController::coordToNotation(int squareIndex)
{
    int row = squareIndex/8;
    int col = squareIndex%8;
    if (row < 0 || row > 7 || col < 0 || col > 7) return "";
    char file, rank;
    if (m_playerColor == Color::WHITE) {
        file = 'a' + col;
        rank = '1' + row;
    } else { // black at bottom
        file = 'h' - col;
        rank = '8' - row;
    }
    return std::string(1, file) + std::string(1, rank);
}
bool ChessController::tryParseCoordinate(const QString& coordinate, int& uiIndex)
{
    const QString trimmed = coordinate.trimmed().toLower();
    if (trimmed.size() != 2)
    {
        return false;
    }

    const QChar fileChar = trimmed.at(0);
    const QChar rankChar = trimmed.at(1);
    if (fileChar < QChar('a') || fileChar > QChar('h') || rankChar < QChar('1') || rankChar > QChar('8'))
    {
        return false;
    }

    const int file = fileChar.toLatin1() - 'a';
    const int rankFromBottom = rankChar.toLatin1() - '1';
    const int square = rankFromBottom * 8 + file;
    uiIndex = squareToUiIndex(square);
    return true;
}

QString ChessController::processRobotCommentary(const QString fen, const int color,
                                                const QString pieceType, const QString pieceNotation,
                                                Move playerMove) {
    std::shared_ptr<Board> cloneBoard = std::make_shared<Board>(fen.toStdString());
    Eval eval(cloneBoard);
    // 1. Get score before execution
    int scoreBefore = eval.evaluate();

    // 2. Play the move using Deepov's internal transition function
    cloneBoard->executeMove(playerMove);
    int scoreAfter = eval.evaluate();

    // 4. Score drop calculation (Delta)
    // Note: Since turn flipped, adjust delta relative to who just moved
    std::cout << "scoreBefore: " << scoreBefore << " scoreAfter:" << scoreAfter << std::endl;
    QString speechText = "";
    QString moveNotation = pieceType+" to "+pieceNotation + " ";
    // Seed random selection
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // 5. Categorize score change
//    int delta = scoreAfter - scoreBefore;
//    if (delta >= 15) { // Loss of 1 whole pawn or more
//        speechText = BLUNDER_PHRASES[std::rand() % BLUNDER_PHRASES.size()];
//    }
//    else if (delta >= 3) { // Slight loss of positional advantage
//        speechText = MISTAKE_PHRASES[std::rand() % MISTAKE_PHRASES.size()];
//    }
//    else if (delta <= 15) { // Huge unexpected strategic gain
//        speechText = BRILLIANT_PHRASES[std::rand() % BRILLIANT_PHRASES.size()];
//    }
//    else
    { // Safe, standard development choice
        speechText = moveNotation + GOOD_PHRASES[std::rand() % GOOD_PHRASES.size()];
    }

    // 6. Direct command execution to offline Text-to-Speech Engine
    std::cout << "[Robot Voice Engine]: " << speechText.toStdString() << std::endl;

    return speechText;
}

