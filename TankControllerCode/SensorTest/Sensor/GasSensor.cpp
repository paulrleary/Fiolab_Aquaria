#include "GasSensor.h"

GasSensor::GasSensor(int csPin, int adcChannel) : ADS1118Sensor(csPin, adcChannel){

}

void GasSensor::initializeCalibration(float slope, float intercept){
  Slope = slope;
  Intercept = intercept;
}

float GasSensor::readCalibrated(){

  int reading = ADS1118Sensor::readSensor();
  float out = Intercept + Slope*reading;
  // float out = reading;
  return out;
  
}


