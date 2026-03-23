#include "HardwareTimerSim.h"
#include "ApplicationSim.h"

HardwareTimerSim::HardwareTimerSim(QObject *parent) : QObject(parent)
{
    m_timerMotion = new QTimer(this);
    connect(m_timerMotion, &QTimer::timeout, this, &HardwareTimerSim::taskLoopMotion);
    m_timerInput = new QTimer(this);
    connect(m_timerInput, &QTimer::timeout, this, &HardwareTimerSim::taskLoopInput);
    m_timerCommand = new QTimer(this);
    connect(m_timerCommand, &QTimer::timeout, this, &HardwareTimerSim::taskLoopCommand);
}

void HardwareTimerSim::setApplication(ApplicationSim* app)
{
    m_app = app;
}

void HardwareTimerSim::setIntervalMotion(int millis)
{
    m_timerMotion->setInterval(millis);
}

void HardwareTimerSim::setIntervalInput(int millis)
{
    m_timerInput->setInterval(millis);
}

void HardwareTimerSim::setIntervalCommand(int millis)
{
    m_timerCommand->setInterval(millis);
}

void HardwareTimerSim::enableTaskMotion(bool enable)
{
    if (!enable) {
        m_timerMotion->stop();
    } else {
        m_timerMotion->start();
    }
}

void HardwareTimerSim::enableTaskInput(bool enable)
{
    if (!enable) {
        m_timerInput->stop();
    } else {
        m_timerInput->start();
    }
}

void HardwareTimerSim::enableTaskCommand(bool enable)
{
    if (!enable) {
        m_timerCommand->stop();
    } else {
        m_timerCommand->start();
    }
}

void HardwareTimerSim::taskLoopMotion()
{
    m_app->executeSmoothMotionLoop(0);
    m_app->executeSmoothMotionLoop(1);
    m_app->executeSmoothMotionLoop(2);
    m_app->executeSmoothMotionLoop(5);
}

void HardwareTimerSim::taskLoopInput()
{
    m_app->updateInputState();
}

void HardwareTimerSim::taskLoopCommand()
{
    m_app->readCommand();
}
