#include <TMCStepper.h>
#include <AccelStepper.h>
#include "EZ2209.h"
#include "EZ2209_R.h"

TMC2209Stepper driverB(&SERIAL_PORT_B, R_SENSE, DRIVER_ADDRESS);
AccelStepper stepperB = AccelStepper(stepperB.DRIVER, STEP_PIN_B, DIR_PIN_B);

bool startup_B = true; // set false after homing
bool stalled_B = false;
bool shaftVal_B=false;

void stallInterruptB(){ // flag set for motor A when motor stalls
	stalled_B = true;
}
void motor_B_setup() {
    SERIAL_PORT_B.begin(115200);      // HW UART driverAs
    attachInterrupt(digitalPinToInterrupt(STALL_PIN_B), stallInterruptB, RISING);
    driverB.begin();             // Initiate pins and registeries
    driverB.rms_current(400);    // Set stepperA current to 600mA. The command is the same as command TMC2130.setCurrent(600, 0.11, 0.5);
    //driverB.en_pwm_mode(1);      // Enable extremely quiet stepping
    driverB.pwm_autoscale(1);
    driverB.microsteps(MICROSTEPS_A);
    driverB.TCOOLTHRS(0xFFFFF); // 20bit max
    driverB.SGTHRS(STALLB_VALUE);

    stepperB.setMaxSpeed(5000); // 100mm/s @ 80 steps/mm
    stepperB.setAcceleration(5000); // 2000mm/s^2
    stepperB.setEnablePin(EN_PIN_B);
    stepperB.setPinsInverted(false, false, true);
    stepperB.enableOutputs();
}

void motorBHome(){
   int homeDelay = 160;
    int backSteps = 50000;
    Serial.println("fast homing x");
    shaftVal_B = true;
    while(!stalled_B){ // Fast home x
        motor_B(10000, homeDelay);
    }
    stalled_B = false;
    delay(1000);
    Serial.println("backing off");
    shaftVal_B = false;
    motor_B(backSteps, homeDelay);

    Serial.println("slow homing x");
    shaftVal_B = true;
    while(!stalled_B){ // Slow home x
        motor_B(10000, homeDelay*2);
    }
    stalled_B = false;
    delay(1000);
    Serial.println("Finished Homing");
    stepperB.setCurrentPosition(0);
    stepperB.enableOutputs();
    delay(10);
}
void motor_B_run(int angle, int stepDelay){
  delayMicroseconds(stepDelay);
	stepperB.move(angle*200*MICROSTEPS_A/360);
    while(stepperB.distanceToGo())
    stepperB.run();
}
// Motor Function
void motor_B(int steps, int stepDelay){
    digitalWrite(EN_PIN_B, LOW);
    driverB.shaft(shaftVal_B);
    for(int i = 0; i<steps; i++){
        digitalWrite(STEP_PIN_B,HIGH);
        delayMicroseconds(stepDelay);
        digitalWrite(STEP_PIN_B,LOW);
        delayMicroseconds(stepDelay);
        if(stalled_B){
            i = steps;
        }
    }
    digitalWrite(EN_PIN_B, HIGH);
}
