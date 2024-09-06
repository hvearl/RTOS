/*
* PROJECT:
* DESCRIPTION:
* AUTHOR: ROB HALL
* CONTACT: https://www.linkedin.com/in/robhall42

* LICENSE: CC-BY-NC (https://creativecommons.org/licenses/by-nc/4.0/)
    * This license allows reusers to distribute, remix, and adapt the project so long as:
    * It is used only for noncommercial purposes only. The project is to remain free for all users.
    * Attribution is given to the original project creator.
    * Attribution is also given to any other users that remix, add to, or adapt this code.

* CHANGELOG
    * 0.0.1 -- 2024-02-11 : Project created

* TO DO
    * Everything

* KNOWN ISSUES
    * None

* NOTES / IMPORTANT USAGE INFORMATION
    * None
*/

#ifndef DRV8825_DRIVER_LIBRARY_DRV8825_H
#define DRV8825_DRIVER_LIBRARY_DRV8825_H

#include "Arduino.h"

class DRV8825 {
public:
    // class constructor
    //  - setting default pins to 255 ensures they are mapped to a pin that does not exist
    //  - most NEMA17 stepper motors have 200 full steps per revolution so this is a good default value
    //  - lead screw pitch is the linear travel per revolution of the lead screw in mm
    DRV8825(char ID[],
            uint8_t dirPin,
            uint8_t stpPin,
            uint8_t slpPin,
            byte m0Pin = 255,
            byte m1Pin = 255,
            byte m2Pin = 255,
            byte enPin = 255,
            byte rstPin = 255,
            byte fltPin = 255,
            int fullStepsPerRev = 200,
            float leadScrewPitch = 8);

    // class methods
    void begin();
    void sleep() const;
    void wake() const;
    void enable() const;
    void disable() const;
    void setMaxSpeed(unsigned long speed);
    void setAcceleration(float acceleration);
    void setStepSize(uint8_t stepSize);
    void setTargetSteps(int steps);
    bool moveToTarget();
		bool accelerateToTarget();

private:
    // class private variables
    // constructor arguments
    char* _id;
		uint8_t _dirPin;
    uint8_t _stpPin;
    uint8_t _slpPin;
    uint8_t _m0Pin;
    uint8_t _m1Pin;
    uint8_t _m2Pin;
    uint8_t _enPin;
    uint8_t _rstPin;
    uint8_t _fltPin;
    int _fullStepsPerRev;
    float _leadScrewPitch;

    // pin initialization checks
    bool _mPinsInitialized = false;
    bool _enPinInitialized = false;
    bool _rstPinInitialized = false;
    bool _fltPinInitialized = false;

    // variables relating to motion
    uint8_t _microsteps;
    int _microstepsPerRev;
    float _stepLength;
    int _target;
    int _stepsToGo;
    float _maxSpeed;
    float _maxRPS;
    float _maxStepsPerSec;
    unsigned long _minDelayMicros;
    float _acceleration;
		unsigned long _previousMicros;
		bool _stepState;
		bool _printedStopped;

		// private methods
    void _checkPinInitializations();
};

#endif //DRV8825_DRIVER_LIBRARY_DRV8825_H
