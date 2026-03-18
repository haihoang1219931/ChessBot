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
#define stepPin1 2 //MOTOR ARM1 STEP
#define dirPin1 5 // MOTOR ARM1 DIR
#define stepPin2 3 //MOTOR ARM2 STEP
#define dirPin2 6 // MOTOR ARM2 DIR
#define stepPin5 3 //MOTOR ARM5 STEP
#define dirPin5 6 // MOTOR ARM5 DIR
#define stepPinCapture 12 //MOTOR CAPTURE STEP
#define dirPinCapture 13 // MOTOR CAPTURE DIR

#define limit1 9 // ARM1 LIMIT
#define limit2 10 // ARM2 LIMIT
#define limit5 11 // ARM5 LIMIT
#define limitGripper A0 // CAPTURE LIMIT Analog

typedef enum {
  STATE_CHECK_SENSOR,
  STATE_SET_DIR,
  STATE_GO_HOME,
  STATE_GO_TO_TARGET,
  STATE_HOME_DONE,
} STATE_HOMING;

ApplicationArduino app;
ApplicationArduino::ApplicationArduino()
{
    initRobot();
    memset(m_buttonPin,NONE_PIN,sizeof(m_buttonPin));
    m_buttonPin[MOTOR_ARM1] = limit1;
    m_buttonPin[MOTOR_ARM2] = limit2;
    m_buttonPin[MOTOR_ARM5] = limit5;

    pinMode(enPin, OUTPUT);
    pinMode(dirPin1, OUTPUT);
    pinMode(stepPin1, OUTPUT);
    pinMode(dirPin2, OUTPUT);
    pinMode(stepPin2, OUTPUT);
    pinMode(dirPin5, OUTPUT);
    pinMode(stepPin5, OUTPUT);
    pinMode(dirPinCapture, OUTPUT);
    pinMode(stepPinCapture, OUTPUT);

    digitalWrite(enPin, HIGH);
    digitalWrite(dirPin1, LOW);
    digitalWrite(stepPin1, HIGH);
    digitalWrite(dirPin2, LOW);
    digitalWrite(stepPin2, HIGH);
    digitalWrite(dirPin5, LOW);
    digitalWrite(stepPin5, HIGH);
    digitalWrite(dirPinCapture, LOW);
    digitalWrite(stepPinCapture, HIGH);
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
    m_minSpace = 2;

    JointParam armPrams[MAX_MOTOR] = {
    // active|   scale=gear_ratio/resolution   |length|init angle|home angle|home step time|min angle|max angle|max step/s|frequency
        {true,  100.0f*(20.0f/360.0f),                0,     -10,        0,       128,           0,       250,      500,   10000.0f},
        {true,  8.0f*18.0f/01.0f*(200.0f/360.0f),   255,       0,      -17,         2,         -17,       150,     5000,   10000.0f},
        {true,  8.0f*70.0f/20.0f*(200.0f/360.0f),    85,     140,       50,         2,          50,       210,     5000,   10000.0f},
        {false,  1.0f/1.0f,                          15,     130,      130,         1,         130,       130,        1,   10000.0f},
        {false,  1.0f/1.0f,                         120,     180,      180,         1,         180,       180,        1,   10000.0f},
        {true,  50.0f/14.0f*100.0f*(20.0f/360.0f),    0,     -10,        0,       128,           0,        45,      500,   10000.0f}
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
  // if(motorID == MOTOR_CAPTURE) {
  //   uint8_t stepPin = 4;
  //   uint8_t dirPin = 7;
  //   uint8_t dirClockPin = 9;
  //   uint8_t dirClockWisePin = 10;
  //   uint8_t dirAnalogRead = A0;
  //   int delayTime = 500;
  //   int sensorHomeValue = 330;
  //   int sensorCaptureValue = 120;
  //   int sensorValue;
  //   int stateGoHome;
  //   int countStep;
  //   int initDir;
  //   stateGoHome = STATE_CHECK_SENSOR;
  //   while(stateGoHome != STATE_DONE) {
  //     switch(stateGoHome){
  //       case STATE_CHECK_SENSOR:{
  //         sensorValue = analogRead(dirAnalogRead);
  //         stateGoHome = STATE_SET_DIR;
  //       }
  //       break;
  //       case STATE_SET_DIR:{
  //         digitalWrite(dirPin, sensorValue > sensorHomeValue ? LOW:HIGH);
  //         initDir = sensorValue > sensorHomeValue ? 1:-1;
  //         stateGoHome = STATE_GO_HOME;
  //         delay(1000);
  //       }
  //       break;
  //       case STATE_GO_HOME:{
  //         sensorValue = analogRead(dirAnalogRead);
  //         if(initDir*sensorValue>initDir*sensorHomeValue) {
  //           digitalWrite(stepPin, HIGH);
  //           delayMicroseconds(delayTime);
  //           digitalWrite(stepPin, LOW);
  //           delayMicroseconds(delayTime);
  //         } else {
  //           digitalWrite(dirPin, LOW);
  //           stateGoHome = STATE_GO_TO_TARGET;
  //           countStep = 0;
  //           delay(1000);
  //         }
  //       }
  //       break;
  //       case STATE_GO_TO_TARGET:{
  //         sensorValue = analogRead(dirAnalogRead);
  //         if(sensorValue>sensorCaptureValue) {
  //           digitalWrite(stepPin, HIGH);
  //           delayMicroseconds(delayTime);
  //           digitalWrite(stepPin, LOW);
  //           delayMicroseconds(delayTime);
  //           countStep++;
  //         } else {
  //           stateGoHome = STATE_HOME_DONE;
  //           Serial.print("countStep:");
  //           Serial.println(countStep);
  //           delay(1000);
  //         }
  //       }
  //       break;
  //       case STATE_HOME_DONE:{

  //       }
  //       break;
  //     }
  //   }
  // }
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
  // this->printf("Limit Gripper pin[%d] Value: %d\r\n", limitGripper, m_limitGripperValue);
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
                    m_limitGripperValue > 330 :
                    m_limitGripperValue < 120;
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
      digitalWrite(dirPin1, direction > 0 ? LOW : HIGH);
    }
    break;
    case MOTOR::MOTOR_ARM2: {
      digitalWrite(dirPin2, direction > 0 ? LOW : HIGH);
    }
    break;
    case MOTOR::MOTOR_ARM5:
    {
      digitalWrite(dirPin5, direction > 0 ? LOW : HIGH);
    }
    break;
    case MOTOR::MOTOR_CAPTURE: 
    {
      digitalWrite(dirPinCapture, direction > 0 ? LOW : HIGH);
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
      // Do nothing
    }
    break;
    case MOTOR::MOTOR_CAPTURE: 
    {
      // Do nothing
    }
    break;
    default: break;
  }
}

void ApplicationArduino::initHardwareTimer(float samplerate)
{
  // initialize timer1
  noInterrupts(); // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 16000000.0f / samplerate; // compare match register for IRQ with selected samplerate
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
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
          nextStatePulse = executePulseStepper2Wires(statePulse, countPulse, numWaitPulse,&PORTD,2);
        }
          break;
        case MOTOR_ARM2: {
          nextStatePulse = executePulseStepper2Wires(statePulse, countPulse, numWaitPulse, &PORTD,3);
        }
          break;
        case MOTOR_ARM5: {
          nextStatePulse = executePulseStepper2Wires(statePulse, countPulse, numWaitPulse,&PORTD,4);
        }
          break;
        case MOTOR_CAPTURE: {
          nextStatePulse = executePulseStepper2Wires(statePulse, countPulse, numWaitPulse,&PORTB,4);
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
  if(enable) {
    initHardwareTimer(5000.0f);
  }
  else
    TIMSK1 &= ~(1 << OCIE1A);
}

void ApplicationArduino::resetPulse(int motorID)
{
  m_robot->updateCountPulse(motorID,0);
  m_robot->updateStatePulse(motorID,STATE_COMMAND1);
}
uint8_t ApplicationArduino::executePulseStepper2Wires(uint8_t statePulse,
  uint32_t countPulse, uint32_t numWaitPulse, volatile uint8_t* portRegister, int bit)
{
#ifdef DEBUG_PULSE
  printf("executePulseStepper2Wires statePulse=%d, countPulse=%d, numWaitPulse=%d\r\n",
          (int)statePulse, (int)countPulse, (int)numWaitPulse);
#endif
  uint8_t nextStatePulse = statePulse;
  switch(statePulse){
    case STATE_COMMAND1: {
      // long start = micros();
      // digitalWrite(stepPin,HIGH);
      *portRegister |= (1 << bit);
      nextStatePulse = countPulse >= (uint32_t)(numWaitPulse/2-1) ? STATE_COMMAND2 : STATE_WAIT1;
      // long duration = micros() - start;
      // Serial.print("Command pulse 2wires duration (microseconds): ");
      // Serial.println(duration);
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
      // digitalWrite(stepPin,LOW);
      *portRegister &= ~(1 << bit);
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

ISR(TIMER1_COMPA_vect)
{
  app.executeSmoothMotionLoop(MOTOR_ARM1);
  app.executeSmoothMotionLoop(MOTOR_ARM2);
  app.executeSmoothMotionLoop(MOTOR_ARM5);
  app.executeSmoothMotionLoop(MOTOR_CAPTURE);
}