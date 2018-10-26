#include "GasController.h"

GasController::GasController(int pin, float cycleperiod, float dutycycle):Pin(pin), DutyCycleTimer(cycleperiod, dutycycle){
	pinMode(Pin, OUTPUT);
	rangeBuffer = 0;
}

void GasController::setCyclePeriod(float cycleperiod = 0){
  cyclePeriod = cycleperiod;
}

void GasController::setDutyCycle(float dutycycle = 0){
  dutyCycle  = dutycycle; 
}

void GasController::setRangeBuffer(float rangebuffer){
	rangeBuffer = rangebuffer;
}

void GasController::updateSetRange(float min, float max){
	SetRangeMin = min;
	SetRangeMax = max;
}

void GasController::updateControl(float currentreading){
	CurrentReading = currentreading;
	if (isTimerComplete()){
		if (CurrentReading > SetRangeMax+rangeBuffer){
			startDutyCycle();
		}
	}
	digitalWrite(Pin, getTimerState())
}