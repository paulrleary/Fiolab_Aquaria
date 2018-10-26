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