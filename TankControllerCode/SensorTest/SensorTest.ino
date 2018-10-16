#include "DOSensor.h"

DOSensor testsensor(8, 2);

void setup() {
  Serial.begin(9600);

}

void loop() {
//  int reading = testsensor.readSensor();
  float reading = testsensor.readCalibrated();
  Serial.println(reading);
//  delay(1000);

}
