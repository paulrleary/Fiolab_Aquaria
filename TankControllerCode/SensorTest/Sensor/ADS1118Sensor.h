#ifndef ADS1118Sensor_h
#define ADS1118Sensor_h

#include "Arduino.h"

class ADS1118Sensor
{
public:
	int CSPin;
  int ADCChannel;

  byte MSBConf;
  byte LSBConf;
 

	ADS1118Sensor(int csPin = 0, int adcChannel = 0);

  int readSensor();
  
  int readADC();
	
	
};
#endif
