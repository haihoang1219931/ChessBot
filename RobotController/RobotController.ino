#include "src/ApplicationArduino.h"

extern ApplicationArduino app;
void setup() {
  Serial.begin(38400);
  Serial.println("======Arduino Serial======");
  app.printf("APP Arduino Init done\r\n");
  delay(100);
  // app.executeCommand("e");
  // app.executeCommand("h2");

  // for(int i=0; i< 10000; i++)  {
  //   // app.loop();
  //   app.executeSmoothMotionLoop(MOTOR_ARM2);
  //   delayMicroseconds(800);
  // }
}
void loop() {
  app.loop();
  delay(1);
}