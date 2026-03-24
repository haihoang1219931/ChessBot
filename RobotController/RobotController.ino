#include "src/ApplicationArduino.h"

extern ApplicationArduino app;
void setup() {
  Serial.begin(38400);
  Serial.println("======Arduino Serial======");
  app.printf("APP Arduino Init done\r\n");
  delay(100);
}
void loop() {
  app.loop();
  delay(1);
}