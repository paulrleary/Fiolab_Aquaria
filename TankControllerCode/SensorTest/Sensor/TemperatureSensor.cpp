#include "TemperatureSensor.h"

TemperatureSensor::TemperatureSensor(int csPin, int adcChannel) : ADS1118Sensor(csPin, adcChannel){

}

void TemperatureSensor::initializeCalibration(float slope, float intercept, float resistor){
  Slope = slope;
  Intercept = intercept;
  Resistor = resistor;
}

float TemperatureSensor::readCalibrated(){

  int reading = ADS1118Sensor::readSensor();

  float resTemp = (0.000125*Resistor*(float)reading)/(5-(0.000125*(float)reading));
  float out = (1/(Intercept+(Slope*log(resTemp))))-273.15;
    // float out = Intercept + Slope*reading;
  // float out = reading;
  return out;
  
}


// tSens [0] = intercept; 
//       tSens [1] = slope; 
//       tSens [2] = resistor;