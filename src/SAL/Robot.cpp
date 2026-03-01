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
    printf("Robot::setMotorParam[%d] param maxSpeed[%d] to Robot[%d]\r\n",
           motorID,param.maxSpeed,
           m_motorParamList[motorID].maxSpeed);
}

void Robot::setState(ROBOT_STATE newState) {
    if(m_state != newState) {
        m_state = newState;
        m_app->printf("ROBOT STATE: %d\r\n",m_state);
    }
}

void Robot::loop() {
    m_elapsedTime = m_app->getSystemTime() - m_startTime;
#ifdef DEBUG_ROBOT
    m_app->printf("Robot time[%ld]\r\n", m_elapsedTime);
#endif
    switch(m_state) {
    case ROBOT_EXECUTE_GO_HOME: {
        executeGohome();
    }
        break;
    case ROBOT_EXECUTE_SEQUENCE: {
        executeMoveSequence();
    }
        break;
    case ROBOT_EXECUTE_DONE: {
        m_app->setMachineState(MACHINE_EXECUTE_COMMAND_DONE);
    }
        break;
    }
}

void Robot::initDirection(int motorID, int direction)
{
    m_motorParamList[motorID].direction = direction;
    m_app->initDirection(motorID, direction);
}

void Robot::requestGoHome(int motorID) {
    m_app->printf("GO HOME\r\n");
    // m_app->enableHardwareTimer(false);
    m_requestMotorID = motorID;
    int startID = motorID == MAX_MOTOR ? 0 : motorID;
    int stopID = motorID == MAX_MOTOR ? MAX_MOTOR : motorID;
    m_app->printf("Request go home from [%d-%d]\r\n",startID,stopID);
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
    m_startTime = m_app->getSystemTime();
    setState(ROBOT_EXECUTE_GO_HOME);
    // m_app->enableHardwareTimer(true);
}

void Robot::executeGohome() {
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
            m_app->printf("Homed M[%d] step[%d]\r\n",
                      motor,m_motorParamList[motor].currentStep);
        }
        m_app->harwareStop(m_requestMotorID);
        setState(ROBOT_EXECUTE_DONE);
    }
}

void Robot::requestGoPosition(int motorID, int targetStep, int stepTime, bool isRelativeMove)
{
    m_requestMotorID = motorID;
    m_motorParamList[motorID].targetStep = targetStep;
    m_motorParamList[motorID].homeStepTime = stepTime;
    m_motorParamList[motorID].direction = (targetStep > m_motorParamList[motorID].currentStep) ? 1 : -1;
    m_startTime = m_app->getSystemTime();
    setState(ROBOT_EXECUTE_POSITION);
}

long Robot::elapsedTime()
{
    return m_elapsedTime;
}

int Robot::angleToStep(int motorID, float angle)
{
    return (int)(angle * m_motorParamList[motorID].scale);
}

bool Robot::isLimitReached(int motorID,
                        MOTOR_LIMIT_TYPE limitType)
{
    return m_app->isLimitReached(motorID, limitType);
}

float Robot::stepToAngle(int motorID, int step)
{
    return (float)step / m_motorParamList[motorID].scale;
}
void Robot::currentStep(int* listCurrentStep, int* numMotor)
{
    *numMotor = MAX_MOTOR;
    for(int i=MOTOR_CAPTURE; i< MAX_MOTOR; i++)
    {
        listCurrentStep[i] = m_motorParamList[i].currentStep;
    }
}

void Robot::currentAngle(float* listCurrentAngle, int* numMotor)
{
    *numMotor = MAX_MOTOR;
    for(int i=MOTOR_CAPTURE; i< MAX_MOTOR; i++)
    {
        listCurrentAngle[i] = stepToAngle(i, m_motorParamList[i].currentStep);
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

void Robot::updateCurrentStep(int motorID)
{
//    printf("R M[%d] currStep[%d] dir[%d]\r\n",
//           motorID,m_motorParamList[motorID].currentStep,
//           m_motorParamList[motorID].direction);
    m_motorParamList[motorID].currentStep += m_motorParamList[motorID].direction;
}

void Robot::resetMoveSequene()
{
    m_curMove = 0;
    m_numMove = 0;
}

void Robot::appendMove(int* jointSteps)
{
//    memcpy(m_moveSequence[m_numMove].jointSteps,jointSteps,sizeof(int)*MAX_MOTOR);
    for(int i=0; i< MAX_MOTOR; i++) {
        if(!m_motorParamList[i].active) continue;
        m_moveSequence[m_numMove].jointSteps[i].steps = jointSteps[i];
        m_app->printf("Robot::appendMove[%d] M[%d] step[%d]\r\n",
                      m_numMove,
                      i,m_moveSequence[m_numMove].jointSteps[i].steps);
    }
    m_numMove++;
}

void Robot::moveSequence(int motorID)
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

void Robot::executeMoveSequence()
{
    switch (m_sequenceState) {
        case ROBOT_MOVE_EXECUTE_INIT:{
            initMove(MOTOR_ARM1, MOTOR_ARM5);
        }
            break;
        case ROBOT_MOVE_EXECUTE_CHECK_RESULT: {
            gotoTarget();
        }
            break;
        case ROBOT_MOVE_CAPTURE_INIT: {
            initMove(MOTOR_CAPTURE, MOTOR_CAPTURE);
        }
        case ROBOT_MOVE_CAPTURE_CHECK_RESULT: {
            capture();
        }
            break;
        case ROBOT_MOVE_DONE: {
            setState(ROBOT_EXECUTE_DONE);
        }
            break;
    }
}

void Robot::initMove(int motorIDFirst, int motorIDLast)
{
    m_app->printf("============= Init Move [%02d/%02d] =============\r\n",
                  m_curMove, m_numMove);
    m_motorIDFirst = motorIDFirst;
    m_motorIDLast = motorIDLast;
    m_app->enableHardwareTimer(false);              
    // Calculate time for each motor to reach target step, 
    // then start with the motor which has longest time to reach target step
    float maxTime = 0;
    for(int i=motorIDFirst; i<= motorIDLast; i++) {
        if(!m_motorParamList[i].active) continue;
        m_motorParamList[i].targetStep = m_moveSequence[m_curMove].jointSteps[i].steps;
        m_motorParamList[i].startStep = m_motorParamList[i].currentStep;
        m_motorParamList[i].direction = (m_motorParamList[i].targetStep > m_motorParamList[i].currentStep) ? 1 : -1;   
        float numStep = (float)abs(m_motorParamList[i].targetStep - m_motorParamList[i].currentStep);
        float time = numStep / m_motorParamList[i].maxSpeed;
        if(time > maxTime) maxTime = time;
        m_app->printf("Motor[%d] numStep[%f][%d->%d] maxSpeed[%d] time[%f] => Max[%f]\r\n",
                      i,
                      numStep, m_motorParamList[i].currentStep, m_moveSequence[m_curMove].jointSteps[i].steps,
                      m_motorParamList[i].maxSpeed, time,
                      maxTime);
    }

    // Set step time for each motor
    for(int i=motorIDFirst; i<= motorIDLast; i++) {
        if(!m_motorParamList[i].active) continue;
        int numStep = abs(m_motorParamList[i].targetStep - m_motorParamList[i].currentStep);
        if(numStep == 0) continue;
        float timerFrequency = m_motorParamList[i].frequency;
        float numPulsePerStep = timerFrequency * maxTime / (float)numStep;
        m_motorList[i]->setupTarget(
            (int)(numStep*0.0f), 
            (int)(numStep*1.0f), 
            (int)(numStep*0.0f), 
            m_motorParamList[i].direction, MOTOR_EXECUTE_CRUISE_SPEED, numPulsePerStep, 21);
        m_app->printf("Motor[%d] numStep[%d] delayTime[%f]\r\n", i, numStep, numPulsePerStep);
    }

    // Initiate direction for each motor
    for(int i=0; i< MAX_MOTOR; i++) {
        if(!m_motorParamList[i].active) continue;
        m_app->initDirection(i, m_motorParamList[i].direction);
    }
    m_app->enableHardwareTimer(true);

    m_sequenceState = motorIDFirst != MOTOR_CAPTURE ?
                ROBOT_MOVE_EXECUTE_CHECK_RESULT:ROBOT_MOVE_CAPTURE_CHECK_RESULT;
}

void Robot::gotoTarget()
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
        m_app->printf("======All motor finished\r\n");
        m_sequenceState = ROBOT_MOVE_CAPTURE_INIT;
    }
}

void Robot::capture()
{
    bool captureDone = true;
    if(!m_motorParamList[MOTOR_CAPTURE].active)
        m_sequenceState = ROBOT_MOVE_DONE;
    if(m_motorParamList[MOTOR_CAPTURE].currentStep != m_motorParamList[MOTOR_CAPTURE].targetStep)
        captureDone = false;
    if(captureDone) {
        if(m_curMove < m_numMove-1) {
            m_curMove++;
            m_sequenceState = ROBOT_MOVE_EXECUTE_INIT;
        } else {
            m_app->enableHardwareTimer(false);
            m_sequenceState = ROBOT_MOVE_DONE;
        }
    }
}
