#include "HardwareTimerSim.h"
#include "ApplicationSim.h"

HardwareTimerSim::HardwareTimerSim(QObject *parent) : QObject(parent)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &HardwareTimerSim::taskLoop);
}

void HardwareTimerSim::setApplication(ApplicationSim* app)
{
    m_app = app;
}

void HardwareTimerSim::setInterval(int millis)
{
    m_timer->setInterval(millis);
}

void HardwareTimerSim::enableTask(bool enable)
{
    if (!enable) {
        m_timer->stop();
        qDebug() << "Task Paused";
    } else {
        m_timer->start();
        qDebug() << "Task Resume";
    }
}

void HardwareTimerSim::taskLoop()
{
    m_app->executeSmoothMotionLoop(0);
    m_app->executeSmoothMotionLoop(1);
    m_app->executeSmoothMotionLoop(2);
    m_app->executeSmoothMotionLoop(5);
}
