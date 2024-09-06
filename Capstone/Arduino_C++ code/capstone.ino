#include "EZ2209_L.h"
#include "EZ2209_R.h"
#include "EZ2209.h"

//serial buffers to communicate with Raspberry Pi
const int BUFFER_SIZE =8;
char x[BUFFER_SIZE];
char z[BUFFER_SIZE];
int xP=0;
int zP=0;

void setup() {
  //Serial.begin(9600);
  motor_A_setup();
 // delay(10);
 // motor_B_setup();
  delay(10);
  motorAHome();
  delay(10);
  //motorBHome();
  delay(1);
}

void loop(){}
  }
  //execute motors
   motor_A_run(152,160);
   //motor_B_run(152,160);
   delay(1000);
   motor_A_run(-152,160);
   //motor_B_run(-152,160);
    delay(5000);
}