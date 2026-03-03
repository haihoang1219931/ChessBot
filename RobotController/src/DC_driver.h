#ifndef DC_driver_h
#define DC_driver_h

#include <Arduino.h>

class DC_driver
{
  public:
    DC_driver(byte in1, byte in2);
    void stop();
    void setDir(int direction);
    void moveStep(int sleepTime);
  private:
    int m_direction;
    byte m_in1;
    byte m_in2;
    
};

#endif

