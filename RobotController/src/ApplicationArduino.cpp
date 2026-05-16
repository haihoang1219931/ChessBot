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
#define enPin1 38
#define stepPin1 A0 //MOTOR ARM1 STEP
#define dirPin1 A1 // MOTOR ARM1 DIR

#define enPin2 A2
#define stepPin2 A6 //MOTOR ARM2 STEP
#define dirPin2 A7 // MOTOR ARM2 DIR

#define enPin5 A8
#define stepPin5 46 //MOTOR ARM5 STEP
#define dirPin5 48 // MOTOR ARM5 DIR

#define enPinCapture 24
#define stepPinCapture 26 //MOTOR CAPTURE STEP
#define dirPinCapture 28 // MOTOR CAPTURE DIR

#define limit1 19 // ARM1 LIMIT
#define limit2 14 // ARM2 LIMIT
#define limit5 18 // ARM5 LIMIT
#define limitGripper A13 // CAPTURE LIMIT Analog

#define FREQUENCY_TIMER1 5000.0f

ApplicationArduino app;
ApplicationArduino::ApplicationArduino()
{
    initRobot();

    pinMode(limit1, INPUT_PULLUP);
    pinMode(limit2, INPUT_PULLUP);
    pinMode(limit5, INPUT_PULLUP);

    pinMode(enPin1, OUTPUT);
    pinMode(dirPin1, OUTPUT);
    pinMode(stepPin1, OUTPUT);

    pinMode(enPin2, OUTPUT);
    pinMode(dirPin2, OUTPUT);
    pinMode(stepPin2, OUTPUT);

    pinMode(enPin5, OUTPUT);
    pinMode(dirPin5, OUTPUT);
    pinMode(stepPin5, OUTPUT);

    pinMode(enPinCapture, OUTPUT);
    pinMode(dirPinCapture, OUTPUT);
    pinMode(stepPinCapture, OUTPUT);

    digitalWrite(enPin1, HIGH);
    digitalWrite(dirPin1, LOW);
    digitalWrite(stepPin1, HIGH);

    digitalWrite(enPin2, HIGH);
    digitalWrite(dirPin2, LOW);
    digitalWrite(stepPin2, HIGH);

    digitalWrite(enPin5, HIGH);
    digitalWrite(dirPin5, LOW);
    digitalWrite(stepPin5, HIGH);

    digitalWrite(enPinCapture, HIGH);
    digitalWrite(dirPinCapture, LOW);
    digitalWrite(stepPinCapture, HIGH);
  
    // Clear the prescaler bits (bits 0, 1, 2)
    ADCSRA &= ~( (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0) );

    // Set prescaler to 16 (ADPS2 = 1, ADPS1 = 0, ADPS0 = 0)
    ADCSRA |= (1 << ADPS2); 

    analogRead(limitGripper);
}

ApplicationArduino::~ApplicationArduino()
{

}

int16_t ApplicationArduino::readA13() {
  ADCSRB |= (1 << MUX5);
  ADMUX = (ADMUX & 0xF8) | 0x05; 
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  return ADC;
}

void ApplicationArduino::initRobot()
{
    m_chessBoard->setChessBoardPosX(77);
    m_chessBoard->setChessBoardPosY(86);
    m_chessBoard->setChessBoardSize(35*8);
    m_chessBoard->setDropZoneSpace(35);
    m_minSpace = 2;

    JointParam armPrams[MAX_MOTOR] = {
    // active|   scale=gear_ratio/resolution   |length|init angle|home angle|home step time|min angle|max angle|min pulse/step|frequency | step accel
        {true,  100.0f*(20.0f/360.0f),                0,      10,        0,         4,           0,       400,       8,   FREQUENCY_TIMER1,      0},
        {true,  4.0f*18.0f/01.0f*(200.0f/360.0f),   255,       0,      -22,         4,         -17,       150,       2,   FREQUENCY_TIMER1,    500},
        {true, 16.0f*70.0f/20.0f*(200.0f/360.0f), 80.27,     140,       52,         8,          50,       210,       2,   FREQUENCY_TIMER1,    250},
        {false,  1.0f/1.0f,                       25.57,     130,      130,         1,         130,       130,       6,   FREQUENCY_TIMER1,      0},
        {false,  1.0f/1.0f,                         120,     180,      180,         1,         180,       180,       6,   FREQUENCY_TIMER1,      0},
        {true,  50.0f/14.0f*100.0f*(20.0f/360.0f),    0,       0,      -45,         8,         -45,         0,       8,   FREQUENCY_TIMER1,    100}
    };

    for(int motor= MOTOR_CAPTURE; motor<= MOTOR_ARM5; motor++) {
        m_robot->setMotorParam(motor,armPrams[motor]);
        m_robot->updateInitAngle(motor,armPrams[motor].initAngle);
    }
}

int ApplicationArduino::printf(const char *fmt, ...) {
    va_start(m_args, fmt);
    int rc = vsprintf(m_buffer, fmt, m_args);
    va_end(m_args);
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
  if(motorID == MOTOR_CAPTURE) {
    uint8_t stepPin = stepPinCapture;
    uint8_t dirPin = dirPinCapture;
    uint8_t dirAnalogRead = limitGripper;
    int delayTime = 500;
    int sensorHomeValue = 630;
    int sensorCaptureValue = 300;
    int sensorValue;
    int stateGoHome;
    int initDir;
    int currentStep = 0;
    stateGoHome = STATE_CHECK_SENSOR;
    while(stateGoHome != STATE_HOME_DONE) {
      switch(stateGoHome){
        case STATE_CHECK_SENSOR:{
          sensorValue = analogRead(dirAnalogRead);
          stateGoHome = STATE_SET_DIR;
        }
        break;
        case STATE_SET_DIR:{
          digitalWrite(dirPin, sensorValue > sensorHomeValue ? LOW:HIGH);
          initDir = sensorValue > sensorHomeValue ? 1:-1;
          stateGoHome = STATE_GO_HOME_1;
          delay(100);
        }
        break;
        case STATE_GO_HOME_1:{
          sensorValue = analogRead(dirAnalogRead);
          if(initDir*sensorValue>initDir*sensorHomeValue) {
            digitalWrite(stepPin, HIGH);
            delayMicroseconds(delayTime);
            digitalWrite(stepPin, LOW);
            delayMicroseconds(delayTime);
          } else {
            digitalWrite(dirPin, LOW);
            stateGoHome = STATE_GO_TO_CAPTURE;
            m_captureCountStep = 0;
            delay(100);
          }
        }
        break;
        case STATE_GO_TO_CAPTURE:{
          sensorValue = analogRead(dirAnalogRead);
          if(sensorValue>sensorCaptureValue) {
            digitalWrite(stepPin, HIGH);
            delayMicroseconds(delayTime);
            digitalWrite(stepPin, LOW);
            delayMicroseconds(delayTime);
            m_captureCountStep++;
          } else {
            stateGoHome = STATE_GO_HOME_2;
            Serial.print("countStep:");
            Serial.println(m_captureCountStep);
            currentStep = m_captureCountStep;
            digitalWrite(dirPin, HIGH);
            delay(100);
          }
        }
        break;
        case STATE_GO_HOME_2:{
          if(currentStep > 0) {
            currentStep --;
            digitalWrite(stepPin, HIGH);
            delayMicroseconds(delayTime);
            digitalWrite(stepPin, LOW);
            delayMicroseconds(delayTime);
          } else {
            m_robot->m_motorParamList[motorID].currentStep = 0;
            stateGoHome = STATE_HOME_DONE;
            delay(100);            
          }
        }
        case STATE_HOME_DONE:{

        }
        break;
      }
    }
  }
}

void ApplicationArduino::hardwareStop(int motorID = MAX_MOTOR)
{
  // Not used
  // digitalWrite(enPin1, HIGH);
  // digitalWrite(enPin2, HIGH);
  digitalWrite(enPin5, HIGH);
  digitalWrite(enPinCapture, HIGH);
}

void ApplicationArduino::checkInput(){
	// for(unsigned int btnID = 0; btnID< MAX_BUTTON; btnID++) {
  //   if(m_buttonPin[btnID] != NONE_PIN) {
  //     pinMode(m_buttonPin[btnID], INPUT_PULLUP);
  //     storeButtonState(btnID, digitalRead(m_buttonPin[btnID]) == LOW);
  //     // this->printf("button[%d] %s\r\n",btnID, digitalRead(m_buttonPin[btnID]) == LOW ? "pressed" : "normal");
  //   }
	// }
  // m_limitGripperValue = analogRead(limitGripper);
  // // this->printf("Limit Gripper pin[%d] Value: %d\r\n", limitGripper, m_limitGripperValue);
}
#define DEBUG_SERIAL
int ApplicationArduino::readSerial(char* output, int length) {
  m_incomingByte = 0;
  while(Serial.available()) {
    delay(3);
    char c = Serial.read();
    m_command[m_incomingByte++] = c;
  }
  if(m_incomingByte > 0) {
    m_command[m_incomingByte] = '\0';
#if defined(DEBUG_SERIAL) && defined(DEBUG_COMMAND)
    Serial.print(m_command);
#endif
    for(int i=0; i< m_incomingByte; i++) {
      output[i] = m_command[i];
#if defined(DEBUG_SERIAL) && defined(DEBUG_COMMAND)
      Serial.print(output[i],HEX);
      Serial.print(" ");
#endif
    }
#if defined(DEBUG_SERIAL) && defined(DEBUG_COMMAND)
    Serial.print("\r\n new command\r\n");
#endif
  }
  return m_incomingByte;
}

bool ApplicationArduino::isLimitReached(int motorID, MOTOR_LIMIT_TYPE limitType)
                      {
  bool limitReached = true;
  switch(motorID){
    case MOTOR::MOTOR_ARM1: {
      limitReached = limitType == MOTOR_LIMIT_MIN || limitType == MOTOR_LIMIT_HOME ? 
                    (PIND & (1 << 2)) == 0 :
                    false;
    }
    break;
    case MOTOR::MOTOR_ARM2: {
      limitReached = limitType == MOTOR_LIMIT_MIN || limitType == MOTOR_LIMIT_HOME ? 
                    (PINJ & (1 << 1)) == 0 :
                    false;
    }
    break;
    case MOTOR::MOTOR_ARM5: {
      limitReached = limitType == MOTOR_LIMIT_MIN || limitType == MOTOR_LIMIT_HOME ? 
                    (PIND & (1 << 3)) == 0 :
                    false;
    }
    break;
    case MOTOR::MOTOR_CAPTURE: {
      m_limitGripperValue = readA13(); // Read the analog value from A13
      limitReached = limitType == MOTOR_LIMIT_MIN || limitType == MOTOR_LIMIT_HOME ? 
                    m_limitGripperValue > 630 :
                    m_limitGripperValue < 300;
    }
    break;
    default: break;
  }
  return limitReached;
}

void ApplicationArduino::enableEngine(bool enable) {
#ifdef DEBUG_COMMAND
  this->printf("%s engine\r\n",enable?"ENABLE":"DISABLE");
#endif
  if(enable) {
    digitalWrite(enPin1, LOW);
    digitalWrite(enPin2, LOW);
    digitalWrite(enPin5, LOW);
    digitalWrite(enPinCapture, LOW);
  } else {
    digitalWrite(enPin1, HIGH);
    digitalWrite(enPin2, HIGH);
    digitalWrite(enPin5, HIGH);
    digitalWrite(enPinCapture, HIGH);
  }
  m_engineEnabled = enable;
}

void ApplicationArduino::initDirection(int motorID, int direction)
{
#ifdef DEBUG_COMMAND
  Serial.print("initDirection motorID[");
  Serial.print(motorID);
  Serial.print("] direction=");
  Serial.println(direction);
#endif
  switch(motorID){
    case MOTOR::MOTOR_ARM1: {
      digitalWrite(dirPin1, direction < 0 ? LOW : HIGH);
    }
    break;
    case MOTOR::MOTOR_ARM2: {
      digitalWrite(dirPin2, direction > 0 ? LOW : HIGH);
    }
    break;
    case MOTOR::MOTOR_ARM5:
    {
      digitalWrite(dirPin5, direction < 0 ? LOW : HIGH);
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

void ApplicationArduino::initHardwareTimer(int timerID, float samplerate)
{
  // initialize timer with CTC mode and 1024 prescaler, and enable compare interrupt
  noInterrupts(); // disable all interrupts
  if(timerID == TIMER_ID_CHECK_COMMAND) {
    TCCR0A = (1 << WGM01); // Set CTC mode
    TCCR0B = (1 << CS02) | (1 << CS00); // 1024 prescaler
    OCR0A = 255; // Compare value
    TIMSK0 = (1 << OCIE0A); // Enable timer compare interrupt
  } else if(timerID == TIMER_ID_EXECUTE_MOTION) {
    TCCR1A = 0; // Set normal mode
    TCCR1B = 0;               // Reset Timer1 Control Register B
    TCNT1  = 0;               // Initialize counter value to 0
    OCR1A = 16000000.0f / samplerate-1; // Compare value (16MHz / (1024 * 0.5Hz) - 1)
    TCCR1B |= (1 << WGM12);   // Turn on CTC mode
    TCCR1B |= (1 << CS10);    // Set CS10 bit for NO prescaler
    TIMSK1 |= (1 << OCIE1A);  // Enable timer compare interrupt
  } else if(timerID == TIMER_ID_UPDATE_INPUT) {
    TCCR2A = (1 << WGM21); // Set CTC mode
    TCCR2B = 1 << CS20; // 1024 prescaler
    OCR2A = 16000000.0f / samplerate-1; // Compare value
    TIMSK2 = (1 << OCIE2A); // Enable timer compare interrupt
  }
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
          nextStatePulse = executePulseStepper2Wires(statePulse, countPulse, numWaitPulse,&PORTF ,0);
        }
          break;
        case MOTOR_ARM2: {
          nextStatePulse = executePulseStepper2Wires(statePulse, countPulse, numWaitPulse, &PORTF,6);
        }
          break;
        case MOTOR_ARM5: {
          nextStatePulse = executePulseStepper2Wires(statePulse, countPulse, numWaitPulse,&PORTL,3);
        }
          break;
        case MOTOR_CAPTURE: {
          nextStatePulse = executePulseStepper2Wires(statePulse, countPulse, numWaitPulse,&PORTA,4);
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
    initHardwareTimer(TIMER_ID_EXECUTE_MOTION, FREQUENCY_TIMER1);
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
      *portRegister |= (1 << bit);
      nextStatePulse = STATE_COMMAND2;
#ifdef DEBUG_PULSE
      Serial.print("STATE_COMMAND1 -> STATE_WAIT1\r\n");
#endif
    }
    break;
    case STATE_COMMAND2: {
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

ISR(TIMER2_COMPA_vect){
  // app.updateInputState();
}