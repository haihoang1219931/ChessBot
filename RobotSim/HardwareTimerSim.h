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
    void setInterval(int millis);
public Q_SLOTS:
    void enableTask(bool enable);
    void taskLoop();
Q_SIGNALS:

private:
    ApplicationSim* m_app;
    QTimer *m_timer;
    int m_frequency;
};

#endif // HARDWARETIMERSIM_H
