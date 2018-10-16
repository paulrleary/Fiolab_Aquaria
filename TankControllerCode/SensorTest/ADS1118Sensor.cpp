//#include "Arduino.h"
#include <SPI.h> 


#define MSBCONF_MASK		0b11000010
#define MSBCONF_CHANNEL(x)	(((int)(x))<<4)|MSBCONF_MASK

ADS1118Sensor::ADS1118Sensor(int csPin, int adcChannel)
{
	CSPin = csPin;

	ADCChannel = adcChannel;

	MSBConf = MSBCONF_CHANNEL(ADCChannel);
//  MSBConf = (adcChannel<<4)|0b11000010;
	
	LSBConf = 0b10101011;
 
  SPI.begin();
  SPI.setBitOrder(MSBFIRST); 
  SPI.setDataMode(SPI_MODE0); 
  SPI.setClockDivider(SPI_CLOCK_DIV16); 

  pinMode(CSPin, OUTPUT);

	
}

	// Reads the ADS 1118 ADC
	int ADS1118Sensor::readADC() {
	  int rawVal = 0; // Raw value received back from the ADS1118
	  byte MSB, LSB; //MSBConf, LSBConf; //The most and least significant bits read from the ADS1118
	  noInterrupts();
	  digitalWrite(CSPin, LOW);
	  MSB = SPI.transfer(MSBConf);
	  LSB = SPI.transfer(LSBConf);
	  digitalWrite(CSPin, HIGH);
	  interrupts();
	//  delay(10);   //delay of 7 or less leads to erratic readings from each channel

	  // Build the raw value from the most and least significant bits
	  rawVal = (MSB << 8) | LSB;

	  return rawVal;
	} // end function readADC

 int ADS1118Sensor::readSensor(){
  SPI.setDataMode(SPI_MODE1);

  int reading = readADC();
  //can add averaging here
  
  SPI.setDataMode(SPI_MODE0);
  return reading;
  
 }

