#include "Actuator.h"

Actuator::Actuator(int pin, float cycleperiod, float dutycycle):Pin(pin), DutyCycleTimer(cycleperiod, dutycycle){
	pinMode(Pin, OUTPUT);
}

Actuator::updateSetRange(float min, float max){
SetRangeMin = min;
SetRangeMax = max;
}

Actuator::updateControl(float currentreading){
	CurrentReading = currentreading;

	/*
	if state is high{
	do nothing until state is low
	}
	else{
	if state should be high{
	set state high(i.e. start cycle)
	}
	}
	*/

}