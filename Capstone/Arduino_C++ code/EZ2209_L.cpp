#include <TMCStepper.h>
#include <AccelStepper.h>
#include "EZ2209.h"
#include "EZ2209_L.h"

TMC2209Stepper driverA(&SERIAL_PORT, R_SENSE, DRIVER_ADDRESS); // Define TMC2209 driver for motor A
AccelStepper stepperA = AccelStepper(stepperA.DRIVER, STEP_PIN, DIR_PIN); // Define AccelStepper for motor A

bool startup = true; // Flag to indicate if the system is in startup mode
bool stalled_A = false; // Flag to indicate if motor A is stalled
bool shaftVal=false; // Value to control the direction of motor rotation

void stallInterruptA(){ // Interrupt service routine to set stall flag for motor A when stalled
	stalled_A = true;
}

void motor_A_setup() {
    SERIAL_PORT.begin(115200);      // Start serial communication
    attachInterrupt(digitalPinToInterrupt(STALL_PIN_A), stallInterruptA, RISING); // Attach stall interrupt for motor A
    driverA.begin();             // Initialize TMC2209 driver for motor A
    driverA.rms_current(200);    // Set stepper current to 600mA
    driverA.pwm_autoscale(1); // Enable automatic PWM scaling
    driverA.microsteps(MICROSTEPS_A); // Set microsteps for motor A
    driverA.TCOOLTHRS(0xFFFFF); // Set thermal cool threshold
    driverA.SGTHRS(STALLA_VALUE); // Set stall guard threshold

    stepperA.setMaxSpeed(5000); // Set maximum speed for motor A
    stepperA.setAcceleration(5000); // Set acceleration for motor A
    stepperA.setEnablePin(EN_PIN); // Set enable pin for motor A
    stepperA.setPinsInverted(false, false, true); // Set pin inversion for motor A
    stepperA.enableOutputs(); // Enable outputs for motor A
}

void motorAHome(){
    int homeDelay = 160; // Delay for homing
    int backSteps = 50000; // Number of steps to move back after homing
    Serial.println("fast homing x"); // Print message for fast homing
    shaftVal = true; // Set shaft value for motor movement
    while(!stalled_A){ // Fast home
        motor(10000, homeDelay); // Move motor at high speed
    }
    stalled_A = false; // Reset stall flag
    delay(1000); // Delay before moving back
    Serial.println("backing off"); // Print message for backing off
    shaftVal = false; // Set shaft value for movement back
    motor(backSteps, homeDelay); // Move motor back

    Serial.println("slow homing x"); // Print message for slow homing
    shaftVal = true; // Set shaft value for slow homing
    while(!stalled_A){ // Slow home
        motor(10000, homeDelay*2); // Move motor at slow speed
    }
    stalled_A = false; // Reset stall flag
    delay(1000); // Delay before finishing homing
    Serial.println("Finished Homing"); // Print message for finishing homing
    stepperA.setCurrentPosition(0); // Set current position to 0
    stepperA.enableOutputs(); // Enable motor outputs
    delay(10); // Small delay
}

void motor_A_run(int angle, int stepDelay){
    delayMicroseconds(stepDelay); // Delay between steps
    stepperA.move(angle*200*MICROSTEPS_A/360); // Calculate steps based on angle and microsteps
    while(stepperA.distanceToGo()) // Move motor until it reaches the target position
        stepperA.run();
}

// Function to move motor
void motor(int steps, int stepDelay){
    digitalWrite(EN_PIN, LOW); // Enable motor
    driverA.shaft(shaftVal); // Set shaft direction
    for(int i = 0; i<steps; i++){ // Loop through steps
        digitalWrite(STEP_PIN,HIGH); // Step high
        delayMicroseconds(stepDelay); // Delay
        digitalWrite(STEP_PIN,LOW); // Step low
        delayMicroseconds(stepDelay); // Delay
        if(stalled_A){ // Check if motor is stalled
            i = steps; // Exit loop
        }
    }
    digitalWrite(EN_PIN, HIGH); // Disable motor
}
