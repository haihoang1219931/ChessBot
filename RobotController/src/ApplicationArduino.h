#ifndef APPLICATIONARDUINO_H
#define APPLICATIONARDUINO_H

#include <Arduino.h>
#include "SAL/ApplicationController.h"
typedef enum {
  TIMER_ID_CHECK_COMMAND,
  TIMER_ID_UPDATE_INPUT,
  TIMER_ID_EXECUTE_MOTION,
} TIMER_ID;
typedef enum {
  STATE_CHECK_SENSOR,
  STATE_FIND_HOME_POS_DIR_CAPTURE,
  STATE_FIND_HOME_POS_DIR_HOME,
  STATE_GO_TO_CAPTURE,
  STATE_GO_HOME,
  STATE_HOME_DONE,
} STATE_HOMING;

class ApplicationArduino : public ApplicationController
{
public:
	ApplicationArduino();
  ~ApplicationArduino();
  void initRobot() override;
  void specificPlatformGohome(int motorID = MAX_MOTOR) override;
  void hardwareStop(int motorID = MAX_MOTOR) override;
  void checkInput() override;
  int printf(const char *fmt, ...) override;
  void msleep(int millis) override;
  long getSystemTime() override;
  void enableEngine(bool enable) override;
  bool isLimitReached(int motor,
                      MOTOR_LIMIT_TYPE limitType) override;
  int readSerial(char* output, int length) override;
  void initDirection(int motorID, int direction) override;
  void moveDoneAction(int motorID) override;
  uint8_t executePulseLoop(int motorID) override;
  void enableHardwareTimer(bool enable) override;
  void resetPulse(int motorID) override;
  uint8_t executePulseStepper2Wires(uint8_t statePulse, uint32_t countPulse, uint32_t numWaitPulse, volatile uint8_t* portRegister, int bit);
  void initHardwareTimer(int timerID, float samplerate = 40000.0f);
  int16_t readA13();
private:
  va_list m_args;
  char m_buffer[256];    
  char m_command[64];
  uint8_t m_incomingByte;
  // uint8_t m_buttonPin[MAX_BUTTON];
  int16_t m_limitGripperValue;

};

#endif // APPLICATIONARDUINO_H
