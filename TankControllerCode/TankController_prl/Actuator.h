#ifndef Actuator_h
#define Actuator_h

#include <Arduino.h>

class Actuator
{
public:
		int Pin;

		int UpdateTime;

		float DutyCycle;
		float CyclePeriod;

		float SetRangeMin;
		float SetRangeMax;

		float CurrentReading;

	 	Actuator(int pin);
		~ Actuator();

		void updateSetRange(float min, float max);

		void updateControl(float currentreading);
	
};

#endif 