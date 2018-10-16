#ifndef TemperatureSensor_h
#define TemperatureSensor_h

#include "ADS1118Sensor.h"

class TemperatureSensor : public ADS1118Sensor
{
  public:

  float Slope;
  float Intercept;
  float Resistor;
  

  TemperatureSensor(int csPin = 0, int adcChannel = 0);

  void initializeCalibration(float slope = 0, float intercept = 0, float resistor = 0);

  float readCalibrated();
};
#endif