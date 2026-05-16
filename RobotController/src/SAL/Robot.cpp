#include "Robot.h"
#include "ApplicationController.h"
#include "SmoothMotion.h"
#include <math.h>
#include <string.h>
#ifdef abs
#undef abs
#endif

#define abs(x) ((x)>0?(x):-(x))

Robot::Robot(ApplicationController* app) :
    m_app(app),
    m_state(ROBOT_INIT)
{
    for(uint32_t motorID=MOTOR_CAPTURE; motorID < MAX_MOTOR; motorID++) {
        m_motorList[motorID] = new SmoothMotion(motorID,this);
    }
}

void Robot::setMotorParam(int motorID, JointParam param)
{
    m_motorParamList[motorID] = param;
#ifdef DEBUG_COMMAND
    printf("Robot::setMotorParam[%d] param minPulsePerStep[%d] to Robot[%d]\r\n",
           motorID,param.minPulsePerStep,
           m_motorParamList[motorID].minPulsePerStep);
#endif
}

void Robot::setState(ROBOT_STATE newState) {
    if(m_state != newState) {
        m_state = newState;
#ifdef DEBUG_COMMAND
        m_app->printf("ROBOT STATE: %d\r\n",m_state);
#endif
    }
}

int Robot::loop() {
    // m_elapsedTime = m_app->getSystemTime() - m_startTime;
#ifdef DEBUG_ROBOT
    m_app->printf("Robot m_state: %d\r\n", m_state);
#endif
    switch(m_state) {
    case ROBOT_EXECUTE_CALIBRATION: {
        if (executeCalib() == ROBOT_MOVE_DONE)
        {
            m_state = ROBOT_STOP_MOTORS;
        }
    }
        break;
    case ROBOT_EXECUTE_GO_HOME: {
        if (executeGohome() == ROBOT_MOVE_DONE)
        {
            m_state = ROBOT_STOP_MOTORS;
        }
    }
        break;
    case ROBOT_EXECUTE_SEQUENCE: {
        if (executeMoveSequence() == ROBOT_MOVE_DONE)
        {
            m_state = ROBOT_STOP_MOTORS;
        }
    }
        break;
    case ROBOT_STOP_MOTORS: {
        m_app->hardwareStop(MAX_MOTOR);
        m_state = ROBOT_EXECUTE_DONE;
    }
        break;
    case ROBOT_EXECUTE_DONE: {
    }
        break;
    }
    return m_state;
}

void Robot::initDirection(int motorID, int direction)
{
    m_motorParamList[motorID].direction = direction;
    m_app->initDirection(motorID, direction);
}

void Robot::requestCalib(int motorID) {
#ifdef DEBUG_COMMAND
    m_app->printf("CALIBRATION\r\n");
#endif
    m_app->enableEngine(true);
    m_app->enableHardwareTimer(false);
    m_requestMotorID = motorID;
    int startID = motorID == MAX_MOTOR ? 0 : motorID;
    int stopID = motorID == MAX_MOTOR ? MAX_MOTOR-1 : motorID;
#ifdef DEBUG_COMMAND
    m_app->printf("Request calib from [%d-%d]\r\n",startID,stopID);
#endif
    for(int motor=startID; motor<= stopID; motor++)
    {
        m_motorParamList[motor].currentStep = 0;
        if(m_motorParamList[motor].active) {
            m_motorList[motor]->setupTarget(
                0,0xFFFF,0,
                -1,
                MOTOR_EXECUTE_HOME,
                m_motorParamList[motor].homeStepTime, 0);
        }
    }
    // m_startTime = m_app->getSystemTime();
    setState(ROBOT_EXECUTE_CALIBRATION);
#ifdef DEBUG_COMMAND
    m_app->printf("Request calib from [%d-%d] done\r\n",startID,stopID);
#endif
    m_app->enableHardwareTimer(true);
}

int Robot::executeCalib() {
    bool allMotorsAtHome = true;
#ifdef DEBUG_ROBOT
    m_app->printf("Robot executeCalib M[%d]\r\n",m_requestMotorID);
#endif
    int startID = m_requestMotorID == MAX_MOTOR ?
                                            MOTOR_CAPTURE : m_requestMotorID;
    int stopID = m_requestMotorID == MAX_MOTOR ?
                                            MAX_MOTOR : m_requestMotorID+1;
    for(int motor = startID; motor< stopID; motor++)
    {
        if(!m_motorParamList[motor].active) continue;
        if(!m_app->isLimitReached(motor, MOTOR_LIMIT_HOME))
        {
#ifdef DEBUG_ROBOT
            m_app->printf("Robot M[%d] P[%d] T[%d]\r\n",
                        motor,
                        m_motorParamList[motor].currentStep,
                        m_motorParamList[motor].targetStep);
            m_app->printf("Robot M[%d] calib\r\n",motor);
#endif
            allMotorsAtHome = false;
        }
    }
    if(allMotorsAtHome) {
        for(int motor = startID; motor< stopID; motor++) {
            if(!m_motorParamList[motor].active) continue;
            int homeStep = angleToStep(motor, m_motorParamList[motor].homeAngle);
            m_motorParamList[motor].calibStep = -m_motorParamList[motor].currentStep + homeStep;
            m_motorParamList[motor].currentStep = homeStep;
#ifdef DEBUG_COMMAND
            m_app->printf("Calib is homed M[%d] step[%d]\r\n",
                      motor,m_motorParamList[motor].calibStep);
#endif
        }
    }
    return allMotorsAtHome ? ROBOT_MOVE_DONE : m_state;
}

void Robot::requestGoHome(int motorID) {
#ifdef DEBUG_COMMAND
    m_app->printf("GO HOME\r\n");
#endif
    m_app->enableHardwareTimer(false);
    m_app->enableEngine(true);
    m_requestMotorID = motorID;
    int startID = motorID == MAX_MOTOR ? MOTOR_ARM1 : motorID;
    int stopID = motorID == MAX_MOTOR ? MAX_MOTOR-1 : motorID;
#ifdef DEBUG_COMMAND
    m_app->printf("Request go home from [%d-%d]\r\n",startID,stopID);
#endif
    for(int motor=startID; motor<= stopID; motor++)
    {
        if(m_motorParamList[motor].active) {
            m_motorList[motor]->setupTarget(
                0,0xFFFF,0,
                -1,
                MOTOR_EXECUTE_HOME,
                m_motorParamList[motor].homeStepTime, 0);
        }
    }
    // m_startTime = m_app->getSystemTime();
    setState(ROBOT_EXECUTE_GO_HOME);
#ifdef DEBUG_COMMAND
    m_app->printf("Request go home from [%d-%d] done\r\n",startID,stopID);
#endif
    m_app->enableHardwareTimer(true);
}

int Robot::executeGohome() {
    bool allMotorsAtHome = true;
#ifdef DEBUG_ROBOT
    m_app->printf("Robot executeGohome M[%d]\r\n",m_requestMotorID);
#endif
    int startID = m_requestMotorID == MAX_MOTOR ?
                                            MOTOR_CAPTURE : m_requestMotorID;
    int stopID = m_requestMotorID == MAX_MOTOR ?
                                            MAX_MOTOR : m_requestMotorID+1;
    for(int motor = startID; motor< stopID; motor++)
    {
        if(!m_motorParamList[motor].active) continue;
        if(!m_app->isLimitReached(motor, MOTOR_LIMIT_HOME))
        {
#ifdef DEBUG_ROBOT
            m_app->printf("Robot M[%d] Time[%d] P[%d] T[%d]\r\n",
                        motor,m_elapsedTime,
                        m_motorParamList[motor].currentStep,
                        m_motorParamList[motor].targetStep);
            m_app->printf("Robot M[%d] gohome\r\n",motor);
#endif
            allMotorsAtHome = false;
        } else {
            m_motorParamList[motor].currentStep = angleToStep(motor, m_motorParamList[motor].homeAngle);
        }
    }
    if(allMotorsAtHome) {
        for(int motor = startID; motor< stopID; motor++) {
            if(!m_motorParamList[motor].active) continue;
#ifdef DEBUG_COMMAND
            m_app->printf("Homed M[%d] step[%d]\r\n",
                      motor,m_motorParamList[motor].currentStep);
#endif
        }
    }
    return allMotorsAtHome ? ROBOT_MOVE_DONE : m_state;
}

void Robot::requestGoPosition(int motorID, int targetStep, int stepTime, bool isRelativeMove)
{
    m_requestMotorID = motorID;
    m_motorParamList[motorID].targetStep = targetStep;
    m_motorParamList[motorID].homeStepTime = stepTime;
    m_motorParamList[motorID].direction = (targetStep > m_motorParamList[motorID].currentStep) ? 1 : -1;
    // m_startTime = m_app->getSystemTime();
    setState(ROBOT_EXECUTE_POSITION);
}

// long Robot::elapsedTime()
// {
//     return m_elapsedTime;
// }

int Robot::angleToStep(int motorID, float angle)
{
    return (int)(angle * m_motorParamList[motorID].scale);
}

bool Robot::isLimitReached(int motorID,
                        MOTOR_LIMIT_TYPE limitType)
{
    return m_app->isLimitReached(motorID, limitType);
}

float Robot::stepToAngle(int motorID, int step, int angleType)
{
    return (float)step / m_motorParamList[motorID].scale *
            (angleType == ANGLE_DEGREE ? 1.0f : M_PI/180.0f);
}
void Robot::currentStep(int* listCurrentStep, int* numMotor)
{
    *numMotor = MAX_MOTOR;
    for(int i=MOTOR_CAPTURE; i< MAX_MOTOR; i++)
    {
        listCurrentStep[i] = m_motorParamList[i].currentStep;
    }
}

void Robot::currentAngle(float* listCurrentAngle, int* numMotor, int angleType)
{
    *numMotor = MAX_MOTOR;
    for(int i=MOTOR_CAPTURE; i< MAX_MOTOR; i++)
    {
        listCurrentAngle[i] = stepToAngle(i, m_motorParamList[i].currentStep,angleType);
    }
}

void Robot::calibAngle(float* listCalibAngle, int* numMotor, int angleType)
{
    *numMotor = MAX_MOTOR;
    for(int i=MOTOR_CAPTURE; i< MAX_MOTOR; i++)
    {
        listCalibAngle[i] = stepToAngle(i,
            m_motorParamList[i].calibStep + (int)(m_motorParamList[i].homeAngle * m_motorParamList[i].scale),
            angleType);
#ifdef DEBUG_COMMAND
        m_app->printf("Robot calibAngle M[%d] step[%d] angle[%.02f]\r\n",
                      i,m_motorParamList[i].calibStep,
                      listCalibAngle[i]);
#endif
    }
}

void Robot::armLength(float* listArmLength, int* numMotor)
{
    *numMotor = MAX_MOTOR;
    for(int i=MOTOR_CAPTURE; i< MAX_MOTOR; i++)
    {
        listArmLength[i] = m_motorParamList[i].length;
    }
}

int Robot::currentDirection(int motorID)
{
    return m_motorParamList[motorID].direction;
}

int Robot::minStep(int motorID)
{
    return (int)(m_motorParamList[motorID].scale * m_motorParamList[motorID].minAngle);
}

int Robot::maxStep(int motorID)
{
    return (int)(m_motorParamList[motorID].scale * m_motorParamList[motorID].maxAngle);
}

float Robot::homeAngle(int motorID)
{
    return m_motorParamList[motorID].homeAngle;
}

int Robot::homeStep(int motorID)
{
    return angleToStep(motorID, m_motorParamList[motorID].homeAngle);
}

void Robot::executeSmoothMotion(int motorID)
{
    m_motorList[motorID]->motionControlLoop();
}

void Robot::resetPulse(int motorID)
{
  m_motorList[motorID]->m_pulseCount = 0;
  m_app->resetPulse(motorID);
}


uint8_t Robot::statePulse(int motorID)
{
    return m_motorList[motorID]->m_statePulse;
}

uint32_t Robot::numWaitPulse(int motorID)
{
    return m_motorList[motorID]->m_numWaitPulse;
}

uint32_t Robot::countPulse(int motorID)
{
    return m_motorList[motorID]->m_pulseCount;
}

void Robot::updateStatePulse(int motorID, uint8_t newState)
{
    m_motorList[motorID]->m_statePulse = newState;
}

void Robot::updateCountPulse(int motorID, uint32_t countPulse)
{
    m_motorList[motorID]->m_pulseCount = countPulse;
}

void Robot::updateNumWaitPulse(int motorID, uint32_t numWaitPulse)
{
    m_motorList[motorID]->m_numWaitPulse = numWaitPulse;
}

void Robot::updateInitAngle(int motorID, float initAngle)
{
    m_motorParamList[motorID].currentStep = angleToStep(motorID,initAngle);
}

float Robot::armLength(int motorID)
{
    return m_motorParamList[motorID].length;
}

int Robot::currentStep(int motorID)
{
    return m_motorParamList[motorID].currentStep;
}

int Robot::calibStep(int motorID)
{
    return m_motorParamList[motorID].calibStep;
}

void Robot::updateCurrentStep(int motorID)
{
//    m_app->printf("R M[%d] currStep[%d/%d] dir[%d]\r\n",
//           motorID,m_motorParamList[motorID].currentStep,
//           m_motorParamList[motorID].targetStep,
//           m_motorParamList[motorID].direction);
    m_motorParamList[motorID].currentStep += m_motorParamList[motorID].direction;
}

void Robot::setMoveTarget(int* jointSteps)
{
    m_app->enableEngine(true);
    m_app->enableHardwareTimer(false);
    for(int i=0; i< MAX_MOTOR; i++) {
        if(!m_motorParamList[i].active) continue;
        m_moveTarget.jointSteps[i].steps = jointSteps[i]-m_motorParamList[i].currentStep;
//        m_app->printf("Robot::setMoveTarget M[%d] step[%d] from J[%d] C[%d]\r\n",
//                      i,m_moveTarget.jointSteps[i].steps,
//                      jointSteps[i],m_motorParamList[i].currentStep);
    }
    m_app->enableHardwareTimer(true);
}

void Robot::moveToTarget(int motorID)
{
#ifdef DEBUG_ROBOT
    m_app->printf("move sequence\r\n");
#endif
    m_requestMotorID = motorID;
    setState(ROBOT_EXECUTE_SEQUENCE);
    m_sequenceState = ROBOT_MOVE_EXECUTE_INIT;
}

uint8_t Robot::pulseLoop(int motorID)
{
    uint8_t newStatePulse = m_app->executePulseLoop(motorID);
    m_motorList[motorID]->m_statePulse = newStatePulse;
    return newStatePulse;
}

int Robot::executeMoveSequence()
{
    switch (m_sequenceState) {
        case ROBOT_MOVE_EXECUTE_INIT:{
            initMove(MOTOR_ARM1, MOTOR_ARM5);
            m_sequenceState = ROBOT_MOVE_EXECUTE_CHECK_RESULT;
        }
            break;
        case ROBOT_MOVE_EXECUTE_CHECK_RESULT: {
            if(gotoTarget() == ROBOT_MOVE_DONE) {
                m_sequenceState = ROBOT_MOVE_CAPTURE_INIT;
            }
        }
            break;
        case ROBOT_MOVE_CAPTURE_INIT: {
            initMove(MOTOR_CAPTURE, MOTOR_CAPTURE);
            m_sequenceState = ROBOT_MOVE_CAPTURE_CHECK_RESULT;
        }
        case ROBOT_MOVE_CAPTURE_CHECK_RESULT: {
            if(capture() == ROBOT_MOVE_DONE) {
                m_sequenceState = ROBOT_MOVE_DONE;
            }
        }
            break;
        case ROBOT_MOVE_DONE: {
            m_state = ROBOT_EXECUTE_DONE;
        }
            break;
    }
    return m_sequenceState;
}

float Robot::delayDecel(float stepCount, float delayCur) {
  float nextDelay = delayCur * (4.0f*stepCount + 1.0f) / (4.0f*stepCount - 1.0f);
  return nextDelay;
}
void Robot::calculateTotalTime(int numStepAccel, int numStepTotal, float minsleep, float homeStepTime,
                               float* totalDelay, float* startDelay) {
    if(numStepTotal < numStepAccel * 2) {
        float delayTime = homeStepTime;
        float accelTime = delayTime;
        float totalTime = 0;
        for(int i=1; i< numStepTotal/2; i++){
            delayTime = delayDecel(numStepTotal/2-i,delayTime);
            accelTime+=delayTime;
        }
        totalTime += accelTime*2 + (float)(numStepTotal - 2 * numStepTotal/2)*homeStepTime;
#ifdef DEBUG_CALCULATE_TIME
        m_app->printf("numstep[%d/%d] totalTime %.02f\r\n",
               numStepAccel,numStepTotal,totalTime);
#endif
        *totalDelay = totalTime;
        *startDelay = delayTime;
    } else {
        float delayTime = minsleep;
        float accelTime = delayTime;
        float totalTime = 0;
        for(int i=1; i< numStepAccel; i++){
            delayTime = delayDecel(numStepAccel-i,delayTime);
            accelTime+=delayTime;
        }
        totalTime += accelTime*2 + (float)(numStepTotal - 2 * numStepAccel)*minsleep;
#ifdef DEBUG_CALCULATE_TIME
        m_app->printf("numstep[%d/%d] totalTime %.02f\r\n",
               numStepAccel,numStepTotal,totalTime);
#endif
        *totalDelay = totalTime;
        *startDelay = delayTime;
    }
}

#define DEBUG_INITMOVE
void Robot::initMove(int motorIDFirst, int motorIDLast)
{
#if defined(DEBUG_INITMOVE) && defined(DEBUG_COMMAND)
    m_app->printf("============= Init Move =============\r\n");
    m_app->printf("Move motor[%d-%d]\r\n",motorIDFirst,motorIDLast);
#endif
    m_motorIDFirst = motorIDFirst;
    m_motorIDLast = motorIDLast;
    m_app->enableEngine(true);
    m_app->enableHardwareTimer(false);              
    // Calculate time for each motor to reach target step, 
    // then start with the motor which has longest time to reach target step
    float maxTime = 0;
    for(int i=motorIDFirst; i<= motorIDLast; i++) {
        if(!m_motorParamList[i].active) continue;
        m_motorParamList[i].targetStep = m_moveTarget.jointSteps[i].steps + m_motorParamList[i].currentStep;
        m_motorParamList[i].startStep = m_motorParamList[i].currentStep;
        m_motorParamList[i].direction = (m_motorParamList[i].targetStep > m_motorParamList[i].currentStep) ? 1 : -1;   
        float numStep = (float)abs(m_motorParamList[i].targetStep - m_motorParamList[i].currentStep);
#if defined(DEBUG_INITMOVE) && defined(DEBUG_COMMAND)
        m_app->printf("calculateTotalTime M[%d]\r\n", i);
#endif
        calculateTotalTime(m_motorParamList[i].numStepAccel, numStep,
                                (float)m_motorParamList[i].minPulsePerStep, (float)m_motorParamList[i].homeStepTime,
                             &m_timeDelay[i],&m_startDelay[i]);
        if(m_timeDelay[i] > maxTime) maxTime = m_timeDelay[i];
#if defined(DEBUG_INITMOVE) && defined(DEBUG_COMMAND)
        m_app->printf("Motor[%d] numStep[%d][%d->%d] minPulsePerStep[%d] time[%d] => Max[%d]\r\n",
                      i,
                      (int)numStep, m_motorParamList[i].currentStep, m_moveTarget.jointSteps[i].steps,
                      m_motorParamList[i].minPulsePerStep, (int)m_timeDelay[i],
                      (int)maxTime);
#endif
    }

    // Set step time for each motor
    for(int i=motorIDFirst; i<= motorIDLast; i++) {
        if(!m_motorParamList[i].active) continue;
        int numStep = abs(m_motorParamList[i].targetStep - m_motorParamList[i].currentStep);
        if(numStep == 0) continue;
        int startDelay = (int)(maxTime/m_timeDelay[i]*m_startDelay[i]);
        int numStepAccel = numStep < 2*m_motorParamList[i].numStepAccel?
                    numStep/2:m_motorParamList[i].numStepAccel;
        int numStepCruise = numStep - 2*numStepAccel;
        uint8_t moveType = (uint8_t) (MOTOR_EXECUTE_INCREASE_SPEED);
        m_motorList[i]->setupTarget(
            numStepAccel,
            numStepCruise,
            numStepAccel,
            m_motorParamList[i].direction,
            moveType,
            startDelay, m_motorParamList[i].minPulsePerStep);
#if defined(DEBUG_INITMOVE) && defined(DEBUG_COMMAND)
        m_app->printf("=== Motor[%d] numStep[%d] delayTime[%d]\r\n",
                      i, numStep, (int)startDelay);
#endif
    }

    // Initiate direction for each motor
    for(int i=0; i< MAX_MOTOR; i++) {
        if(!m_motorParamList[i].active) continue;
        m_app->initDirection(i, m_motorParamList[i].direction);
    }
    m_app->enableHardwareTimer(true);
}

int Robot::gotoTarget()
{
    bool allMotorsFinished = true;
    for(int motor = m_motorIDFirst; motor<= m_motorIDLast; motor++)
    {
        if(!m_motorParamList[motor].active) continue;
        if(m_motorParamList[motor].currentStep != m_motorParamList[motor].targetStep) {
            allMotorsFinished = false;
            break;
        }
    }
    if(allMotorsFinished) {
//        m_app->printf("======All motor finished\r\n");
    }
    return allMotorsFinished? ROBOT_MOVE_DONE:m_sequenceState;
}

int Robot::capture()
{
    bool captureDone = true;
    if(!m_motorParamList[MOTOR_CAPTURE].active)
        return ROBOT_MOVE_DONE;
    if(m_motorParamList[MOTOR_CAPTURE].currentStep != m_motorParamList[MOTOR_CAPTURE].targetStep)
        captureDone = false;
    if(captureDone) {
//        m_app->printf("======Capture finished\r\n");
    }
    return captureDone ? ROBOT_MOVE_DONE:m_sequenceState;
}
