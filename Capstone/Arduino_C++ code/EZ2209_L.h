#ifndef EZ2209_L_H
#define EZ2209_L_H

#include <TMCStepper.h>
#include <AccelStepper.h>
#include "EZ2209.H"
//PINOUT
#define DIR_PIN          1 // Direction
#define STEP_PIN         2// Step
#define STALL_PIN_A      3 // Connected to DIAG pin on the TMC2209
#define EN_PIN           4 // Enable
//SERIAL PINS TX2, RX2
#define SERIAL_PORT Serial2 // TMC2209 HardwareSerial port
#define STALLA_VALUE 50  

extern TMC2209Stepper driverA;
extern AccelStepper stepperA;
//FLAGS
extern bool startup;
extern bool stalled_A;
extern bool shaftVal;
//FUNCTIONS
void stallInterruptX();
void motorAHome();
void motor_A_setup();
void motor_A_run(int angle, int stepDelay);
void motor(int steps, int stepDelay);

#endif
