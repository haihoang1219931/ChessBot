#ifndef APPLICATIONARDUINO_H
#define APPLICATIONARDUINO_H

#include <Arduino.h>
#include "SAL/ApplicationController.h"

class ApplicationArduino : public ApplicationController
{
public:
	ApplicationArduino();
  ~ApplicationArduino();
  void initRobot() override;
  void specificPlatformGohome(int motorID = MAX_MOTOR) override;
  void harwareStop(int motorID = MAX_MOTOR) override;
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

private:
  void initHardwareTimer(float samplerate = 40000.0f);
private:
  uint8_t m_buttonPin[MAX_BUTTON];
  int16_t m_limitGripperValue;
};

#endif // APPLICATIONARDUINO_H
