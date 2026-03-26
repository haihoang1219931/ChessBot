#ifndef CHESS_ALGO_H
#define CHESS_ALGO_H

#include "SimpleChess.h"


// Finds the best move for the current player, using endgame strategy if appropriate
Move findBestMove(Game &g);

// Returns true if the game is in an endgame state (e.g., low material)
bool isEndgame(const Game &g);

#endif // CHESS_ALGO_H
