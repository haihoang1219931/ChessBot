#include "ChessBoard.h"
#include <string.h>
#include <stdio.h>

ChessBoard::ChessBoard(float x, float y, float rect, float dropZoneSpace):
    m_chessBoardPosX(x),
    m_chessBoardPosY(y),
    m_chessBoardRect(rect),
    m_dropZoneSpace(dropZoneSpace)
{
    resetDropZoneMap();

    for(int rowId = 0; rowId < 8; rowId ++){
        for(int colId = 0; colId < 2; colId ++){
            m_cellCalibsDropZonePlayer[rowId][colId].calibbed = false;
            m_cellCalibsDropZoneBot[rowId][colId].calibbed = false;
        }
    }
    for(int rowId = 0; rowId < 8; rowId ++){
        for(int colId = 0; colId < 8; colId ++){
            m_cellCalibsChessBoard[rowId][colId].calibbed = false;
        }
    }
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

void ChessBoard::setChessBoardSideSpace(float value)
{
    m_chessBoardSideSpace = value;
}

DropPoint ChessBoard::getFreeDropPoint(ZONE_TYPE zone, uint8_t promotePiece)
{
    DropPoint freePoint;
    bool foundDropPoint = false;
    for(int rowId = 0; rowId < 8; rowId ++){
        for(int colId = 0; colId < 2; colId ++){
            if(zone == ZONE_PLAYER?
                    m_dropZoneMapPlayer[rowId][colId] == promotePiece:
                    m_dropZoneMapBot[rowId][colId] == 0) {
                freePoint.location = convertDropPoint(rowId,colId, zone);
                freePoint.rowID = rowId;
                freePoint.colID = colId;
                freePoint.zoneType = zone;
                freePoint.valid = true;
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
    if(!m_cellCalibsChessBoard[row][col].calibbed) {
        float squareLength = m_chessBoardRect;
        convertValue.x = -(float)(col-centerCol) * squareLength
                - squareLength/2 + m_chessBoardPosX;
        convertValue.y = (float)(row-centerRow) * squareLength
                + squareLength/2 + m_chessBoardPosY;
        if(row >= 4) {
            convertValue.y += m_chessBoardSideSpace;
        }
    } else {
        convertValue = m_cellCalibsChessBoard[row][col];
    }
    
    return convertValue;
}

Point ChessBoard::convertDropPoint(int row, int col, ZONE_TYPE zone) {
    Point convertValue;
    if(!(zone == ZONE_PLAYER?
                m_cellCalibsDropZonePlayer[row][col].calibbed:
                m_cellCalibsDropZoneBot[row][col].calibbed)) {
        convertValue.x = zone == ZONE_PLAYER?
                m_chessBoardPosX - 8*m_chessBoardRect - m_dropZoneSpace
                    - (float)col*m_chessBoardRect - m_chessBoardRect/2:
                m_chessBoardPosX + 2*m_chessBoardRect + m_dropZoneSpace
                    - (float)col*m_chessBoardRect - m_chessBoardRect/2;
        convertValue.y = m_chessBoardPosY +
                    (float)row * m_chessBoardRect +
                    m_chessBoardRect/2;
    } else {
        convertValue = zone == ZONE_PLAYER?
                m_cellCalibsDropZonePlayer[row][col]:
                m_cellCalibsDropZoneBot[row][col];
    }
#ifdef DEBUG_COMMAND
    printf("Drop[%s] (%d,%d) = [%d,%d]\r\n",
           zone == ZONE_PLAYER?"Player":"Bot",
           row,col,
           (int)convertValue.x,(int)convertValue.y);
#endif
           return convertValue;
}

void ChessBoard::updateDropZone(uint8_t piece, int row, int col, ZONE_TYPE zone)
{
    if(zone == ZONE_PLAYER) {
        m_dropZoneMapPlayer[row][col] = piece;
    } else {
        m_dropZoneMapBot[row][col] = piece;
    }
}

void ChessBoard::resetDropZoneMap()
{
    for(int rowId = 0; rowId < 8; rowId++) {
        for(int colId = 0; colId < 2; colId++) {
            m_dropZoneMapPlayer[rowId][colId] = 0;
            m_dropZoneMapBot[rowId][colId] = 0;
        }
    }
    m_dropZoneMapPlayer[0][0] = 'q';
    m_dropZoneMapPlayer[1][0] = 'r';
    m_dropZoneMapPlayer[2][0] = 'n';
    m_dropZoneMapPlayer[3][0] = 'b';

    m_dropZoneMapBot[4][0] = 'q';
    m_dropZoneMapBot[5][0] = 'r';
    m_dropZoneMapBot[6][0] = 'n';
    m_dropZoneMapBot[7][0] = 'b';
}

void ChessBoard::moveGuestPieceOut(uint8_t piece) {
    for(int rowId = 0; rowId < 8; rowId ++){
        for(int colId = 0; colId < 8; colId ++){
            if(m_dropZoneMapPlayer[rowId][colId] == 0) {
                m_dropZoneMapPlayer[rowId][colId] = piece;
                break;
            }
        }
    }
}

void ChessBoard::promotePiece(uint8_t piece) {

}

void ChessBoard::setCalibChessBoardPoint(int row, int col, Point point) {
    m_cellCalibsChessBoard[row][col].x = point.x;
    m_cellCalibsChessBoard[row][col].y = point.y;
    m_cellCalibsChessBoard[row][col].z = point.z;
    m_cellCalibsChessBoard[row][col].calibbed = true;
}

void ChessBoard::setCalibDropZonePoint(int row, int col, ZONE_TYPE zone, Point point) {
    if(zone == ZONE_BOT) {
        m_cellCalibsDropZoneBot[row][col].x = point.x;
        m_cellCalibsDropZoneBot[row][col].y = point.y;
        m_cellCalibsDropZoneBot[row][col].z = point.z;
        m_cellCalibsDropZoneBot[row][col].calibbed = true;
    } else {
        m_cellCalibsDropZonePlayer[row][col].x = point.x;
        m_cellCalibsDropZonePlayer[row][col].y = point.y;
        m_cellCalibsDropZonePlayer[row][col].z = point.z;
        m_cellCalibsDropZonePlayer[row][col].calibbed = true;
    }
}

void ChessBoard::resetCalibrationToFormula() {
    // Reset all chessboard calibration points to uncalibrated (use formula)
    for(int rowId = 0; rowId < 8; rowId++) {
        for(int colId = 0; colId < 8; colId++) {
            m_cellCalibsChessBoard[rowId][colId].calibbed = false;
        }
    }
    
    // Reset all dropzone calibration points to uncalibrated (use formula)
    for(int rowId = 0; rowId < 8; rowId++) {
        for(int colId = 0; colId < 2; colId++) {
            m_cellCalibsDropZonePlayer[rowId][colId].calibbed = false;
            m_cellCalibsDropZoneBot[rowId][colId].calibbed = false;
        }
    }
}
