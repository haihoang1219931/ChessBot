#pragma once

#include <vector>
#include <cassert>
#include <iostream>
#include <random>
#include <array>
#include <limits>


class Game;
enum class gameState { PLAYING, WON_WHITE, WON_BLACK, DRAW };
enum class pieceName { EMPTY, PAWN, ROOK, BISHOP, KNIGHT, KING, QUEEN };
enum class pieceColor { EMPTY, WHITE, BLACK };

struct Move {
	Move(unsigned f_x=0, unsigned f_y=0, unsigned t_x=0, unsigned t_y=0, bool c=false, bool e=false, pieceName p = pieceName::EMPTY)
		: from_x(f_x), from_y(f_y), to_x(t_x), to_y(t_y), castles(c), enpassant(e), promotePiece(p) {}
	unsigned from_x, from_y, to_x, to_y;
	bool castles, enpassant;
	pieceName promotePiece;
};

struct Piece {
	Piece(pieceName n = pieceName::EMPTY, pieceColor c = pieceColor::EMPTY) : name(n), color(c), moved(false) {}
	Piece(const Piece* p) : name(p->name), color(p->color), moved(p->moved) {}
	void print() {
		switch(name) {
			case pieceName::EMPTY:	std::cout << "."; break;
            case pieceName::PAWN:	(color != pieceColor::WHITE) ? std::cout << "P" : std::cout << "p";	break;
            case pieceName::ROOK:	(color != pieceColor::WHITE) ? std::cout << "R" : std::cout << "r";	break;
            case pieceName::BISHOP:	(color != pieceColor::WHITE) ? std::cout << "B" : std::cout << "b";	break;
            case pieceName::KNIGHT:	(color != pieceColor::WHITE) ? std::cout << "N" : std::cout << "n";	break;
            case pieceName::KING:	(color != pieceColor::WHITE) ? std::cout << "K" : std::cout << "k";	break;
            case pieceName::QUEEN:	(color != pieceColor::WHITE) ? std::cout << "Q" : std::cout << "q";	break;
		}
	}
    int value() {
        int pieceValue = 0;
        switch(name) {
            case pieceName::EMPTY:	; break;
            case pieceName::PAWN:	pieceValue = (color == pieceColor::WHITE) ? 8 : 14;	break;
            case pieceName::ROOK:	pieceValue = (color == pieceColor::WHITE) ? 5 : 11;	break;
            case pieceName::BISHOP:	pieceValue = (color == pieceColor::WHITE) ? 6 : 12;	break;
            case pieceName::KNIGHT:	pieceValue = (color == pieceColor::WHITE) ? 7 : 13;	break;
            case pieceName::KING:	pieceValue = (color == pieceColor::WHITE) ? 3 :  9;	break;
            case pieceName::QUEEN:	pieceValue = (color == pieceColor::WHITE) ? 4 : 10;	break;
        }
        return pieceValue;
    }
	pieceName name;
	pieceColor color;

	bool moved;
	std::vector<Move> legalMoves;
};

struct Board {
	Board();
	Board(const Board &b);
	bool equals(const Game &g) const;
	Piece* board[8][8];
	bool inCheck;
};

class Game {
	public:
		Game();
		Game(const Game &g);
		void move(const Move &m);
		Piece* getPiece(const unsigned &x, const unsigned &y) const;
		
		pieceColor currentPlayer() const;
		pieceColor nextPlayer() const;
		gameState state;
		Game* previous;
		void undo();
        void setTurn(int _turn);

        void resetGame();
		Board position;
		unsigned fiftyMoveRule;

		bool isInCheck() const;
		void movePieceTo(const Move m);
		void checkIfEndPosition();
		void calculateAllPossibleMoves(const pieceColor &player) const;
		bool preventsCheck(const Move &m) const;
		std::vector<Move> getLegalMoves(const unsigned &x, const unsigned &y) const;
		std::vector<Move> getPossibleMoves(const unsigned &x, const unsigned &y) const;
        int evaluateBoard() const;
        unsigned turn;
};
