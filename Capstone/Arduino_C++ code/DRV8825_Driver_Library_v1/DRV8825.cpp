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

#include "Arduino.h"
#include "DRV8825.h"

DRV8825::DRV8825(char ID[],
                 uint8_t dirPin,
                 uint8_t stpPin,
                 uint8_t slpPin,
                 byte m0Pin,
                 byte m1Pin,
                 byte m2Pin,
                 byte enPin,
                 byte rstPin,
                 byte fltPin,
                 int fullStepsPerRev,
                 float leadScrewPitch) {

  // create private member variables
  _id = ID;
	_dirPin = dirPin;
  _stpPin = stpPin;
  _slpPin = slpPin;
  _m0Pin = m0Pin;
  _m1Pin = m1Pin;
  _m2Pin = m2Pin;
  _enPin = enPin;
  _rstPin = rstPin;
  _fltPin = fltPin;
  _fullStepsPerRev = fullStepsPerRev;
  _leadScrewPitch = leadScrewPitch;
  
  // initial motor states
  _microsteps = 1; // default to full steps
  _minDelayMicros = 100; // smallest time in between steps in us based on maxSpeed
	_maxSpeed = 3; // revs/sec
  _target = 0;
  _stepsToGo = 0;
	_previousMicros = 0;
	_printedStopped = false;
	_stepState = false;
}

void DRV8825::begin() {
  _checkPinInitializations();

  pinMode(_stpPin, OUTPUT);
  pinMode(_dirPin, OUTPUT);

  if (_mPinsInitialized) {
    pinMode(_m0Pin, OUTPUT);
    pinMode(_m1Pin, OUTPUT);
    pinMode(_m2Pin, OUTPUT);
  }
  if (_enPinInitialized) {
    pinMode(_enPin, OUTPUT);
		digitalWrite(_enPin, LOW);
  }
  if (_rstPinInitialized) {
    pinMode(_rstPin, OUTPUT);
		digitalWrite(_rstPin, HIGH);
  }
  if (_fltPinInitialized) {
    pinMode(_fltPin, INPUT);
  }


}

void DRV8825::_checkPinInitializations() {
  if (_m0Pin != 255 && _m1Pin != 255 && _m2Pin != 255) {
    _mPinsInitialized = true;
  }
  else {
    _mPinsInitialized = false;
  }

  if (_enPin != 255) {
    _enPinInitialized = true;
  }
  else {
    _enPinInitialized = false;
  }

  if (_rstPin != 255) {
    _rstPinInitialized = true;
  }
  else {
    _rstPinInitialized = false;
  }

  if (_fltPin != 255) {
    _fltPinInitialized = true;
  }
  else {
    _fltPinInitialized = false;
  }
}

void DRV8825::sleep() const {
  digitalWrite(_slpPin, LOW);
//  Serial.println("Time for sleep... Goodnight!");
}

void DRV8825::wake() const {
  digitalWrite(_slpPin, HIGH);
//  Serial.println("Rise and shine!");
  delay(1); // driver needs 1msec to stabilize the charge pump before taking a step
}

void DRV8825::enable() const {
  if (_enPin != 255) {
    digitalWrite(_enPin, LOW);
    delay(1); // unsure if this is necessary or only needed for waking up
  }
  else {
    Serial.println("Enable pin was not defined in the class constructor.");
  }
}

void DRV8825::disable() const {
  if (_enPin != 255) {
    digitalWrite(_enPin, HIGH);
  }
  else {
    Serial.println("Enable pin was not defined in the class constructor.");
  }
}

void DRV8825::setMaxSpeed(unsigned long speed) {
  _minDelayMicros = speed;
	Serial.println(_minDelayMicros);
}

void DRV8825::setAcceleration(float acceleration) {
  _acceleration = acceleration;
}

void DRV8825::setStepSize(uint8_t stepSize){
  _microsteps = stepSize;
  if (_mPinsInitialized) {
    switch (stepSize) {
      case 1:
        digitalWrite(_m0Pin, LOW);
        digitalWrite(_m1Pin, LOW);
        digitalWrite(_m2Pin, LOW);
        break;
      case 2:
        digitalWrite(_m0Pin, HIGH);
        digitalWrite(_m1Pin, LOW);
        digitalWrite(_m2Pin, LOW);
        break;
      case 4:
        digitalWrite(_m0Pin, LOW);
        digitalWrite(_m1Pin, HIGH);
        digitalWrite(_m2Pin, LOW);
        break;
      case 8:
        digitalWrite(_m0Pin, HIGH);
        digitalWrite(_m1Pin, HIGH);
        digitalWrite(_m2Pin, LOW);
        break;
      case 16:
        digitalWrite(_m0Pin, LOW);
        digitalWrite(_m1Pin, LOW);
        digitalWrite(_m2Pin, HIGH);
        break;
      case 32:
        digitalWrite(_m0Pin, HIGH);
        digitalWrite(_m1Pin, HIGH);
        digitalWrite(_m2Pin, HIGH);
        break;
      default:
        stepSize = 1;
        digitalWrite(_m0Pin, LOW);
        digitalWrite(_m1Pin, LOW);
        digitalWrite(_m2Pin, LOW);
        Serial.println("Invalid step size. Input must be 1, 2, 4, 8, 16, or 32. Defaulting to full steps.");
        break;
    }

    // print successful update
    if (stepSize == 1) {
      Serial.print("Step size successfully set to full steps");
    }
    else {
      Serial.print("Step size successfully set to 1/");
      Serial.print(stepSize);
      Serial.println(" steps.");
    }
  }
  else {
    Serial.println("Microstep select pins were not defined in the class constructor.");
  }
}

void DRV8825::setTargetSteps(int steps) {
  _target = steps;
  _stepsToGo = _target;
	_printedStopped = false;
}

bool DRV8825::moveToTarget() {

	if (_stepsToGo == 0) {
		if (_printedStopped) {
			Serial.println("No more steps to move.");
			_printedStopped = true;
		}
		return false;
	}

	else {
		unsigned long _currentMicros = micros();
		Serial.println(_currentMicros);
		Serial.println(_stepsToGo);
		if (_stepsToGo > 0) {
			digitalWrite(_dirPin, HIGH);
			_stepsToGo--;
		}
		else {
			digitalWrite(_dirPin, LOW);
			_stepsToGo++;
		}

		if (_stepState == LOW && _currentMicros - _previousMicros >= _minDelayMicros) {
			digitalWrite(_stpPin, HIGH);
			_stepState = HIGH;
			_previousMicros = micros();
			return true;
		}
		else if (_stepState == HIGH && _currentMicros - _previousMicros >= _minDelayMicros) {
			digitalWrite(_stpPin, LOW);
			_stepState = LOW;
			_previousMicros = micros();
			return true;
		}
		else {
			return true;
		}
	}
}

bool DRV8825::accelerateToTarget() {
	if (_stepsToGo == 0) {
		Serial.println("No more steps to move.");
		return false;
	} else if (_stepsToGo > 0) {
		digitalWrite(_dirPin, HIGH);
		digitalWrite(_stpPin, HIGH);
		delayMicroseconds(_minDelayMicros);
		digitalWrite(_stpPin, LOW);
		delayMicroseconds(_minDelayMicros);
		_stepsToGo--;
		return true;
	} else {
		digitalWrite(_dirPin, LOW);
		digitalWrite(_stpPin, HIGH);
		delayMicroseconds(_minDelayMicros);
		digitalWrite(_stpPin, LOW);
		delayMicroseconds(_minDelayMicros);
		_stepsToGo++;
		return true;
	}
}
