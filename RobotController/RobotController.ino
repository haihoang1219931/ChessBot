#include "src/ApplicationArduino.h"

extern ApplicationArduino app;
void setup() {
  Serial.begin(38400);
  Serial.println("======Arduino Serial======");
  app.printf("APP Arduino Init done\r\n");
  delay(100);
  app.initHardwareTimer(TIMER_ID_CHECK_COMMAND, 100.0f);
}
void loop() {
  app.loop();
  delay(1);
}