#ifndef DOSensor_h
#define DOSensor_h

#include "ADS1118Sensor.h"

class DOSensor : public ADS1118Sensor
{
  public:

  float Slope;
  float Intercept;
  

  DOSensor(int csPin, int adcChannel);

  void initializeCalibration(float slope, float intercept);

  float readCalibrated();
};
#endif

