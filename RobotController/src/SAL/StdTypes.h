#ifndef STDTYPES_H
#define STDTYPES_H

#include <stdio.h>

#define MAX_MOVE_SEQUENCE 6
#define MAX_BUTTON 6
//#define MAX_MOTOR 6
#define MAX_COMMAND_LENGTH 256
#define MAX_PARAMS_CHESSBOARD 4

#if defined(__linux__) || defined(_WIN32)
 #ifndef _UINT8_T_DECLARED
 #define _UINT8_T_DECLARED
 typedef unsigned char uint8_t;
 #endif

 #ifndef _UINT16_T_DECLARED
 #define _UINT16_T_DECLARED
 typedef unsigned short uint16_t;
 #endif

 #ifndef _UINT32_T_DECLARED
 #define _UINT32_T_DECLARED
 typedef unsigned int uint32_t;
 #endif
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef enum {
    ANGLE_RAD,
    ANGLE_DEGREE
}ANGLE_TYPE;

typedef enum {
    MOTOR_EXECUTE_WAIT_COMMAND,
    MOTOR_EXECUTE_INCREASE_SPEED,
    MOTOR_EXECUTE_CRUISE_SPEED,
    MOTOR_EXECUTE_DECREASE_SPEED,
    MOTOR_EXECUTE_HOME,
    MOTOR_EXECUTE_DONE,
} MOTOR_CONTROL_STATE;

typedef enum {
  // For 4 wires stepper
  STATE_COMMAND1,
  STATE_COMMAND2,
  STATE_COMMAND3,
  STATE_COMMAND4,
  STATE_COMMAND5,
  STATE_COMMAND6,
  STATE_COMMAND7,
  STATE_COMMAND8,

  // Wait state
  STATE_WAIT1,
  STATE_WAIT2,
  STATE_WAIT3,
  STATE_WAIT4,
  STATE_WAIT5,
  STATE_WAIT6,
  STATE_WAIT7,
  STATE_WAIT8,

  // Done state
  STATE_PENDING,
  STATE_DONE,
} STATE_PULSE;

typedef enum {
    MOTOR_LIMIT_MIN,
    MOTOR_LIMIT_MAX,
    MOTOR_LIMIT_HOME,
}MOTOR_LIMIT_TYPE;

typedef enum {
    MOTOR_ENABLED,
    MOTOR_DISABLED
}MOTOR_ACTIVE_STATE;

typedef enum {
  MOTOR_TYPE_STEPPER_2_WIRES,
  MOTOR_TYPE_STEPPER_4_WIRES,
} MOTOR_TYPE;

typedef enum{
    ROBOT_INIT,
    ROBOT_EXECUTE_GO_HOME,
    ROBOT_EXECUTE_POSITION,
    ROBOT_EXECUTE_SEQUENCE,
    ROBOT_EXECUTE_DONE,
}ROBOT_STATE;

typedef enum{
    ROBOT_MOVE_EXECUTE_INIT,
    ROBOT_MOVE_EXECUTE_CHECK_RESULT,
    ROBOT_MOVE_CAPTURE_INIT,
    ROBOT_MOVE_CAPTURE_CHECK_RESULT,
    ROBOT_MOVE_DONE,
}ROBOT_SEQUENCE_STATE;

typedef enum {
    MACHINE_INIT,
    MACHINE_WAIT_COMMAND,
    MACHINE_EXECUTE_HOME,
    MACHINE_EXECUTE_POSITION,
    MACHINE_EXECUTE_POSITION_STANDBY,
    MACHINE_EXECUTE_COMMAND,
    MACHINE_EXECUTE_COMMAND_DONE,
}MACHINE_STATE;

typedef enum {
    MOTION_JOINT_SPACE,
    MOTION_CARTESION_SPACE,
}MOTION_SPACES;

typedef enum {
    MOVE_NORMAL,
    MOVE_ATTACK,
    MOVE_PASTPAWN,
    MOVE_CASTLE,
    MOVE_PROMOTE,
}MOVE_TYPE;

typedef enum {
    ZONE_MACHINE,
    ZONE_GUEST
}ZONE_TYPE;

typedef enum {
    COMMAND_NORMAL,
    COMMAND_LINE,
    COMMAND_ARC,
}COMMAND_TYPE;

typedef enum {
    COMMAND_STATE_INIT,
    COMMAND_STATE_EXECUTE,
    COMMAND_STATE_EXECUTE_THEN_RECAL,
    COMMAND_STATE_EXECUTE_THEN_DONE,
    COMMAND_STATE_DONE,
}COMMAND_STATE;

typedef enum {
    COMMAND_SEQUENCE_STATE_INIT,
    COMMAND_SEQUENCE_STATE_EXECUTE,
    COMMAND_SEQUENCE_STATE_DONE,
}COMMAND_SEQUENCE_STATE;

typedef struct{
    float x;
    float y;
    float z;
}Point;

typedef struct {
    double length;
    double angle_rad;
} Edge;

typedef struct{
    bool active;
    float scale;
    float length;
    float initAngle;
    float homeAngle;
    int homeStepTime;
    float minAngle;
    float maxAngle;
    int minPulsePerStep;
    float frequency;
    int numStepAccel;
    int currentStep;
    int startStep;
    int targetStep;
    int direction;
}JointParam;


typedef enum {
  MOTOR_CAPTURE,
  MOTOR_ARM1,
  MOTOR_ARM2,
  MOTOR_ARM3,
  MOTOR_ARM4,
  MOTOR_ARM5,
  MAX_MOTOR
} MOTOR;

typedef struct {
    int steps;
    int stepTime;
    int active;
} Joint;

typedef struct {
    Joint jointSteps[MAX_MOTOR];
} Move;

typedef struct {
    float x;
    float y;
    float updownAngle;
    int captureStep;
    int type;
} Command;

#endif // STDTYPES_H
