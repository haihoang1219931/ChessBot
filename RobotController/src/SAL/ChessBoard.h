#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include "StdTypes.h"
class ChessBoard
{
public:
    ChessBoard(float x = 0, float y = 0, float rect = 0, float dropZoneSpace = 0);
    float getChessBoardPosX();
    float getChessBoardPosY();
    float getChessBoardSize();
    void setChessBoardPosX(float value);
    void setChessBoardPosY(float value);
    void setChessBoardSize(float value);
    void setDropZoneSpace(float value);
    Point getFreeDropPoint(ZONE_TYPE zone, uint8_t promote = 0);
    Point convertPoint(int row, int col);
    void updateDropZone(uint8_t piece, int row, int col, ZONE_TYPE zone);
    void moveGuestPieceOut(uint8_t piece);
    void promotePiece(uint8_t piece);

private:
    Point convertDropPoint(int row, int col, ZONE_TYPE zone);

private:
    float m_chessBoardPosX;
    float m_chessBoardPosY;
    float m_chessBoardRect;
    float m_dropZoneSpace;

    uint8_t m_dropZoneMap[8][2];
    uint8_t m_dropZoneMapGuest[8][2];

};

#endif // CHESSBOARD_H
