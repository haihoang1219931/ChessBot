#include "ApplicationArduino.h"
#include <Arduino.h>
#include "SAL/Button.h"
#include "SAL/ChessBoard.h"
#include "SAL/Robot.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "SAL/StdTypes.h"

#define NONE_PIN  0xFF
#define enPin 8
#define stepXPin 2 //X.STEP
#define dirXPin 5 // X.DIR
#define stepYPin 4 //Y.STEP
#define dirYPin 7 // Y.DIR

#define limitX 13 // X.LIMIT
#define limitY 10 // Y.LIMIT

#define miniStepperUpdownPin1 34
#define miniStepperUpdownPin2 36
#define miniStepperUpdownPin3 38
#define miniStepperUpdownPin4 40

#define miniStepperGripperPin1 24
#define miniStepperGripperPin2 26
#define miniStepperGripperPin3 28
#define miniStepperGripperPin4 30

#define limitUpdown 11
#define limitGripper 62 // A8

ApplicationArduino app;
ApplicationArduino::ApplicationArduino()
{
    initRobot();
    memset(m_buttonPin,NONE_PIN,sizeof(m_buttonPin));
    m_buttonPin[MOTOR_ARM1] = limitX;
    m_buttonPin[MOTOR_ARM2] = limitY;
    m_buttonPin[MOTOR_ARM5] = limitUpdown;

    pinMode(enPin, OUTPUT);
    pinMode(dirXPin, OUTPUT);
    pinMode(stepXPin, OUTPUT);
    pinMode(dirYPin, OUTPUT);
    pinMode(stepYPin, OUTPUT);

    digitalWrite(enPin, HIGH);
    digitalWrite(dirXPin, LOW);
    digitalWrite(stepXPin, HIGH);
    digitalWrite(dirYPin, LOW);
    digitalWrite(stepYPin, HIGH);
    
    pinMode(limitX, INPUT_PULLUP);
    pinMode(limitY, INPUT_PULLUP);
    pinMode(limitUpdown, INPUT_PULLUP);

    pinMode(miniStepperUpdownPin1, OUTPUT);
    pinMode(miniStepperUpdownPin2, OUTPUT);
    pinMode(miniStepperUpdownPin3, OUTPUT);
    pinMode(miniStepperUpdownPin4, OUTPUT);

    pinMode(miniStepperGripperPin1, OUTPUT);
    pinMode(miniStepperGripperPin2, OUTPUT);
    pinMode(miniStepperGripperPin3, OUTPUT);
    pinMode(miniStepperGripperPin4, OUTPUT);

    initHardwareTimer(20000.0f);
}

ApplicationArduino::~ApplicationArduino()
{

}

void ApplicationArduino::initRobot()
{
    m_chessBoard->setChessBoardPosX(31-31*8/2);
    m_chessBoard->setChessBoardPosY(100);
    m_chessBoard->setChessBoardSize(31*8);
    m_chessBoard->setDropZoneSpace(31);

    JointParam armPrams[MAX_MOTOR] = {
    // active|   scale=gear_ratio/resolution   |length|init angle|home angle|home step time|min angle|max angle|max step/s|frequency
        {true,  1.0f/1.0f,                            0,     100,        0,        64,           0,       250,     5000,   20000.0f},
        {true,  8.0f*18.0f/01.0f*(200.0f/360.0f),   255,       0,      -17,         2,         -17,       150,    10000,   20000.0f},
        {true,  8.0f*70.0f/20.0f*(200.0f/360.0f),    85,     140,       50,         2,          50,       210,    10000,   20000.0f},
        {false,  1.0f/1.0f,                          15,     130,      130,         1,         130,       130,        1,   20000.0f},
        {false,  1.0f/1.0f,                         120,     180,      180,         1,         180,       180,        1,   20000.0f},
        {true,  50.0f/14.0f*(512.0f/360.0f),          0,      20,        0,        16,           0,        45,     1250,   20000.0f}
    };

    for(int motor= MOTOR_CAPTURE; motor<= MOTOR_ARM5; motor++) {
        m_robot->setMotorParam(motor,armPrams[motor]);
        m_robot->updateInitAngle(motor,armPrams[motor].initAngle);
    }
}

int ApplicationArduino::printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char m_buffer[128];
    int rc = vsprintf(m_buffer, fmt, args);
    va_end(args);
    Serial.print((const char*)m_buffer);
    return rc;
}
void ApplicationArduino::msleep(int millis) {
  delay(millis);
}

long ApplicationArduino::getSystemTime() {
	return m_appTimer;
}

void ApplicationArduino::specificPlatformGohome(int motorID)
{
  // Not used, go home is handled in Robot::executeGoHome
}

void ApplicationArduino::harwareStop(int motorID = MAX_MOTOR)
{
  // Not used
}

void ApplicationArduino::checkInput(){
	for(unsigned int btnID = 0; btnID< MAX_BUTTON; btnID++) {
    if(m_buttonPin[btnID] != NONE_PIN) {
      pinMode(m_buttonPin[btnID], INPUT_PULLUP);
      storeButtonState(btnID, digitalRead(m_buttonPin[btnID]) == LOW);
      // this->printf("button[%d] %s\r\n",btnID, digitalRead(m_buttonPin[btnID]) == LOW ? "pressed" : "normal");
    }
	}
  m_limitGripperValue = analogRead(limitGripper);
  this->printf("Limit Gripper pin[%d] Value: %d\r\n", limitGripper, m_limitGripperValue);
}
#define DEBUG_SERIAL
int ApplicationArduino::readSerial(char* output, int length) {
  String command;
  while(Serial.available()) {
    delay(3);
    char c = Serial.read();
    command += c;  
  }
  if(command.length()>0) {
    Serial.print(command);
    for(int i=0; i< command.length(); i++) {
      output[i] = command.charAt(i);
#ifdef DEBUG_SERIAL
      Serial.print(output[i],HEX);
      Serial.print(" ");
#endif
    }
#ifdef DEBUG_SERIAL
    Serial.print("\n");
#endif
  }
  return command.length();
}

bool ApplicationArduino::isLimitReached(int motorID, MOTOR_LIMIT_TYPE limitType)
                      {
  bool limitReached = true;
  switch(motorID){
    case MOTOR::MOTOR_ARM1: {
      limitReached = limitType == MOTOR_LIMIT_MIN || limitType == MOTOR_LIMIT_HOME ? 
                    (m_buttonList[MOTOR_ARM1]->buttonState() != BUTTON_STATE::BUTTON_NOMAL) :
                    false;
    }
    break;
    case MOTOR::MOTOR_ARM2: {
      limitReached = limitType == MOTOR_LIMIT_MIN || limitType == MOTOR_LIMIT_HOME ? 
                    (m_buttonList[MOTOR_ARM2]->buttonState() != BUTTON_STATE::BUTTON_NOMAL) :
                    false;
    }
    break;
    case MOTOR::MOTOR_ARM5: {
      limitReached = limitType == MOTOR_LIMIT_MIN || limitType == MOTOR_LIMIT_HOME ? 
                    (m_buttonList[MOTOR_ARM5]->buttonState() != BUTTON_STATE::BUTTON_NOMAL) :
                    false;
    }
    break;
    case MOTOR::MOTOR_CAPTURE: {
      limitReached = limitType == MOTOR_LIMIT_MIN || limitType == MOTOR_LIMIT_HOME ? 
                    m_limitGripperValue >= 500 :
                    m_limitGripperValue <= 200;
    }
    break;
    default: break;
  }
  return limitReached;
}

void ApplicationArduino::enableEngine(bool enable) {
  this->printf("%s engine\r\n",enable?"ENABLE":"DISABLE");
  if(enable) {
    digitalWrite(enPin, LOW);
  } else {
    digitalWrite(enPin, HIGH);
  }
  
}

void ApplicationArduino::initDirection(int motorID, int direction)
{
  Serial.print("initDirection motorID=");
  Serial.print(motorID);
  Serial.print(", direction=");
  Serial.println(direction);
  switch(motorID){
    case MOTOR::MOTOR_ARM1: {
      digitalWrite(dirXPin, direction > 0 ? LOW : HIGH);
    }
    break;
    case MOTOR::MOTOR_ARM2: {
      digitalWrite(dirYPin, direction > 0 ? LOW : HIGH);
    }
    break;
    case MOTOR::MOTOR_ARM5:
    {
      // Do nothing, direction is controlled by step sequence
    }
    break;
    case MOTOR::MOTOR_CAPTURE: 
    {
      // Do nothing, direction is controlled by step sequence
    }
    break;
    default: break;
  }
  
}

void ApplicationArduino::moveDoneAction(int motorID)
{
  switch(motorID){
    case MOTOR::MOTOR_ARM1: {
      // Do nothing
    }
    break;
    case MOTOR::MOTOR_ARM2: {
      // Do nothing
    }
    break;
    case MOTOR::MOTOR_ARM5:
    {
      digitalWrite(miniStepperUpdownPin1, LOW);
      digitalWrite(miniStepperUpdownPin2, LOW);
      digitalWrite(miniStepperUpdownPin3, LOW);
      digitalWrite(miniStepperUpdownPin4, LOW);
    }
    break;
    case MOTOR::MOTOR_CAPTURE: 
    {
      digitalWrite(miniStepperGripperPin1, LOW);
      digitalWrite(miniStepperGripperPin2, LOW);
      digitalWrite(miniStepperGripperPin3, LOW);
      digitalWrite(miniStepperGripperPin4, LOW);
    }
    break;
    default: break;
  }
}

void ApplicationArduino::initHardwareTimer(float samplerate)
{
  enableHardwareTimer(false);
  // initialize timer1
  noInterrupts(); // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 16000000.0f / samplerate; // compare match register for IRQ with selected samplerate
  TCCR1B |= (1 << WGM12); // CTC mode
  TCCR1B |= (1 << CS10); // no prescaler
  interrupts(); // enable all interrupts
}
// #define DEBUG_PULSE
uint8_t ApplicationArduino::executePulseLoop(int motorID)
{
    uint8_t statePulse = m_robot->statePulse(motorID);
    uint32_t countPulse = m_robot->countPulse(motorID);
    uint32_t numWaitPulse = m_robot->numWaitPulse(motorID);
    uint8_t nextStatePulse = statePulse;
#ifdef DEBUG_PULSE
    printf("p S[%d] C[%d/%d]\r\n",
          statePulse, (int)countPulse, (int)numWaitPulse);
#endif
    if(countPulse < numWaitPulse) {
        switch (motorID)
        {
        case MOTOR_ARM1: {
          nextStatePulse = executePulseStepper2Wires(statePulse, countPulse, numWaitPulse,stepXPin);
        }
          break;
        case MOTOR_ARM2: {
          nextStatePulse = executePulseStepper2Wires(statePulse, countPulse, numWaitPulse, stepYPin);
        }
          break;
        case MOTOR_ARM5: {
          int direction = m_robot->currentDirection(motorID);
          nextStatePulse = executePulseStepper4Wires(statePulse, countPulse, numWaitPulse,
            direction, miniStepperUpdownPin1, miniStepperUpdownPin2, miniStepperUpdownPin3, miniStepperUpdownPin4);
        }
          break;
        case MOTOR_CAPTURE: {
          int direction = m_robot->currentDirection(motorID);
          nextStatePulse = executePulseStepper4Wires(statePulse, countPulse, numWaitPulse,
            direction, miniStepperGripperPin1, miniStepperGripperPin2, miniStepperGripperPin3, miniStepperGripperPin4);
        }
          break;
        default:
          break;
        }
        m_robot->updateCountPulse(motorID,countPulse+1);
        m_robot->updateStatePulse(motorID,nextStatePulse);
        return nextStatePulse;
    } else {
        return STATE_DONE;
    }
}

void ApplicationArduino::enableHardwareTimer(bool enable)
{
  if(enable)
    TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  else
    TIMSK1 &= ~(1 << OCIE1A);
}

void ApplicationArduino::resetPulse(int motorID)
{
  m_robot->updateCountPulse(motorID,0);
  m_robot->updateStatePulse(motorID,STATE_COMMAND1);
}
uint8_t ApplicationArduino::executePulseStepper2Wires(uint8_t statePulse,
  uint32_t countPulse, uint32_t numWaitPulse, int stepPin)
{
#ifdef DEBUG_PULSE
  printf("executePulseStepper2Wires statePulse=%d, countPulse=%d, numWaitPulse=%d\r\n",
          (int)statePulse, (int)countPulse, (int)numWaitPulse);
#endif
  uint8_t nextStatePulse = statePulse;
  switch(statePulse){
    case STATE_COMMAND1: {
      digitalWrite(stepPin,HIGH);
      nextStatePulse = countPulse >= (uint32_t)(numWaitPulse/2-1) ? STATE_COMMAND2 : STATE_WAIT1;
#ifdef DEBUG_PULSE
      Serial.print("STATE_COMMAND1 -> STATE_WAIT1\r\n");
#endif
    }
    break;
    case STATE_WAIT1: {
      if(countPulse >= (uint32_t)(numWaitPulse/2-1)) {
#ifdef DEBUG_PULSE
        Serial.print("STATE_WAIT1 -> STATE_COMMAND2\r\n");
#endif
        nextStatePulse = STATE_COMMAND2;
      }
    }
    break;
    case STATE_COMMAND2: {
      digitalWrite(stepPin,LOW);
      nextStatePulse = countPulse >= (uint32_t)(numWaitPulse-1) ? STATE_DONE : STATE_WAIT2;
#ifdef DEBUG_PULSE
      Serial.print("STATE_COMMAND2 -> STATE_WAIT2\r\n");
#endif
    }
    break;
    case STATE_WAIT2: {
      if(countPulse >= (uint32_t)(numWaitPulse-1)) {
        nextStatePulse = STATE_DONE;
#ifdef DEBUG_PULSE
        Serial.print("STATE_WAIT2 -> STATE_DONE\r\n");
#endif
      }
    }
    break;
  }
#ifdef DEBUG_PULSE
  Serial.print("nextStatePulse = ");
  Serial.print(nextStatePulse);
  Serial.print("\r\n");
#endif
  return nextStatePulse;
}

uint8_t ApplicationArduino::executePulseStepper4Wires(uint8_t statePulse, uint32_t countPulse, uint32_t numWaitPulse, 
  int direction, int stepPin1, int stepPin2, int stepPin3, int stepPin4)
{
  uint8_t nextStatePulse = statePulse;
  switch(statePulse){
    case STATE_COMMAND1: {
      digitalWrite(stepPin1, direction < 0 ? HIGH:HIGH);
      digitalWrite(stepPin2, direction < 0 ? LOW :LOW );
      digitalWrite(stepPin3, direction < 0 ? LOW :LOW );
      digitalWrite(stepPin4, direction < 0 ? LOW :HIGH);
      nextStatePulse = STATE_WAIT1;
    }
    break;
    case STATE_WAIT1: {
      if(countPulse >= (uint32_t)(1*numWaitPulse/8)) nextStatePulse = STATE_COMMAND2;
    }
    break;
    case STATE_COMMAND2: {
      digitalWrite(stepPin1, direction < 0 ? HIGH:LOW );
      digitalWrite(stepPin2, direction < 0 ? HIGH:LOW );
      digitalWrite(stepPin3, direction < 0 ? LOW :LOW );
      digitalWrite(stepPin4, direction < 0 ? LOW :HIGH);
      nextStatePulse = STATE_WAIT2;
    }
    break;
    case STATE_WAIT2: {
      if(countPulse >= (uint32_t)(2*numWaitPulse/8)) nextStatePulse = STATE_COMMAND3;
    }
    break;
    case STATE_COMMAND3: {
      digitalWrite(stepPin1, direction < 0 ? LOW :LOW );
      digitalWrite(stepPin2, direction < 0 ? HIGH:LOW );
      digitalWrite(stepPin3, direction < 0 ? LOW :HIGH);
      digitalWrite(stepPin4, direction < 0 ? LOW :HIGH);
      nextStatePulse = STATE_WAIT3;
    }
    break;
    case STATE_WAIT3: {
      if(countPulse >= (uint32_t)(3*numWaitPulse/8)) nextStatePulse = STATE_COMMAND4;
    }
    break;
    case STATE_COMMAND4: {
      digitalWrite(stepPin1, direction < 0 ? LOW :LOW );
      digitalWrite(stepPin2, direction < 0 ? HIGH:LOW );
      digitalWrite(stepPin3, direction < 0 ? HIGH:HIGH);
      digitalWrite(stepPin4, direction < 0 ? LOW :LOW );
      nextStatePulse = STATE_WAIT4;
    }
    break;
    case STATE_WAIT4: {
      if(countPulse >= (uint32_t)(4*numWaitPulse/8)) nextStatePulse = STATE_COMMAND5;
    }
    break;
    case STATE_COMMAND5: {
      digitalWrite(stepPin1, direction < 0 ? LOW :LOW );
      digitalWrite(stepPin2, direction < 0 ? LOW :HIGH);
      digitalWrite(stepPin3, direction < 0 ? HIGH:HIGH);
      digitalWrite(stepPin4, direction < 0 ? LOW :LOW );
      nextStatePulse = STATE_WAIT5;
    }
    break;
    case STATE_WAIT5: {
      if(countPulse >= (uint32_t)(5*numWaitPulse/8)) nextStatePulse = STATE_COMMAND6;
    }
    break;
    case STATE_COMMAND6: {
      digitalWrite(stepPin1, direction < 0 ? LOW :LOW );
      digitalWrite(stepPin2, direction < 0 ? LOW :HIGH);
      digitalWrite(stepPin3, direction < 0 ? HIGH:LOW );
      digitalWrite(stepPin4, direction < 0 ? HIGH:LOW );
      nextStatePulse = STATE_WAIT6;
    }
    break;
    case STATE_WAIT6: {
      if(countPulse >= (uint32_t)(6*numWaitPulse/8)) nextStatePulse = STATE_COMMAND7;
    }
    break;
    case STATE_COMMAND7: {
      digitalWrite(stepPin1, direction < 0 ? LOW :HIGH);
      digitalWrite(stepPin2, direction < 0 ? LOW :HIGH);
      digitalWrite(stepPin3, direction < 0 ? LOW :LOW );
      digitalWrite(stepPin4, direction < 0 ? HIGH:LOW );
      nextStatePulse = STATE_WAIT7;
    }
    break;
    case STATE_WAIT7: {
      if(countPulse >= (uint32_t)(7*numWaitPulse/8)) nextStatePulse = STATE_COMMAND8;
    }
    break;
    case STATE_COMMAND8: {
      digitalWrite(stepPin1, direction < 0 ? HIGH:HIGH);
      digitalWrite(stepPin2, direction < 0 ? LOW :LOW );
      digitalWrite(stepPin3, direction < 0 ? LOW :LOW );
      digitalWrite(stepPin4, direction < 0 ? HIGH:LOW );
      nextStatePulse = STATE_WAIT8;
    }
    break;
    case STATE_WAIT8: {
      if(countPulse >= (uint32_t)(8*numWaitPulse/8)) nextStatePulse = STATE_DONE;
    }
    break;
  }
  return nextStatePulse;
}

ISR(TIMER1_COMPA_vect)
{
  app.executeSmoothMotionLoop(MOTOR_ARM1);
  app.executeSmoothMotionLoop(MOTOR_ARM2);
  app.executeSmoothMotionLoop(MOTOR_ARM5);
  app.executeSmoothMotionLoop(MOTOR_CAPTURE);
}

// uint8_t statePulse = STATE_COMMAND1;
// uint8_t numWaitPulse = 4;
// uint8_t pulseCount = 0;

// ISR(TIMER1_COMPA_vect)
// {
//   switch(statePulse){
//     case STATE_COMMAND1: {
//       digitalWrite(2,HIGH);
//       statePulse = STATE_WAIT1;
//       pulseCount = 0;
//     }
//     break;
//     case STATE_WAIT1: {
//       pulseCount++;
//       if(pulseCount >= numWaitPulse/2) statePulse = STATE_COMMAND2;
//     }
//     break;
//     case STATE_COMMAND2: {
//       digitalWrite(2,LOW);
//       statePulse = STATE_WAIT2;
//       pulseCount = 0;
//     }
//     break;
//     case STATE_WAIT2: {
//       pulseCount++;
//       if(pulseCount >= numWaitPulse) statePulse = STATE_COMMAND1;
//     }
//     break;
//   }
// }