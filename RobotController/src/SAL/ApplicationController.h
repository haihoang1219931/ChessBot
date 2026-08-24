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
    void goToHomeToCalibPosition();
    int executeReadyPositionLoop();
    int executeHomeToCalibPositionLoop();
    void goToCalibPosition();
    void gotoPosition(float x, float y, float upAngleInDegree);
    bool executeSequence(MOVE_TYPE moveType,
                         uint8_t startRow, uint8_t startCol, uint8_t stopRow, uint8_t stopCol, bool straightMove = false,
                         uint8_t dropCaptureSide = 255, uint8_t dropCaptureRow = 255, uint8_t dropCaptureCol = 255,
                         uint8_t promoteSide = 255, uint8_t promoteRow = 255, uint8_t promoteCol = 255,
                         uint8_t dropPawnPromoteSide = 255, uint8_t dropPawnToPromoteRow = 255, uint8_t dropPawnToPromoteCol = 255);
    void sendCalibrationProgress();
    bool calculateSequenceMoveStraight(uint8_t startRow, uint8_t startCol, uint8_t stopRow, uint8_t stopCol);
    bool calculateSequenceMove(uint8_t startRow, uint8_t startCol, int upAngleInDegree, bool isCapture);
    bool calculateSequenceMoveTest(uint8_t targetRow, uint8_t targetCol);
    bool calculateSequenceMoveNormal(uint8_t startRow, uint8_t startCol,
                         uint8_t stopRow, uint8_t stopCol, bool straightMove);
    bool calculateSequenceAttack(uint8_t startRow, uint8_t startCol,
                         uint8_t stopRow, uint8_t stopCol,
                         bool straightMove,
                         uint8_t dropCaptureSide, uint8_t dropCaptureRow, uint8_t dropCaptureCol);
    bool calculateSequencePastPawn(uint8_t startRow, uint8_t startCol,
                         uint8_t stopRow, uint8_t stopCol, bool straightMove,
                         uint8_t dropCaptureSide, uint8_t dropCaptureRow, uint8_t dropCaptureCol);
    bool calculateSequencePromotePiece(uint8_t startRow, uint8_t startCol,
                         uint8_t stopRow, uint8_t stopCol,
                         uint8_t promoteSide, uint8_t promoteRow, uint8_t promoteCol,
                         uint8_t dropPawnPromoteSide, uint8_t dropPawnToPromoteRow, uint8_t dropPawnToPromoteCol,
                         uint8_t dropCaptureSide = 255, uint8_t dropCaptureRow = 255, uint8_t dropCaptureCol = 255);
    bool calculateSequenceCastle(uint8_t kingRow, uint8_t kingCol,
                                 uint8_t rookRow, uint8_t rookCol, bool straightMove);
    void calculatePolygonEdgeA2345(float upAngleInDegree, float* edge, float* angleA2A2345);
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
    virtual void specificPlatformGohome(int motorID = MAX_MOTOR, bool stopOtherStepper = true) = 0;
    virtual void hardwareStop(int motorID = MAX_MOTOR) = 0;
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
    Point m_promotePiecePoint;
    Point m_dropCapturePoint;
    Point m_dropPieceBotPoint;
    Point m_startPoint;
    Point m_stopPoint;
    Point m_pawnPoint;
    uint8_t m_standByCommandState;
    uint8_t m_homeCalibCommandState;
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
