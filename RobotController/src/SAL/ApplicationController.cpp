#include "ApplicationController.h"
#include "Button.h"
#include "Robot.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define ROBOT_VERSION "1.0.0"
ApplicationController::ApplicationController()
{
    for(int i=0;i< MAX_BUTTON; i++) {
        m_buttonList[i] = new Button(this);
    }
    m_robot = new Robot(this);
    m_chessBoard = new ChessBoard();
    m_machineState = MACHINE_WAIT_COMMAND;
    m_appTimer = 0;
    m_engineEnabled = false;
    m_calibrationReceivedCount = 0;
    m_calibrationChessboardCount = 0;
    m_calibrationRightDropzoneCount = 0;
    m_calibrationLeftDropzoneCount = 0;
    memset(m_calibrationChessboardCalibrated, 0, sizeof(m_calibrationChessboardCalibrated));
    memset(m_calibrationRightDropzoneCalibrated, 0, sizeof(m_calibrationRightDropzoneCalibrated));
    memset(m_calibrationLeftDropzoneCalibrated, 0, sizeof(m_calibrationLeftDropzoneCalibrated));
    m_calibrationInProgress = false;
}

ApplicationController::~ApplicationController() {

}

void ApplicationController::loop() {
    m_appTimer++;
#ifdef DEBUG_APP
    this->printf("APP Timer[%d] m_machineState[%d]\r\n",m_appTimer,m_machineState);
#endif
    readCommand();
    switch(m_machineState) {
        case MACHINE_WAIT_COMMAND: {            
            break;
        }
        case MACHINE_EXECUTE_CALIBRATION:
        case MACHINE_EXECUTE_HOME:
        case MACHINE_EXECUTE_POSITION:
        {
            if(m_robot->loop() == ROBOT_EXECUTE_DONE)
                setMachineState(MACHINE_EXECUTE_COMMAND_DONE);
            break;
        }
        case MACHINE_EXECUTE_COMMAND: {
            if(executeCommandSequenceLoop() == COMMAND_SEQUENCE_STATE_DONE)
                setMachineState(MACHINE_EXECUTE_POSITION_STANDBY);
            break;
        }
        case MACHINE_EXECUTE_POSITION_STANDBY: {
            goToReadyPosition();
            break;
        }
        case MACHINE_EXECUTE_COMMAND_DONE: {
#ifdef DEBUG_COMMAND
            this->printf("_%04d EXECUTE DONE\r\n",m_comCommandID);
#endif
            setMachineState(MACHINE_WAIT_COMMAND);        
            break;
        }
    }
}

void ApplicationController::readCommand()
{
    memset(m_commandRead,0,sizeof(m_commandRead));
    int incomingBytes = readSerial(m_commandRead,sizeof(m_commandRead));
    if(incomingBytes>0) executeCommand(m_commandRead);
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
    int commandType = m_sequenceCommand[m_curCommandId].type;
#ifdef DEBUG_COMMAND
   printf("executeCommandLoop state[%d] m_curCommandId[%d]\r\n",
          commandType,m_curCommandId);
#endif
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
        calculateJoints(m_sequenceCommand[m_curCommandId].x,
                        m_sequenceCommand[m_curCommandId].y,
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
        m_curPos = currentPos();
        m_tarPos = {m_sequenceCommand[m_curCommandId].x,
                        m_sequenceCommand[m_curCommandId].y,
                        0};
        m_curPointInCommand = 0;
        m_numPointInCommand = (uint32_t)(distance(m_curPos.x,m_curPos.y,m_tarPos.x,m_tarPos.y))/
                (uint32_t)m_minSpace;
        m_commandState = COMMAND_STATE_EXECUTE;
    }
        break;
    case COMMAND_STATE_EXECUTE: {
        int jointSteps[MAX_MOTOR];
        if(m_curPointInCommand < m_numPointInCommand) {
            m_nextPoint = calculateNextPointInLine(m_curPos,m_tarPos,(float)m_curPointInCommand);
            m_commandState = COMMAND_STATE_EXECUTE_THEN_RECAL;
        } else {
            m_nextPoint.x = m_sequenceCommand[m_curCommandId].x;
            m_nextPoint.y = m_sequenceCommand[m_curCommandId].y;
            m_nextPoint.updownAngle = m_sequenceCommand[m_curCommandId].updownAngle;
            m_nextPoint.captureStep = m_sequenceCommand[m_curCommandId].captureStep;
            m_commandState = COMMAND_STATE_EXECUTE_THEN_DONE;
        }
        m_curPointInCommand++;
        calculateJoints(m_nextPoint.x,
                        m_nextPoint.y,
                        m_nextPoint.updownAngle,
                        jointSteps);
        jointSteps[MOTOR_CAPTURE] = m_sequenceCommand[m_curCommandId].captureStep;
        m_robot->setMoveTarget(jointSteps);
        m_robot->moveToTarget(MAX_MOTOR);
    }
        break;
    case COMMAND_STATE_EXECUTE_THEN_RECAL: {
        if(m_robot->loop() == ROBOT_EXECUTE_DONE) {
            m_commandState = COMMAND_STATE_EXECUTE;
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
#ifdef DEBUG_COMMAND        
        this->printf("APP STATE: %d\r\n",m_machineState);
#endif
    }
}

void ApplicationController::sendCalibrationProgress() {
    if(m_calibrationInProgress) {
        this->printf("R%d/16 L%d/16 C%d/64\r\n", 
                    m_calibrationRightDropzoneCount,
                    m_calibrationLeftDropzoneCount,
                    m_calibrationChessboardCount);
    }
}

void ApplicationController::executeCommand(char* command) {
#ifdef DEBUG_COMMAND
    this->printf("Command: [%s]\r\n",command);
#endif
    if(command[0] == 'v') {
        this->printf("[v]%s\r\n",ROBOT_VERSION);
    } else if(strlen(command)>=3 && command[0] == 'c'&&command[1] == 'm'&&command[2] == 'd') {
        this->printf("[cmd]%04d\r\n",m_comCommandID);
    } else if(command[0] == '_') {
        if(strlen(command)>=5){
            char commandID[8];
            commandID[0] = command[1];
            commandID[1] = command[2];
            commandID[2] = command[3];
            commandID[3] = command[4];
            commandID[4] = 0;
            int comCommandID = atoi(commandID); // command from PC, must response
            if (m_comCommandID == comCommandID) {
                this->printf("[_%04d]%s\r\n",comCommandID,
                    m_machineState == MACHINE_WAIT_COMMAND?"DONE":"WAIT");
            } else {
                this->printf("[_%04d]INVALID\r\n",comCommandID);
            }
        }
    }
    else if(command[0] == 'b' && strlen(command)>=2 && command[1]>='0') {
        if(command[1]-'0' < MAX_BUTTON) {
            int buttonID = command[1]-'0';
            this->printf("[B%c] %s\r\n",command[1],
                m_buttonList[buttonID]->buttonState() == BUTTON_NOMAL?
                "NORMAL":"PRESSED");
        }
        else {
            this->printf("[B%c] INVALID\r\n",command[1]);
        }        
    }
    else if(command[0] == 's' && strlen(command)>=2 && command[1]>='0') {
        if(command[1]-'0' < MAX_MOTOR) {
            int motorID = command[1]-'0';
            this->printf("[M%c] %d\r\n",command[1],
                m_robot->currentStep(motorID));
        }
        else {
            this->printf("[M%c] INVALID\r\n",command[1]);
        }        
    }
    else if(command[0] == 'e' && strlen(command)>=2) {
        if(command[1] == 'e') {
            this->enableEngine(true);
            setMachineState(MACHINE_EXECUTE_COMMAND_DONE);
            this->printf("[ee] Engine enabled\r\n");
        } else if(command[1] == 'd'){
            this->enableEngine(false);
            setMachineState(MACHINE_EXECUTE_COMMAND_DONE);
            this->printf("[ed] Engine disabled\r\n");
        } else if(command[1] == 's') {
            this->printf("[es] %s\r\n", m_engineEnabled?"Enabled":"Disabled");
            setMachineState(MACHINE_EXECUTE_COMMAND_DONE);
        }
    }
    else if(command[0] == 'd') {
        m_comCommandID ++;
        this->enableEngine(false);
        setMachineState(MACHINE_EXECUTE_COMMAND_DONE);
        this->printf("[d] Engine disabled\r\n");
    }
    else if(command[0] == 'r' && strlen(command)>=2) {
        if(command[1] == 'a') {
            m_comCommandID ++;
            goToReadyPosition();
            this->printf("[r] Ready position confirmed\r\n");
        }
    }
    else if(command[0] == 'm' && strlen(command)>=2) {
        if(command[1] == 'l') {
            m_comCommandID ++;
            calculateSequenceMoveStraight(0,0,7,0);
            this->printf("[m] Straight move confirmed\r\n");
        } else if(command[1] == 'c') {
            m_comCommandID ++;
            calculateSequenceMoveStraight(0,0,7,0);
            this->printf("[m] Straight move confirmed\r\n");
        }
    }
    else if(command[0] == 'h' && strlen(command)>=2) {
        if(command[1] == 'a') {
            m_comCommandID ++;
            goToHome(MAX_MOTOR);
            this->printf("[ha] Home position confirmed\r\n");
        } else if(command[1] == 's') {
            bool allAtHome = true;
            for(int motorID=0; motorID< MAX_MOTOR; motorID++) {
                if(!m_robot->isLimitReached(motorID,MOTOR_LIMIT_HOME)) {
                    allAtHome = false;
                    break;
                }
            }
            this->printf("[hs] %s\r\n", allAtHome ? "TRUE" : "FALSE");
        } else if(command[1] >= '0' && command[1] <= '5')
        {
            m_comCommandID ++;
            goToHome(command[1]-'0');
            this->printf("[h%c] Home position confirmed\r\n", command[1]);
        }
    }
    else if(command[0] == 'l' && strlen(command)>=2) {        
        if( command[1] == 'c' && strlen(command)>=5) {
            m_comCommandID ++;
            int rowId,colId;
            if(sscanf(command, "lccbr%dc%d", &rowId, &colId) == 2) {
                // lccbr0c1
                Point chessBoardPoint = m_chessBoard->convertPoint(rowId,colId);
                this->printf("CB r[%d] c[%d] x[%d] y[%d]\r\n", 
                    rowId, 
                    colId,
                    (int)(chessBoardPoint.x*10.0f), 
                    (int)(chessBoardPoint.y*10.0f));
            } else if(sscanf(command, "lcdpr%dc%d", &rowId, &colId) == 2) {
                // lcdpr0c1
                Point dropZonePoint = m_chessBoard->convertDropPoint(rowId,colId,ZONE_PLAYER);
                this->printf("DP r[%d] c[%d] x[%d] y[%d]\r\n", 
                    rowId,
                    colId,
                    (int)(dropZonePoint.x*10.0f), 
                    (int)(dropZonePoint.y*10.0f));
            } else if(sscanf(command, "lcdbr%dc%d", &rowId, &colId) == 2) {
                // lcdbpr0c1
                Point dropZonePoint = m_chessBoard->convertDropPoint(rowId,colId,ZONE_BOT);
                this->printf("DB r[%d] c[%d] x[%d] y[%d]\r\n", 
                    rowId,
                    colId,
                    (int)(dropZonePoint.x*10.0f), 
                    (int)(dropZonePoint.y*10.0f));
            }
            setMachineState(MACHINE_EXECUTE_COMMAND_DONE);
        } if( command[1] == 'l' && strlen(command)>=5) {
            // llcbr0c1y12345x67890
            m_comCommandID ++;
            if(command[2] == 'c' && command[3] == 'b') {
                int rowId,colId,xPos,yPos;
                if(sscanf(command, "llcbr%dc%dx%dy%d", &rowId, &colId, &xPos, &yPos) == 4) {
                    m_chessBoard->setCalibChessBoardPoint(rowId,colId,
                    {(float)xPos/10.0f,(float)yPos/10.0f,0,true});
                }
            } else if(command[2] == 'd' && (command[3] == 'p' || command[3] == 'b')) {
                // lldpr0c1x12345y67890
                // lldbr0c1x12345y67890
                int rowId,colId,xPos,yPos;
                if(sscanf(command, "lldpr%dc%dx%dy%d", &rowId, &colId, &xPos, &yPos) == 4) {
                    m_chessBoard->setCalibDropZonePoint(rowId,colId,ZONE_PLAYER,
                    {(float)xPos/10.0f,(float)yPos/10.0f,0,true});
                } else if(sscanf(command, "lldbr%dc%dx%dy%d", &rowId, &colId, &xPos, &yPos) == 4) {
                    m_chessBoard->setCalibDropZonePoint(rowId,colId,ZONE_BOT,
                    {(float)xPos/10.0f,(float)yPos/10.0f,0,true});
                }
            }
            setMachineState(MACHINE_EXECUTE_COMMAND_DONE);
        } else if( command[1] == 's') {
            m_comCommandID ++;
            Point calPosistion = calibPos();
            this->printf("RS %d %d\r\n", 
                (int)(calPosistion.x*10.0f), 
                (int)(calPosistion.y*10.0f));
            setMachineState(MACHINE_EXECUTE_COMMAND_DONE);
        } else if( command[1] == 'r') {
            m_comCommandID ++;
            goToCalibPosition();
            this->printf("[lr] Go to calib confirmed\r\n");
        } else if(command[1] == 'a') {
            m_comCommandID ++;
            calibToHome(MAX_MOTOR);
            this->printf("[la] Calib to home confirmed\r\n");
        }
        else if(command[1] >= '0' && command[1] <= '5')
        {
            m_comCommandID ++;
            calibToHome(command[1]-'0');
            this->printf("[l%d] Calib to home confirmed\r\n", command[1]-'0');
        }
    }
    else if(command[0] == 'c' && strlen(command)>=5) {
        m_comCommandID ++;
        executeSequence(MOVE_NORMAL, command[2]-'0',command[1]-'0',
                command[4]-'0',command[3]-'0');
        this->printf("[%s] Normal seq confirmed\r\n", command);
    }else if(command[0] == 'C' && strlen(command)>=7 && command[1] == 'S' && command[2] == 'T') {
        m_comCommandID ++;
        executeSequence(MOVE_CASTLE, command[4]-'0',command[3]-'0',
                command[6]-'0',command[5]-'0');
        this->printf("[%s] Castle confirmed\r\n", command);
    }else if(command[0] == 'a' && strlen(command)>=5) {
        m_comCommandID ++;
        executeSequence(MOVE_ATTACK, command[2]-'0',command[1]-'0',
                command[4]-'0',command[3]-'0');
        this->printf("[%s] Attack confirmed\r\n", command);
    }else if(command[0] == 'p' && strlen(command)>=6 && command[1] == 'p') {
        m_comCommandID ++;
        executeSequence(MOVE_PASTPAWN, command[3]-'0',command[2]-'0',
                command[5]-'0',command[4]-'0');
        this->printf("[%s] Past pawn confirmed\r\n", command);
    }else if(command[0] == 'p' && strlen(command)>=6) {
        m_comCommandID ++;
        executeSequence(MOVE_PROMOTE, command[3]-'0',command[2]-'0',
                command[5]-'0',command[4]-'0',command[1]);
        this->printf("[%s] Promote confirmed\r\n", command);

    }else if(command[0] == 't' && strlen(command)>=2)
    {
        m_comCommandID ++;
        int xPos,yPos;
        int row,col;
        int cmd;
        if(sscanf(command, "tx%dy%d", &xPos, &yPos) == 2) {
            gotoPosition((float)xPos/10.0f, (float)yPos/10.0f, 0);
            this->printf("[%s] Pos confirmed\r\n", command);
        } else if(sscanf(command, "ts%c",&cmd) == 1) {
            this->printf("[%s] TS confirmed\r\n", command);
            Point currentPosition = currentPos();
            this->printf("TS x[%d] y[%d] m[1][%d] m[2][%d] m[5][%d]\r\n",
                (int)(currentPosition.x*10.0f),
                (int)(currentPosition.y*10.0f),
                m_robot->currentStep(1),
                m_robot->currentStep(2),
                m_robot->currentStep(5)
            );
        }
    }else if(strncmp(command, "CALIB_START", 11) == 0) {
        // Start calibration data transmission
        m_calibrationReceivedCount = 0;
        m_calibrationChessboardCount = 0;
        m_calibrationRightDropzoneCount = 0;
        m_calibrationLeftDropzoneCount = 0;
        memset(m_calibrationChessboardCalibrated, 0, sizeof(m_calibrationChessboardCalibrated));
        memset(m_calibrationRightDropzoneCalibrated, 0, sizeof(m_calibrationRightDropzoneCalibrated));
        memset(m_calibrationLeftDropzoneCalibrated, 0, sizeof(m_calibrationLeftDropzoneCalibrated));
        m_calibrationInProgress = true;
        m_chessBoard->resetCalibrationToFormula(); // Reset all to formula values initially
        this->printf("CALIB_START_ACK\r\n");
    }
    else if(strncmp(command, "SC r", 4) == 0) {
        // Set chessboard calibration point: SC r[row] c[col] x[x] y[y]
        if(m_calibrationInProgress) {
            int row, col, x, y;
            if(sscanf(command, "SC r%d c%d x%d y%d", &row, &col, &x, &y) == 4) {
                if(row >= 0 && row < 8 && col >= 0 && col < 8) {
                    // Only update if this cell hasn't been calibrated yet
                    if(!m_calibrationChessboardCalibrated[row][col]) {
                        m_chessBoard->setCalibChessBoardPoint(row, col, {(float)x, (float)y, 0, true});
                        m_calibrationChessboardCalibrated[row][col] = true;
                        m_calibrationChessboardCount++;
                        m_calibrationReceivedCount++;
                        sendCalibrationProgress();
                    } else {
                        // Cell already calibrated, just update the value without incrementing counters
                        m_chessBoard->setCalibChessBoardPoint(row, col, {(float)x, (float)y, 0, true});
                    }
                }
            }
        }
    }
    else if(strncmp(command, "SR r", 4) == 0) {
        // Set right dropzone calibration point: SR r[row] c[col] x[x] y[y]
        if(m_calibrationInProgress) {
            int row, col, x, y;
            if(sscanf(command, "SR r%d c%d x%d y%d", &row, &col, &x, &y) == 4) {
                if(row >= 0 && row < 8 && col >= 0 && col < 2) {
                    // Only update if this cell hasn't been calibrated yet
                    if(!m_calibrationRightDropzoneCalibrated[row][col]) {
                        m_chessBoard->setCalibDropZonePoint(row, col, ZONE_BOT, {(float)x, (float)y, 0, true});
                        m_calibrationRightDropzoneCalibrated[row][col] = true;
                        m_calibrationRightDropzoneCount++;
                        m_calibrationReceivedCount++;
                        sendCalibrationProgress();
                    } else {
                        // Cell already calibrated, just update the value without incrementing counters
                        m_chessBoard->setCalibDropZonePoint(row, col, ZONE_BOT, {(float)x, (float)y, 0, true});
                    }
                }
            }
        }
    }
    else if(strncmp(command, "SL r", 4) == 0) {
        // Set left dropzone calibration point: SL r[row] c[col] x[x] y[y]
        if(m_calibrationInProgress) {
            int row, col, x, y;
            if(sscanf(command, "SL r%d c%d x%d y%d", &row, &col, &x, &y) == 4) {
                if(row >= 0 && row < 8 && col >= 0 && col < 2) {
                    // Only update if this cell hasn't been calibrated yet
                    if(!m_calibrationLeftDropzoneCalibrated[row][col]) {
                        m_chessBoard->setCalibDropZonePoint(row, col, ZONE_PLAYER, {(float)x, (float)y, 0, true});
                        m_calibrationLeftDropzoneCalibrated[row][col] = true;
                        m_calibrationLeftDropzoneCount++;
                        m_calibrationReceivedCount++;
                        sendCalibrationProgress();
                    } else {
                        // Cell already calibrated, just update the value without incrementing counters
                        m_chessBoard->setCalibDropZonePoint(row, col, ZONE_PLAYER, {(float)x, (float)y, 0, true});
                    }
                }
            }
        }
    }
    else if(strncmp(command, "CALIB_END", 9) == 0) {
        // End calibration data transmission
        if(m_calibrationInProgress) {
            m_calibrationInProgress = false;
            if(m_calibrationReceivedCount == 96) {
                // All 96 cells received (64 chessboard + 16 right + 16 left)
                this->printf("CALIB_OK\r\n");
            } else {
                // Incomplete calibration data - reset to formula values
                m_chessBoard->resetCalibrationToFormula();
                memset(m_calibrationChessboardCalibrated, 0, sizeof(m_calibrationChessboardCalibrated));
                memset(m_calibrationRightDropzoneCalibrated, 0, sizeof(m_calibrationRightDropzoneCalibrated));
                memset(m_calibrationLeftDropzoneCalibrated, 0, sizeof(m_calibrationLeftDropzoneCalibrated));
                this->printf("CALIB_ERROR\r\n");
            }
        }
    }
    else if(strncmp(command, "CALIB_ABORT", 11) == 0) {
        // Abort calibration data transmission
        if(m_calibrationInProgress) {
            m_calibrationInProgress = false;
            // Reset all calibration data to formula values
            m_chessBoard->resetCalibrationToFormula();
            memset(m_calibrationChessboardCalibrated, 0, sizeof(m_calibrationChessboardCalibrated));
            memset(m_calibrationRightDropzoneCalibrated, 0, sizeof(m_calibrationRightDropzoneCalibrated));
            memset(m_calibrationLeftDropzoneCalibrated, 0, sizeof(m_calibrationLeftDropzoneCalibrated));
            this->printf("CALIB_ABORT_ACK\r\n");
        }
    }
    else {
        this->printf("[%s] Unknown Command\r\n", command);
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
//#define DEBUG_KINEMATIC
bool ApplicationController::inverseKinematic(float x, float y, float a1, float a2, float* p1, float* p2)
{
//    if(sqrtf(x*x+y*y) > fabs(a1+a2) || sqrtf(x*x+y*y) < fabs(a1-a2)) return false;
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

void ApplicationController::calculatePolygonEdgeA2345(float upAngleInDegree, float* edge, float* angleA2A2345)
{
#ifdef DEBUG_KINEMATIC
    this->printf("ApplicationController::calculatePolygonEdgeA2345\r\n");
#endif
    float upAngle = upAngleInDegree/180.0f*M_PI;
    float a2, a3, a45, a23, a2345, q1, q2, xFK, yFK;
    float angleA3A23 = 0;
    a2 = m_robot->armLength(MOTOR_ARM2);
    a3 = m_robot->armLength(MOTOR_ARM3);
    q1 = 0;
    q2 = (180.0f - m_robot->homeAngle(MOTOR_ARM3))/180.0f*M_PI;
    forwardKinematic(a2,a3,q1,q2,&xFK,&yFK);
    a23 = sqrt(a2*a2 + a3*a3 - 2*a2*a3*cos(M_PI-q2));
    angleA3A23 = acosf((a3*a3 + a23*a23 - a2*a2)/(2*a3*a23)); // Angle between A3 and A23
    a45 = m_robot->armLength(MOTOR_ARM4) * cos(upAngle)
            + m_robot->armLength(MOTOR_ARM5);
    q1 = 0;
    q2 = (180.0f - m_robot->homeAngle(MOTOR_ARM4))/180.0f*M_PI + angleA3A23;
    forwardKinematic(a23,a45,q1,q2,&xFK,&yFK);
    a2345 = sqrt(a23*a23 + a45*a45 - 2*a23*a45*cos(M_PI-q2));
    *angleA2A2345 = 2*M_PI - acosf((a45*a45 + a2345*a2345 - a23*a23)/(2*a45*a2345))
            - m_robot->homeAngle(MOTOR_ARM3)/180.0f*M_PI
            - m_robot->homeAngle(MOTOR_ARM4)/180.0f*M_PI;
    *edge = a2345;
#ifdef DEBUG_KINEMATIC
    this->printf("ApplicationController::calculatePolygonEdgeA2345 done a2345[%.02f] angleA2A2345[%.02f]\r\n",
                 a2345,*angleA2A2345/M_PI*180.0f);
#endif
}

void ApplicationController::calculateJoints(float xPos, float yPos, float upAngleInDegree, int* jointSteps)
{    
    float a1 = m_robot->armLength(MOTOR_ARM1);
    float a2345 = 0;
    float angleA2A2345 = 0;
#ifdef DEBUG_KINEMATIC
    this->printf("=== calculatePolygonEdgeA2345\r\n");
#endif
    calculatePolygonEdgeA2345(upAngleInDegree,&a2345,&angleA2A2345);
#ifdef DEBUG_KINEMATIC    
    this->printf("calculatePolygonEdgeA2345 ===\r\n");
#endif
    float q1 = 0;
    float q2 = 0;
#ifdef DEBUG_KINEMATIC
    this->printf("xPos[%d]\r\n",(int)xPos);
    this->printf("yPos[%d]\r\n",(int)yPos);
    this->printf("a1[%d]\r\n",(int)a1);
    this->printf("a2345[%d]\r\n",(int)a2345);
    this->printf("angleA2A2345[%d]\r\n",(int)(angleA2A2345*180.f/M_PI));
    this->printf("Inverse\r\n");
#endif
    inverseKinematic(xPos, yPos, a1, a2345, &q1, &q2);
#ifdef DEBUG_KINEMATIC
    this->printf("Inverse done\r\n");
#endif
    float xPosFK = 0, yPosFK = 0;
    forwardKinematic(a1,a2345,q1,q2,&xPosFK,&yPosFK);
    if(xPosFK*xPos < 0 || yPosFK * yPos < 0) q1 = q1 - M_PI;
#ifdef DEBUG_KINEMATIC
    this->printf("arm1Angle[%d]=[%d] arm2Angle[%d]=[%d]\r\n",
                 (int)(q1/M_PI*180.0f),
                 (int)((q1)/M_PI*180.0f),
                 (int)((q2)/M_PI*180.0f),
                 (int)((M_PI - q2 + angleA2A2345)/M_PI*180.0f));
#endif
    jointSteps[MOTOR_ARM1] = m_robot->angleToStep(
                MOTOR_ARM1,
                q1/M_PI*180.0f);
    jointSteps[MOTOR_ARM2] = m_robot->angleToStep(
                MOTOR_ARM2,
                (M_PI - q2 + angleA2A2345)/M_PI*180.0f);
    jointSteps[MOTOR_ARM3] = m_robot->homeStep(MOTOR_ARM3);
    jointSteps[MOTOR_ARM4] = m_robot->homeStep(MOTOR_ARM4);
    jointSteps[MOTOR_ARM5] = m_robot->angleToStep(
                MOTOR_ARM5,
                upAngleInDegree);
#ifdef DEBUG_KINEMATIC
    this->printf("calculateJoints upAngleInDegree[%f]\r\n",upAngleInDegree);
#endif
}

Point ApplicationController::calibPos()
{
    Point resultPos;
    float a1 = m_robot->armLength(MOTOR_ARM1);
    float listCalibAngle[MAX_MOTOR];
    int numMotor;
    m_robot->calibAngle(listCalibAngle,&numMotor,ANGLE_RAD);
    float a2 = 0;
    float q2Offset = 0;
    calculatePolygonEdgeA2345(listCalibAngle[MOTOR_ARM5]/M_PI*180.0f,&a2,&q2Offset);
    float xPosFK = 0, yPosFK = 0;
    float q1 = listCalibAngle[MOTOR_ARM1] - M_PI/2.0f;
    float q2 = M_PI/2.0f - q2Offset;
    forwardKinematic(a1,a2,q1,q2,&xPosFK,&yPosFK);
    resultPos.x = xPosFK;
    resultPos.y = yPosFK;
    return resultPos;
}

Command ApplicationController::calculateNextPointInLine(Point currPos, Point targetPos, float numPointInCommand)
{
    Command nextPoint;
    float dx = targetPos.x - currPos.x;
    float dy = targetPos.y - currPos.y;
    float angle = int(dx*10) == 0 ? M_PI_2:atan(dy/dx);
    nextPoint.x = currPos.x + numPointInCommand * m_minSpace * fabs(cos(angle))*(dx>0?1:-1);
    nextPoint.y = currPos.y + numPointInCommand * m_minSpace * fabs(sin(angle))*(dy>0?1:-1);
    return nextPoint;
}

Point ApplicationController::currentPos()
{
    Point resultPos;
    float a1 = m_robot->armLength(MOTOR_ARM1);
    float listCurrentAngle[MAX_MOTOR];
    int numMotor;
    m_robot->currentAngle(listCurrentAngle,&numMotor,ANGLE_RAD);
    printf("Angle M1[%f] M2[%f] M5[%f]\r\n",
           listCurrentAngle[MOTOR_ARM1]/M_PI*180.0f,
           listCurrentAngle[MOTOR_ARM2]/M_PI*180.0f,
           listCurrentAngle[MOTOR_ARM5]/M_PI*180.0f);
    float a2345 = 0;
    float angleA2A2345 = 0;
    calculatePolygonEdgeA2345(listCurrentAngle[MOTOR_ARM5]/M_PI*180.0f,&a2345,&angleA2A2345);
    float xPosFK = 0, yPosFK = 0;
    float q1 = listCurrentAngle[MOTOR_ARM1];
    float q2 = M_PI - listCurrentAngle[MOTOR_ARM2] + angleA2A2345;
    forwardKinematic(a1,a2345,q1,q2,&xPosFK,&yPosFK);
    resultPos.x = xPosFK;
    resultPos.y = yPosFK;
    return resultPos;
}

void ApplicationController::goToHome(int motorID)
{
    if(motorID == MOTOR_CAPTURE) {
        specificPlatformGohome(MOTOR_CAPTURE);
        m_robot->setState(ROBOT_EXECUTE_DONE);
    } else {
        if(motorID == MAX_MOTOR) {
            specificPlatformGohome(MOTOR_CAPTURE);
        }
        m_robot->requestGoHome(motorID);
        setMachineState(MACHINE_EXECUTE_HOME);
    }
}

void ApplicationController::calibToHome(int motorID)
{
    m_robot->requestCalib(motorID);
    setMachineState(MACHINE_EXECUTE_CALIBRATION);
}
void ApplicationController::goToReadyPosition() {
    int jointSteps[MAX_MOTOR];
    jointSteps[MOTOR_CAPTURE] = 0;
    jointSteps[MOTOR_ARM1] = m_robot->angleToStep(MOTOR_ARM1,0);
    jointSteps[MOTOR_ARM2] = m_robot->angleToStep(MOTOR_ARM2,90+m_robot->homeAngle(MOTOR_ARM2));
    jointSteps[MOTOR_ARM3] = m_robot->homeAngle(MOTOR_ARM3);
    jointSteps[MOTOR_ARM4] = m_robot->homeAngle(MOTOR_ARM4);
    jointSteps[MOTOR_ARM5] = m_robot->angleToStep(MOTOR_ARM5,m_robot->homeAngle(MOTOR_ARM5));
    m_robot->setMoveTarget(jointSteps);
    m_robot->moveToTarget(MAX_MOTOR);
    setMachineState(MACHINE_EXECUTE_POSITION);
}

void ApplicationController::goToCalibPosition() {
    int jointSteps[MAX_MOTOR];
    jointSteps[MOTOR_CAPTURE] = 0;
    jointSteps[MOTOR_ARM1] = m_robot->calibStep(MOTOR_ARM1);
    jointSteps[MOTOR_ARM2] = m_robot->calibStep(MOTOR_ARM2);
    jointSteps[MOTOR_ARM3] = 0;
    jointSteps[MOTOR_ARM4] = 0;
    jointSteps[MOTOR_ARM5] = m_robot->calibStep(MOTOR_ARM5);
    m_robot->setMoveTarget(jointSteps);
    m_robot->moveToTarget(MAX_MOTOR);
    setMachineState(MACHINE_EXECUTE_POSITION);    
}
void ApplicationController::gotoPosition(float x, float y, float upAngleInDegree) {
    int jointSteps[MAX_MOTOR];
    jointSteps[MOTOR_CAPTURE] = m_robot->homeStep(MOTOR_CAPTURE);
    jointSteps[MOTOR_ARM3] = m_robot->homeStep(MOTOR_ARM3);
    jointSteps[MOTOR_ARM4] = m_robot->homeStep(MOTOR_ARM4);
    calculateJoints(x, y, upAngleInDegree, jointSteps);
    m_robot->setMoveTarget(jointSteps);
    m_robot->moveToTarget(MAX_MOTOR);
    setMachineState(MACHINE_EXECUTE_POSITION);
}
void ApplicationController::executeSequence(
        MOVE_TYPE moveType,
        int startCol, int startRow,
        int stopCol, int stopRow,
        char promotePiece) {
#ifdef DEBUG_COMMAND
    this->printf("Go to Pos [%d,%d] to [%d,%d] \r\n",
                 startCol, startRow, stopCol, stopRow);
#endif
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
    m_commandSequenceState = COMMAND_SEQUENCE_STATE_INIT;
}

void ApplicationController::calculateSequenceMoveStraight(int startCol, int startRow,int stopCol, int stopRow)
{
    Point startPoint = m_chessBoard->convertPoint(startRow,startCol);
    Point endPoint = m_chessBoard->convertPoint(stopRow,stopCol);
    clearSequenceMove();
//    float upAngles[2] = {0,0};
//    Point position[2] = {startPoint,endPoint};
//    int captureStep[2] = {0,0};
//    int numStep = 2;
//    for(int seqStep = 0; seqStep < numStep; seqStep++)
//    {
//        m_sequenceCommand[seqStep].x = position[seqStep].x;
//        m_sequenceCommand[seqStep].y = position[seqStep].y;
//        m_sequenceCommand[seqStep].updownAngle = upAngles[seqStep];
//        m_sequenceCommand[seqStep].captureStep = captureStep[seqStep];
//        m_sequenceCommand[seqStep].type = seqStep != 1 ?
//                    COMMAND_NORMAL : COMMAND_LINE;
//        m_numCommand++;
//    }
    m_sequenceCommand[0].x = endPoint.x;
    m_sequenceCommand[0].y = endPoint.y;
    m_sequenceCommand[0].updownAngle = 0;
    m_sequenceCommand[0].captureStep = 0;
    m_sequenceCommand[0].type = COMMAND_LINE;
    m_numCommand = 1;
    initSequenceMove(MAX_MOTOR);
    setMachineState(MACHINE_EXECUTE_COMMAND);
    m_commandSequenceState = COMMAND_SEQUENCE_STATE_INIT;
}

void ApplicationController::calculateSequenceMove(int startCol, int startRow, int upAngleInDegree, bool isCapture)
{
#ifdef DEBUG_COMMAND
    printf("ApplicationController::calculateSequenceMove\r\n");
#endif
    Point targetPoint = m_chessBoard->convertPoint(startRow,startCol);
    int jointSteps[MAX_MOTOR];
    clearSequenceMove();
    
    jointSteps[MOTOR_CAPTURE] = isCapture?415:0;
    // Inverse axis Oxy -> Oyx
    calculateJoints(targetPoint.y, -targetPoint.x, upAngleInDegree, jointSteps);

    m_robot->setMoveTarget(jointSteps);
    m_robot->moveToTarget(MAX_MOTOR);
    setMachineState(MACHINE_EXECUTE_POSITION);
}

void ApplicationController::calculateSequenceMoveNormal(int startCol, int startRow,
                     int stopCol, int stopRow)
{
    // append move from start -> stop -> standy
    Point startPoint = m_chessBoard->convertPoint(startRow,startCol);
#ifdef DEBUG_COMMAND
    printf("start[%d,%d] to Point(%d,%d)\r\n",
           startRow,startCol,
           (int)(startPoint.x*10), (int)(startPoint.y*10));
#endif
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
    Point dropPoint = m_chessBoard->getFreeDropPoint(ZONE_BOT);
    Point startPoint = m_chessBoard->convertPoint(startRow,startCol);
    Point stopPoint = m_chessBoard->convertPoint(stopRow,stopCol);

    clearSequenceMove();
    appendSequenceMove(stopPoint, dropPoint);
    appendSequenceMove(startPoint, stopPoint);
}

void ApplicationController::calculateSequencePastPawn(int startCol, int startRow,
                     int stopCol, int stopRow)
{
    // append move from attack pawn -> drop -> start -> stop -> standby
    Point dropPoint = m_chessBoard->getFreeDropPoint(ZONE_PLAYER);
    Point pawnPoint = m_chessBoard->convertPoint(startRow,stopCol);
    Point startPoint = m_chessBoard->convertPoint(startRow,startCol);
    Point stopPoint = m_chessBoard->convertPoint(stopRow,stopCol);
    clearSequenceMove();
    appendSequenceMove(pawnPoint, dropPoint);
    appendSequenceMove(startPoint, stopPoint);
}

void ApplicationController::calculateSequencePromotePiece(int startCol, int startRow,
                     int stopCol, int stopRow, char promotePiece)
{
    // append move from attack piece -> drop -> promote -> stop -> start -> drop -> standby
    Point promotePiecePoint = m_chessBoard->getFreeDropPoint(ZONE_BOT,promotePiece);
    Point dropPiecePoint = m_chessBoard->getFreeDropPoint(ZONE_BOT);
    Point startPoint = m_chessBoard->convertPoint(startRow,startCol);
    Point stopPoint = m_chessBoard->convertPoint(stopRow,stopCol);

    clearSequenceMove();
    if(startCol != stopCol){
        // pawn attack piece, move attack piece -> drop
        Point dropPoint = m_chessBoard->getFreeDropPoint(ZONE_BOT);
        appendSequenceMove(stopPoint, dropPoint);
    }
    appendSequenceMove(promotePiecePoint, stopPoint);
    appendSequenceMove(startPoint, dropPiecePoint);
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
    appendSequenceMove(kingPoint, kingNewPoint, true);
    appendSequenceMove(rookPoint, rookNewPoint);
}

float ApplicationController::distance(float x1, float y1, float x2, float y2)
{
    float dx = x2-x1;
    float dy = y2-y1;
    float result = sqrt(dx*dx + dy*dy);
    return result;
}

void ApplicationController::clearSequenceMove() {
    m_numCommand = 0;
    m_curCommandId = 0;
}
void ApplicationController::appendSequenceMove(Point start, Point stop, bool straightMove) {
    if(!straightMove) {
        float upAngles[6] = {-45.0f,0.0f,-45.0f,
                             -45.0f,0.0f,-45.0f};
        Point position[6] = {start,start,start,
                              stop,stop,stop};
        int captureStep[6] = {0,490,490,
                               490,0,0};
        int numStep = 6;
        for(int seqStep = 0; seqStep < numStep; seqStep++)
        {
            m_sequenceCommand[m_numCommand].x = position[seqStep].x;
            m_sequenceCommand[m_numCommand].y = position[seqStep].y;
            m_sequenceCommand[m_numCommand].updownAngle = upAngles[seqStep];
            m_sequenceCommand[m_numCommand].captureStep = captureStep[seqStep];
            m_sequenceCommand[m_numCommand].type = COMMAND_NORMAL;
            m_numCommand++;
        }
    } else {
        float upAngles[6] = {-45.0f,0.0f,0.0f,0.0f,
                              0.0f,-45.0f};
        Point position[6] = {start,start,start,stop,stop,stop};
        int captureStep[6] = {0,0,490,490,0,0};
        int numStep = 6;
        for(int seqStep = 0; seqStep < numStep; seqStep++)
        {
            m_sequenceCommand[m_numCommand].x = position[seqStep].x;
            m_sequenceCommand[m_numCommand].y = position[seqStep].y;
            m_sequenceCommand[m_numCommand].updownAngle = upAngles[seqStep];
            m_sequenceCommand[m_numCommand].captureStep = captureStep[seqStep];
            m_sequenceCommand[m_numCommand].type = COMMAND_NORMAL;
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

