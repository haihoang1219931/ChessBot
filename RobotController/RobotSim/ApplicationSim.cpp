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
    m_chessBoard->setChessBoardPosX(31-31*8/2);
    m_chessBoard->setChessBoardPosY(100);
    m_chessBoard->setChessBoardSize(31*8);
    m_chessBoard->setDropZoneSpace(31);
    m_minSpace = 2;

    JointParam armPrams[MAX_MOTOR] = {
    // active|   scale=gear_ratio/resolution   |length|init angle|home angle|home step time|min angle|max angle|min pulse/step|frequency | step accel
        {true,  100.0f*(20.0f/360.0f),                0,      10,        0,         2,           0,       250,       2,   FREQUENCY_TIMER1,     50},
        {true,  1.0f*18.0f/01.0f*(200.0f/360.0f),   255,       0,      -17,         8,         -17,       150,       2,   FREQUENCY_TIMER1,    150},
        {true,  1.0f*70.0f/20.0f*(200.0f/360.0f),    85,     140,       50,         8,          50,       210,       2,   FREQUENCY_TIMER1,    150},
        {false,  1.0f/1.0f,                          15,     130,      130,         1,         130,       130,       1,   FREQUENCY_TIMER1,      0},
        {false,  1.0f/1.0f,                         120,     180,      180,         1,         180,       180,       1,   FREQUENCY_TIMER1,      0},
        {true,  50.0f/14.0f*100.0f*(20.0f/360.0f),    0,      10,        0,         2,           0,        45,       2,   FREQUENCY_TIMER1,     50}
    };
    for(int motor= MOTOR_CAPTURE; motor<= MOTOR_ARM5; motor++) {
        printf("ApplicationSim::initRobot[%d] param maxSpeed[%d]\r\n",
               motor,armPrams[motor].minPulsePerStep);
        m_robot->setMotorParam(motor,armPrams[motor]);
        m_robot->updateInitAngle(motor,armPrams[motor].initAngle);
    }
}

void ApplicationSim::specificPlatformGohome(int motorID)
{
    m_mainProcess->changeTimerPeriod(1);
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
                == m_robot->minStep(motorID);
    else if(limitType == MOTOR_LIMIT_MAX)
        result = m_robot->currentStep(motorID)
                == m_robot->maxStep(motorID);
    else result = m_robot->currentStep(motorID)
            == m_robot->homeStep(motorID);
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

void ApplicationSim::simulateReceivedCommand(char* command, int length)
{
    memcpy(m_command,command,length);
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
