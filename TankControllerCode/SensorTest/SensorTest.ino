#include "GasSensor.h"
#include "TemperatureSensor.h"

GasSensor testsensor1(8, 2);
TemperatureSensor testsensor2(8, 2);
//#include "Sensor.h"
void setup() {
  Serial.begin(9600);
  testsensor1.initializeCalibration(3, 5);
  testsensor2.initializeCalibration(1000, 5, 1000);

}

void loop() {
//  int reading = testsensor.readSensor();
  float reading = testsensor1.readCalibrated();
  Serial.println(reading);
//  delay(1000);

}
