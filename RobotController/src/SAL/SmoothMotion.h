#ifndef SMOOTHMOTION_H
#define SMOOTHMOTION_H
#include "StdTypes.h"

class Robot;
class SmoothMotion {
  public:
    SmoothMotion(uint32_t id, Robot* robot);
    void init();
    void setupTarget(uint32_t stepsAccel, uint32_t stepsCruise, uint32_t stepsDecel, 
      int direction, uint8_t moveType, uint32_t accelStartWaitPulse, uint32_t minWaitPulse);
    void motionControlLoop();
    uint8_t pulseLoop();
    void resetPulse();
    void homing();
    void increaseSpeed();
    void cruiseSpeed();
    void decreaseSpeed();
    void resetAccelSteps();
    void resetCruiseSteps();
    void resetDecelSteps();
    void increaseAccelSteps();
    void increaseCruiseSteps();
    void increaseDecelSteps();
    float delayAccel(float stepCount, float delayCur);
    float delayDecel(float stepCount, float delayCur);
    uint32_t getCurrentSteps();
    void changeStateControl(int newState);
    
  public:
    Robot* m_robot;
    uint32_t m_id;
    uint32_t m_minWaitPulse;
    
    float m_numWaitPulse;
    uint32_t m_pulseCount;

    
    uint32_t m_numStepAccel;
    uint32_t m_numStepCruise;
    uint32_t m_numStepDecel;

    uint32_t m_stepCountAccel;
    uint32_t m_stepCountCruise;
    uint32_t m_stepCountDecel;

    int m_targetStep;
    int m_targetDirection;

    uint8_t m_moveType;
    uint8_t m_statePulse;
    uint8_t m_stateControl;
};

#endif // SMOOTHMOTION_H
