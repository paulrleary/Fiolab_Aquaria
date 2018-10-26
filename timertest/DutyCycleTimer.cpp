#include "DutyCycleTimer.h"

DutyCycleTimer::DutyCycleTimer(float cycleperiod, float dutycycle):cyclePeriod(cycleperiod), dutyCycle(dutycycle){
	startTime = 0;
}

void DutyCycleTimer::setCyclePeriod(float cycleperiod = 0){
  cyclePeriod = cycleperiod;
}

void DutyCycleTimer::setDutyCyclePeriod(float dutycycle = 0){
  dutyCycle  = dutycycle; 
}

void DutyCycleTimer::startDutyCycle(){
    startDutyCycle(dutyCycle);
}

void DutyCycleTimer::startDutyCycle(float dutycycle){
  startTime = millis();
  dutyCycle  = dutycycle;
}

bool DutyCycleTimer::isTimerComplete(){
  return ((millis() - startTime) > cyclePeriod);
}

bool DutyCycleTimer::getTimerState(){
  return ((millis() - startTime) <= cyclePeriod*dutyCycle);
}



