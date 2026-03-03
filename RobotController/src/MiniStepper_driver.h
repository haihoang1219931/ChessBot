#ifndef MINISTEPPER_DRIVER_H
#define MINISTEPPER_DRIVER_H

#include "Arduino.h"

class MiniStepper_driver
{
  public:
    MiniStepper_driver(int pin1, int pin2, int pin3, int pin4);
    void enable(bool enableDriver);
    void setDir(int dir);
    void moveStep(int sleepTime);    
  public:
    int m_pin1;
    int m_pin2;
    int m_pin3;
    int m_pin4;
    int m_dir;
};

#endif

