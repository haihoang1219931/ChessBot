#ifndef ROBOT_H
#define ROBOT_H

#include "StdTypes.h"

class SmoothMotion;
class ApplicationController;
class Robot
{
public:
    Robot(ApplicationController* app);
    void setMotorParam(int motorID, JointParam param);
 
    void loop();
    void setState(ROBOT_STATE newState);
    void initDirection(int motorID, int direction);
    void requestGoHome(int motorID = MAX_MOTOR);
    void executeGohome();
    void requestGoPosition(int motorID, int targetStep, int stepTime, bool isRelativeMove);
    void resetMoveSequene();
    void appendMove(int* jointSteps);
    void moveSequence(int motorID = MAX_MOTOR);
    uint8_t pulseLoop(int motorID);
    void executeMoveSequence();
    void initMove(int motorIDFirst, int motorIDLast);
    void gotoTarget();
    void capture();
    long elapsedTime();
    bool isLimitReached(int motorID,
                            MOTOR_LIMIT_TYPE limitType);
    int angleToStep(int motorID, float angle);
    float stepToAngle(int motorID, int step);
    void currentStep(int* listCurrentStep, int* numMotor);
    void currentAngle(float* listCurrentStep, int* numMotor);
    void armLength(float* listArmLength, int* numMotor);
    int currentDirection(int motorID);
    
    uint8_t statePulse(int motorID);
    uint32_t numWaitPulse(int motorID);
    uint32_t countPulse(int motorID);
    void updateStatePulse(int motorID, uint8_t newState);
    void updateCountPulse(int motorID, uint32_t countPulse);
    void updateNumWaitPulse(int motorID, uint32_t numWaitPulse);
    void updateInitAngle(int motorID, float initAngle);
    float armLength(int motorID);
    int currentStep(int motorID);
    void updateCurrentStep(int motorID);
    int minStep(int motorID);
    int maxStep(int motorID);
    float homeAngle(int motorID);
    int homeStep(int motorID);
    void executeSmoothMotion(int motorID);
    void resetPulse(int motorID);
private:
    ApplicationController* m_app;
    SmoothMotion* m_motorList[MAX_MOTOR];
    JointParam m_motorParamList[MAX_MOTOR];
    ROBOT_STATE m_state;
    ROBOT_SEQUENCE_STATE m_sequenceState;
    Move m_moveSequence[MAX_MOVE_SEQUENCE];
    int m_motorIDFirst;
    int m_motorIDLast;
    int m_curMove;
    int m_numMove;
    int m_numMotor;
    int m_requestMotorID;
    long m_startTime;
    long m_elapsedTime;
};

#endif // ROBOT_H
