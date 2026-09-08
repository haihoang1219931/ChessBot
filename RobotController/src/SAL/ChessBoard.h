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
    void setChessBoardSideSpace(float value);
    Point convertChessBoardPoint(int row, int col);
    void setCalibChessBoardPoint(int row, int col, Point point);
    void setCalibDropZonePoint(int row, int col, ZONE_TYPE zone, Point point);
    void resetCalibrationToFormula();
    Point convertDropPoint(int row, int col, ZONE_TYPE zone);

private:
    float m_chessBoardPosX;
    float m_chessBoardPosY;
    float m_chessBoardSideSpace;
    float m_chessBoardRect;
    float m_dropZoneSpace;
    Point m_cellCalibsChessBoard[8][8];
    Point m_cellCalibsDropZonePlayer[8][2];
    Point m_cellCalibsDropZoneBot[8][2];
};

#endif // CHESSBOARD_H
