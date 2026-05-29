#include "src/ApplicationArduino.h"
#include "src/SAL/Robot.h"
extern ApplicationArduino app;
void setup() {
  Serial.begin(38400);
  Serial.println("======Arduino Serial======");
  app.printf("APP Arduino Init done\r\n");
  delay(100);
  // delay(2900);
  // app.enableEngine(true);
  // int jointSteps[MAX_MOTOR] = {0,1200,273,0,0,0};
  // app.m_robot->setMoveTarget(jointSteps);
  // app.m_robot->initMove(MOTOR_ARM1, MOTOR_ARM1);
  // delay(1000);
  // unsigned long startTime, endTime , duration; 
  // for(int i=0 ;i< 20000; i++) {
  //   startTime = micros();
  //   app.executeSmoothMotionLoop(MOTOR_ARM1);
  //   app.executeSmoothMotionLoop(MOTOR_ARM2);
  //   app.executeSmoothMotionLoop(MOTOR_ARM5);
  //   app.executeSmoothMotionLoop(MOTOR_CAPTURE);
  //   endTime = micros(); 
  //   duration = endTime - startTime;
  //   // if(duration > 500) {
  //   //   Serial.print("i[");
  //   //   Serial.print(i);
  //   //   Serial.print("] ");
  //   //   Serial.println(duration);
  //   // }
  //   delayMicroseconds(20);
  // }
  // delay(3000);
  // app.enableEngine(false);
}
void loop() {
  app.loop();
  delay(30);
}