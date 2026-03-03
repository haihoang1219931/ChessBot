#include "MainProcess.h"
#include "VideoDisplay/VideoRender.h"
#include "ApplicationSim.h"
#include "HardwareTimerSim.h"
#include "../src/SAL/Robot.h"
#include <stdio.h>
MainProcess::MainProcess(QObject *parent) :
    QObject(parent)
{
    m_application = new ApplicationSim(this);
    m_hardwareTimer = new HardwareTimerSim();
    m_hardwareTimer->setApplication(m_application);
    memset(m_renderData,0,sizeof(m_renderData));
    m_timer = new QTimer();
    connect(m_timer, &QTimer::timeout, this, &MainProcess::taskLoop);
    changeSleepTime(30);
    changeTimerPeriod(1);
    for(int i=0; i< MAX_MOTOR; i++)
    {
        m_listAngle.append(0);
        m_listArmLength.append(0);
    }
    for(int i=0; i< MAX_PARAMS_CHESSBOARD; i++)
    {
        m_chessBoardInfo.append(0);
    }
    syncRobot();
}

MainProcess::~MainProcess()
{
    stopService();
}

void MainProcess::enableHardwareTimer(bool enable)
{
    m_hardwareTimer->enableTask(enable);
}

void MainProcess::pause(bool pause){
    if(pause == true){
        m_timer->stop();
    }else{
        m_timer->start();
    }
    m_hardwareTimer->enableTask(!pause);
}
void MainProcess::setBlackSide(bool isBlack) {
//    m_application->chessController()->setChessPiece(isBlack);
}

void MainProcess::setOpponentMove(int startRow, int startCol, int stopRow, int stopCol, char promotePiece) {
//    ChessMove chessMove = {startRow,startCol,stopRow,stopCol,false,false,promotePiece};
//    m_application->chessController()->setOpponentMove(chessMove);
}

void MainProcess::setNextMove(int startRow, int startCol, int stopRow, int stopCol, char promotePiece) {
//    ChessMove chessMove = {startRow,startCol,stopRow,stopCol,false,false,promotePiece};
//    m_application->chessController()->setNextMove(chessMove);
}

void MainProcess::syncRobot()
{
    float listArmLength[MAX_MOTOR];
    int numArm;
    m_application->getCurrentArmLength(listArmLength,&numArm);
    printf("syncRobot numArm[%d]\r\n",numArm);
    for(int i=MOTOR_CAPTURE; i< numArm; i++)
    {
        m_listArmLength[i] = listArmLength[i];
        printf("m_listArmLength[%d] %f\r\n",
               i,m_listArmLength[i].toFloat());
    }

    float listAngle[MAX_MOTOR];
    int numMotor;
    printf("numMotor[%d]\r\n",numMotor);
    m_application->getCurrentPosition(listAngle,&numMotor);
    for(int i=0; i< numMotor; i++)
    {
        m_listAngle[i] = listAngle[i];
        printf("m_listAngle[%d] %f\r\n",
               i,m_listAngle[i].toFloat());
    }

    float chessBoardInfo[MAX_PARAMS_CHESSBOARD];
    int numParam;
    m_application->getChessBoardParams(chessBoardInfo,&numParam);
    for(int i=0; i< numParam; i++)
    {
        m_chessBoardInfo[i] = chessBoardInfo[i];
        printf("chessBoardInfo[%d] %f\r\n",
               i,m_chessBoardInfo[i].toFloat());
    }
    Q_EMIT listArmLengthChanged();
    Q_EMIT listAngleChanged();
    Q_EMIT chessBoardInfoChanged();
}

void MainProcess::taskLoop() {
    m_application->loop();
    updateRobotStep();
}
void MainProcess::startService() {
    printf("Start\r\n");
    m_timer->start();
}

void MainProcess::stopService() {
    printf("Stop\r\n");
    m_timer->stop();
}
void MainProcess::setRender(VideoRender* render)
{
    m_render = render;
}
void MainProcess::updateScreen() {

}
void MainProcess::executeCommand(QString command)
{
    m_application->simulateReceivedCommand((char*)command.toStdString().c_str(),command.length());
}

void MainProcess::updateRobotStep()
{
    float listSteps[MAX_MOTOR];
    int numMotor;
    for(int i=0; i< MAX_MOTOR; i++){
        listSteps[i] = 0;
    }
    m_application->getCurrentPosition(listSteps,&numMotor);
    for(int i=0; i< numMotor; i++){
        setListAngle(i,listSteps[i]);
    }
    Q_EMIT listAngleChanged();
    Q_EMIT listArmLengthChanged();
}

void MainProcess::changeSleepTime(int sleepTime)
{
    m_timer->setInterval(sleepTime);

}
void MainProcess::changeTimerPeriod(int sleepTime)
{
    m_hardwareTimer->setInterval(sleepTime);
}
QVariantList MainProcess::listAngle()
{
    return m_listAngle;
}

QVariantList MainProcess::listArmLength()
{
    return m_listArmLength;
}

QVariantList MainProcess::chessBoardInfo()
{
    return m_chessBoardInfo;
}

void MainProcess::setListAngle(int motorID, float angle)
{
    m_listAngle[motorID] = angle;
}

void MainProcess::setListArmLength(int motorID, float length)
{
    m_listArmLength[motorID] = length;
}

void MainProcess::setChessBoardInfo(float posX, float posY, float size)
{
    m_chessBoardInfo[0] = posX;
    m_chessBoardInfo[1] = posY;
    m_chessBoardInfo[2] = size;
    Q_EMIT chessBoardInfoChanged();
}
