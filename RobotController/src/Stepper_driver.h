#ifndef STEPPER_DRIVER_H
#define STEPPER_DRIVER_H

#include "Arduino.h"

class Stepper_driver
{
  public:
    Stepper_driver(int enablePin, int stepPin, int dirPin);
    void init();
    void enable(bool enableDriver);
    void setDir(int dir);
    void moveStep(int sleepTime);    
  private:
    int m_enablePin;
    int m_stepPin;
    int m_dirPin;
};

#endif

