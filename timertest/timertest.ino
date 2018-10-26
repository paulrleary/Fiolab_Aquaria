#include "DutyCycleTimer.h"

DutyCycleTimer timer(10);

void setup(){
	Serial.begin(9600); 
  pinMode(13, OUTPUT);
 
}

float num = 0.1;
float increment = 0.01;

void loop(){

  
  
  if (timer.isTimerComplete()){
    
    num += increment;
    if(num >= 1.0 || num <= 0.0){
      increment *= -1;
    }
  
     timer.startDutyCycle(num);
  }
  digitalWrite(13, timer.getTimerState());
	
}
