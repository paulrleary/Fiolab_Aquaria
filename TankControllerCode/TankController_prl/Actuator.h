#ifndef Actuator_h
#define Actuator_h

#include <Arduino.h>

#include "DutyCycleTimer.h"

class Actuator : public DutyCycleTimer
{
public:
		int Pin;

		float SetRangeMin;
		float SetRangeMax;

		float CurrentReading;

	 	Actuator(int pin, float cycleperiod = 0, float dutycycle = 0);
		~ Actuator();

		void updateSetRange(float min, float max);

		void updateControl(float currentreading);
	
};

#endif 