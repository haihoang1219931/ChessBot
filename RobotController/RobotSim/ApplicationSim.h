#ifndef APPLICATIONSIM_H
#define APPLICATIONSIM_H

#include "../src/SAL/ApplicationController.h"
class MainProcess;
class ApplicationSim: public ApplicationController
{
public:
    ApplicationSim(MainProcess* mainProcess);
    ~ApplicationSim();
    void initRobot() override;
    void specificPlatformGohome(int motorID = MAX_MOTOR) override;
    void hardwareStop(int motorID = MAX_MOTOR) override;
    void checkInput() override;
    int printf(const char *fmt, ...) override;
    void msleep(int millis) override;
    long getSystemTime() override;
    void enableEngine(bool enable) override;
    bool isLimitReached(int motorID,
                            MOTOR_LIMIT_TYPE limitType) override;
    int readSerial(char* output, int length) override;
    void initDirection(int motorID, int direction) override ;
    void moveDoneAction(int motorID) override;
    uint8_t executePulseLoop(int motorID) override;
    void enableHardwareTimer(bool enable) override;
    void resetPulse(int motorID) override;
    void simulateReceivedCommand(char* command);
private:
    MainProcess* m_mainProcess;
    char m_command[MAX_COMMAND_LENGTH];
};

#endif // APPLICATIONSIM_H
