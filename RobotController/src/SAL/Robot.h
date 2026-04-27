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
 
    int loop();
    void setState(ROBOT_STATE newState);
    void initDirection(int motorID, int direction);
    void requestCalib(int motorID = MAX_MOTOR);
    int executeCalib();
    void requestGoHome(int motorID = MAX_MOTOR);    
    int executeGohome();
    void requestGoPosition(int motorID, int targetStep, int stepTime, bool isRelativeMove);
    void setMoveTarget(int* jointSteps);
    void moveToTarget(int motorID = MAX_MOTOR);
    uint8_t pulseLoop(int motorID);
    int executeMoveSequence();
    void initMove(int motorIDFirst, int motorIDLast);
    int gotoTarget();
    int capture();
    // long elapsedTime();
    bool isLimitReached(int motorID,
                            MOTOR_LIMIT_TYPE limitType);
    int angleToStep(int motorID, float angle);
    float stepToAngle(int motorID, int step, int angleType = ANGLE_DEGREE);
    void currentStep(int* listCurrentStep, int* numMotor);
    void currentAngle(float* listCurrentAngle, int* numMotor, int angleType = ANGLE_DEGREE);
    void calibAngle(float* listCalibAngle, int* numMotor, int angleType = ANGLE_DEGREE);
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
    int calibStep(int motorID);
    void updateCurrentStep(int motorID);
    int minStep(int motorID);
    int maxStep(int motorID);
    float homeAngle(int motorID);
    int homeStep(int motorID);
    void executeSmoothMotion(int motorID);
    void resetPulse(int motorID);
    float delayDecel(float stepCount, float delayCur);
    void calculateTotalTime(int numStepAccel, int numStepTotal, float minsleep, float homeStepTime,
                            float* totalDelay, float* startDelay);

public:
    ApplicationController* m_app;
    SmoothMotion* m_motorList[MAX_MOTOR];
    JointParam m_motorParamList[MAX_MOTOR];
    float m_timeDelay[MAX_MOTOR];
    float m_startDelay[MAX_MOTOR];
    ROBOT_STATE m_state;
    ROBOT_SEQUENCE_STATE m_sequenceState;
    Move m_moveTarget;
    uint8_t m_motorIDFirst;
    uint8_t m_motorIDLast;
    uint8_t m_numMotor;
    uint8_t m_requestMotorID;
    // long m_startTime;
    // long m_elapsedTime;
};

#endif // ROBOT_H
