#pragma once

#include <QObject>
#include <QStringList>
#include <QSet>
#include <memory>
#include <vector>

#include "Board.hpp"
#include "Move.hpp"

class ChessController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList board READ board NOTIFY boardChanged)
    Q_PROPERTY(int selectedSquare READ selectedSquare NOTIFY selectedSquareChanged)
    Q_PROPERTY(QString sideToMove READ sideToMove NOTIFY sideToMoveChanged)
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(bool promotionPending READ promotionPending NOTIFY promotionPendingChanged)
    Q_PROPERTY(int checkedKingSquare READ checkedKingSquare NOTIFY checkedKingSquareChanged)
    Q_PROPERTY(int engineLevel READ engineLevel NOTIFY engineLevelChanged)
    Q_PROPERTY(int playerColor READ playerColor NOTIFY playerColorChanged)

public:
    explicit ChessController(QObject* parent = nullptr);

    QStringList board() const;
    int selectedSquare() const;
    QString sideToMove() const;
    QString status() const;
    void setStatus(QString status);
    std::vector<Move> moveHistory() const;
    bool promotionPending() const;
    int checkedKingSquare() const;
    int engineLevel() const;
    int playerColor() const;
    QString buildResultText() const;
    Move botMove() const;
    void playEngineMove();
    QString pieceType(QString square);
    QString extractFEN();
    QString processRobotCommentary(const QString fen, const int color,
                                   const QString pieceType, const QString pieceNotation, Move playerMove);
    bool isValidMoveByCoordinates(const QString& startSquare, const QString& stopSquare, Move& chosenMove,
                                           QChar promotionSuffix = QChar());
    bool moveByCoordinates(const QString& startSquare, const QString& stopSquare, Move& chosenMove,
                                           QChar promotionSuffix = QChar());
    int uiIndexToSquare(int uiIndex);
    int squareToUiIndex(int square);
    QString uiIndexToPieceType(int uiIndex);
    QString uiIndexToSquareNotation(int uiIndex);
    std::string coordToNotation(int squareIndex);
    Q_INVOKABLE void newGame(QString lastMove = "");
    Q_INVOKABLE void clickSquare(int uiIndex);
    Q_INVOKABLE bool moveByUiSquares(int startUiIndex, int stopUiIndex, Move& chosenMove);
    Q_INVOKABLE bool moveByUiIndex(int startUiIndex,
                                   int stopUiIndex,
                                   Move& move,
                                   QChar promotionSuffix);
    Q_INVOKABLE QStringList findBestMoveCoordinates() const;
    Q_INVOKABLE bool isValidDestination(int uiIndex) const;
    Q_INVOKABLE void choosePromotion(const QString& pieceLetter);
    Q_INVOKABLE void cancelPromotion();
    Q_INVOKABLE void setEngineLevel(int level);
    Q_INVOKABLE void setPlayerColor(int color);
    Q_INVOKABLE void undoMove();


Q_SIGNALS:
    void boardChanged();
    void selectedSquareChanged();
    void sideToMoveChanged();
    void statusChanged();
    void promotionPendingChanged();
    void checkedKingSquareChanged();
    void engineLevelChanged();
    void playerColorChanged();

private:
    void refreshBoardModel();
    void refreshSelectionMoves();
    void refreshCheckState();
    void clearSelection();
    bool tryFindLegalMove(int originSquare, int destinationSquare, Move& outMove, QChar promotionSuffix = QChar());
    QString pieceCodeAtSquare(int square) const;
    QString convertPieceText(QString pieceShortName);
    bool tryParseCoordinate(const QString& coordinate, int& uiIndex);

private:
    std::shared_ptr<Board> m_board;
    int m_selectedUiSquare;
    QSet<int> m_validDestinationUiSquares;
    QStringList m_boardModel;
    QString m_status;
    std::vector<Move> m_moveHistory;
    bool m_promotionPending;
    int m_checkedKingSquare;
    std::vector<Move> m_pendingPromotionMoves;
    int m_engineDepth;
    int m_playerColor; // 0 = White, 1 = Black
    Move m_botMove;
};
