#include "HardwareSerial.h"
/*
  DC_driver.cpp - Library to use DC motor drivers such as the L298N.
  Created by Csermendy Bendeguz, March 29, 2025.
*/

#include "Arduino.h"
#include "DC_driver.h"

DC_driver::DC_driver(byte in1, byte in2 )
{
  m_in1 = in1;
  m_in2 = in2;
}

void DC_driver::setDir(int direction)
{
  pinMode(m_in1, OUTPUT);
  pinMode(m_in2, OUTPUT);
  m_direction = direction;
  digitalWrite(m_in1, LOW);
  digitalWrite(m_in2, LOW);
}

void DC_driver::moveStep(int sleepTime)
{
  digitalWrite(m_in1, m_direction > 0 ? HIGH:LOW);
  digitalWrite(m_in2, m_direction > 0 ? LOW:HIGH);
}

void DC_driver::stop() {
  digitalWrite(m_in1, LOW);
  digitalWrite(m_in2, LOW);
}