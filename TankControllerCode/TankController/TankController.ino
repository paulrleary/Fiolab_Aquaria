/*
FIOLAB TANK CONTROLLER ARDUINO AND SHIELD

v2.1

2017-06-29

Natalie Low and Crystal Ng

Contact: hnn.low@gmail.com

The Fiolab control system consists of 12 individual Arduino Unos that can read temperature, DO, and pH from sensors 
in each individual tank, log those values to an SD card, and regulate temperature, DO, and pH for the tank based on 
a time series of defined setpoints contained in a text file on the SD card. Each of these individual Arduino Unos is 
connected to a master Arduino Mega via an I2C connection. The master Arduino queries each individual Uno for the current 
tank conditions and the status of the actuators, consolidates the information, logs it to its own SD card, and sends 
it to an online datastream.


This is the code for the individual tank controller Arduino Unos. EAGLE schematics and an electronics parts list for 
the tank controller shield will be available on Github (https://github.com/lowhnn/FiolabAquaria). The code, schematics,
and parts for the master Arduino will able be available.

v2: replaced the 12-bit ADC with the 16-bit ADS1118 and added working code for pH control 
v2.1: added code for the Arduino to track the current ramp line and remember it upon resetting (i.e. power outages)

//////////////////////////////////////////////////////////////////////////////////////
ABOUT THE TANK CONTROLLER SHIELD:

The Fiolab Tank Controller Arduino Shield contains the following components:

 - DS3234 Real Time Clock (http://datasheets.maximintegrated.com/en/ds/DS3234.pdf) 
 - Battery holder for a CR1220 3V coin battery to power the clock
 - Micro SD card reader with a LP2985 3V voltage regulator (http://www.ti.com/lit/ds/symlink/lp2985-n.pdf) 
   and a HC4050M logic shifter to convert TTL and CMOS (http://www.ti.com/lit/ds/symlink/cd74hc4049.pdf)
 - ADS1118 4-channel, 16-bit ADC (http://www.ti.com/lit/ds/symlink/ads1118.pdf)
 - ULN2003A transistor array (http://www.ti.com/lit/ds/symlink/uln2003a.pdf)
 - A toggle switch to switch the ramp cycle on
 - 2 LEDs to indicate (a) whether the ramp is on; and (b) if there is an error with the SD card

 The RTC, SD card reader, and ADC are on the SPI bus and use pins D8-D13 on the Arduino. The SD card
 reader is a native SPI device. SPI RTC and ADC devices were chosen in order to avoid having multiple 
 RTCs and ADC on the same I2C bus as both the tank Arduinos and the master Arduino.

 The ADS1118 ADC is connected to three screw terminals, marked DO, pH, and T. 
 

 The DS3234 RTC runs in SPI Mode 3 (it can theoretically also run in Mode 1) while the other 2 components
 run in SPI Mode 0, so it is necessary to switch between SPI modes in the code. Also note that there is a
 10K resistor between the ground pins of the DS3234 and the shield's ground plane. During the breadboarding
 phase, I found that the RTC would not work without a sufficiently large resistor between it and ground. The 
 RTC is powered by a coin battery when not powered by the power supply. The battery holder is the only 
 component located on the bottom side of the shield.

 The SD card reader is a fairly humble and straightforward component. It does not need much introducing.
 
 The ULN2003AN is connected to four digital output pins (D4-D7), two of which (D5 and D6) have PWM capabilities,
 and to the screw terminals marked TC1 and TC2 (for temperature control), SOL1 and SOL 2 (for solenoid valves). 
 It is powered separately from the other components on the shield, since the actuators draw more current.

 There are screw terminals for 2 separate power supplies. The first is marked MCU (for microcontroller unit),
 and it supplies power to the Arduino and all shield components except the ULN2003A. This should be connected 
 to the battery-backed 12V 1.5A power supply. The second screw terminal is marked ACT (for actuators) and it
 supplies power to the ULN2003A and associated actuators (2 solenoids, 2 solid-state relays for the heater and
 chillers). This is connected to the 12V 12.5A power supply.
 
 There is also a screw terminal for I2C lines. These are used solely for communication with the master Arduino.
 
  List of Arduino UNO/Atmega328 pins used by the shield and the code:
  A0 - Toggle switch for turning on the ramp. Has external pulldown so on = HIGH
  A1 - Indicator LED for SD errors
  A2 - Indicator LED for active ramp
  A4 - internally connected to SDA pin
  A5 - internally connected to SCL pin
  D4 - SOL2
  D5 - TC2
  D6 - TC1
  D7 - SOL1
  D8 - ADC chip select for SPI
  D9 - SD chip select for SPI
  D10 - RTC chip select for SPI
  D11 - standard SPI MOSI pin
  D12 - standard SPI MISO pin
  D13 - standard SPI clock pin
  

//////////////////////////////////////////////////////////////////////////////////////
ABOUT THE CODE:

The purpose of the tank controller Arduinos is to both regulate and record the values of
temperature, DO, and pH in the tank.

This is a quick overview of what the code does:

(1) On startup, the Arduino retrieves several settings from text files on the SD card.
    - It retrieves its tank ID number from the TANKID.TXT file and assigns itself the corresponding
      I2C address. It also assigns the right filenames for the ramp file (RAMPxx.TXT) and the log
      file (LOGxx.TXT), where xx = the tank ID number in the range of 01 - 12.
    - It retrieves the number of lines in the ramp file from the RAMPLEN.TXT file for use in running
      the ramp
    - It retrieves the current ramp line position from the RAMPPOS.TXT file to determine which line
      on the ramp to start from. If running a new experiment this value is set to 1. If the Arduino
      resets in the middle of an experiment (e.g., power loss) this allows the ramp to start from 
      where it left off. 
    - It retrieves calibration coefficients from the files TCAL.TXT, DOCAL.TXT, and PHCAL.TXT and
      and stores them for use in converting the ADC output to tank variables
    Note: the list of settings text files and the format of the text files can be found in the README
    files on the SD cards themselves. 

(2) It initialises all devices and enters a loop, which continues till the ramp is switched on:
      - Reads inputs from sensors via the MCP3204 ADC. Takes an average of 50 digital readings and
        converts them into real world values of Temp, DO, or pH
      - Reads current time from the DS3234 RTC
      - Logs timestamped sensor values to the SD card
      - Sends sensor values to the Master Arduino via I2C when requested by the Master.
      
(3) When the ramp switch is turned on, it turns on the ramp indicator LED and enters a new loop:
       - Reads and parses comma-separated values from a RAMPxx.TXT on the SD card, which contains the
        timepoints and tank variable setpoints. The setpoints consist of a minimum and maximum
        allowed value for each tank variable. 
          - It starts reading from the ramp line as specified in RAMPPOS.TXT
          - The structure of RAMPxx.TXT: [timePoint],[tempMax],[tempMin],[DOMax],[DOMin],[pHMax],[pHMin]
          - just set a crazy extreme value for any parameters that are not going to be regulated
      - Reads current time from the DS3234 RTC
      - Reads inputs from sensors via the ADS1118 ADC. Takes an average of 20 digital readings and
        converts them into real world values of Temp, DO, or pH
      - Compares the current sensor inputs against the ramp values and turn on/off actuators as
        appropriate.
      - When the timepoint on ramp line is reached, reads and parses comma-separated values from 
        the next line of the RAMPXX.TXT file
          - Also records the new ramp line to RAMPPOS.TXT
      - Logs timestamped sensor values and actuator statuses to the SD card
          - Structure of LOGxx.TXT: [timeStamp],[Temp],[DO],[pH],[Heater],[Chiller],[SolN2],[SolCO2],[SolAir],[counter]
      - Sends sensor values and actuator statuses to the Master Arduino via I2C when requrested 
        by the Master. 

*/




// Load libraries
#include <Wire.h>
#include <SdFat.h>
#include <SPI.h> 
//#include <RTClib.h>
#include <RTC_DS3234.h> //PRL:Working for now, but need to come back to this one.  I don't like that we would have to rely on an external library, but having lots of trouble when using this one locally.

#define ECHO_TO_SERIAL 1     // echo data to serial port

// Tank-specific and sensor-specific variables
// These will be defined by the settings file on the SD card
byte myAddress; // I2C address of this Arduino
char logFile [10]; // name of the text file to log to
char rampFile [11]; // name of the text file with ramp values
int rampLength; // number of lines in ramp file
float tSens [3], doSens [2], phSens [2]; //sensor regression coefficients

  
// Define pins
  // actuator pins
  #define solN2Pin 7 // SOL1
  #define heaterPin 6 // TC 1, uses PWM
  #define chillerPin 5 // TC 2 
//  #define solAirPin 4 
  #define solCO2Pin 4 // SOL2 
  #define solN2Ind 3 // for monitoring SOL1 status
  #define heaterInd 2 // for monitoring TC1 status
 #define solCO2Ind A3 // for monitoring SOL2 status
  
  // SPI devices
  #define adcCSPin 8
  #define RTCPin 10
  #define SDcardPin 9 
  
  // Ramp switch and indicator
  #define switchPin A0
  #define sdLEDPin A1
  #define rampLEDPin A2

  // Define ADC Channels
  #define TempCh 0
  #define DOCh 2
  #define pHCh 1

// Define union with of one variable represented as float and byte array
// this is for conversion of float sensor values to bytes during I2C transmission
typedef union float2bytes_t { 
  float f; 
  byte b[sizeof(float)]; 
}; 

// List of possible commands from Master Arduino, coded as bytes
enum {
  // commands to request current sensor readings
  VALUE_TEMP = 1,
  VALUE_DO = 2,
  VALUE_PH = 3,
  //commands to request current status of actuators
  STATUS_RAMP = 4,
  STATUS_SOL_DO = 5,
//  STATUS_SOL_AIR = 6,
  STATUS_HEATER = 7,
  STATUS_CHILLER = 8,
  STATUS_SOL_CO2 = 9
};

// Instantiate SPI devices
SdFat sd;
SdFile myFile;
RTC_DS3234 RTC(RTCPin);

// Store error messages in flash memory 
#define error(s) sd.errorHalt_P(PSTR(s))

// Define global variables
  // Variable to store the next line position in the ramp file
  int lineToLoad; 

  // Variables to store sensor readings
  float currentTemp, currentDO, currentpH;

  // Variables to store ramp values
  float timeMinutes, tempMax, tempMin, DOMax, DOMin, pHMax, pHMin; 
  
  // Variables to store real time and logging/ramp intervals
  unsigned long startTime;
  unsigned long serialPoint;
  unsigned long logPoint;
  unsigned long logInterval = 300000; // Logs to SD card every 5 min
  int serialInterval = 10000; // reports to serial every 10 sec 
  DateTime now;

  // Variable to store incoming commands from Master
  char command = 0;
  
  // Variable to store bubble rate for N2 and CO2 solenoids
  float N2bubbleRate;
  float CO2bubbleRate;


//////////////////////////////////////////////

void setup() {
     
  // Pin setup
    // Output Pins for actuators
    pinMode(solN2Pin, OUTPUT);
    pinMode(heaterPin, OUTPUT);
    pinMode(chillerPin, OUTPUT);
    pinMode(solCO2Pin, OUTPUT);
//    pinMode(solAirPin, OUTPUT);
    pinMode(solN2Ind, OUTPUT);
    pinMode(heaterInd, OUTPUT);
    pinMode(solCO2Ind, OUTPUT);  

    // Pins for ramp switch and indicator LEDs
    pinMode(switchPin, INPUT); 
    pinMode(rampLEDPin, OUTPUT);
    pinMode(sdLEDPin, OUTPUT);
    
    // Pins for SPI devices
    pinMode (adcCSPin, OUTPUT);
    digitalWrite(adcCSPin, HIGH); 
    
  // Initialize serial communications for testing and debugging code
    Serial.begin(9600);
    
  // Initialise SD card 
  if (!sd.begin(SDcardPin, SPI_HALF_SPEED)) {
    digitalWrite(sdLEDPin,HIGH);
    Serial.println(F("Failed to initialise SD card."));
  }
   
  // Load settings from SD card
   
    // Retrieve tank ID
    getTankID(); 
    
    #if ECHO_TO_SERIAL
      Serial.print(F("My I2C address: "));Serial.println(myAddress);
      Serial.print(F("Logging to "));Serial.println(logFile);
      Serial.print(F("Loading ramp from "));Serial.println(rampFile);
    #endif //ECHO_TO_SERIAL

    // Retrieve length of ramp file
    getRampLength();

    #if ECHO_TO_SERIAL
      Serial.print(F("Ramp length = "));Serial.println(rampLength);
    #endif //ECHO_TO_SERIAL

    // Retrieve current ramp position
    getRampPos(); 
    
    #if ECHO_TO_SERIAL
      Serial.print(F("lineToLoad: "));Serial.println(lineToLoad);
    #endif //ECHO_TO_SERIAL
      
    // Retrieve sensor calibration values
    loadSensors();
    
    #if ECHO_TO_SERIAL
      Serial.print(F("Thermistor intercept: "));Serial.print(tSens[0]);Serial.print(F(", slope: "));Serial.print(tSens[1]);
      Serial.print(F(" FixedRes: "));Serial.println(tSens[2]);
      Serial.print(F("DO intercept: "));Serial.print(doSens[0]);Serial.print(F(", DO slope: "));Serial.println(doSens[1]);
      Serial.print(F("pH intercept: "));Serial.print(phSens[0]);Serial.print(F(", pH slope: "));Serial.println(phSens[1]);
    #endif /ECHO_TO_SERIAL
   
  // Begin standard SPI for SD card and ADC
  SPI.begin();
  SPI.setBitOrder(MSBFIRST); 
  SPI.setDataMode(SPI_MODE0); 
  SPI.setClockDivider(SPI_CLOCK_DIV16); 
    
  // Begin I2C communication with Master Arduino
  Wire.begin (myAddress); // begin I2C
  Wire.onReceive (receiveEvent);  // interrupt handler for incoming messages
  Wire.onRequest (requestEvent);  // interrupt handler for when data is wanted
    
  // Initialise RTC
  startRTC();

  // Initialise logPoint
  logPoint = millis();
  
  // Basic loop to run before ramp is turned on:
  // Reads sensors and logs data at regular time intervals
  while (digitalRead(switchPin)==LOW){ // while ramp switch is off

    // Keep green ramp indicator LED off
    digitalWrite(rampLEDPin, LOW); 
    
    // Read the current time from RTC
    getTime(); 

    // Read sensor values from ADC
    readSensors(); 

    // If next logging timepoint has been reached, log data to SD card
    if (millis() - logPoint > logInterval) {

        // Log tank data to SD card
        LogToSD();
        
        // Update logPoint
        logPoint = millis();
      }

    // If next serial report timepoint has been reached, report data to serial monitor
    if (millis() - serialPoint > serialInterval) {

        // Report tank data to serial monitor
        serialReport();
        
        // Update logPoint
        serialPoint = millis();
      }


      delay(500);  // I2C communication with Master fails w/o adding a short delay here
                  // increase the delay time if having I2C issues 
      
      
  } // end basic pre-ramp loop when ramp switch is flipped

  // ----- RAMP BEGINS -----

  // Store ramp start time 
  startTime = millis(); 

  // Turn on green ramp indicator LED
  digitalWrite(rampLEDPin, HIGH); 
  
} // end setup loop

//////////////////////////////////////////////////////////////////////////////////////

void loop() {

  // Loop to run while the ramp is on (i.e. the program hasn't reached the last line in the ramp file)
  while (lineToLoad <= rampLength) { 

    // Get values from the next line from the ramp file
    getRampLine(); 
    #if ECHO_TO_SERIAL
      Serial.print(F("Ramp timepoint: "));Serial.println(timeMinutes);
    #endif
    
    
    // Loop to run while the next time-point in the ramp has not been reached
    // This reads sensors and regulates variables against the setpoints in the current ramp line
    while (millis()-startTime < timeMinutes*60000) {
      
      // Read the current time from RTC
      getTime(); 

      // Read sensor values from ADC
      readSensors(); 
      
      // Check tank values against the current setpoint and activate/deactivate sensors
      updateActuators(); 
      DOSolPulse();
      CO2SolPulse();

      // If next logging timepoint has been reached, log data to SD card
      if (millis() - logPoint > logInterval) {

        // Log tank data to SD card
        LogToSD();
        
        // Update logPoint
        logPoint = millis();
      }

      // If next serial report timepoint has been reached, report data to serial monitor
      if (millis() - serialPoint > serialInterval) {

        // Report tank data to serial monitor
        serialReport();
        
        // Update logPoint
        serialPoint = millis();
      }

      delay(500); // delay to allow I2C communication with Master

    } //end of one time point on ramp (one line of the ramp)

    lineToLoad++; // update the number for the next ramp line to be read
    Serial.print(F("LTL update: "));Serial.println(lineToLoad);

    updateRampPos(); // write the new lineToLoad value to the SD card
    
    
  } // end of ramp while-loop (the last line is reached)

  // ----- RAMP ENDS -----
  Serial.println(F("Ramp end."));

  
  
  // Turn off green ramp indicator LED; ramp is complete
  digitalWrite(rampLEDPin, LOW); 
  
  // Turn off all actuators if they are still on
  digitalWrite(heaterPin, LOW); 
  //analogWrite(heaterPin,0); // if using PWM control of heater  
  digitalWrite(chillerPin, LOW);
  digitalWrite(solN2Pin, LOW);
//  digitalWrite(solAirPin,LOW);
  digitalWrite(solCO2Pin,LOW);

  // Start the post-ramp loop (identical to pre-ramp loop)
  // Reads sensors and logs data at regular time intervals

  // Read the current time from RTC
  getTime(); 

  // Read sensor values from ADC
  readSensors(); 

  // If next logging timepoint has been reached, log data to SD card
  if (millis() - logPoint > logInterval) {
    
    // Log tank data to SD card
    LogToSD();
        
    // Update logPoint
    logPoint = millis();
    
  }

  // If next serial report timepoint has been reached, report data to serial monitor
  if (millis() - serialPoint > serialInterval) {
    
    // Report tank data to serial monitor
    serialReport();
        
    // Update logPoint
    serialPoint = millis();
    
    }

    delay(500); 

    
} // end main loop

//////////////////////////////////////////////

// ------------------------------------------------------------------
// ------------------------- FUNCTIONS USED -------------------------
// ------------------------------------------------------------------


// Sub-functions used in the main code, in approximate order of use:
//
//    getTankID 
//    getRampLength
//    getRampPos
//    loadSensors
//      getSensorCoef
//    receiveEvent
//    requestEvent
//      sendSensor
//      sendActStat
//    startRTC
//    getTime
//    readSensors
//      readADC
//    LogToSD
//    serialReport
//    getRampLine
//    updateActuators
//    DOSolPulse
//    CO2SolPulse
//    updateRampPos



// Function to retrieve the ID number of the tank/controller from the TANKID.TXT file
// on the SD card, and use that to assign this Arduino the correct I2C address and determine
// the filenames of the ramp file and log file to read from/write to
void getTankID()  {
  char readChar;
  char tempAddress [3];
  
  if (!myFile.open("TANKID.TXT", O_READ)) sd.errorHalt("Failed to load tank ID");
  
  while (myFile.available()) { 
    readChar = myFile.read();
    for (int i = 0; i < 3; i++) {
      if (readChar != ';') {
        tempAddress[i] = readChar;
        readChar = myFile.read();
      } else {
        tempAddress[i] = '\0';
        break;
      }
    }
    
    myAddress = atoi(tempAddress);

    // define ramp and log file names based on tank number
    strcpy(rampFile,"RAMP");
    strcpy(logFile,"LOG");

    if (myAddress < 10){
      rampFile[4] = '0';
      rampFile [5] = tempAddress [0];
      logFile[3] = '0';
      logFile[4] = tempAddress [0];
    } else {
      logFile [3] = tempAddress [0];
      logFile [4] = tempAddress [1];
      rampFile [4] = tempAddress [0];
      rampFile [5] = tempAddress [1];
    }

    rampFile[6]='.';rampFile[7]='T';rampFile[8]='X';rampFile[9]='T';
    logFile[5]='.';logFile[6]='T';logFile[7]='X';logFile[8]='T';
  }
  myFile.close();
} // end function getTankID



// Function to retrieve the ramp length (# of lines) in the ramp file
void getRampLength() {
  char readChar;
  char tempVal [4];
   
  if (!myFile.open("RAMPLEN.TXT", O_READ)) sd.errorHalt("Failed to load ramp length");
  while (myFile.available()) { 
    readChar = myFile.read();
    for (int i = 0; i < 4; i++) {
      if (readChar != ';') {
        tempVal[i] = readChar;
        readChar = myFile.read();
      } else {
      tempVal[i] = '\0';
      break;
      }
    }
    rampLength = atoi(tempVal); // store ramp length as int
  }
  myFile.close(); 
} // end function getRampLength


// Function to retrieve the current line position in the ramp from the 'RAMPPOS.TXT' file
// and store it in the variable lineToLoad. When starting a new experiment, lineToLoad
// is set to 1.
void getRampPos() {
  char readChar;
  char tempVal [4];

   
  if (!myFile.open("RAMPPOS.TXT", O_READ)) sd.errorHalt("Failed to load ramp position");
  while (myFile.available()) { 
    readChar = myFile.read();
    for (int i = 0; i < 5; i++) {
        if (readChar != ';') {
        tempVal[i] = readChar;
        readChar = myFile.read();
      } else {
      tempVal[i] = ';';
//      tempVal[i] = '\0';
      break;
      }
    }
//    Serial.print(F("tempVal = "));Serial.println(tempVal);
    lineToLoad = atoi(tempVal); // store ramp position as int
  }
  myFile.close(); 

} // end function getRampPos



// Function to retrieve calibration coefficients for all sensors. Calls getSensorCoef() function.
void loadSensors() {
  getSensorCoef(TempCh);
  getSensorCoef(DOCh);
  getSensorCoef(pHCh);
} // end function loadSensors



// Function to retrieve calibration coefficients (the intercept and slope from the regression  
// of Temp/DO/pH against the digital output from the ADC). Called by loadSensors().
// Note that "tempVal" in this means temporary value, not anything to do with temperature!!
void getSensorCoef(byte sensor) {
  char readChar;
  char tempVal [10];
  float intercept, slope, resistor;
 
  switch (sensor) {
    case 0:
      if (!myFile.open("TCAL.TXT", O_READ)) sd.errorHalt("Failed to load temperature sensor coefficients");
      break;
    case 1:
      if (!myFile.open("PHCAL.TXT", O_READ)) sd.errorHalt("Failed to load pH sensor coefficients");
      break;   
    case 2:
      if (!myFile.open("DOCAL.TXT", O_READ)) sd.errorHalt("Failed to load DO sensor coefficients");
      break;
  }

  while (myFile.available()) {
    readChar = myFile.read();
    for (byte i = 0; i < 13; i++) { // first param is the regression intercept
      if (readChar !=',') {
        tempVal[i] = readChar;
        readChar = myFile.read();
      } else {
        tempVal[i] = '\0';
        break;
      }
    }
    intercept = atof(tempVal);

    readChar = myFile.read();
    for (byte i = 0; i < 13; i++) {   // second param is the regression slope
      if ((readChar !=';') & (readChar !=',')) {
        tempVal[i] = readChar;
        readChar = myFile.read();
      } else {
        tempVal[i] = '\0';
        break;
      }
    }
    slope = atof(tempVal);
    
  if (sensor == 0) {  // thermistor has an extra parameter for the fixed resistor
      readChar = myFile.read();
      for (byte i = 0; i < 6; i++) {
        if (readChar !=';') {
          tempVal[i] = readChar;
          readChar = myFile.read();
        } else {
          tempVal[i] = '\0';
          break;
        } 
      }
      resistor = atof(tempVal);
    }
  }
  myFile.close();

  switch (sensor) {
    case 0:
      tSens [0] = intercept; 
      tSens [1] = slope; 
      tSens [2] = resistor;
      break;
    case 1:
      phSens [0] = intercept;
      phSens [1] = slope;
      break;
    case 2:
      doSens [0] = intercept;
      doSens [1] = slope;
      break;

  }
} // end function getSensorCoef



// Function to receive and store data-request command from Master Arduino 
// via the I2C connection.
void receiveEvent (int howMany){
  command = Wire.read ();  // remember command for when we get request
  //Serial.print("Command received "); Serial.println(command); 
}  // end function receiveEvent



// Function to send the requested data to Master over the I2C connection, based on 
// the command received and stored by the receiveEvent() function. Calls the 
//  sendSensor() function.
void requestEvent (){
  switch (command){
    case VALUE_TEMP: sendSensor(currentTemp);break;// send Temperature reading
    case VALUE_DO: sendSensor(currentDO);break;   // send DO reading
    case VALUE_PH: sendSensor(currentpH);break; // send pH reading
    case STATUS_SOL_DO: sendActStat(solN2Ind);break; // send N2 solenoid status
    case STATUS_SOL_CO2: sendActStat(solCO2Ind);break; // send CO2 solenoid status
//    case STATUS_SOL_AIR: sendActStat(solAirPin);break; // send Air solenoid status
    case STATUS_HEATER: sendActStat(heaterInd);break; // send heater status
    case STATUS_CHILLER: sendActStat(chillerPin);break; // send chiller status
    case STATUS_RAMP: sendActStat(rampLEDPin);break; // send ramp status
  }
} // end function requestEvent



// Function to convert a floating-point sensor value into a byte array to send
// to the Master via the I2C connection. Called by the requestEvent() function.
void sendSensor(float floatSensor){
  float2bytes_t f2bSensor;
  byte bytesSensor [4];
  f2bSensor.f = floatSensor;
//  int i;
//  for (i=1; i <= 4; i++) bytesSensor[i] = f2bSensor.b[i];
//  Wire.write(bytesSensor,4);  
  Wire.write(f2bSensor.b,4);
} // end function sendSensor



// Function to send the status of an actuator to the Master via the I2C connection.
// Called by the requestEvent() function.
void sendActStat(byte actPin){
  boolean actStatus = digitalRead(actPin);
  Wire.write(actStatus);
} // sendActStat



// Function to initialise the DS3234 RTC and set clock if it is not running.
// Switches SPI mode to mode 3 for the RTC and then switches the mode back to 0
// for the other SPI devices.
void startRTC(){
  SPI.setDataMode(SPI_MODE3); 
  RTC.begin();
  if (! RTC.isrunning()) { // check if RTC is running
    Serial.println(F("RTC is NOT running!"));
  } else {
    now = RTC.now(); 
    Serial.print(F("RTC is running. Time is "));
    Serial.print(now.year(), DEC);Serial.print('-');
    Serial.print(now.month(), DEC);Serial.print('-');
    Serial.print(now.day(), DEC);Serial.print(' ');
    Serial.print(now.hour(), DEC);Serial.print(':');
    Serial.print(now.minute(), DEC);Serial.print(':');
    Serial.print(now.second(), DEC);Serial.println();
  }
  SPI.setDataMode(SPI_MODE0); 
} // end function startRTC



// Function to get the current time from the DS3234 RTC.
// Switches SPI mode to mode 3 for the RTC and then switches the mode back to 0
// for the other SPI devices
void getTime() {
  SPI.setDataMode(SPI_MODE3); 
  now = RTC.now(); 
  SPI.setDataMode(SPI_MODE0);
} // end function getTime



// Function to read 20 consecutive values of all tank sensors via the ADC, average 
// the readings, and convert the averaged digital value into temp/DO/pH values using
// the sensor regression coefficients. Calls the readADC() function
void readSensors() {
  long pHSum = 0;
  long tempSum = 0;
  long DOSum = 0;
  int adcpH, adcTemp, adcDO;
  byte n = 0;
  SPI.setDataMode(SPI_MODE1);
  for (byte i = 0; i < 20; i++) {
    adcpH = readADC(0);
    delay(10);
    adcpH = readADC(0);
    adcTemp = readADC(1);
    delay(10);
    adcTemp = readADC(1);
    adcDO = readADC(2);
    delay(10);
    adcDO = readADC(2);

    pHSum = pHSum + adcpH;
    tempSum = tempSum + adcTemp;
    DOSum = DOSum + adcDO;
    n++;
  }
  int avgpH = pHSum/n;
  int avgTemp = tempSum/n;
  int avgDO = DOSum/n;

//  Conversion via regression coefs here
//  float resTemp = (tSens[2]*1000)*((4095/(float)avgTemp)-1);
//  currentTemp = 1/(log(resTemp/tSens[0])/tSens[1] + 1/298.15) - 273.15;
  float resTemp = (0.000125*tSens[2]*(float)avgTemp)/(5-(0.000125*(float)avgTemp));
  currentTemp = (1/(tSens[0]+(tSens[1]*log(resTemp))))-273.15;
  currentDO = doSens[0] + avgDO*doSens[1];
  currentpH = phSens[0] + avgpH*phSens[1];
    
  SPI.setDataMode(SPI_MODE0);
}



// Reads the ADS 1118 ADC
int readADC(byte channel) {
  int rawVal = 0; // Raw value received back from the ADS1118
  byte MSB, LSB, MSBConf, LSBConf; //The most and least significant bits read from the ADS1118
  switch(channel) { // Most Significant Bit configuration register - this specifies which channel to read from
    case 0: MSBConf = 0b11000010;
    break;
    case 1: MSBConf = 0b11010010;
    break;
    case 2: MSBConf = 0b11100010;
    break;
  } 
  LSBConf=0b10101011; // Least Significant Bit configuration register
  noInterrupts();
  digitalWrite(adcCSPin, LOW);
  MSB = SPI.transfer(MSBConf);
  LSB = SPI.transfer(LSBConf);
  digitalWrite(adcCSPin, HIGH);
  interrupts();
//  delay(10);   //delay of 7 or less leads to erratic readings from each channel

  // Build the raw value from the most and least significant bits
  rawVal = (MSB << 8) | LSB;
  return rawVal;
} // end function readADC
 


// Function to log tank values and actuator statuses to the SD card.
void LogToSD() {
  if (myFile.open(logFile, O_RDWR | O_CREAT | O_AT_END)) {
    digitalWrite(sdLEDPin, LOW); // turn off SD error LED
    myFile.print(now.year(),DEC);myFile.print("-");
    myFile.print(now.month(),DEC);myFile.print("-");
    myFile.print(now.day(),DEC);myFile.print(" ");
    myFile.print(now.hour(),DEC);myFile.print(":");
    myFile.print(now.minute(),DEC);myFile.print(":");
    myFile.print(now.second(),DEC);myFile.print(",");
    myFile.print(currentTemp);myFile.print(",");
    myFile.print(currentDO),myFile.print(",");
    myFile.print(currentpH),myFile.print(",");
    myFile.print(digitalRead(rampLEDPin));myFile.print(",");
    myFile.print(digitalRead(heaterInd));myFile.print(",");
    myFile.print(digitalRead(chillerPin));myFile.print(",");
    myFile.print(digitalRead(solN2Ind));myFile.print(",");
    myFile.println(digitalRead(solCO2Ind));
//    myFile.println(digitalRead(solAirPin));
    myFile.close();
  } else {
    Serial.println("Failed to open or create log file");
    digitalWrite(sdLEDPin, HIGH); // turn on SD error LED
  }
} // end function LogToSD



//Function to print sensor reading and ramp values to the serial monitor.
void serialReport(){
  Serial.print(now.year(), DEC);Serial.print('-');
  Serial.print(now.month(), DEC);Serial.print('-');
  Serial.print(now.day(), DEC);Serial.print(' ');
  Serial.print(now.hour(), DEC);Serial.print(':');
  Serial.print(now.minute(), DEC);Serial.print(':');
  Serial.print(now.second(), DEC);Serial.println();
  Serial.print(F("Temp: "));Serial.print(currentTemp);Serial.print(F(" degC. "));
  if(digitalRead(rampLEDPin)==HIGH){
    Serial.print(F("Allowed range: "));Serial.print(tempMin);Serial.print(F(" - "));
    Serial.print(tempMax);Serial.print(F(", "));
    if (digitalRead(heaterInd)==1) Serial.print(F("HEATER"));
    else if (digitalRead(chillerPin)==1) Serial.print(F("CHILLER"));
    else Serial.print(F("NOTHING"));
    Serial.println(F(" is on."));
  }
  Serial.println(F(""));
  Serial.print(F("DO: "));Serial.print(currentDO);Serial.print(F(" mg/L. "));
  if(digitalRead(rampLEDPin)==HIGH){
    Serial.print(F("Allowed range: "));Serial.print(DOMin);Serial.print(F(" - "));
    Serial.print(DOMax);Serial.print(F(", "));
    if (digitalRead(solN2Ind)==1) Serial.print(F("N2"));
//    else if (digitalRead(solAirPin)==1) Serial.print(F("AIR"));
    else Serial.print(F("NOTHING"));
    Serial.println(F(" is on."));
  }
  Serial.println(F("")); 
  Serial.print(F("pH: "));Serial.print(currentpH);Serial.print(F(". "));
  if(digitalRead(rampLEDPin)==HIGH){
    Serial.print(F("Allowed range: "));Serial.print(pHMin);Serial.print(F(" - "));
    Serial.print(pHMax);Serial.print(F(", "));
    if (digitalRead(solCO2Ind)==1) Serial.print(F("CO2"));
//    else if (digitalRead(solAirPin)==1) Serial.print(F("AIR"));
    else Serial.print(F("NOTHING"));
    Serial.println(F(" is on."));
  } 
  Serial.println(F(""));Serial.println(F(""));  
} // end function serialReport



// Function to open comma-separated text file on SD card containing ramp info, 
// read, process and store time point (in minutes) and threshold values for temp/DO
// for the current line of the ramp.
void getRampLine(){
  int lineIndex = 0;
  int lineNumber = 0;
  char rampLine [40]; 
  char readChar;
  
  if (myFile.open(rampFile, O_READ)) {
    Serial.println("Opened ramp file");
    while (myFile.available()) {
      readChar = myFile.read(); // Gets one character from serial buffer
      if (readChar != '\n'){ 
        rampLine[lineIndex] = readChar; // store incoming chars to buffer
        lineIndex++; // Increment buffer index to write next char
      } 
      else if (readChar == '\n'){ // if end of line is reached
        lineNumber++; // update the line number
        lineIndex++; 
        if (lineNumber == lineToLoad){  //if this is the correct line to load
          Serial.print(F("Line loaded: "));Serial.println(lineNumber);
          lineNumber = 0;
          lineIndex = 0;
          break; // break out of the "while" loop to process data
        } 
        else { //not the correct line to load, reset and continue
          lineIndex = 0; // reset buffer index so next line of ramp replaces current one in buffer
        }
      }
    }
    myFile.close(); //close file
    
    // process the data in the line buffer
    char* lineposition;
    lineposition = strtok(rampLine, ",");
    float tempvals [7]; 
    for (int i = 0; i < 7; i++){ 
        tempvals [i] = atof(lineposition); 
        lineposition = strtok(NULL,",");
    }
      
    // store each value for the current ramp line
    timeMinutes = tempvals [0];
    tempMax = tempvals [1];
    tempMin = tempvals [2];
    DOMax = tempvals [3];
    DOMin = tempvals [4]; 
    pHMax = tempvals [5];
    pHMin = tempvals [6];   
  } else {
    digitalWrite(sdLEDPin, HIGH);
    Serial.println(F("Failed to open ramp file")); 
  } 
} // end function getRampLine



// Function to check current tank conditions against ramp values and turn actuators
// off or on as needed.
void updateActuators(){

// Code for PWM control of heater
if (currentTemp > tempMax) { // turn on chiller if temp exceeds upper threshold
    analogWrite(heaterPin, 0);
    digitalWrite(heaterInd, LOW);
    digitalWrite(chillerPin, HIGH);
  } else if (currentTemp + 2 < tempMin) { // turn on heater at full duty cycle if temp
    analogWrite(heaterPin, 255);        // is more than 2 degC below lower threshold
    digitalWrite(heaterInd, HIGH);
    digitalWrite(chillerPin, LOW);
  } else if ((currentTemp + 2 > tempMin) & (currentTemp + 1 < tempMin)) { 
    analogWrite(heaterPin, 191);        // heater at 75% duty cycle if temp is less
    digitalWrite(heaterInd, HIGH);
    digitalWrite(chillerPin, LOW);      // than 1.5 degC below threshold
  } else if ((currentTemp + 1 > tempMin) & (currentTemp < tempMin)) { 
    analogWrite(heaterPin, 127);        // heater at 50% duty cycle if temp is less
    digitalWrite(heaterInd, HIGH);
    digitalWrite(chillerPin, LOW);      // than 1 degC below threshold
  }  else { // turn off heater and chiller if temp is within desired range
    analogWrite(heaterPin, 0);
    digitalWrite(heaterInd, LOW);
    digitalWrite(chillerPin, LOW); 
  }

  // BubbleRate control of N2 solenoid, use with DOSolPulse() function
  if (currentDO > DOMax + 0.1) {
//    digitalWrite(solAirPin,LOW);
    digitalWrite(solN2Ind,HIGH);
    N2bubbleRate = 0.3;
 // } else if ((currentDO < DOMax + 2) & (currentDO > DOMax)) {
    //digitalWrite(solAirPin,LOW);
    //digitalWrite(solN2Ind,HIGH);
    //bubbleRate = 0.3;
  } else if ((currentDO < DOMax + 0.1) & (currentDO > DOMin)) {
//    digitalWrite(solAirPin,LOW);
    digitalWrite(solN2Ind,LOW);
    N2bubbleRate = 0;
  } else if (currentDO < DOMin) {
//    digitalWrite(solAirPin, HIGH);
    digitalWrite(solN2Ind,LOW);
    N2bubbleRate = 0;
  }

  // BubbleRate control of CO2 solenoid, use with CO2SolPulse() function
  if (currentpH > pHMax + 0.03) { 
//    digitalWrite(solAirPin,LOW);
    digitalWrite(solCO2Ind, HIGH);
    CO2bubbleRate = 0.033;
  } else if ((currentpH < pHMax + 0.03) & (currentpH > pHMin)) {
//    digitalWrite(solAirPin,LOW);
    digitalWrite(solCO2Ind, LOW);
    CO2bubbleRate = 0;
  } else if (currentpH < pHMin) {
//    digitalWrite(solAirPin, HIGH);
    digitalWrite(solCO2Ind,LOW);
    CO2bubbleRate = 0;
  }
} // end function updateActuators



// Function to manually PWM control the N2 solenoid
// Turns on the solenoid for a fraction of a second, as determined
// by the bubbleRate variable
void DOSolPulse() {
  unsigned long time1 = millis();
  unsigned long time2 = millis();
  while (time2 < (time1 + 1000*N2bubbleRate)) {
    digitalWrite(solN2Pin, HIGH);
    time2 = millis();
  }
  digitalWrite(solN2Pin, LOW);
} // end function DOSolPulse 



// Function to manually PWM control the CO2 solenoid
// Turns on the solenoid for a fraction of a second, as determined
// by the bubbleRate variable
void CO2SolPulse() {
  unsigned long time1 = millis();
  unsigned long time2 = millis();
  while (time2 < (time1 + 3000*CO2bubbleRate)) {
    digitalWrite(solCO2Pin, HIGH);
    time2 = millis();
  }
  digitalWrite(solCO2Pin, LOW);
} // end function CO2SolPulse

// Function to write the current lineToLoad value to the SD card
// This allows the value to be retrieved and the ramp to continue
// where it left off, if the Arduino loses power and resets
void updateRampPos() { 
  char buffer [4];
  // open RAMPPOS.TXT and clear current value before writing
  if (myFile.open("RAMPPOS.TXT", O_RDWR | O_TRUNC)) {
    digitalWrite(sdLEDPin, LOW); // turn off SD error LED
    myFile.print(lineToLoad);myFile.print(';');
    myFile.close();
  } else {
    Serial.println(F("Failed to open or create RAMPPOS file"));
    digitalWrite(sdLEDPin, HIGH); // turn on SD error LED
  }
}

