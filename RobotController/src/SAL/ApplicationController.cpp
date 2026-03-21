#include "ApplicationController.h"
#include "Button.h"
#include "Robot.h"
#include "ChessBoard.h"
#include "CommandReader.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

ApplicationController::ApplicationController()
{
    for(int i=0;i< MAX_BUTTON; i++) {
        m_buttonList[i] = new Button(this);
    }
    m_commandReader = new CommandReader(this);
    m_robot = new Robot(this);
    m_chessBoard = new ChessBoard();
    m_machineState = MACHINE_WAIT_COMMAND;
    m_appTimer = 0;
}

ApplicationController::~ApplicationController() {

}

void ApplicationController::loop() {
    m_appTimer++;
#ifdef DEBUG_APP
    this->printf("APP Timer[%d] m_machineState[%d]\r\n",m_appTimer,m_machineState);
#endif
    switch(m_machineState) {
        case MACHINE_WAIT_COMMAND: {
            m_commandReader->loop();
            break;
        }
        case MACHINE_EXECUTE_HOME:
        case MACHINE_EXECUTE_POSITION:
        {
            updateInputState();
            if(m_robot->loop() == ROBOT_MOVE_DONE)
                m_machineState = MACHINE_EXECUTE_COMMAND_DONE;
            break;
        }
        case MACHINE_EXECUTE_COMMAND: {
            if(executeCommandSequenceLoop() == COMMAND_SEQUENCE_STATE_DONE)
                m_machineState = MACHINE_EXECUTE_POSITION_STANDBY;
            break;
        }
        case MACHINE_EXECUTE_POSITION_STANDBY: {
            goToReadyPosition();
            break;
        }
        case MACHINE_EXECUTE_COMMAND_DONE: {
            this->printf("_%04d DON",m_comCommandID);
            setMachineState(MACHINE_WAIT_COMMAND);        
            break;
        }
    }
}
int ApplicationController::executeCommandSequenceLoop()
{
    switch (m_commandSequenceState) {
    case COMMAND_SEQUENCE_STATE_INIT:
    {
        m_commandSequenceState = COMMAND_SEQUENCE_STATE_EXECUTE;
        m_commandState = COMMAND_STATE_INIT;
    }
        break;
    case COMMAND_SEQUENCE_STATE_EXECUTE:
    {
        if(executeCommandLoop() == COMMAND_STATE_DONE) {
            if(m_curCommandId >= m_numCommand - 1) {
                m_commandSequenceState = COMMAND_SEQUENCE_STATE_DONE;
            } else {
                m_commandState = COMMAND_STATE_INIT;
                m_curCommandId++;
            }
        }
    }
        break;
    case COMMAND_SEQUENCE_STATE_DONE:
    {

    }
        break;
    }
    return m_commandSequenceState;
}
int ApplicationController::executeCommandLoop()
{
    updateInputState();
    int commandType = m_sequenceCommand[m_curCommandId].type;
    switch (commandType) {
    case COMMAND_NORMAL: {
        executeCommandNormal();
    }
        break;
    case COMMAND_LINE: {
        executeCommandLine();
    }
        break;
    }
    return m_commandState;
}

int ApplicationController::executeCommandNormal()
{
    switch (m_commandState) {
    case COMMAND_STATE_INIT: {
        int jointSteps[MAX_MOTOR];
        calculateJoints(m_sequenceCommand[m_curCommandId].y,
                        m_sequenceCommand[m_curCommandId].x,
                        m_sequenceCommand[m_curCommandId].updownAngle,
                        jointSteps);
        jointSteps[MOTOR_CAPTURE] = m_sequenceCommand[m_curCommandId].captureStep;
        m_robot->setMoveTarget(jointSteps);
        m_robot->moveToTarget(MAX_MOTOR);
        m_commandState = COMMAND_STATE_EXECUTE_THEN_DONE;
    }
        break;
    case COMMAND_STATE_EXECUTE_THEN_DONE: {
        if(m_robot->loop() == ROBOT_EXECUTE_DONE) {
            m_commandState = COMMAND_STATE_DONE;
        }
    }
        break;
    case COMMAND_STATE_DONE: {

    }
        break;
    }
    return m_commandState;
}

int ApplicationController::executeCommandLine()
{
    switch (m_commandState) {
    case COMMAND_STATE_INIT: {
        int jointSteps[MAX_MOTOR];
        Point curPos = currentPos();
        Point tarPos;
        tarPos.x = m_sequenceCommand[m_curCommandId].x;
        tarPos.y = m_sequenceCommand[m_curCommandId].y;

        Command nextPoint;
        if(distance(curPos.x,curPos.y,
                    m_sequenceCommand[m_curCommandId].x,
                    m_sequenceCommand[m_curCommandId].y) > m_minSpace) {
            nextPoint = calculateNextPointInLine(curPos,tarPos);
            m_commandState = COMMAND_STATE_EXECUTE_THEN_RECAL;
        } else {
            nextPoint.x = m_sequenceCommand[m_curCommandId].x;
            nextPoint.y = m_sequenceCommand[m_curCommandId].y;
            nextPoint.updownAngle = m_sequenceCommand[m_curCommandId].updownAngle;
            nextPoint.captureStep = m_sequenceCommand[m_curCommandId].captureStep;
            m_commandState = COMMAND_STATE_EXECUTE_THEN_DONE;
        }

        calculateJoints(nextPoint.y,
                        nextPoint.x,
                        nextPoint.updownAngle,
                        jointSteps);
        jointSteps[MOTOR_CAPTURE] = m_sequenceCommand[m_curCommandId].captureStep;
        m_robot->setMoveTarget(jointSteps);
        m_robot->moveToTarget(MAX_MOTOR);
    }
        break;
    case COMMAND_STATE_EXECUTE_THEN_RECAL: {
        if(m_robot->loop() == ROBOT_EXECUTE_DONE) {
            m_commandState = COMMAND_STATE_INIT;
        }
    }
        break;
    case COMMAND_STATE_EXECUTE_THEN_DONE: {
        if(m_robot->loop() == ROBOT_EXECUTE_DONE) {
            m_commandState = COMMAND_STATE_DONE;
        }
    }
        break;
    case COMMAND_STATE_DONE: {

    }
        break;
    }
}

void ApplicationController::storeButtonState(int btnID, bool pressed)
{
    m_buttonList[btnID]->setPressed(pressed);
}

void ApplicationController::updateInputState() {
    checkInput();
    for(int i=0;i< MAX_BUTTON; i++) {
        m_buttonList[i]->checkState();
    }
}

MACHINE_STATE ApplicationController::stateMachine() {
    return m_machineState;
}

void ApplicationController::getCurrentPosition(float* listAngles, int* numMotor)
{
    if(m_robot == NULL) return;
    m_robot->currentAngle(listAngles,numMotor);
}

void ApplicationController::getCurrentArmLength(float* listArmLength, int* numArm)
{
    if(m_robot == NULL) return;
    m_robot->armLength(listArmLength,numArm);
}

void ApplicationController::getChessBoardParams(float* listParam, int* numParam)
{
    *numParam = 3;
    listParam[0] = m_chessBoard->getChessBoardPosX();
    listParam[1] = m_chessBoard->getChessBoardPosY();
    listParam[2] = m_chessBoard->getChessBoardSize();
}

void ApplicationController::setMachineState(MACHINE_STATE machineState) {
    if(machineState != m_machineState) {
        m_machineState = machineState;
        this->printf("APP STATE: %d\r\n",m_machineState);
    }
}

void ApplicationController::executeCommand(char* command) {
    this->printf("Command: [%s]\r\n",command);
    if(command[0] == '_') {
        if(strlen(command)>=5){
            char commandID[8];
            commandID[0] = command[1];
            commandID[1] = command[2];
            commandID[2] = command[3];
            commandID[3] = command[4];
            commandID[4] = 0;
            int comCommandID = atoi(commandID); // command from PC, must response
            if(m_comCommandID > comCommandID) {
                this->printf("_%04d NOK",comCommandID);
            } else {
                m_comCommandID = comCommandID;
                this->printf("_%04d ACK",comCommandID);
            }
        }
    }
    else if(command[0] == 'b' && strlen(command)>=2 && command[1]>='0') {
        if(command[1]-'0' < MAX_BUTTON) {
            int buttonID = command[1]-'0';
            this->printf("B%c %s\r\n",command[1],
                m_buttonList[buttonID]->buttonState() == BUTTON_NOMAL?
                "NORMAL":"PRESSED");
        }
        else {
            this->printf("B%c INVALID\r\n",command[1]);
        }        
    }
    else if(command[0] == 's' && strlen(command)>=2 && command[1]>='0') {
        if(command[1]-'0' < MAX_MOTOR) {
            int motorID = command[1]-'0';
            this->printf("M%c %d\r\n",command[1],
                m_robot->currentStep(motorID));
        }
        else {
            this->printf("B%c INVALID\r\n",command[1]);
        }        
    }
    else if(command[0] == 'e') {
        this->enableEngine(true);
    }
    else if(command[0] == 'd') {
        this->enableEngine(false);
    }
    else if(command[0] == 'r') {
        goToReadyPosition();
    }
    else if(command[0] == 'm' && strlen(command)>=5) {
        calculateSequenceMove(command[2]-'0',command[1]-'0', 
            command[3] == 'u'?0:45, command[4] == 'c');
    }
    else if(command[0] == 'h') {
        if(strlen(command)>=2 && command[1] == 'a') {
            goToHome(MAX_MOTOR);
        }
        else if(strlen(command)>=2 && command[1] >= '0' && command[1] <= '5')
        {
            goToHome(command[1]-'0');
        }
    }
    else if(command[0] == 'c' && strlen(command)>=3) {
        executeSequence(MOVE_NORMAL, command[2]-'0',command[1]-'0',
                0,7);
    }else if(command[0] == 'c' && strlen(command)>=2 && command[1] == 'n' ) {
        executeSequence(MOVE_CASTLE, 3, 0,
                0,0);
    }else if(command[0] == 'c' && strlen(command)>=2 && command[1] == 'l' ) {
        executeSequence(MOVE_CASTLE, 3, 0,
                7,0);
    }else if(command[0] == 'a' && strlen(command)>=2 && command[1] == 't' ) {
        executeSequence(MOVE_ATTACK, 0, 0,
                4,4);
    }else if(command[0] == 'p' && command[1] == 'p' ) {
        executeSequence(MOVE_PASTPAWN, 2, 4,
                3,5);
    }else if(command[0] == 'p' && strlen(command)>=3 && command[1] == 'r' && command[2] == 'a' ) {
        executeSequence(MOVE_PROMOTE, 2, 6,
                3, 7, 'q');
    }else if(command[0] == 'p' && strlen(command)>=3 && command[1] == 'r' && command[2] == 'n' ) {
        executeSequence(MOVE_PROMOTE, 2, 6,
                2, 7);
    }else if(command[0] == 'p' && strlen(command)>=2 && command[1] >= '0' && command[1] <= '5')
    {
        int motorID = command[1]-'0';
        char angleStr[8];
        angleStr[0] = command[3];
        angleStr[1] = command[4];
        angleStr[2] = command[5];
        angleStr[3] = command[6];
        angleStr[4] = 0;
        float angle = atoi(angleStr);

        char stepTimeStr[8];
        stepTimeStr[0] = command[8];
        stepTimeStr[1] = command[9];
        stepTimeStr[2] = command[10];
        stepTimeStr[3] = command[11];
        stepTimeStr[4] = 0;
        int stepTime = atoi(stepTimeStr);

        bool isRelative = command[13] == 'r';

        m_robot->requestGoPosition(motorID,
                m_robot->angleToStep(motorID, angle),
                stepTime, isRelative);
        setMachineState(MACHINE_EXECUTE_COMMAND);
    }
    else {
        this->printf("Unknown Command\r\n");
    }
}

void ApplicationController::executeSingleMotor(int motorID,
                         int targetStep,
                         int direction,
                         int stepTime)
{
    clearSequenceMove();
    int listCurrentStep[MAX_MOTOR];
    float captureStep;
    int numMotor;
    m_robot->currentStep(listCurrentStep,&numMotor);
    for(int motor = 0; motor < numMotor; motor++){
        if(motor == motorID) listCurrentStep[motor] += targetStep*direction;
    }
    initSequenceMove(MAX_MOTOR);
}

bool ApplicationController::inverseKinematic(float x, float y, float a1, float a2, float* p1, float* p2)
{
    if(sqrtf(x*x+y*y) > fabs(a1+a2) || sqrtf(x*x+y*y) < fabs(a1-a2)) return false;
    *p2 = acos((x*x+y*y-a1*a1-a2*a2)/(2*a1*a2));
    *p1 = atan(y/x) - atan((a2*sin(*p2))/(a1+a2*cos(*p2)));
    *p1 =  *p1 < 0?*p1+M_PI:*p1;
#ifdef DEBUG_KINEMATIC
    this->printf("IK: x[%.2f] y[%.2f] a1[%.2f] a2[%.2f] q1[%.2f] q2[%.2f]\r\n",
                 x,y,a1,a2,
                 (*p1/M_PI*180.0f),(*p2/M_PI*180.0f));
#endif
    return true;
}

void ApplicationController::forwardKinematic(float a1, float a2, float p1, float p2, float* x, float* y) {
    *x = a1 * cos(p1) + a2 * cos(p1 + p2);
    *y = a1 * sin(p1) + a2 * sin(p1 + p2);
#ifdef DEBUG_KINEMATIC
    this->printf("FK: x[%.2f] y[%.2f] a1[%.2f] a2[%.2f] q1[%.2f] q2[%.2f]\r\n",
                 *x,*y,a1,a2,
                 (p1/M_PI*180.0f),(p2/M_PI*180.0f));
#endif
}

void ApplicationController::calculatePolygonEdge(float upAngleInDegree, float* edge, float* angle)
{
    float upAngle = (45-upAngleInDegree)/180.0f*M_PI;
    float a1, a2, q1, q2, xFK, yFK;
    float bufAngle = 0;
    float lastEdge;
    a1 = m_robot->armLength(MOTOR_ARM2);
    a2 = m_robot->armLength(MOTOR_ARM3);
    q1 = 0;
    q2 = (180.0f - m_robot->homeAngle(MOTOR_ARM3))/180.0f*M_PI + bufAngle;
    forwardKinematic(a1,a2,q1,q2,&xFK,&yFK);
    lastEdge = sqrt(xFK*xFK + yFK*yFK);
    bufAngle = acosf((a2*a2 + lastEdge*lastEdge - a1*a1)/(2*a2*lastEdge));
    a1 = lastEdge;
    a2 = m_robot->armLength(MOTOR_ARM4) * cos(upAngle)
            + m_robot->armLength(MOTOR_ARM5);
    q1 = 0;
    q2 = (180.0f - m_robot->homeAngle(MOTOR_ARM4))/180.0f*M_PI + bufAngle;
    forwardKinematic(a1,a2,q1,q2,&xFK,&yFK);
    lastEdge = sqrt(xFK*xFK + yFK*yFK);
    bufAngle = acosf((a2*a2 + lastEdge*lastEdge - a1*a1)/(2*a2*lastEdge));
    *edge = lastEdge;
    *angle = bufAngle;
}

void ApplicationController::calculateJoints(float xPos, float yPos, float upAngleInDegree, int* jointSteps)
{    
    float a1 = m_robot->armLength(MOTOR_ARM1);
    float a2 = 0;
    float q2Offset = 0;
#ifdef DEBUG_KINEMATIC
    this->printf("=== calculatePolygonEdge\r\n");
#endif
    calculatePolygonEdge(upAngleInDegree,&a2,&q2Offset);
#ifdef DEBUG_KINEMATIC    
    this->printf("calculatePolygonEdge ===\r\n");
#endif
    float q1 = 0;
    float q2 = 0;
#ifdef DEBUG_KINEMATIC
    this->printf("xPos[%d]\r\n",(int)xPos);
    this->printf("yPos[%d]\r\n",(int)yPos);
    this->printf("a1[%d]\r\n",(int)a1);
    this->printf("a2[%d]\r\n",(int)a2);
    this->printf("q2Offset[%d]\r\n",(int)(q2Offset*180.f/M_PI));
#endif
    inverseKinematic(xPos, yPos, a1, a2, &q1, &q2);

    float xPosFK = 0, yPosFK = 0;
    forwardKinematic(a1,a2,q1,q2,&xPosFK,&yPosFK);
    if(xPosFK*xPos < 0 || yPosFK * yPos < 0) q1 = q1 - M_PI;
#ifdef DEBUG_KINEMATIC
    this->printf("arm1Angle[%d]=[%d] arm2Angle[%d]=[%d] arm3Angle[%d] q2Offset[%d]\r\n",
                 (int)(q1/M_PI*180.0f),
                 (int)((M_PI/2 + q1)/M_PI*180.0f),
                 (int)((q2)/M_PI*180.0f),
                 (int)((M_PI - q2 - q2Offset)/M_PI*180.0f),
                 (int)(upAngleInDegree),
                 (int)(q2Offset/M_PI*180.0f));
#endif
    jointSteps[MOTOR_ARM1] = m_robot->angleToStep(
                MOTOR_ARM1,
                (M_PI/2 + q1)/M_PI*180.0f);
    jointSteps[MOTOR_ARM2] = m_robot->angleToStep(
                MOTOR_ARM2,
                (M_PI - q2 - q2Offset)/M_PI*180.0f +
                m_robot->homeAngle(MOTOR_ARM2));
    jointSteps[MOTOR_ARM3] = 0;
    jointSteps[MOTOR_ARM4] = 0;
    jointSteps[MOTOR_ARM5] = m_robot->angleToStep(
                MOTOR_ARM5,
                upAngleInDegree);
}

Command ApplicationController::calculateNextPointInLine(Point currPos, Point targetPos)
{
    Command nextPoint;
    float dx = targetPos.x - currPos.x;
    float dy = targetPos.y - currPos.y;
    float angle = int(dx*10) == 0 ? M_PI_2:atan(dy/dx);
    nextPoint.x = targetPos.x + m_minSpace * cos(angle);
    nextPoint.y = targetPos.y + m_minSpace * sin(angle);
    return nextPoint;
}

Point ApplicationController::currentPos()
{
    Point resultPos;
    float a1 = m_robot->armLength(MOTOR_ARM1);
//    float a2, a21, a22;
//    float q2Offset = 0;
    float listCurrentAngle[MAX_MOTOR];
    int numMotor;
    m_robot->currentAngle(listCurrentAngle,&numMotor);
//    a21 = m_robot->armLength(MOTOR_ARM2);
//    a22 = m_robot->armLength(MOTOR_ARM3) +
//            m_robot->armLength(MOTOR_ARM4) *
//            sin(listCurrentAngle[MOTOR_ARM5]);
//    a2 = sqrt(a21*a21+a22*a22-2.0f*a21*a22*
//            cos(listCurrentAngle[MOTOR_ARM3]));
    float a2 = 0;
    float q2Offset = 0;
    calculatePolygonEdge(listCurrentAngle[MOTOR_ARM5],&a2,&q2Offset);
    float xPosFK = 0, yPosFK = 0;
    float q1 = listCurrentAngle[MOTOR_ARM1];
    float q2 = 180.0f - q2Offset/M_PI*180.0f;
    forwardKinematic(a1,a2,q1,q2,&xPosFK,&yPosFK);
    resultPos.x = xPosFK;
    resultPos.y = yPosFK;
    return resultPos;
}

void ApplicationController::goToHome(int motorID)
{
    specificPlatformGohome(motorID);
    m_robot->requestGoHome(motorID);
    setMachineState(MACHINE_EXECUTE_HOME);
}

void ApplicationController::goToReadyPosition() {
    int jointSteps[MAX_MOTOR];
    jointSteps[MOTOR_CAPTURE] = 0;
    jointSteps[MOTOR_ARM1] = m_robot->angleToStep(MOTOR_ARM1,0);
    jointSteps[MOTOR_ARM2] = m_robot->angleToStep(MOTOR_ARM2,90+m_robot->homeAngle(MOTOR_ARM2));
    jointSteps[MOTOR_ARM3] = 0;
    jointSteps[MOTOR_ARM4] = 0;
    jointSteps[MOTOR_ARM5] = m_robot->angleToStep(MOTOR_ARM5,0);
    m_robot->setMoveTarget(jointSteps);
    m_robot->moveToTarget(MAX_MOTOR);
    setMachineState(MACHINE_EXECUTE_POSITION);
}

void ApplicationController::executeSequence(
        MOVE_TYPE moveType,
        int startCol, int startRow,
        int stopCol, int stopRow,
        char promotePiece) {
    this->printf("Go to Pos [%d,%d] to [%d,%d] \r\n",
                 startCol, startRow, stopCol, stopRow);

    // Attack: Move piece out -> Move attack piece -> Return to prepare
    // No attack: Move attack piece -> Return to prepare
    // Castle: Move king -> Move rook -> Return to prepare
    // Promote: Move pawn -> Move promote piece -> Return to prepare
    switch (moveType) {
    case MOVE_NORMAL:
        calculateSequenceMoveNormal(startCol, startRow, stopCol, stopRow);
        break;
    case MOVE_ATTACK:
        calculateSequenceAttack(startCol, startRow, stopCol, stopRow);
        break;
    case MOVE_PASTPAWN:
        calculateSequencePastPawn(startCol, startRow, stopCol, stopRow);
        break;
    case MOVE_CASTLE:
        calculateSequenceCastle(startCol, startRow, stopCol, stopRow);
        break;
    case MOVE_PROMOTE:
        calculateSequencePromotePiece(startCol, startRow, stopCol, stopRow, promotePiece);
        break;
    }

    setMachineState(MACHINE_EXECUTE_COMMAND);
}

void ApplicationController::calculateSequenceMove(int startCol, int startRow, int upAngleInDegree, bool isCapture)
{
    printf("ApplicationController::calculateSequenceMove\r\n");
    Point targetPoint = m_chessBoard->convertPoint(startRow,startCol);
    int jointSteps[MAX_MOTOR];
    clearSequenceMove();
    
    jointSteps[MOTOR_CAPTURE] = isCapture?415:0;
    // Inverse axis Oxy -> Oyx
    calculateJoints(targetPoint.y, targetPoint.x, upAngleInDegree, jointSteps);

    m_robot->setMoveTarget(jointSteps);
    m_robot->moveToTarget(MAX_MOTOR);
    setMachineState(MACHINE_EXECUTE_POSITION);
}

void ApplicationController::calculateSequenceMoveNormal(int startCol, int startRow,
                     int stopCol, int stopRow)
{
    // append move from start -> stop -> standy
    Point startPoint = m_chessBoard->convertPoint(startRow,startCol);
    printf("start[%d,%d] to Point(%f,%f)\r\n",
           startRow,startCol,
           startPoint.x,startPoint.y);
    Point stopPoint = m_chessBoard->convertPoint(stopRow,stopCol);
    clearSequenceMove();
    appendSequenceMove(startPoint, stopPoint);
    setMachineState(MACHINE_EXECUTE_COMMAND);
    m_commandSequenceState = COMMAND_SEQUENCE_STATE_INIT;
}

void ApplicationController::calculateSequenceAttack(int startCol, int startRow,
                     int stopCol, int stopRow)
{
    // get free drop point
    // append move from stop -> drop -> start -> stop -> standby
    Point dropPoint = m_chessBoard->getFreeDropPoint(ZONE_MACHINE);
    Point startPoint = m_chessBoard->convertPoint(startRow,startCol);
    Point stopPoint = m_chessBoard->convertPoint(stopRow,stopCol);

    clearSequenceMove();
    appendSequenceMove(stopPoint, dropPoint);
    appendSequenceMove(startPoint, stopPoint);
    initSequenceMove(MAX_MOTOR);
}

void ApplicationController::calculateSequencePastPawn(int startCol, int startRow,
                     int stopCol, int stopRow)
{
    // append move from attack pawn -> drop -> start -> stop -> standby
    Point dropPoint = m_chessBoard->getFreeDropPoint(ZONE_MACHINE);
    Point pawnPoint = m_chessBoard->convertPoint(startRow,stopCol);
    Point startPoint = m_chessBoard->convertPoint(startRow,startCol);
    Point stopPoint = m_chessBoard->convertPoint(stopRow,stopCol);
    clearSequenceMove();
    appendSequenceMove(pawnPoint, dropPoint);
    appendSequenceMove(startPoint, stopPoint);
    initSequenceMove(MAX_MOTOR);
}

void ApplicationController::calculateSequencePromotePiece(int startCol, int startRow,
                     int stopCol, int stopRow, char promotePiece)
{
    // append move from attack piece -> drop -> promote -> stop -> start -> drop -> standby
    Point promotePiecePoint = m_chessBoard->getFreeDropPoint(ZONE_GUEST,promotePiece);
    Point dropPiecePoint = m_chessBoard->getFreeDropPoint(ZONE_GUEST);
    Point startPoint = m_chessBoard->convertPoint(startRow,startCol);
    Point stopPoint = m_chessBoard->convertPoint(stopRow,stopCol);

    clearSequenceMove();
    if(startCol != stopCol){
        // pawn attack piece, move attack piece -> drop
        Point dropPoint = m_chessBoard->getFreeDropPoint(ZONE_MACHINE);
        appendSequenceMove(stopPoint, dropPoint);
    }
    appendSequenceMove(promotePiecePoint, stopPoint);
    appendSequenceMove(startPoint, dropPiecePoint);
    initSequenceMove(MAX_MOTOR);
}

void ApplicationController::calculateSequenceCastle(int kingCol, int kingRow,
                                                    int rookCol, int rookRow)
{

    // append move king -> new point -> rook -> new point
    Point kingPoint = m_chessBoard->convertPoint(kingRow,kingCol);
    Point rookPoint = m_chessBoard->convertPoint(rookRow,rookCol);
    Point kingNewPoint;
    Point rookNewPoint;
    if(kingCol > rookCol) {
        kingNewPoint = m_chessBoard->convertPoint(kingRow,kingCol-2);
        rookNewPoint = m_chessBoard->convertPoint(kingRow,kingCol-1);
    } else {
        kingNewPoint = m_chessBoard->convertPoint(kingRow,kingCol+2);
        rookNewPoint = m_chessBoard->convertPoint(kingRow,kingCol+1);
    }
    clearSequenceMove();
    appendSequenceMove(rookPoint, rookNewPoint, true);
    appendSequenceMove(kingPoint, kingNewPoint);
    initSequenceMove(MAX_MOTOR);
}

float ApplicationController::distance(float x1, float y1, float x2, float y2)
{
    float dx = x2-x1;
    float dy = y2-y1;
    return sqrt(dx*dx + dy*dy);
}

void ApplicationController::clearSequenceMove() {
    m_numCommand = 0;
    m_curCommandId = 0;
}
void ApplicationController::appendSequenceMove(Point start, Point stop, bool straightMove) {
    if(!straightMove) {
        float upAngles[6] = {0.0f,45.0f,0.0f,
                              0.0f,45.0f,0.0f};
        Point position[6] = {start,start,start,
                              stop,stop,stop};
        int captureStep[6] = {0,415,415,
                               415,0,0};
        int numStep = 6;
        for(int seqStep = 0; seqStep < numStep; seqStep++)
        {
            m_sequenceCommand[seqStep].x = position[seqStep].x;
            m_sequenceCommand[seqStep].y = position[seqStep].y;
            m_sequenceCommand[seqStep].updownAngle = upAngles[seqStep];
            m_sequenceCommand[seqStep].captureStep = captureStep[seqStep];
            m_sequenceCommand[seqStep].type = COMMAND_NORMAL;
            m_numCommand++;
        }
    } else {
        float upAngles[4] = {0.0f,45.0f,45.0f,
                              0.0f};
        Point position[4] = {start,start,stop,stop};
        int captureStep[4] = {0,415,0,0};
        int numStep = 4;
        for(int seqStep = 0; seqStep < numStep; seqStep++)
        {
            m_sequenceCommand[seqStep].x = position[seqStep].x;
            m_sequenceCommand[seqStep].y = position[seqStep].y;
            m_sequenceCommand[seqStep].updownAngle = upAngles[seqStep];
            m_sequenceCommand[seqStep].captureStep = captureStep[seqStep];
            m_sequenceCommand[seqStep].type = seqStep != 2 ?
                        COMMAND_NORMAL : COMMAND_LINE;
            m_numCommand++;
        }
    }
}

void ApplicationController::initSequenceMove(int numberOfJoints) {
    m_robot->moveToTarget(numberOfJoints);
}

void ApplicationController::executeSmoothMotionLoop(int motorID)
{
    m_robot->executeSmoothMotion(motorID);
}

