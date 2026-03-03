#include "MiniStepper_driver.h"

MiniStepper_driver::MiniStepper_driver(int pin1, int pin2, int pin3, int pin4):
  m_pin1(pin1),
  m_pin2(pin2),
  m_pin3(pin3),
  m_pin4(pin4)
{
  pinMode(m_pin1, OUTPUT);
  pinMode(m_pin2, OUTPUT);
  pinMode(m_pin3, OUTPUT);
  pinMode(m_pin4, OUTPUT);
}

void MiniStepper_driver::enable(bool enableDriver)
{
  if(enableDriver) {
    // Enable driver
  } else {
    // Disable driver
    digitalWrite(m_pin1, LOW);
    digitalWrite(m_pin2, LOW);
    digitalWrite(m_pin3, LOW);
    digitalWrite(m_pin4, LOW);
  }
}

void MiniStepper_driver::setDir(int dir)
{
  m_dir = dir;
}

void MiniStepper_driver::moveStep(int sleepTime)
{
  if(m_dir < 0)
  {
    //1
    digitalWrite(m_pin1, HIGH);
    digitalWrite(m_pin2, LOW);
    digitalWrite(m_pin3, LOW);
    digitalWrite(m_pin4, LOW);
    delayMicroseconds(sleepTime);
    //2
    digitalWrite(m_pin1, HIGH);
    digitalWrite(m_pin2, HIGH);
    digitalWrite(m_pin3, LOW);
    digitalWrite(m_pin4, LOW);
    delayMicroseconds(sleepTime);
    //3
    digitalWrite(m_pin1, LOW);
    digitalWrite(m_pin2, HIGH);
    digitalWrite(m_pin3, LOW);
    digitalWrite(m_pin4, LOW);
    delayMicroseconds(sleepTime);
    //4
    digitalWrite(m_pin1, LOW);
    digitalWrite(m_pin2, HIGH);
    digitalWrite(m_pin3, HIGH);
    digitalWrite(m_pin4, LOW);
    delayMicroseconds(sleepTime);
    //5
    digitalWrite(m_pin1, LOW);
    digitalWrite(m_pin2, LOW);
    digitalWrite(m_pin3, HIGH);
    digitalWrite(m_pin4, LOW);
    delayMicroseconds(sleepTime);
    //6
    digitalWrite(m_pin1, LOW);
    digitalWrite(m_pin2, LOW);
    digitalWrite(m_pin3, HIGH);
    digitalWrite(m_pin4, HIGH);
    delayMicroseconds(sleepTime);
    //7
    digitalWrite(m_pin1, LOW);
    digitalWrite(m_pin2, LOW);
    digitalWrite(m_pin3, LOW);
    digitalWrite(m_pin4, HIGH);
    delayMicroseconds(sleepTime);
    //8
    digitalWrite(m_pin1, HIGH);
    digitalWrite(m_pin2, LOW);
    digitalWrite(m_pin3, LOW);
    digitalWrite(m_pin4, HIGH);
    delayMicroseconds(sleepTime);
  } 
  
  else{
    //1
    digitalWrite(m_pin1, HIGH);
    digitalWrite(m_pin2, LOW);
    digitalWrite(m_pin3, LOW);
    digitalWrite(m_pin4, HIGH);
    delayMicroseconds(sleepTime);
    //2
    digitalWrite(m_pin1, LOW);
    digitalWrite(m_pin2, LOW);
    digitalWrite(m_pin3, LOW);
    digitalWrite(m_pin4, HIGH);
    delayMicroseconds(sleepTime);
    //3
    digitalWrite(m_pin1, LOW);
    digitalWrite(m_pin2, LOW);
    digitalWrite(m_pin3, HIGH);
    digitalWrite(m_pin4, HIGH);
    delayMicroseconds(sleepTime);
    //4
    digitalWrite(m_pin1, LOW);
    digitalWrite(m_pin2, LOW);
    digitalWrite(m_pin3, HIGH);
    digitalWrite(m_pin4, LOW);
    delayMicroseconds(sleepTime);
    //5
    digitalWrite(m_pin1, LOW);
    digitalWrite(m_pin2, HIGH);
    digitalWrite(m_pin3, HIGH);
    digitalWrite(m_pin4, LOW);
    delayMicroseconds(sleepTime);
    //6
    digitalWrite(m_pin1, LOW);
    digitalWrite(m_pin2, HIGH);
    digitalWrite(m_pin3, LOW);
    digitalWrite(m_pin4, LOW);
    delayMicroseconds(sleepTime);
    //7
    digitalWrite(m_pin1, HIGH);
    digitalWrite(m_pin2, HIGH);
    digitalWrite(m_pin3, LOW);
    digitalWrite(m_pin4, LOW);
    delayMicroseconds(sleepTime);
    //8
    digitalWrite(m_pin1, HIGH);
    digitalWrite(m_pin2, LOW);
    digitalWrite(m_pin3, LOW);
    digitalWrite(m_pin4, LOW);
    delayMicroseconds(sleepTime);
  }
}