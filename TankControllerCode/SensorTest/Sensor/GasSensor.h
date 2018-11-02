#ifndef GasSensor_h
#define GasSensor_h

#include "ADS1118Sensor.h"

class GasSensor : public ADS1118Sensor
{
  public:

  float Slope;
  float Intercept;
  

  GasSensor(int csPin = 0, int adcChannel = 0);

  void initializeCalibration(float slope = 0, float intercept = 0);

  float readCalibrated();
};
#endif

