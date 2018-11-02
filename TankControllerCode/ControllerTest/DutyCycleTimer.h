#ifndef DutyCycleTimer_h
#define DutyCycleTimer_h

#include <Arduino.h>

class DutyCycleTimer{
  public:
	float cyclePeriod;
  volatile float dutyCycle;
	float startTime;

	DutyCycleTimer(float cycleperiod = 0, float dutycycle = 0);

  void setCyclePeriod(float cycleperiod = 0);

  void setDutyCycle(float dutycycle = 0);
  
	void startDutyCycle();
	void startDutyCycle(float dutycycle);

  bool isTimerComplete();
  bool getTimerState();

};

#endif
