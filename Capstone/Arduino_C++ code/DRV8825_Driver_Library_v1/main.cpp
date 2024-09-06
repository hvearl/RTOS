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
    * 0.0.2 -- 2024-02-12 : Asynchronous movement of 2 motors with independent speeds and targets

* TO DO
    * Fix motor buzzing when the other motor reaches its target
    * Implement acceleration

* KNOWN ISSUES
    * When one motor completes its movement, the other motor seizes and buzzes until its
      target counter reaches 0.

* NOTES / IMPORTANT USAGE INFORMATION
    * None
*/


// INCLUDE LIBRARIES
#include "Arduino.h"
#include <DRV8825.h>


// DEFINE GLOBAL VARIABLES


// DEFINE PIN NUMBERS
#define en2Pin 10


// INSTANTIATE OBJECTS AND CLASSES
DRV8825 m1("m1", 0, 1, 2, 4, 5, 6, 7, 3, 255, 200, 8);
DRV8825 m2("m2", 8, 9, 10, 12, 14, 15, 16, 13, 255, 200, 8);


void splash();  // pre-declaration of function defined at the bottom of this file - necessary for some reason...


// RUNS ONCE WHEN TURNED ON OR THE RESET BUTTON IS PRESSED
void setup() {
  Serial.begin(230400);  // initialize serial monitor
  splash();  // write project information to serial monitor

  // DEFINE PINMODES

  // INITIALIZE HARDWARE
  m1.begin();
  m1.setStepSize(16);
  m1.wake();
  m1.setTargetSteps(50000);
	m1.setMaxSpeed(20);

  m2.begin();
	m2.setStepSize(16);
  m2.wake();
  m2.setTargetSteps(50000);
	m2.setMaxSpeed(10);
}


// REPEATS FOREVER44
void loop() {
  bool m1running = m1.moveToTarget();
  if (!m1running) {
    m1.sleep();
  }
  bool m2running = m2.moveToTarget();
  if (!m2running) {
    m2.sleep();
  }
}


// WRITES PROJECT INFO TO THE SERIAL MONITOR AT STARTUP
void splash() {
  static char PROJECT[] = "";
  static char AUTHOR[] = "Rob Hall";
  static char VERSION[] = "0.0.1";
  static char UPDATED[] = "2024-02-11";
  Serial.print(PROJECT);
  Serial.print("Author: ");
  Serial.println(AUTHOR);
  Serial.print(" Version: ");
  Serial.println(VERSION);
  Serial.print("Updated: ");
  Serial.println(UPDATED);
  Serial.println();
}
