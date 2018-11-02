#ifndef DualController_h
#define DualController_h

#include <Arduino.h>
#include "SingleController.h"

class DualController{

public:

	SingleController NegativeController;
	SingleController PositiveController;
	
	DualController(int negativecontrollerpin = 0, int positivecontrollerpin = 0,  float negativecontrollercycleperiod = 0, float positivecontrollercycleperiod = 0, float negativecontrollerdutycycle = 0, float positivecontrollerdutycycle = 0);
	~DualController();
  
	void setAllCyclePeriod(float cycleperiod = 0);
	void setAllDutyCycle(float dutycycle = 0);
	void setAllRangeBuffer(float rangebuffer = 0);

	void setNegativePin(int pin = 0);
	void setNegativeCyclePeriod(float cycleperiod = 0);
	void setNegativeDutyCycle(float dutycycle = 0);
	void setNegativeRangeBuffer(float rangebuffer = 0);

	void setPositivePin(int pin = 0);
	void setPositiveCyclePeriod(float cycleperiod = 0);
	void setPositiveDutyCycle(float dutycycle = 0);
	void setPositiveRangeBuffer(float rangebuffer = 0);

	void updateSetRange(float min, float max);
	void updateControl(float currentreading);	
};	
#endif
