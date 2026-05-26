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

QStringList ChessController::moveHistory() const
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

void ChessController::newGame()
{
    m_board = std::make_shared<Board>();
    globalTT.clearTT();
    m_moveHistory.clear();
    Q_EMIT moveHistoryChanged();
    m_promotionPending = false;
    m_pendingPromotionMoves.clear();
    Q_EMIT promotionPendingChanged();
    m_status = "New game";
    clearSelection();
    refreshBoardModel();
    refreshCheckState();
    Q_EMIT statusChanged();
    Q_EMIT sideToMoveChanged();

    if (m_playerColor == 1) // player is Black — engine plays White's first move
    {
        playEngineMove();
        Q_EMIT boardChanged();
    }
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
            m_status = "Select your own piece";
            Q_EMIT statusChanged();
            return;
        }

        m_selectedUiSquare = uiIndex;
        refreshSelectionMoves();
        Q_EMIT selectedSquareChanged();
        Q_EMIT boardChanged();
        return;
    }

    if (!moveByUiSquares(m_selectedUiSquare, uiIndex))
    {
        clearSelection();
        Q_EMIT boardChanged();
        return;
    }

    Q_EMIT boardChanged();
}

bool ChessController::moveByUiSquares(int startUiIndex, int stopUiIndex)
{
    if (startUiIndex < 0 || startUiIndex >= 64 || stopUiIndex < 0 || stopUiIndex >= 64)
    {
        return false;
    }

    if (m_promotionPending)
    {
        m_status = "Promotion pending";
        Q_EMIT statusChanged();
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
        m_status = "Select your own piece";
        Q_EMIT statusChanged();
        return false;
    }

    Move chosenMove;
    if (!tryFindLegalMove(originSquare, destinationSquare, chosenMove))
    {
        Q_EMIT boardChanged();
        return false;
    }
    m_board->executeMove(chosenMove);
    m_moveHistory.append(QString::fromStdString(chosenMove.toShortString()));
    Q_EMIT moveHistoryChanged();
    clearSelection();
    refreshBoardModel();
    refreshCheckState();
    Q_EMIT sideToMoveChanged();

    const QString resultAfterPlayer = buildResultText();
    if (!resultAfterPlayer.isEmpty())
    {
        m_status = resultAfterPlayer;
        Q_EMIT statusChanged();
        Q_EMIT boardChanged();
        return true;
    }

    playEngineMove();
    return true;
}

bool ChessController::moveByCoordinates(const QString& startSquare, const QString& stopSquare)
{
    int startUiIndex = -1;
    if (!tryParseCoordinate(startSquare, startUiIndex))
    {
        m_status = "Invalid coordinate";
        Q_EMIT statusChanged();
        return false;
    }

    const QString stopTrimmed = stopSquare.trimmed().toLower();
    if (stopTrimmed.size() != 2 && stopTrimmed.size() != 3)
    {
        m_status = "Invalid coordinate";
        Q_EMIT statusChanged();
        return false;
    }

    int stopUiIndex = -1;
    if (!tryParseCoordinate(stopTrimmed.left(2), stopUiIndex))
    {
        m_status = "Invalid coordinate";
        Q_EMIT statusChanged();
        return false;
    }

    QChar promotionSuffix;
    if (stopTrimmed.size() == 3)
    {
        promotionSuffix = stopTrimmed.at(2);
        if (promotionSuffix != 'q' && promotionSuffix != 'r' && promotionSuffix != 'b' && promotionSuffix != 'n')
        {
            m_status = "Invalid promotion piece";
            Q_EMIT statusChanged();
            return false;
        }
    }

    if (startUiIndex < 0 || startUiIndex >= 64 || stopUiIndex < 0 || stopUiIndex >= 64)
    {
        return false;
    }

    if (m_promotionPending)
    {
        m_status = "Promotion pending";
        Q_EMIT statusChanged();
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
        m_status = "Select your own piece";
        Q_EMIT statusChanged();
        return false;
    }

    Move chosenMove;
    if (!tryFindLegalMove(originSquare, destinationSquare, chosenMove, promotionSuffix))
    {
        Q_EMIT boardChanged();
        return false;
    }

    m_board->executeMove(chosenMove);
    m_moveHistory.append(QString::fromStdString(chosenMove.toShortString()));
    Q_EMIT moveHistoryChanged();
    clearSelection();
    refreshBoardModel();
    refreshCheckState();
    Q_EMIT sideToMoveChanged();

    const QString resultAfterPlayer = buildResultText();
    if (!resultAfterPlayer.isEmpty())
    {
        m_status = resultAfterPlayer;
        Q_EMIT statusChanged();
        Q_EMIT boardChanged();
        return true;
    }
    return true;
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
        m_status = "Promotion selection failed";
        Q_EMIT statusChanged();
        return;
    }

    m_board->executeMove(chosenMove);
    m_moveHistory.append(QString::fromStdString(chosenMove.toShortString()));
    Q_EMIT moveHistoryChanged();
    clearSelection();
    refreshBoardModel();
    refreshCheckState();
    Q_EMIT sideToMoveChanged();

    const QString resultAfterPlayer = buildResultText();
    if (!resultAfterPlayer.isEmpty())
    {
        m_status = resultAfterPlayer;
        Q_EMIT statusChanged();
        Q_EMIT boardChanged();
        return;
    }

    playEngineMove();
    Q_EMIT boardChanged();
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

    const QString bestMoveText = QString::fromStdString(Utils::Move16ToShortString(search.myBestMove));

    MoveGen moveGen(m_board);
    const auto legalMoves = moveGen.generateMoves();
    for (Move move : legalMoves)
    {
        if (QString::fromStdString(move.toShortString()) == bestMoveText)
        {
            m_board->executeMove(move);
            m_botMove = Move(move.getMove());
            m_moveHistory.append(bestMoveText);
            Q_EMIT moveHistoryChanged();
            refreshBoardModel();
            refreshCheckState();
            Q_EMIT sideToMoveChanged();

            const QString result = buildResultText();
            m_status = result.isEmpty() ? QString("Engine played ") + bestMoveText : result;
            Q_EMIT statusChanged();
            return;
        }
    }

    m_status = "No legal engine move";
    Q_EMIT statusChanged();
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
        m_status = "Invalid promotion piece for move";
        Q_EMIT statusChanged();
        return false;
    }

    if (!m_pendingPromotionMoves.empty())
    {
        m_promotionPending = true;
        m_status = "Choose promotion piece";
        Q_EMIT promotionPendingChanged();
        Q_EMIT statusChanged();
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
