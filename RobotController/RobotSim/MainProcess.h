#ifndef MAINPROCESS_H
#define MAINPROCESS_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include <QList>

class HardwareTimerSim;
class ApplicationSim;
class VideoRender;
class MainProcess : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList listAngle READ listAngle NOTIFY listAngleChanged)
    Q_PROPERTY(QVariantList listArmLength READ listArmLength NOTIFY listArmLengthChanged)
    Q_PROPERTY(QVariantList chessBoardInfo READ chessBoardInfo NOTIFY chessBoardInfoChanged)
public:
    explicit MainProcess(QObject *parent = nullptr);
    ~MainProcess();

    QVariantList listAngle();
    QVariantList listArmLength();
    QVariantList chessBoardInfo();
    void syncRobot();

public Q_SLOTS:
    void setListAngle(int motorID, float angle);
    void setListArmLength(int motorID, float length);
    void setChessBoardInfo(float posX, float posY, float size);

Q_SIGNALS:
    void readyToUpdate();
    void listAngleChanged();
    void listArmLengthChanged();
    void chessBoardInfoChanged();

public Q_SLOTS:
    void taskLoop();
    void startService();
    void stopService();
    void setRender(VideoRender* render);
    void updateScreen();
    void pause(bool pause);
    void enableHardwareTimer(bool enable);
    void executeCommand(QString command);
    void setBlackSide(bool isBlack);
    void setOpponentMove(int startRow, int startCol, int stopRow, int stopCol, char promotePiece);
    void setNextMove(int startRow, int startCol, int stopRow, int stopCol, char promotePiece);
    void updateRobotStep();
    void changeSleepTime(int sleepTime);
    void changeTimerPeriod(int sleepTime);
private:
    unsigned char m_renderData[93110400]; // 1920x1080 I420
    ApplicationSim* m_application;
    HardwareTimerSim* m_hardwareTimer;
    VideoRender* m_render = nullptr;
    float m_captureStep;
    QVariantList m_listAngle;
    QVariantList m_listArmLength;
    QVariantList m_chessBoardInfo;
    QTimer *m_timer;
};

#endif // MAINPROCESS_H
