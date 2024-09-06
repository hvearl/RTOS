#ifndef EZ2209_R_H
#define EZ2209_R_H

#include <TMCStepper.h>
#include <AccelStepper.h>
#include "EZ2209.h"
//PINOUT
#define DIR_PIN_B          26 // Direction
#define STEP_PIN_B        27// Step
#define STALL_PIN_B      28// Connected to DIAG pin on the TMC2209
#define EN_PIN_B         29 // Enable
//SERIAL PINS TX6, RX6
#define SERIAL_PORT_B Serial6 // TMC2209 HardwareSerial port
#define STALLB_VALUE 50

extern TMC2209Stepper driverB;
extern AccelStepper stepperB;
//FLAGS
extern bool startup_B;
extern bool stalled_B;
extern bool shaftVal_B;
//FUNCTIONS
void stallInterruptB();
void motorBHome();
void motor_B_setup();
void motor_B_run(int angle, int stepDelay);
void motor_B(int steps, int stepDelay);

#endif
