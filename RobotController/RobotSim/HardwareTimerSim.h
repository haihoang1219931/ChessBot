#ifndef HARDWARETIMERSIM_H
#define HARDWARETIMERSIM_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QDebug>
class ApplicationSim;
class HardwareTimerSim : public QObject
{
    Q_OBJECT
public:
    explicit HardwareTimerSim(QObject *parent = nullptr);
    void setApplication(ApplicationSim* app);
    void setIntervalMotion(int millis);
    void setIntervalInput(int millis);
public Q_SLOTS:
    void enableTaskMotion(bool enable);
    void enableTaskInput(bool enable);
    void taskLoopMotion();
    void taskLoopInput();
Q_SIGNALS:

private:
    ApplicationSim* m_app;
    QTimer *m_timerMotion;
    QTimer *m_timerInput;
    int m_frequency;
};

#endif // HARDWARETIMERSIM_H
