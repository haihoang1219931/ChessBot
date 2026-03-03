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
  uint8_t executePulseStepper2Wires(uint8_t statePulse, uint32_t countPulse, uint32_t numWaitPulse, int stepPin);
  uint8_t executePulseStepper4Wires(uint8_t statePulse, uint32_t countPulse, uint32_t numWaitPulse, int direction,
    int stepPin1, int stepPin2, int stepPin3, int stepPin4);

private:
  void initHardwareTimer(float samplerate = 40000.0f);
private:
  int m_buttonPin[MAX_BUTTON];
  int m_limitGripperValue;
};

#endif // APPLICATIONARDUINO_H
