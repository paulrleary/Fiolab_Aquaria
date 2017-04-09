/*

FIOLAB MASTER ARDUINO AND SHIELD 

v1

2015-10-09

Natalie Low

Contact: hnn.low@gmail.com





//////////////////////////////////////////////////////////////////////////////////////
ABOUT THE MASTER ARDUINO AND SHIELDS:

The Fiolab Master Arduino is an Arduino Mega 2560 with two shields: the official Arduino 
Ethernet shield, and a custom shield with connectors for all the tank Arduinos, power
supply, a button, an indicator LED, and a pressure sensor for the nitrogen cylinder. There are
no components on the custom shield, only board-to-wire connectors and resistors.

Links to the official Arduino products:
https://www.arduino.cc/en/Main/arduinoBoardMega2560
https://www.arduino.cc/en/Main/ArduinoEthernetShield




List of Arduino Mega/Atmega1280 pins used by the shields and the code:
A0 - analog input for the cylinder pressure sensor
A4 - internally connected to SDA pin
A5 - internally connected to SCL pin
D4 - SD chip select for SPI (Ethernet shield)
D7 - LED
D8 - LED
D9 - digital input for push-button to scroll the LCD

//////////////////////////////////////////////////////////////////////////////////////
ABOUT THE CODE:

This is a quick overview of what the code does:

(1) On startup, the Master Arduino retrieves the current time from an NTP server via the
    ethernet connection (using the Time.h library) in order to keep time.

(2) Master Arduino retrieves information about the seawater variables (Temp, DO, pH), 
    the active tanks, and which 

    The file INCLVARS.TXT has three comma-separated boolean values, corresponding to
    Temperature, DO, and pH in that order. A '0' means that the 

    For example, 

    0,0,1, 
    
    
    The format of the other text files on the Master Arduino's SD card should be twelve 
    comma-separated boolean values (i.e., 0 or 1) that indicate inactive/active status 
    for the corresponding tank. THERE SHOULD BE A COMMA AFTER THE LAST (12TH) VALUE. 
    For example, if only tanks 2,4.6,8 have chillers, then INCLCHIL.TXT will contain

    0,1,0,1,0,1,0,1,0,0,0,0,

   
    
    
    
    INCLTANK:
    INCLNITR
    INCLAIR
    INCLHEAR
    INCLCHIL

(3) Master Arduino sends requests for tank data to each tank Arduino via I2C, receives 
    and stores the data. Data received are: (a) current Temp, DO, and pH; (b) on/off 
    status of N2 solenoid, air solenoid, heater, chiller.

(4) Master Arduino reads the analog input from the pressure sensor on the nitrogen 
    cylinder.

(5) At a defined interval (logInterval), the Master Arduino logs data from all tanks to
    the SD card, sends data for each active tank to its respective  Sparkfun datastream, 
    and updates the LCD screen with the new values.

(6) At a different interval (cyInterval), the Master Arduino sends the current nitrogen 
    cylinder pressure to a Sparkfun datastream (data.sparkfun.com/streams/9JVJZYK0oacrNANDKv3r).
    It also runs a check on the pressure: if pressure falls below 700 PSI, the green 
    indicator LED goes off and a first alert is sent to the cylinder’s Twitter feed 
    (twitter.com/FioLabN2), which a user can set up to receive text message alerts from. If pressure falls below 500 PSI, the red indicator LED turns on and a second alert is sent to the Twitter feed.

(7) When the red scroll button is pressed, the LCD screen scrolls to display the current
    conditions indifferent sets of tanks. Page 1: T1-4; Page 2: T5-8; Page 3: T9-12





*/


// Load libraries
#include <Wire.h>
#include <LiquidCrystal.h>
#include <SdFat.h>
#include <SPI.h> 
#include <Time.h> 
//#include <RTClib.h>
//#include <RTC_DS3234.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#define ECHO_TO_SERIAL 1 // echo data to serial port

// Define Pins
#define buttonPin 9
#define SDcardPin 4 
#define EthernetPin 10
#define cyPressurePin A0 
#define cyLowLED 6
#define cyHighLED 7

// Instantiate objects
SdFat sd;
SdFile myFile;
LiquidCrystal lcd(0);
EthernetClient client;
EthernetUDP Udp;

// Variables to specify active sensors, tanks, and actuators
byte INCLUDE_VARS [3];
byte INCLUDE_TANKS [12];
byte INCLUDE_SOLN2 [12];
byte INCLUDE_SOLAIR [12];
byte INCLUDE_HEATER [12];
byte INCLUDE_CHILLER [12];
byte INCLUDE_SOLPH [12];

// Variables to store incoming tank data
float TempVals[12], DOVals[12], pHVals[12];
boolean RampStat[12], SolN2Stat[12], SolAirStat[12], HeaterStat[12], ChillerStat[12], SolpHStat[12];

// Variables for cylinder pressure and twitter alerts
float cyPressure;
byte tweetIndex; 
unsigned int cyInterval = 300000; // interval for checking cylinder pressure
unsigned long cyPoint;
float cyCoefs[2] = {-1253.355, 4.52456}; // intercept and slope for pressure calibration
                                        // calibrated 17 Sep 2015

// Variables for communication, logging, and reporting
  // For I2C with tank Arduinos
  byte TANK_ID [12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; 
  
  // For sending data to the cloud
  byte mac[] = { 0x90, 0xA2, 0xDA, 0x0F, 0xCC, 0x52 };
  IPAddress ip(171, 66, 246, 163); // address to manually assign if DHCP fails
  char dataServer[] = "data.sparkfun.com"; // server for Sparkfun data stream
  char ttServer[] = "api.thingspeak.com"; // server for connecting to Twitter via ThingTweet
  String ttAPI = "I3R6CP96RCV824GB"; // API for connecting to Twitter via ThingTweet

  // For setting time over Ethernet
  IPAddress timeServer(132, 163, 4, 101); // NTP server
  const int timeZone = -7;  // Set timezone to PDT
  //const int timeZone = -8;  // Set timezone to PST
  unsigned int localPort = 8888;
  const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
  byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

  
  // For logging to SD card
  char logFile[] = "MLOG.TXT";

  // For LCD reporting
  byte lcdPage = 0;
  unsigned long lcdPoint;
  int lcdInterval = 5000;
  //boolean buttonState1, buttonState2;

  // For logging time invervals
  unsigned long logPoint;
  int logInterval = 300000; // intervals for logging data to SD or cloud datastream
                           // should be > 9000 to work with Phant server

  // For printing to the serial monitor
  unsigned long serialPoint;
  int serialInterval = 10000;
                       

// possible commands to give Slave Arduinos, coded as bytes
enum {
  // commands to request current sensor readings
  VALUE_TEMP = 1,
  VALUE_DO = 2,
  VALUE_PH = 3,
  
  //commands to request current status of actuators
  STATUS_RAMP = 4,
  STATUS_SOL_DO = 5,
  STATUS_SOL_AIR = 6,
  STATUS_HEATER = 7,
  STATUS_CHILLER = 8,
  STATUS_SOL_PH = 9
};

// Define union with of one variable represented as float and byte array
// this is for the conversion of received bytes to float sensor values
typedef union float2bytes_t {    
  float f; 
  char b[sizeof(float)]; 
}; 

// Store error messages in flash memory
#define error(s) sd.errorHalt_P(PSTR(s))


//////////////////////////////////////////////
  
void setup () {
  
  // Pin setup
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(cyLowLED, OUTPUT);
  pinMode(cyHighLED, OUTPUT);

    // Pins for SPI devices
  pinMode (EthernetPin, OUTPUT);
  digitalWrite(EthernetPin, HIGH);
  pinMode (SDcardPin, OUTPUT);
  digitalWrite(SDcardPin, HIGH);

  
  // Initialise I2C and Serial communications
  Serial.begin (9600);
  #if ECHO_TO_SERIAL
    Serial.println("Serial initialized");
  #endif //ECHO_TO_SERIAL
  Wire.begin();   
  #if ECHO_TO_SERIAL
    Serial.println("I2C initialized");
  #endif //ECHO_TO_SERIAL

  // Begin standard SPI 
  SPI.begin();
  SPI.setBitOrder(MSBFIRST); 
  SPI.setDataMode(SPI_MODE0); 
  SPI.setClockDivider(SPI_CLOCK_DIV4);
  #if ECHO_TO_SERIAL
    Serial.println("SPI initialized");
  #endif //ECHO_TO_SERIAL 

  // Initialize LCD
  lcd.begin(20, 4); // initialize LCD
  #if ECHO_TO_SERIAL
    Serial.println("LCD initialized, turning on Backlight");
  #endif //ECHO_TO_SERIAL
  lcd.setBacklight(HIGH);
  lcd.print("Starting Master...");

  // Initialise Ethernet
  lcd.setCursor(0, 1);
  lcd.print("Ethernet...");
  startEthernet();
  lcd.setCursor(12,1);
  lcd.print("done.");

  // Get time from NTP server
  lcd.setCursor(0,2);
  lcd.print("NTP sync...");
  Udp.begin(localPort);
  Serial.println("Waiting for NTP sync...");
  setSyncProvider(getNtpTime);
  #if ECHO_TO_SERIAL
    Serial.print("Time retrieved: ");
    Serial.print(year());Serial.print("-");
    Serial.print(month());Serial.print("-");
    Serial.print(day());Serial.print(" ");
    Serial.print(hour());Serial.print(":");
    Serial.print(minute());Serial.print(":");
    Serial.println(second());
  #endif //ECHO_TO_SERIAL
  lcd.setCursor(12,2);
  lcd.print("done.");
  
  // Initialise SD card or print error
  if (!sd.begin(SDcardPin, SPI_HALF_SPEED)) sd.initErrorHalt("SD initialization failed");
  #if ECHO_TO_SERIAL
    Serial.println("SD card initialised");
  #endif //ECHO_TO_SERIAL
  
  // Read SD card here and obtain parameters
  lcd.setCursor(0,3);
  lcd.print("SD settings...");
  getMasterSettings();
  #if ECHO_TO_SERIAL
    Serial.println("All settings loaded");
  #endif //ECHO_TO_SERIAL
  lcd.setCursor(15,3);
  lcd.print("done.");
  
  delay(20);
  lcd.clear();

  // First sensor readings and LCD update
    // Get sensor values from each tank
    for (byte i = 0; i < 12; i++) getTankVals(i);

    // Get actuator statuses from each tank 
    for (byte i = 0; i < 12; i++) getTankActs(i);

    // Report to LCD
    LCDUpdate(lcdPage);

  // Initialise logPoint
  logPoint = millis();
  cyPoint = millis();
  lcdPoint = millis();
  serialPoint = millis();
  #if ECHO_TO_SERIAL
    Serial.println(F("LogPoints initialised"));
  #endif //ECHO_TO_SERIAL



  // First check of cylinder pressure
  pressureRead();
  checkPressure();
  #if ECHO_TO_SERIAL
    Serial.print(F("Cylinder Pressure = "));Serial.print(cyPressure);Serial.println(" psi");
  #endif //ECHO_TO_SERIAL

  
  

}  

//////////////////////////////////////////////

void loop(){

  // Get sensor values from each tank
  for (byte i = 0; i < 12; i++) getTankVals(i);

  // Get actuator statuses from each tank 
  for (byte i = 0; i < 12; i++) getTankActs(i);

  // Read cylinder pressure
  pressureRead();
  
  // If next logging timepoint has been reached, log and report data
  if (millis() - logPoint > logInterval) {

  #if ECHO_TO_SERIAL
    Serial.print(F("Logpoint at"));Serial.println(millis());
  #endif //ECHO_TO_SERIAL

    // Update the LCD screen
    LCDUpdate(lcdPage);

    // Log all tank data to SD card
    DataToSD();

    // Stream all tank data to cloud
    DataToCloud();

    // update logPoint
    logPoint = millis();
  }


  #if ECHO_TO_SERIAL
    // If next serial-report timepoint has been reached, log and report data
    if (millis() - serialPoint > serialInterval) {
      
      // Send all tank data to serial port
      DataToSerial();
  
      // update timepoint
      serialPoint = millis();
    }
  #endif //ECHO_TO_SERIAL

    

  // If next cylinder-check timepoint has been reached, send data to 
  // cloud and initialise indicator lights and twitter alert if necessary
  if (millis() - cyPoint > cyInterval) {
    
    // Send cylinder pressure to cloud
    PressureToCloud();

    // Check cylinder pressure and send twitter alert if necessary
    checkPressure();
    cyPoint = millis();
    
  }

  // If next lcd refresh timepoint has been reached, 
    if (millis() - lcdPoint > lcdInterval) {
      lcdPage++;
      if (lcdPage > 2) lcdPage = 0;
      LCDUpdate(lcdPage);
      lcdPoint = millis();
    
  }

//  // If button is pressed, scroll LCD
//  buttonState1 = digitalRead(buttonPin);
//  if (buttonState1 == LOW) {
//    delay(10);
//    buttonState2 = digitalRead(buttonPin);
//    if (buttonState1 = buttonState2) {
//      lcdPage++;
//      if (lcdPage > 2) lcdPage = 0;
//      Serial.print("Current LCD Page: ");
//      Serial.println(lcdPage);
//      lcd.clear();
//      LCDUpdate(lcdPage);
//    }
//  }


}
  
//////////////////////////////////////////////
// FUNCTIONS USED

// Function to initialize Ethernet connection
// Attempts to configure via DHCP first
void startEthernet(){
  Serial.println("Setting up Ethernet...");
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Failed to configure Ethernet using DHCP"));
    // no point in carrying on, so do nothing forevermore:
    // try to configure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
  // give the Ethernet shield a second to initialize:
  delay(1000);
}


// Function to
time_t getNtpTime() {
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}


// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}


// Function to load info on which tanks and tank variables are being logged, as well as which
// actuators are present in which tanks.
void getMasterSettings() {
  for (byte i = 0; i < 7; i++) getWhichArray(i);
}


//Function to obtain info on which tanks, tank variables, and actuators are active
void getWhichArray(byte whichVar){
  char readChar;
  byte whichArrayLength;
  char tempVal[2];
  
  switch (whichVar) {
    case 0: // Which variables (Temp, DO, pH) are being logged
      if (!myFile.open("INCLVARS.TXT", O_READ)) sd.errorHalt("Failed to open INCLVARS file");
      whichArrayLength = 3;
      break; 
    case 1: // Which tanks (1-12) are active and logging
      if (!myFile.open("INCLTANK.TXT", O_READ)) sd.errorHalt("Failed to open INCLTANK file");
      whichArrayLength = 12;
      break;
    case 2: // Which tanks (1-12) have active heaters
      if (!myFile.open("INCLHEAT.TXT", O_READ)) sd.errorHalt("Failed to open INCLHEAT file");
      whichArrayLength = 12;
      break;
    case 3: // Which tanks (1-12) have active chillers
      if (!myFile.open("INCLCHIL.TXT", O_READ)) sd.errorHalt("Failed to open INCLCHIL file");
      whichArrayLength = 12;
      break;
    case 4: // Which tanks (1-12) have active N2 solenoids
      if (!myFile.open("INCLNITR.TXT", O_READ)) sd.errorHalt("Failed to open INCLNITR file");
      whichArrayLength = 12;
      break; 
    case 5: // Which tanks (1-12) have active air solenoids
      if (!myFile.open("INCLAIR.TXT", O_READ)) sd.errorHalt("Failed to open INCLAIR file");
      whichArrayLength = 12;
      break;  
    case 6: // Which tanks (1-12) have active CO2 solenoids
      if (!myFile.open("INCLCO2.TXT", O_READ)) sd.errorHalt("Failed to open INCLCO2 file");
      whichArrayLength = 12;
      break; 
  }
  
  while(myFile.available()){
    for (byte j = 0; j < whichArrayLength; j++) {
      readChar = myFile.read();
      for (byte i = 0; i < 2; i++) {
        if ((readChar != ',') & (readChar != ';')) { 
          tempVal[i] = readChar;
          readChar = myFile.read();
        } else {
          tempVal[i] = '\0';
          break;
        }
      }
      //process tempVal
      int tempInt = atoi(tempVal); 
      switch (whichVar) {
        case 0: INCLUDE_VARS [j] = (byte)(tempInt); break;
        case 1: INCLUDE_TANKS [j] = (byte)(tempInt); break;
        case 2: INCLUDE_HEATER [j] = (byte)(tempInt); break;
        case 3: INCLUDE_CHILLER [j] = (byte)(tempInt); break;
        case 4: INCLUDE_SOLN2 [j] = (byte)(tempInt); break;
        case 5: INCLUDE_SOLAIR [j] = (byte)(tempInt); break;
        case 6: INCLUDE_SOLPH [j] = (byte)(tempInt); break;
      }   
    }
  }
  myFile.close();
  #if ECHO_TO_SERIAL
    Serial.print(F("Loaded settings files"));Serial.println(whichVar);
  #endif
}


void startLCD() {
  lcd.begin(20, 4); // initialise LCD
  Serial.println("LCD initialized, turning on Backlight");
  lcd.setBacklight(HIGH);
  lcd.print(F("Loading tanks: "));
  lcd.setCursor(0, 1);  
  for (byte i = 0; i < 12; i++) {
    if (INCLUDE_TANKS [i] == 1) {
      lcd.print(i+1);lcd.print(" ");
    }
  }
  delay(1500);
}


void pressureRead() {
  unsigned int pressureSum = 0;
  for (byte i = 0; i < 30; i++) {
    pressureSum = pressureSum + analogRead(A0);
  }
  int avgPressure = pressureSum/30;
  cyPressure = cyCoefs[0] + cyCoefs[1]*avgPressure; 
}


// Function to check the cylinder pressure and send Twitter alert if pressure is low
void checkPressure() {
  char tweetString [64];
  if ((cyPressure < 700) & (cyPressure > 500)) {
    sprintf(tweetString,"Cylinder Pressure dropped below 700 PSI at %02d:%02d:%02d on %04d/%02d/%02d",
            //now.hour(),now.minute(),now.second(),now.year(),now.month(),now.day());
            hour(),minute(),second(),year(),month(),day());
    digitalWrite(cyLowLED, LOW);
    digitalWrite(cyHighLED, LOW);
    if (tweetIndex < 1) {
      tweetAlert(tweetString);
      tweetIndex++;
    }
  } else if (cyPressure < 500) {
    sprintf(tweetString,"Cylinder Pressure dropped below 500 PSI at %02d:%02d:%02d on %04d/%02d/%02d",
        //now.hour(),now.minute(),now.second(),now.year(),now.month(),now.day());
        hour(),minute(),second(),year(),month(),day());
    digitalWrite(cyLowLED, HIGH);
    digitalWrite(cyHighLED, LOW);
    if (tweetIndex < 2) {
      tweetAlert(tweetString);
      tweetIndex++;
    }
  } else {
    digitalWrite(cyLowLED, LOW);
    digitalWrite(cyHighLED, HIGH);
    tweetIndex = 0;
  }
}


// Function to post an alert message to the FioLabN2 twitter feed
void tweetAlert (String alertMessage) {
  long lastConnectionTime = 0; 
  boolean lastConnected = false;
  int failedCounter = 0;
  
    if (client.connect(ttServer, 80)) { 
      // Create HTTP POST Data. Request should look like:
      // POST /apps/thingtweet/1/statuses/update HTTP/1.1
      // Host: api.thingspeak.com
      // Connection: close
      // Content-Type: application/x-www-form-urlencoded
      // Content-Length: (number of characters in message)api_key=(thingtweet_api_key)&status=[status]
      alertMessage = "api_key="+ttAPI+"&status="+alertMessage;
      client.print("POST /apps/thingtweet/1/statuses/update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(alertMessage.length());
      client.print("\n\n");
      client.print(alertMessage);
      lastConnectionTime = millis();
      if (client.connected()) {
        Serial.println("Connecting to ThingSpeak...");
        Serial.println();
        failedCounter = 0;
      } else {
        failedCounter++;
        Serial.println("Connection to ThingSpeak failed ("+String(failedCounter, DEC)+")");   
        Serial.println();
      }
    } else {
      failedCounter++;
      Serial.println("Connection to ThingSpeak Failed ("+String(failedCounter, DEC)+")");   
      Serial.println();   
      lastConnectionTime = millis(); 
  }
}


// Function to request all sensor data from a given tank and store them in
// the data arways TempVals, DOVals, and pHVals.
// It checks the status of tanks and sensor types to only request data from active 
// tanks, and only for the variables of interest.
  // Note that tankNo starts from 0, not 1, because it is indexing values in arrays,
  // and indexing in C++ starts from 0. 
void getTankVals(const byte tankNo){
  if (INCLUDE_TANKS[tankNo]==1){ 
    if(INCLUDE_VARS[0]==1) TempVals[tankNo] = getSensorVal(TANK_ID[tankNo], VALUE_TEMP);
    if(INCLUDE_VARS[1]==1) DOVals[tankNo] = getSensorVal(TANK_ID[tankNo], VALUE_DO);
    if(INCLUDE_VARS[2]==1) pHVals[tankNo] = getSensorVal(TANK_ID[tankNo], VALUE_PH);
  }
} 


// Function to request sensor data from a tank, receive the 4-byte array,
// then convert and return the corresponding float value. 
// This function is called from the getTankVals() function
float getSensorVal(const byte tankID, const byte whichVal){
  byte bytesSensor[4];
  float2bytes_t f2bSensor;
  float floatSensor;
  byte i=0, j;
  sendCommand (tankID,whichVal, 4);
  while(Wire.available()) bytesSensor[i++] = Wire.read ();
  for (j = 1; j <= 4; j++) f2bSensor.b[j] = bytesSensor[j];
  floatSensor = f2bSensor.f;
  return(floatSensor);
}

// Function to send command to slave to request a given type of data
// This function is called from the getTankVals() function, through  getSensorVal()
void sendCommand (const byte tankID, const byte cmd, const int responseSize){
  //command and expected response size (bytes) - figure out how to transfer floats?
  Wire.beginTransmission (tankID);
  Wire.write (cmd);
  Wire.endTransmission ();
  Wire.requestFrom (tankID, 4);
  }


// Function to request actuator status from a tank.
boolean getActStat(const byte tankID, const byte whichVal){
 sendCommand(tankID, whichVal, 1);
 boolean statActuator = Wire.read(); 
 return(statActuator);
}


// Function to obtain the status of all actuators in a tank
// Checks statuses only for tanks that have been listed as active
void getTankActs(const byte tankNo){
  if (INCLUDE_TANKS[tankNo]==1){ 
    RampStat[tankNo] = getActStat(TANK_ID[tankNo], STATUS_RAMP);
    if(INCLUDE_SOLN2[tankNo]==1) SolN2Stat[tankNo] = getActStat(TANK_ID[tankNo], STATUS_SOL_DO);
    if(INCLUDE_SOLAIR[tankNo]==1) SolAirStat[tankNo] = getActStat(TANK_ID[tankNo], STATUS_SOL_AIR);
    if(INCLUDE_HEATER[tankNo]==1) HeaterStat[tankNo] = getActStat(TANK_ID[tankNo], STATUS_HEATER);
    if(INCLUDE_CHILLER[tankNo]==1) ChillerStat[tankNo] = getActStat(TANK_ID[tankNo], STATUS_CHILLER);
  }
}


// Function to display T/DO values and actuator statuses on the LCD screen, for
// four tanks at a time. It checks the INCLUDE_TANKS array and only displays readings
// for active tanks. The lcdIndex variable {0,1,2} and indicates which page of tanks
// is on display.
void LCDUpdate(byte lcdIndex){
  lcd.clear();
  for (byte i = 0; i < 4; i++){
    lcd.setCursor(0, i);lcd.print(F("T"));lcd.print(4*lcdPage+1+i);lcd.print(F(" "));
    if(INCLUDE_TANKS[4*lcdIndex+i]==1){
      lcd.print(TempVals[4*lcdIndex+i],1);lcd.print((char)223);lcd.print(F("C"));
      if (HeaterStat[4*lcdIndex+i]==1) lcd.print(F("+")); // this is if heater is on
      else if (ChillerStat[4*lcdIndex+i]==1) lcd.print(F("-")); // this is if chiller is on
      lcd.setCursor(12, i);lcd.print(DOVals[4*lcdIndex+i],1);lcd.print(F("mg/L"));
      if (SolN2Stat[4*lcdIndex+i]==1) lcd.print(F("-")); // this is if N2 is on
      else if (SolAirStat[4*lcdIndex+i]==1) lcd.print(F("+"));  // this is if Air is on 
  }
  else lcd.print(F("- - - - - - - - "));
  }
}


// Function to write the timestamped conditions of each tank to the SD card
void DataToSD() {
  if (!myFile.open(logFile, O_RDWR | O_CREAT | O_APPEND)) {
  sd.errorHalt("opening log file for write failed");
  }
  
  // Log time
  myFile.print(year());myFile.print("-");
  myFile.print(month());myFile.print("-");
  myFile.print(day());myFile.print(" ");
  myFile.print(hour());myFile.print(":");
  myFile.print(minute());myFile.print(":");
  myFile.print(second());
  
  // Log values for each active tank
  for (byte i = 0; i <= 11; i++){
    if(INCLUDE_TANKS[i]==1){
      if(INCLUDE_VARS[0]==1) { myFile.print(",");myFile.print(TempVals[i]);}
      if(INCLUDE_VARS[1]==1) { myFile.print(",");myFile.print(DOVals[i]);}
      if(INCLUDE_VARS[2]==1) { myFile.print(",");myFile.print(pHVals[i]);}
    } else {
      if(INCLUDE_VARS[0]==1) { myFile.print(",");myFile.print("NA");}
      if(INCLUDE_VARS[1]==1) { myFile.print(",");myFile.print("NA");}
      if(INCLUDE_VARS[2]==1) { myFile.print(",");myFile.print("NA");}
    }
  }
  // Log cylinder pressure
  myFile.print(",");myFile.println(cyPressure);
  myFile.close();
}


// Function to stream tank data to the cloud, for all active tanks
void DataToCloud() {
  for (byte i = 0; i < 12; i++) {
    if (INCLUDE_TANKS [i] == 1) {
      streamTank(i);
    }
  }
}


// Function to consolidate all sensor readings and actuator statuses from a single
// tank and stream them to the correct data stream at data.sparkfun.com
void streamTank(byte tankNo){
  const String fieldNames[9] = {"temperature","do","ph","ramp","heater","chiller","soln2","solair","solph"};
  String fieldData[9];
  
  // Consolidate data from given tank into string
  // [Temp],[DO],[pH],[Ramp],[Heater],[Chiller],[SolN2],[SolAir],[SolCO2]
  if (INCLUDE_VARS[0]==1) {
    if ((TempVals[tankNo] < 1000) & (TempVals[tankNo] > 0.01)) {
      fieldData[0] = String(TempVals[tankNo]);
    } else {
      fieldData[0] = String("NA");
    }
  } else {
    fieldData[0] = String("NA");
  }
  if (INCLUDE_VARS[1]==1) {
    if ((DOVals[tankNo] < 1000) & (DOVals[tankNo] > 0.01)) {
      fieldData[1] = String(DOVals[tankNo]);
    } else {
      fieldData[1] = String("NA");
    }  
  } else {
    fieldData[1] = String("NA");
  }
  if (INCLUDE_VARS[2]==1) {
    if (pHVals[tankNo] < 1000) {
      fieldData[2] = String(pHVals[tankNo]);
    } else {
      fieldData[2] = String("NA");
    }
  } else {
    fieldData[2] = String("NA");
  }
  fieldData[3] = String(RampStat[tankNo]);
  if (INCLUDE_HEATER[tankNo]==1) {
    fieldData[4] = String(HeaterStat[tankNo]);
  } else {
    fieldData[4] = String("NA");
  }
  if (INCLUDE_CHILLER[tankNo]==1) {
    fieldData[5] = String(ChillerStat[tankNo]);
  } else {
    fieldData[5] = String("NA");
  }
  if (INCLUDE_SOLN2[tankNo]==1) {
    fieldData[6] = String(SolN2Stat[tankNo]);
  } else {
    fieldData[6] = String("NA");
  }
  if (INCLUDE_SOLAIR[tankNo]==1) {
    fieldData[7] = String(SolAirStat[tankNo]);
  } else {
    fieldData[7] = String("NA");
  }
  if (INCLUDE_SOLPH[tankNo]==1) {
    fieldData[8] = String(SolpHStat[tankNo]);
  } else {
    fieldData[8] = String("NA");
  }

  // Make a TCP connection to remote host
  if (client.connect(dataServer, 80)) {
    // Post the data! Request should look a little something like:
    // GET /input/[publicKey]?private_key=[privateKey]&chiller=[value]&do=[value]&heater=[value]&ph=[value]&solair=[value]&soln2=[value]&solph=[value]&temperature=[value] HTTP/1.1\n
    // Host: data.sparkfun.com\n
    // Connection: close\n
    // \n
    client.print("GET /input/");
    switch (tankNo) { // public key for appropriate data stream
      case 0: client.print(String("yAVdJd5M5bszq66oN0Rm")); break;
      case 1: client.print(String("DJ6V1d6oxwHa6EE4AoK5")); break;
      case 2: client.print(String("6JA62EM448f0nRRWNGOM")); break;
      case 3: client.print(String("ZGg6dDMooNiZ3KK8DO15")); break;
      case 4: client.print(String("KJ1v8zmoOzhVJKZrO8Mx")); break;
      case 5: client.print(String("3J5XWQ6pNaIJmYX9j1EY")); break;
      case 6: client.print(String("v0Aa4XxEj4h3YgRE6Krz")); break;
      case 7: client.print(String("XGNZgNR5xvIp49V185ya")); break;
      case 8: client.print(String("g6qJpOAMnNIX09r8bGRp")); break;
      case 9: client.print(String("QGXZb8MlWzT67LO3NZzn")); break;
      case 10: client.print(String("EJEoWnz66xFqom0jnAVd")); break;
      case 11: client.print(String("jqKodQlvz6H2RjLgMbny")); break;
    }
    client.print("?private_key=");
    switch (tankNo) { // private key for appropriate data stream
      case 0: client.print(String("4WAE6ED0DRs2gZZ9pDbz")); break;
      case 1: client.print(String("P4bgJNbw5GH6wbb8azv4")); break;
      case 2: client.print(String("WwyX9ADZZjs0mrrgRZ12")); break;
      case 3: client.print(String("2mAqZokMM2hj6wwdvoJl")); break;
      case 4: client.print(String("vzRDZbGVYbUp8zw67gdn")); break;
      case 5: client.print(String("oZB8p9wrKWHkBZ7wEmjZ")); break;
      case 6: client.print(String("aP7wNYgW5NSPR2jK4NMa")); break;
      case 7: client.print(String("1JAzRA9oP0Sq5R6jg2B8")); break;
      case 8: client.print(String("qzYD7PZ91aI7PzbyqKWG")); break;
      case 9: client.print(String("JqAYgD1omdigPYmKZ61j")); break;
      case 10: client.print(String("dqMwED244vCrR0kAbBgD")); break;
      case 11: client.print(String("zzVK9nk40rh8bRgaXe96")); break;
    }
    for (byte i = 0; i < 9; i++){
      client.print("&");
      client.print(fieldNames[i]);
      client.print("=");
      client.print(fieldData[i]);
    }
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(dataServer);
    client.println("Connection: close");
    client.println();
    Serial.println(F("Data sent to cloud"));
  } else {
    Serial.println(F("Connection failed"));
  }
  // Check for a response from the server, and route it out the serial port.
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      #if ECHO_TO_SERIAL
        Serial.print(c);
       #endif //ECHO_TO_SERIAL
    }      
  }
  Serial.println();
  client.stop(); 
}


void DataToSerial() {
  Serial.print(year());Serial.print("-");
  Serial.print(month());Serial.print("-");
  Serial.print(day());Serial.print(" ");
  Serial.print(hour());Serial.print(":");
  Serial.print(minute());Serial.print(":");
  Serial.println(second());

  Serial.print("Cylinder is at ");Serial.print(cyPressure);Serial.println(" psi.");
  
  for (int i = 0; i <= 11; i++){
    Serial.print("Tank ");Serial.print(i+1);Serial.print(": "); 
      if(INCLUDE_TANKS[i]==1){
        Serial.print(TempVals[i]);Serial.print(" degC");
        if (HeaterStat[i]==1) Serial.print("+"); // this is if heater is on
        else if (ChillerStat[i]==1) Serial.print("-"); // this is if chiller is on
        Serial.print(" ");Serial.print(DOVals[i],1);
        if (SolN2Stat[i]==1) Serial.println("mg/L-");
        else if (SolAirStat[i]==1) Serial.println("mg/L+");
        else Serial.println("mg/L");
        
      }
      else Serial.println("- - - - -");
  }
  Serial.println("");
 }

// Function to send current N2 cylinder pressure to the sparkfun data stream
void PressureToCloud() {
// First, send pressure value to sparkfun datastream  
  if (client.connect(dataServer, 80)) { // Make a TCP connection to remote host
    // Post the data. Request should look a little something like:
    // GET /input/[publicKey]?private_key=[privateKey]&chiller=[value] HTTP/1.1\n
    // Host: data.sparkfun.com\n
    // Connection: close\n
    // \n
    client.print("GET /input/9JVJZYK0oacrNANDKv3r?private_key=xzazyBr5k0s4BJBbe724&psi=");
    client.print(cyPressure);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(dataServer);
    client.println("Connection: close");
    client.println();
    Serial.println(F("Data sent to cloud"));
  } else {
    Serial.println(F("Connection failed"));
  }
  // Check for a response from the server, and route it out the serial port.
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      Serial.print(c);
    }      
  }
  Serial.println();
  client.stop();
}

