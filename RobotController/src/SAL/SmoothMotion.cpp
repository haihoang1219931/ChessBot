#include "SmoothMotion.h"
#include "Robot.h"
SmoothMotion::SmoothMotion(uint8_t id, Robot* robot):
  m_robot(robot),
  m_id(id),
  m_stateControl(MOTOR_EXECUTE_WAIT_COMMAND)
{
}

// void setAcc(float acceleration) {
//   if(acceleration <= 0){return;}
//   first_step_delay =1000*(sqrt(2*1.8)/acceleration);

//   Serial.print("Acceleration ->  ");
//   Serial.println(acceleration);
// }

void SmoothMotion::setupTarget(
  uint32_t stepsAccel, uint32_t stepsCruise, uint32_t stepsDecel, 
  int direction, uint8_t moveType,  uint32_t accelStartWaitPulse, uint32_t minWaitPulse)
{
  m_numStepAccel = stepsAccel;
  m_numStepCruise = stepsCruise;
  m_numStepDecel = stepsDecel;
  m_moveType = moveType;
  m_numWaitPulse = (float)accelStartWaitPulse;
  m_minWaitPulse = minWaitPulse;
  m_robot->initDirection(m_id,direction);
  resetPulse();
  resetAccelSteps();
  resetCruiseSteps();
  resetDecelSteps();
  changeStateControl(m_moveType);
}

float SmoothMotion::delayAccel(float stepCount, float delayCur) {
#ifndef DEBUG_COUNT_STEP
  float nextDelay = delayCur * (4.0f*stepCount - 1.0f) / (4.0f*stepCount + 1.0f);
  return nextDelay > m_minWaitPulse ? nextDelay: m_minWaitPulse;
#else
  return delayCur;
#endif
}

float SmoothMotion::delayDecel(float stepCount, float delayCur) {
#ifndef DEBUG_COUNT_STEP
  float nextDelay = delayCur * (4.0f*stepCount + 1.0f) / (4.0f*stepCount - 1.0f);
  return nextDelay;
#else
  return delayCur;
#endif
}

void SmoothMotion::changeStateControl(int newState)
{
  if(m_stateControl != newState) {
    m_stateControl = newState;
    // switch(m_stateControl){
    //   case MOTOR_EXECUTE_INCREASE_SPEED: Serial.println("Accel");
    //   break;
    //   case MOTOR_EXECUTE_CRUISE_SPEED: Serial.println("Cruise");
    //   break;
    //   case MOTOR_EXECUTE_DECREASE_SPEED: Serial.println("Decel");
    //   break;
    // }
  }
}

void SmoothMotion::motionControlLoop() {
  switch (m_stateControl) {
    case MOTOR_EXECUTE_INCREASE_SPEED:{
      increaseSpeed();
    }
    break;
    case MOTOR_EXECUTE_CRUISE_SPEED:{
      cruiseSpeed();
    }
    break;
    case MOTOR_EXECUTE_DECREASE_SPEED:{
      decreaseSpeed();
    }
    break;
    case MOTOR_EXECUTE_HOME:{
      homing();
    }
    break;
    case MOTOR_EXECUTE_DONE: {
      m_stateControl = MOTOR_EXECUTE_WAIT_COMMAND;
    }
    break;
  }
}
void SmoothMotion::homing()
{
    if(m_robot->isLimitReached(m_id,MOTOR_LIMIT_HOME)){
      changeStateControl(MOTOR_EXECUTE_DONE);
      return;
    }
    m_statePulse = pulseLoop();
    if(m_statePulse != STATE_DONE) return;
    resetPulse();
    increaseCruiseSteps();
}
void SmoothMotion::increaseSpeed() {
  if(m_stepCountAccel >= m_numStepAccel){
    changeStateControl(MOTOR_EXECUTE_CRUISE_SPEED);
    return;
  }
  m_statePulse = pulseLoop();
  if(m_statePulse != STATE_DONE) return;
  resetPulse();
  increaseAccelSteps();
  m_numWaitPulse = delayAccel(m_stepCountAccel,m_numWaitPulse);

}

void SmoothMotion::cruiseSpeed() {
  if(m_stepCountCruise >= m_numStepCruise){
    changeStateControl(m_moveType == MOTOR_EXECUTE_INCREASE_SPEED?
                           MOTOR_EXECUTE_DECREASE_SPEED:MOTOR_EXECUTE_DONE);
    return;
  }
  m_statePulse = pulseLoop();
  if(m_statePulse != STATE_DONE) return;
  resetPulse();
  increaseCruiseSteps();
}

void SmoothMotion::decreaseSpeed() {
  if(m_stepCountDecel >= m_numStepDecel) {
    changeStateControl(MOTOR_EXECUTE_DONE);
    return;
  }
  m_statePulse = pulseLoop();
  if(m_statePulse != STATE_DONE) return;
  increaseDecelSteps();
  m_numWaitPulse = delayDecel(m_numStepDecel - m_stepCountDecel,m_numWaitPulse);
}

void SmoothMotion::resetAccelSteps()
{
    m_stepCountAccel = 0;
}

void SmoothMotion::resetCruiseSteps()
{
    m_stepCountCruise = 0;
}

void SmoothMotion::resetDecelSteps()
{
    m_stepCountDecel = 0;
}

void SmoothMotion::increaseAccelSteps()
{
    m_stepCountAccel++;
    m_robot->updateCurrentStep((int)m_id);
}

void SmoothMotion::increaseCruiseSteps()
{
    m_stepCountCruise++;
    m_robot->updateCurrentStep((int)m_id);
}

void SmoothMotion::increaseDecelSteps()
{
    m_stepCountDecel++;
    m_robot->updateCurrentStep((int)m_id);
}

uint8_t SmoothMotion::pulseLoop()
{
  return m_robot->pulseLoop((int)m_id);
}

void SmoothMotion::resetPulse()
{
  m_robot->resetPulse((int)m_id);
}

uint32_t SmoothMotion::getCurrentSteps()
{
  return m_stepCountAccel + m_stepCountCruise + m_stepCountDecel;
}
