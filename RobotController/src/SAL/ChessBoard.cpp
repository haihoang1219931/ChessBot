#include "ChessBoard.h"
#include <string.h>
#include <stdio.h>

ChessBoard::ChessBoard(float x, float y, float rect, float dropZoneSpace):
    m_chessBoardPosX(x),
    m_chessBoardPosY(y),
    m_chessBoardRect(rect),
    m_dropZoneSpace(dropZoneSpace)
{
    memset(m_dropZoneMap,0,16);
    memset(m_dropZoneMapGuest,0,16);
//    m_dropZoneMap[0][0] = 'c';
//    m_dropZoneMap[0][1] = 'c';
//    m_dropZoneMap[1][0] = 'c';
//    m_dropZoneMap[1][1] = 'c';
//    m_dropZoneMap[2][0] = 'c';
//    m_dropZoneMap[2][1] = 'c';

    m_dropZoneMapGuest[0][0] = 'q';
    m_dropZoneMapGuest[1][0] = 'r';
    m_dropZoneMapGuest[2][0] = 'k';
    m_dropZoneMapGuest[3][0] = 'b';
}

float ChessBoard::getChessBoardPosX()
{
    return m_chessBoardPosX;
}

float ChessBoard::getChessBoardPosY()
{
    return m_chessBoardPosY;
}

float ChessBoard::getChessBoardSize()
{
    return m_chessBoardRect*8;
}

void ChessBoard::setChessBoardPosX(float value)
{
    m_chessBoardPosX = value;
}

void ChessBoard::setChessBoardPosY(float value)
{
    m_chessBoardPosY = value;
}

void ChessBoard::setChessBoardSize(float value)
{
    m_chessBoardRect = value/8;
}

void ChessBoard::setDropZoneSpace(float value)
{
    m_dropZoneSpace = value;
}

Point ChessBoard::getFreeDropPoint(ZONE_TYPE zone, uint8_t promotePiece)
{
    Point freePoint;
    bool foundDropPoint = false;
    for(int rowId = 0; rowId < 8; rowId ++){
        for(int colId = 0; colId < 2; colId ++){
            if(zone == ZONE_MACHINE?
                    m_dropZoneMap[rowId][colId] == promotePiece:
                    m_dropZoneMapGuest[rowId][colId] == promotePiece) {
                freePoint = convertDropPoint(rowId,colId, zone);
                foundDropPoint = true;
                break;
            }
        }
        if(foundDropPoint) break;
    }
    return freePoint;
}

Point ChessBoard::convertPoint(int row, int col)
{
    int centerCol = 0;
    int centerRow = 0;
    Point convertValue;
    float squareLength = m_chessBoardRect;
    convertValue.x = (float)(col-centerCol) * squareLength
            + squareLength/2 + m_chessBoardPosX;
    convertValue.y = (float)(row-centerRow) * squareLength
            + squareLength/2 + m_chessBoardPosY;
    return convertValue;
}

Point ChessBoard::convertDropPoint(int row, int col, ZONE_TYPE zone) {
    Point convertValue;
    convertValue.x = zone == ZONE_GUEST?
                m_chessBoardPosX+8*m_chessBoardRect+m_dropZoneSpace+
                    col*m_chessBoardRect + m_chessBoardRect/2:
                m_chessBoardPosX-(m_dropZoneSpace+
                    (1-col)*m_chessBoardRect + m_chessBoardRect/2);
    convertValue.y = m_chessBoardPosY +
                    row * m_chessBoardRect +
                    m_chessBoardRect/2;
    printf("Drop[%s] (%d,%d) = [%.02f,%.02f]\r\n",
           zone == ZONE_GUEST?"Guest":"Machine",
           row,col,
           convertValue.x,convertValue.y);
    return convertValue;
}

void ChessBoard::updateDropZone(uint8_t piece, int row, int col, ZONE_TYPE zone)
{
    if(zone == ZONE_GUEST) {
        m_dropZoneMapGuest[row][col] = piece;
    } else {
        m_dropZoneMap[row][col] = piece;
    }
}

void ChessBoard::moveGuestPieceOut(uint8_t piece) {
    for(int rowId = 0; rowId < 8; rowId ++){
        for(int colId = 0; colId < 8; colId ++){
            if(m_dropZoneMap[rowId][colId] == 0) {
                m_dropZoneMap[rowId][colId] = piece;
                break;
            }
        }
    }
}

void ChessBoard::promotePiece(uint8_t piece) {

}
