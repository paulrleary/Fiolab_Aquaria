#include "SingleController.h"

SingleController::SingleController(int pin, int direction, float cycleperiod, float dutycycle): Pin(pin), Direction(direction), DutyCycleTimer(cycleperiod, dutycycle){
	pinMode(Pin, OUTPUT);
	rangeBuffer = 0;
}

void SingleController::setPin(int pin){
	Pin = pin;
}

void SingleController::setDirection(int direction){
	Direction = direction;
}

void SingleController::setCyclePeriod(float cycleperiod){
  cyclePeriod = cycleperiod;
}

void SingleController::setDutyCycle(float dutycycle){
  dutyCycle  = dutycycle; 
}

void SingleController::setRangeBuffer(float rangebuffer){
	rangeBuffer = rangebuffer;
}

void SingleController::updateSetRange(float min, float max){
	SetRangeMin = min;
	SetRangeMax = max;
}

void SingleController::updateControl(float currentreading){
	CurrentReading = currentreading;
	if (isTimerComplete()){
		if(Direction < 0){
			if (CurrentReading > SetRangeMax+rangeBuffer){
				startDutyCycle();
			}
		}
		if(Direction > 0){
			if (CurrentReading < SetRangeMin-rangeBuffer){
				startDutyCycle();
			}
		}
	}
	digitalWrite(Pin, getTimerState())
}