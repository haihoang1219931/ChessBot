#include "ApplicationSim.h"
#include "MainProcess.h"
#include "../src/SAL/Robot.h"
#include "../src/SAL/ChessBoard.h"
#include <stdio.h>
#include <stdarg.h>
#ifdef __linux__
#include <time.h>
#include <string.h>
#elif _WIN32
#include <time.h>
#include <windows.h>
#else
#endif
ApplicationSim::ApplicationSim(MainProcess* mainProcess):
    m_mainProcess(mainProcess)
{
    memset(m_command,0x00U, sizeof(m_command));
    initRobot();
}

ApplicationSim::~ApplicationSim()
{

}

#define FREQUENCY_TIMER1 1250.0f
void ApplicationSim::initRobot()
{
    m_chessBoard->setChessBoardPosX(-31.0f+31.0f*8.0f/2.0f);
    m_chessBoard->setChessBoardPosY(100);
    m_chessBoard->setChessBoardSize(31*8);
    m_chessBoard->setDropZoneSpace(31);
    m_minSpace = 2;

    JointParam armPrams[MAX_MOTOR] = {
    // active|   scale=gear_ratio/resolution   |length|init angle|home angle|home step time|min angle|max angle|min pulse/step|frequency | step accel
        {true,  100.0f*(20.0f/360.0f),                0,      10,        0,        15,           0,       250,      15,   FREQUENCY_TIMER1,      0},
        {true,  1,   255,       0,      -15,        18,         -15,       150,       6,   FREQUENCY_TIMER1,    350},
        {true,  1, 80.27,     140,       50,        64,          50,       210,      12,   FREQUENCY_TIMER1,     75},
        {false,  1.0f/1.0f,                       25.57,     130,      130,         1,         130,       130,       6,   FREQUENCY_TIMER1,      0},
        {false,  1.0f/1.0f,                         120,     180,      180,         1,         180,       180,       6,   FREQUENCY_TIMER1,      0},
        {true,                                  1,    0,       -45,      -45,         6,         -45,         0,       6,   FREQUENCY_TIMER1,      0}
    };

    for(int motor= MOTOR_CAPTURE; motor<= MOTOR_ARM5; motor++) {
        m_robot->setMotorParam(motor,armPrams[motor]);
        m_robot->updateInitAngle(motor,armPrams[motor].initAngle);
    }
    Point c00 = m_chessBoard->convertDropPoint(0,0,ZONE_PLAYER);
    printf("c00 x(%.2f) y(%.2f)\r\n",c00.x,c00.y);
    Point c07 = m_chessBoard->convertDropPoint(0,0,ZONE_BOT);
    printf("c07 x(%.2f) y(%.2f)\r\n",c07.x,c07.y);
    Point c77 = m_chessBoard->convertPoint(7,7);
    printf("c77 x(%.2f) y(%.2f)\r\n",c77.x,c77.y);
    /**
     * CB r[7] c[7] x[1395] y[3325]
        simulateReceivedCommand:[tx1395y3325]
        [tx1395y3325] Pos confirmed
        simulateReceivedCommand:[ts0]
        [ts0] TS confirmed
        TS x[3489] y[1313] m[1][787] m[2][262] m[5][892]
     */
//    Point targetPosition;
//    targetPosition.x = 316.4;
//    targetPosition.y = 163.4;
//    int jointSteps[MAX_MOTOR];
//    jointSteps[MOTOR_CAPTURE] = m_robot->homeAngle(MOTOR_CAPTURE);
//    jointSteps[MOTOR_ARM3] = m_robot->homeAngle(MOTOR_ARM3);
//    jointSteps[MOTOR_ARM4] = m_robot->homeAngle(MOTOR_ARM4);
//    calculateJoints(targetPosition.x, targetPosition.y, -45, jointSteps);
//    this->printf("calculateJoints x[%d] y[%d] m[1][%d] m[2][%d] m[5][%d]\r\n",
//        (int)(targetPosition.x),
//        (int)(targetPosition.y),
//        jointSteps[MOTOR_ARM1],
//        jointSteps[MOTOR_ARM2],
//        jointSteps[MOTOR_ARM5]
//    );
//    m_robot->m_motorParamList[MOTOR_ARM1].currentStep = jointSteps[MOTOR_ARM1];
//    m_robot->m_motorParamList[MOTOR_ARM2].currentStep = jointSteps[MOTOR_ARM2];
//    m_robot->m_motorParamList[MOTOR_ARM5].currentStep = jointSteps[MOTOR_ARM5];
//    Point currentPosition = currentPos();
//    this->printf("TS x[%d] y[%d] m[1][%d] m[2][%d] m[5][%d]\r\n",
//        (int)(currentPosition.x*10.0f),
//        (int)(currentPosition.y*10.0f),
//        m_robot->currentStep(1),
//        m_robot->currentStep(2),
//        m_robot->currentStep(5)
//    );

}

void ApplicationSim::specificPlatformGohome(int motorID)
{
    m_mainProcess->changeTimerPeriodMotion(1);
    m_mainProcess->changeTimerPeriodInput(1000);
}

void ApplicationSim::harwareStop(int motorID)
{
    //@todo: consider to optimize code
}

void ApplicationSim::checkInput()
{
    // Simulation do nothing here
}

int ApplicationSim::printf(const char *fmt, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    int rc = vsprintf(buffer, fmt, args);
    va_end(args);
    ::printf("%s",buffer);
    fflush(stdout);
    return rc;
}

void ApplicationSim::msleep(int millis)
{
#ifdef __linux__
    //linux code goes here
    struct timespec ts = { millis / 1000, (millis % 1000) * 1000 * 1000 };
    nanosleep(&ts, NULL);
#elif _WIN32
    // windows code goes here
    Sleep(millis);
#else
#endif
}

long ApplicationSim::getSystemTime()
{
#ifdef USE_HARD_TIMER
#ifdef __linux__
    struct timeval curTime;
    gettimeofday(&curTime, NULL);
    return (curTime.tv_usec + curTime.tv_sec*1000000);
#elif _WIN32
    struct timeb curTime;
    ftime(&curTime);
    return (long)(1000*curTime.time + curTime.millitm);
#else
    return 0;
#endif
#else
    return m_appTimer;
#endif
}

bool ApplicationSim::isLimitReached(int motorID,
                        MOTOR_LIMIT_TYPE limitType)
{
    bool result = false;

    if(limitType == MOTOR_LIMIT_MIN)
        result = m_robot->currentStep(motorID)
                <= m_robot->minStep(motorID);
    else if(limitType == MOTOR_LIMIT_MAX)
        result = m_robot->currentStep(motorID)
                >= m_robot->maxStep(motorID);
    else result = m_robot->currentStep(motorID)
            <= m_robot->homeStep(motorID);
#ifdef DEBUG_SIM
    char strLimit[3][8] = {
        {"MIN"},
        {"MAX"},
        {"HOME"}
    };
    printf("SIM M[%d] limit[%s][%s] home[%d] min[%d] cur[%d] max[%d] dir[%d]\r\n",
           motorID,
           strLimit[limitType],
           result?"true":"false",
           m_robot->homeStep(motorID),
           m_robot->minStep(motorID),
           m_robot->currentStep(motorID),
           m_robot->maxStep(motorID),
           m_robot->dir(motorID));
#endif
    return result;
}

void ApplicationSim::enableEngine(bool enable)
{

}

int ApplicationSim::readSerial(char* output, int length)
{
    int commandLength = strlen(m_command);
    memcpy(output,m_command,strlen(m_command));
    memset(m_command,0x00U, sizeof(m_command));
    return commandLength;
}

void ApplicationSim::initDirection(int motorID, int direction)
{

}

void ApplicationSim::moveDoneAction(int motorID)
{
#ifdef DEBUG_SIM
    this->printf("Sim M[%d] move done\r\n",motorID);
#endif
}

void ApplicationSim::simulateReceivedCommand(char* command)
{
    memset(m_command,0x00,sizeof(m_command));
    memcpy(m_command,command,strlen(command));
    printf("simulateReceivedCommand:[%s]\r\n",m_command);
}

uint8_t ApplicationSim::executePulseLoop(int motorID)
{
    uint8_t statePulse = m_robot->statePulse(motorID);
    uint32_t countPulse = m_robot->countPulse(motorID);
    uint32_t numWaitPulse = m_robot->numWaitPulse(motorID);
#ifdef DEBUG_SIM
    printf("p motorID[%d] S[%d] C[%d/%d]\r\n",
           motorID,
           statePulse, countPulse, numWaitPulse);
#endif
    if(countPulse < numWaitPulse) {
        m_robot->updateCountPulse(motorID,countPulse+1);
        m_robot->updateStatePulse(motorID,STATE_PENDING);
        return STATE_PENDING;
    } else {
        return STATE_DONE;
    }
}

void ApplicationSim::resetPulse(int motorID)
{
    m_robot->updateCountPulse(motorID,0);
}

void ApplicationSim::enableHardwareTimer(bool enable)
{
    m_mainProcess->enableHardwareTimer(enable);
}
