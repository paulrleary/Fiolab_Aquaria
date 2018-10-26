#ifndef SingleController_h
#define SingleController_h

#include <Arduino.h>
#include "DutyCycleTimer.h"

class SingleController : public DutyCycleTimer
{
public:
		int Pin;
		int Direction;
		
		float SetRangeMin;
		float SetRangeMax;
		float RangeBuffer;

		float CurrentReading;

	 	SingleController(int pin = 0, int direction = -1, float cycleperiod = 0, float dutycycle = 0);
		~ SingleController();

		void setPin(int pin = 0);

		void setDirection(int direction = -1);

		void setCyclePeriod(float cycleperiod = 0);

  		void setDutyCycle(float dutycycle = 0);

		void updateSetRange(float min, float max);

		void updateControl(float currentreading);
	
};

#endif 