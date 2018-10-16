#include "DOSensor.h"

DOSensor::DOSensor(int csPin, int adcChannel) : ADS1118Sensor(csPin, adcChannel){
//  CSPin = csPin;
//  ADCChannel = adcChannel;  
}

void DOSensor::initializeCalibration(float slope, float intercept){
  Slope = slope;
  Intercept = intercept;
}

float DOSensor::readCalibrated(){

  int reading = ADS1118Sensor::readSensor();
  float out = 0.9*reading;
  
}


