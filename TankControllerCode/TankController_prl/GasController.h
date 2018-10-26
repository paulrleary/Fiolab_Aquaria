#ifndef GasController_h
#define GasController_h

#include <Arduino.h>
#include "DutyCycleTimer.h"

class GasController : public DutyCycleTimer
{
public:
		int Pin;

		float SetRangeMin;
		float SetRangeMax;
		float RangeBuffer;

		float CurrentReading;

	 	GasController(int pin, float cycleperiod = 0, float dutycycle = 0);
		~ GasController();

		void setCyclePeriod(float cycleperiod = 0);

  		void setDutyCyclePeriod(float dutycycle = 0);

		void updateSetRange(float min, float max);

		void updateControl(float currentreading);
	
};

#endif 