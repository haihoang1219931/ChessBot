#include "SimpleChess.h"
// Returns true if player has two queens and opponent only has king
bool isTwoQueenEndgame(const Game &g, pieceColor player) {
    int myQueens = 0, oppKings = 0, oppOther = 0;
    pieceColor opp = (player == pieceColor::WHITE) ? pieceColor::BLACK : pieceColor::WHITE;
    for (unsigned i = 0; i < 8; ++i) {
        for (unsigned j = 0; j < 8; ++j) {
            Piece* p = g.getPiece(i, j);
            if (p->color == player && p->name == pieceName::QUEEN) myQueens++;
            if (p->color == opp) {
                if (p->name == pieceName::KING) oppKings++;
                else if (p->name != pieceName::EMPTY && p->name != pieceName::PAWN) oppOther++;
            }
        }
    }
    // Allow any number of pawns for both sides, but only two queens for player and only king (and pawns) for opponent
    return (myQueens == 2 && oppKings == 1 && oppOther == 0);
}

// Evaluation for two-queen endgame: reward restricting opponent king
int twoQueenEndgameEvaluate(const Game &g, pieceColor player) {
    pieceColor opp = (player == pieceColor::WHITE) ? pieceColor::BLACK : pieceColor::WHITE;
    int score = 0;
    int kingX = -1, kingY = -1;
    // Find opponent king
    for (unsigned i = 0; i < 8; ++i) {
        for (unsigned j = 0; j < 8; ++j) {
            Piece* p = g.getPiece(i, j);
            if (p->color == opp && p->name == pieceName::KING) {
                kingX = i; kingY = j;
            }
        }
    }
    // Reward king being on edge/corner
    if (kingX == 0 || kingX == 7) score += 5;
    if (kingY == 0 || kingY == 7) score += 5;
    if ((kingX == 0 || kingX == 7) && (kingY == 0 || kingY == 7)) score += 10;
    // Penalize stalemate (no legal moves for opponent)
    bool oppHasMoves = false;
    for (unsigned i = 0; i < 8; ++i) {
        for (unsigned j = 0; j < 8; ++j) {
            Piece* p = g.getPiece(i, j);
            if (p->color == opp && !p->legalMoves.empty()) oppHasMoves = true;
        }
    }
    if (!oppHasMoves) score -= 1000;
    return score;
}

#include "ChessAlgo.h"
#include <limits>

// Helper: returns true if only kings and pawns or very low material remain
bool isEndgame(const Game &g) {
    int minorMajorCount = 0;
    int pawnCount = 0;
    for (unsigned i = 0; i < 8; ++i) {
        for (unsigned j = 0; j < 8; ++j) {
            Piece* p = g.getPiece(i, j);
            if (p->name == pieceName::PAWN) pawnCount++;
            else if (p->name != pieceName::EMPTY && p->name != pieceName::KING) minorMajorCount++;
        }
    }
    // Endgame if no queens and <=1 minor/major piece per side, or only pawns and kings
    return minorMajorCount <= 2;
}

// Endgame evaluation: reward king activity and pawn advancement
int endgameEvaluate(const Game &g, pieceColor player) {
    int score = 0;
    for (unsigned i = 0; i < 8; ++i) {
        for (unsigned j = 0; j < 8; ++j) {
            Piece* p = g.getPiece(i, j);
            if (p->color == player) {
                if (p->name == pieceName::KING) {
                    // Reward king centralization
                    int distCenter = abs(3 - (int)i) + abs(3 - (int)j);
                    score -= distCenter;
                } else if (p->name == pieceName::PAWN) {
                    // Reward pawn advancement
                    score += (player == pieceColor::WHITE) ? (int)i : (7 - (int)i);
                }
            }
        }
    }
    return score;
}

Move findBestMove(Game &g) {
    pieceColor player = g.currentPlayer();
    int bestScore = (player == pieceColor::WHITE) ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    Move bestMove;
    bool foundMove = false;
    bool endgame = isEndgame(g);
    bool twoQueen = isTwoQueenEndgame(g, player);
    printf("Finding best move for %s (endgame: %d, two-queen endgame: %d)\n", (player == pieceColor::WHITE) ? "WHITE" : "BLACK", endgame, twoQueen);
    for (unsigned i = 0; i < 8; ++i) {
        for (unsigned j = 0; j < 8; ++j) {
            Piece* p = g.getPiece(i, j);
            if (p->color == player && !p->legalMoves.empty()) {
                for (const Move& m : p->legalMoves) {
                    Game gCopy = g;
                    gCopy.move(m);
                    int score = 0;
                    if (twoQueen) score = twoQueenEndgameEvaluate(gCopy, player);
                    else if (endgame) score = endgameEvaluate(gCopy, player);
                    else score = gCopy.evaluateBoard();
                    if ((player == pieceColor::WHITE && score > bestScore) ||
                        (player == pieceColor::BLACK && score < bestScore) ||
                        !foundMove) {
                        bestScore = score;
                        bestMove = m;
                        foundMove = true;
                    }
                }
            }
        }
    }
    return bestMove;
}
