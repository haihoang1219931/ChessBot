#ifndef APPLICATIONCONTROLLER_H
#define APPLICATIONCONTROLLER_H

#include "StdTypes.h"

class Button;
class Robot;
class ChessBoard;

class ApplicationController
{
public:
    explicit ApplicationController();
    virtual ~ApplicationController();
    
    void loop();
    void readCommand();
    void storeButtonState(int btnID, bool pressed);
    void updateInputState();
    MACHINE_STATE stateMachine();
    void getCurrentPosition(float* listCurrentStep, int* numMotor);
    void getCurrentArmLength(float* listArmLength, int* numArm);
    void getChessBoardParams(float* listParam, int* numParam);
    void executeCommand(char* command);
    void executeSingleMotor(int motorID,
                         int targetStep,
                         int direction,
                         int stepTime);
    void setMachineState(MACHINE_STATE machineState);
    bool inverseKinematic(float x, float y,
                           float a1, float a2,float* p1, float* p2);
    void forwardKinematic(float a1, float a2, float p1, float p2, float* x, float* y);
    int executeCommandSequenceLoop();
    int executeCommandLoop();
    int executeCommandNormal();
    int executeCommandLine();
    void goToHome(int motorID);
    void calibToHome(int motorID);
    void goToReadyPosition();
    void goToCalibPosition();
    void gotoPosition(float x, float y, float upAngleInDegree);
    void executeSequence(MOVE_TYPE moveType,
                         int startCol, int startRow,
                         int stopCol, int stopRow,
                         char promotePiece = 0);
    void sendCalibrationProgress();
    void calculateSequenceMoveStraight(int startCol, int startRow,int stopCol, int stopRow);
    void calculateSequenceMove(int startCol, int startRow, int upAngleInDegree, bool isCapture);
    void calculateSequenceMoveNormal(int startCol, int startRow,
                         int stopCol, int stopRow);
    void calculateSequenceAttack(int startCol, int startRow,
                         int stopCol, int stopRow);
    void calculateSequencePastPawn(int startCol, int startRow,
                         int stopCol, int stopRow);
    void calculateSequencePromotePiece(int startCol, int startRow,
                         int stopCol, int stopRow, char promotePiece);
    void calculateSequenceCastle(int kingCol, int kingRow,
                                 int rookCol, int rookRow);
    void calculatePolygonEdge(float upAngleInDegree, float* edge, float* angle);
    void calculateJoints(float xPos, float yPos, float upAngleInDegree, int* jointSteps);
    Point calibPos();
    Command calculateNextPointInLine(Point currPos, Point targetPos, float numPointInCommand);
    Point currentPos();
    float distance(float x1, float y1, float x2, float y2);
    void clearSequenceMove();
    void appendSequenceMove(Point start, Point stop, bool straightMove = false);
    void initSequenceMove(int numberOfJoints);
    void executeSmoothMotionLoop(int motorID);
    virtual void initRobot() = 0;
    virtual void specificPlatformGohome(int motorID = MAX_MOTOR) = 0;
    virtual void harwareStop(int motorID = MAX_MOTOR) = 0;
    virtual void checkInput() = 0;
    virtual int printf(const char *fmt, ...) = 0;
    virtual void msleep(int millis) = 0;
    virtual long getSystemTime() = 0;
    virtual void enableEngine(bool enable) = 0;
    virtual bool isLimitReached(int motorID,
                        MOTOR_LIMIT_TYPE limitType) = 0;
    virtual int readSerial(char* output, int length) = 0;
    virtual void initDirection(int motorID, int direction) = 0;
    virtual void moveDoneAction(int motorID) = 0;
    virtual uint8_t executePulseLoop(int motorID) = 0;
    virtual void enableHardwareTimer(bool enable) = 0;
    virtual void resetPulse(int motorID) = 0;

public:
    MACHINE_STATE m_machineState;
    Button* m_buttonList[MAX_BUTTON];
    Robot* m_robot;
    ChessBoard* m_chessBoard;
    Command m_sequenceCommand[MAX_MOVE_SEQUENCE];
    Command m_nextPoint;
    char m_commandRead[64];
    uint8_t m_numCommand;
    uint8_t m_curCommandId;
    uint8_t m_commandState;
    uint8_t m_commandSequenceState;
    uint8_t m_comCommandID = 0;
    uint32_t m_curPointInCommand;
    uint32_t m_numPointInCommand;
    Point m_curPos;
    Point m_tarPos;
    int m_calibrationReceivedCount;
    int m_calibrationChessboardCount;
    int m_calibrationRightDropzoneCount;
    int m_calibrationLeftDropzoneCount;
    bool m_calibrationChessboardCalibrated[8][8];
    bool m_calibrationRightDropzoneCalibrated[8][2];
    bool m_calibrationLeftDropzoneCalibrated[8][2];
    bool m_calibrationInProgress;
    int m_appTimer;
    float m_minSpace;
    bool m_engineEnabled;
};

#endif // APPLICATIONCONTROLLER_H
