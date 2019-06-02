/////////////////////////////////////////////////////////////////
// Dehydrator.ino
// by: Truglodite
// updated: 5-25-2019
//
// A simple Arduino bang bang relay controller for a food dehydrator.
//
// All user customizible settings are in configuration.h.
/////////////////////////////////////////////////////////////////

//#include <EEPROM.h>
#include <OneWire.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <LiquidCrystal_I2C.h>
#include <avr/pgmspace.h>
#include "configuration.h"

//(more) global vars///////////////////////////////////////////////
unsigned long currentMillis = debounce + 1; //make sure we read boot buttons
unsigned long dallasReadTime = 0; //store time when last temp read was sent
unsigned long dhtReadTime = 0; //store time when last temp read was sent
unsigned long lastDisplayTime = 0; //store time when last display update was sent
unsigned long lastButton = 0; //debouncing & repeat time storage
unsigned long heaterOffTime = 0; //holder for min switch timer and heater fan timer
unsigned long heaterOnTime = 0;  //holder for min switch timer
unsigned long dryStartTime = 0;
unsigned long filamentMillis = defaultDryTime * 3600000;  //time to dry
int dallasState = 0;  //temperature state machine case
byte mode = 4; //flag for mode: 0=dry, 1=hold, 2=filament select, 3=hysterisis, 4=off (default 4, boot to off mode)
#ifdef dallasSensorEnabled
  double tempArray[dallasBufferSize] = {};  //array to store temperature readings
  int i = 0;    //temperature buffer index
#endif
#ifndef dallasSensorEnabled
  double tempArray[dhtBufferSize] = {};  //array to store temperature readings
#endif
double tempAverage = 22.0;
double tempDesired = defaultTemp;
double tempHolding = defaultTempHolding;
int dhtRetries = 0;
double dhtArray[dhtBufferSize] = {};
int j = 0;    //humidity buffer index
double humidAverage = 20.0;
double humidDesired = defaultHumid;
bool fanStatus = true;
bool heaterStatus = false;
bool dryingFlag = false;
bool bootup = true;
int currentFilament = 4;  //we'll bootup with dessicant
double heaterPercent = 0.0;
double tempHysteresis = tempHysteresisDefault;

//dallas sensor
#ifdef dallasSensorEnabled
  OneWire oneWire(dallasPin);
  DallasTemperature dallas(&oneWire);
  DeviceAddress dallasAddress;
#endif
//dht sensor
DHT dht(dhtPin,dhtType);
//i2c display
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);//Banggood blue 20x4 i2c display... or 0x27

void setup() {
  //pins
  pinMode(selectPin,INPUT_PULLUP);       //...   select button w/pull-up
  pinMode(upPin,INPUT_PULLUP);           //...   up button w/ pull-up
  pinMode(downPin,INPUT_PULLUP);         //...   down button w/pull-up
  pinMode(heaterPin,OUTPUT);             //...   heater signal
  pinMode(fanPin,OUTPUT);                //...   heater fan signal
  pinMode(ledPin,OUTPUT);                //...   LED indicator

  //safely init the outputs
  digitalWrite(fanPin,HIGH);
  digitalWrite(heaterPin,LOW);
  digitalWrite(ledPin,LOW);

  #ifdef dallasSensorEnabled
    //start dallas
    dallas.begin();
    dallas.getAddress(dallasAddress, 0);  //just one sensor at index 0
    dallas.setResolution(dallasAddress, dallasResolution);    //set res
    dallas.setWaitForConversion(false); //instead we will use non-blocking code to manually time our readings
  #endif
  //start dht
  dht.begin();

  readButtons(); // check for bootup mode buttons

  //start lcd
  lcd.begin(16,2);
  lcd.clear();
  lcd.setCursor(0,0);                          //shamelessness... :P
  //       ("1234567890123456");
  lcd.print(F("Smart Dehydrator"));
  lcd.setCursor(0,1);
  lcd.print(F("by: Kevin Bernas"));
  delay(splashTime);  //window of time to release a "hold on boot" button

  bootup = 0;  // no more bootup modes
}

void(* resetFunc)(void) = 0; //reset function at address 0

void loop() {
  currentMillis = millis(); // grabbing time 1/loop is more than enough for this
  readButtons();
  #ifdef dallasSensorEnabled
    readDallasTemp();
  #endif
  readDHT();

  // control heater output, depending on mode
  switch(mode)  {

    // drying mode (constant fan, fixed temp, for X time)
    case 0:  {
      if(!dryingFlag)  {  // just started drying...
        dryStartTime = currentMillis;  //start dry timer
        dryingFlag = 1;  // only do once per dry mode
      }
      if(currentMillis - dryStartTime >= filamentMillis) {  //dried long enough...
        dryingFlag = 0;  // reset drying flag for next dry mode
        mode = 1;  // change to holding mode
        break;
      }
      if(heaterStatus && tempAverage >= tempDesired + tempHysteresis && currentMillis - heaterOnTime > heaterMinSwitchTime) {
        // heater on, it's too hot, and we have been on long enough
        digitalWrite(heaterPin, LOW); // heater off
        digitalWrite(ledPin, LOW);
        heaterStatus = false;
        unsigned long a = currentMillis - heaterOnTime; // update heater percent
        unsigned long b = currentMillis - heaterOffTime;
        heaterPercent = 100.0 * a / b;
        heaterOffTime = currentMillis;
      }
      else if(!heaterStatus && tempAverage <= tempDesired - tempHysteresis && currentMillis - heaterOffTime > heaterMinSwitchTime)  {
        // we're off, it's too cold, and we have been off long enough
        heaterStatus = true;  // heater on
        digitalWrite(heaterPin, HIGH);
        digitalWrite(ledPin, HIGH);
        unsigned long a = heaterOffTime - heaterOnTime; // update heater percent
        unsigned long b = currentMillis - heaterOnTime;
        heaterPercent = 100.0 * a / b;
        heaterOnTime = currentMillis;
      }
      break;
    }

    // holding mode (fixed humidity)
    case 1: {
      if(!heaterStatus && humidAverage >= humidDesired + humidHysteresis && currentMillis - heaterOffTime > heaterMinSwitchTime && tempAverage < tempHolding - tempHysteresis ) {
        // off, too much humidity, have been off long enough, and not too hot
        heaterStatus = true;  // heater on
        digitalWrite(heaterPin, HIGH);
        digitalWrite(ledPin, HIGH);
        unsigned long a = heaterOffTime - heaterOnTime; // update heater percent
        unsigned long b = currentMillis - heaterOnTime;
        heaterPercent = 100.0 * a / b;
        heaterOnTime = currentMillis;
      }
      else if((heaterStatus && humidAverage <= humidDesired - humidHysteresis && currentMillis - heaterOnTime > heaterMinSwitchTime) || (tempAverage > tempHolding + tempHysteresis && heaterStatus))  {
        // on, dry enough, and have been on long enough, or on and too hot
        digitalWrite(heaterPin, LOW); // heater off
        digitalWrite(ledPin, LOW);
        heaterStatus = false;
        unsigned long a = currentMillis - heaterOnTime; // update heater percent
        unsigned long b = currentMillis - heaterOffTime;
        heaterPercent = 100.0 * a / b;
        heaterOffTime = currentMillis;
      }
      // update heater fan
      if(!fanStatus && heaterStatus) {
        // fan off & heater on
        digitalWrite(fanPin,HIGH);
        fanStatus = true;
      }
      else if(!heaterStatus && fanStatus && currentMillis - heaterOffTime > heaterFanDelay)  {
        // heater off, fan on, & heater off long enough...
        digitalWrite(fanPin,LOW);  // fan off
        fanStatus = false;
      }
      break;
    }

    // filament select mode
    case 2: {
      digitalWrite(heaterPin, LOW); // heater off
      digitalWrite(ledPin, LOW);
      heaterStatus = false;
      digitalWrite(fanPin,LOW);  // fan off
      fanStatus = false;
      break;
    }

    // bootup hysterisis adjust mode
    case 3:  {
      digitalWrite(heaterPin, LOW); // heater off
      digitalWrite(ledPin, LOW);
      heaterStatus = false;
      digitalWrite(fanPin,LOW);  // fan off
      fanStatus = false;
      break;
    }
    //off
    case 4:  {
      digitalWrite(heaterPin, LOW); //heater off
      digitalWrite(ledPin, LOW);
      heaterStatus = false;
      digitalWrite(fanPin,LOW);  //fan off
      fanStatus = false;
      break;
    }
  }

  if(currentMillis - lastDisplayTime > displayPeriod) {  // limit flickering
    updateDisplay();
  }
}  //enaloop...

//Read Buttons Routine/////////////////////////////////////////////////////
void readButtons(void)  {
  if(currentMillis - lastButton > debounce)  {  // debounce
    // check for bootup mode
    if(bootup)  {
      if(!digitalRead(selectPin)) mode = 3;
      return;
    }

    switch(mode)  {
      // drying mode
      case 0: {
        if(!digitalRead(selectPin)) {  //select button, abort to holding mode
          lastButton = currentMillis;
          dryingFlag = 0; //reset drying flag
          mode = 1;  //holding mode
          break;
        }
        else if(!digitalRead(upPin)) { //up pushed, increment desired temp
          lastButton = currentMillis;
          tempDesired = tempDesired + tempIncrement;
          if(tempDesired > maxTemp) {  //stay within range
            tempDesired = maxTemp;
          }
          break;
        }
        else if(!digitalRead(downPin))  { //down pushed, decrement desired temp
          lastButton = currentMillis;
          tempDesired = tempDesired - tempIncrement;
          if(tempDesired < minTemp) {  //stay within range
            tempDesired = minTemp;
          }
          break;
        }
        else break;; //no button
      }

      // holding mode
      case 1: {
        if(!digitalRead(selectPin)) {  //select button, turn off heater and switch to filament select mode
          lastButton = currentMillis;
          digitalWrite(heaterPin,LOW);  //turn heater off
          digitalWrite(ledPin,LOW);
          heaterStatus = LOW;
          mode = 4;  //change to off mode
          //delay(heaterDelay);  //will this prevent button & lcd glitches without hardware upgrades?
          break;
        }
        else if(!digitalRead(upPin)) { //up button, increment desired humidity
          lastButton = currentMillis;
          humidDesired += humidIncrement;
          if(humidDesired > humidMax)  {  //stay within range
            humidDesired = humidMax;
          }
          break;
        }
        else if(!digitalRead(downPin))  { //down pushed, decrement desired humidity
          lastButton = currentMillis;
          humidDesired -= humidIncrement;
          if(humidDesired < humidMin)  {  //stay within range
            humidDesired = humidMin;
          }
          break;
        }
        else break;  //no button
      }

      //filament select mode buttons
      case 2: {
        if(!digitalRead(selectPin)) {  //select button, switch to drying mode
          lastButton = currentMillis;
          tempDesired = filamentData[currentFilament][0]; //update temp, time, & humid
          filamentMillis = filamentData[currentFilament][1] * 3600000;
          tempHolding = filamentData[currentFilament][2];
          humidDesired = filamentData[currentFilament][3];
          mode = 0;  //switch to drying mode
          digitalWrite(fanPin,HIGH);  //turn fan on
          fanStatus = true;
          break;
        }
        else if(!digitalRead(upPin)) { //up pushed, next filament
          lastButton = currentMillis;
          currentFilament++;
          if(currentFilament >= filamentCount)  {  //loop around list
            currentFilament = 0;
          }
          break;
        }
        else if(!digitalRead(downPin))  { //down pushed, previous filament
          lastButton = currentMillis;
          currentFilament--;
          if(currentFilament < 0)  {  //loop around list
            currentFilament = filamentCount-1;
          }
          break;
        }
        else break;  //no button
      }

      // bootup temp hysteresis mode
      case 3: {
        if(!digitalRead(selectPin)) {  // select button, switch to holding mode
          lastButton = currentMillis;
          mode = 4;  // change to off mode
          break;
        }
        else if(!digitalRead(upPin)) { //up button, increment desired humidity
          lastButton = currentMillis;
          tempHysteresis += tempHysteresisIncrement;
          if(tempHysteresis > tempHysteresisMax)  {  //stay within range
            tempHysteresis = tempHysteresisMax;
          }
          break;
        }
        else if(!digitalRead(downPin))  { //down pushed, decrement desired humidity
          lastButton = currentMillis;
          tempHysteresis -= tempHysteresisIncrement;
          if(tempHysteresis < tempHysteresisMin)  {  //stay within range
            tempHysteresis = tempHysteresisMin;
          }
          break;
        }
        else break;  //no button
      }

      //off mode
      case 4: {
        if(!digitalRead(selectPin)) {  //select button, switch to holding mode
          lastButton = currentMillis;
          mode = 2;  //change to filament select mode
          break;
        }
        else break;  //no button
      }
    }//end of mode switch
    return;
  }//end of debounced
  else return;    //Debouncing
}

// update lcd routine /////////////////////////////////////////////////////////
// AVOID USING lcd.clear(); !!!
// print padding assumes humidity <=99%, and temp <=99C
void updateDisplay(void)  {

  lastDisplayTime = currentMillis;  //update timer

  //drying mode
  if(!mode) {
    unsigned long a = currentMillis - dryStartTime; //elapsed
    a = filamentMillis - a;  //remaining
    double hoursRemaining = a/3600000.0;
    lcd.setCursor(0,0);
    //          "1234567890123456";
    //          "eta:X.Xhr RH:XX%"
    lcd.print(F("eta:"));
    if(hoursRemaining<10)  {
      lcd.print(hoursRemaining,1);  //resolution padding
    }
    else  {
      lcd.print(F(" "));
      lcd.print(hoursRemaining,0);
    }
    lcd.print(F("hr RH:"));
    if(humidAverage<10) {
      lcd.print(F(" "));  //padding...
    }
    lcd.print(humidAverage,0);
    lcd.print(F("%"));
    // actual temp/desired temp & actual humid/desired humid
    //            "1234567890123456";
    //            "XX.X/XXdC h:XXX%"
    lcd.setCursor(0,1);
    if(tempAverage<10) { //padding before actual temp...
      lcd.print(F(" "));
    }
    lcd.print(tempAverage,1);
    lcd.print(F("/"));
    if(tempDesired<10) { //padding before desired temp...
      lcd.print(F(" "));
    }
    lcd.print(tempDesired,0);
    lcd.print(char(223)); //degree symbol
    lcd.print(F("C h:"));

    if(heaterPercent<10) {
      lcd.print(F("  "));  //padding...
    }
    else if(heaterPercent<100) {
      lcd.print(F(" "));  //padding...
    }
    lcd.print(heaterPercent,0);
    lcd.print(F("%"));
    return;
  }

  //Holding Mode
  if(mode ==1) {
    lcd.setCursor(0,0);
    //            "Hold Heater:XXX%"
    //            "1234567890123456"
    lcd.print(F("Hold Heater:"));
    if(heaterPercent<10) {  //padding
      lcd.print(F("  "));
    }
    else if(heaterPercent<100) {
      lcd.print(F(" "));
    }
    lcd.print(heaterPercent,0);
    lcd.print(F("%"));
    // actual temp & actual humid/desired humid
    //            "XX.XdC  XX.X/XX%"
    //            "1234567890123456";
    lcd.setCursor(0,1);
    if(tempAverage<10) { //padding before actual temp...
      lcd.print(F(" "));
    }
    lcd.print(tempAverage,1);
    lcd.print(char(223)); //degree symbol
    lcd.print(F("C  "));
    if(humidAverage<10) {
      lcd.print(F(" "));  //padding...
    }
    lcd.print(humidAverage,1);
    lcd.print(F("/"));
    if(humidDesired<10) {
      lcd.print(F(" "));  //padding...
    }
    lcd.print(humidDesired,0);
    lcd.print(F("%"));
    return;
  }

  //Filament Select Mode
  if(mode == 2) {
    lcd.setCursor(0,0);
    //         ("1234567890123456");
    //          " Filament Name  "
    lcd.print(filamentName[currentFilament]);
    lcd.setCursor(0,1);
    //         ("1234567890123456");
    //          "T:XXdC ETA:XXhrs"
    lcd.print(F("T:"));
    lcd.print(filamentData[currentFilament][0],0);
    lcd.print(char(223)); //degree symbol
    lcd.print(F("C ETA:"));
    if(filamentData[currentFilament][1]<10) {
      lcd.print(F(" "));  //padding...
    }
    lcd.print(filamentData[currentFilament][1],0);
    lcd.print(F("hrs"));
    return;
  }

  //Bootup Mode
  if(mode == 3) {
    lcd.setCursor(0,0);
    //         ("1234567890123456");
    lcd.print(F("Temp Hysteresis "));
    lcd.setCursor(0,1);
    //         ("1234567890123456");
    lcd.print(F("   +/- "));
    lcd.print(tempHysteresis,1);
    lcd.print(char(223)); //degree symbol
    lcd.print(F("C    "));
    return;
  }

  //Off Mode
  if(mode == 4) {
    lcd.setCursor(0,0);
    //         ("1234567890123456");
    lcd.print(F(" Dehydrator OFF "));
    lcd.setCursor(0,1);
    //         ("1234567890123456");
    //          "H:XX.XdC H:XX.X%"
    lcd.setCursor(0,1);
    lcd.print(F("T:"));
    if(tempAverage<10) { // padding before actual temp...
      lcd.print(F(" "));
    }
    lcd.print(tempAverage,1);
    lcd.print(char(223)); // degree symbol
    lcd.print(F("C H:"));

    if(humidAverage<10) {
      lcd.print(F(" "));  // padding...
    }
    lcd.print(humidAverage,1);
    lcd.print(F("%"));
    return;
  }
  return;  //this shouldn't ever happen
}

// Update temperature average (non-blocking) //////////////////////////////////
#ifdef dallasSensorEnabled
void readDallasTemp(void)  {
  switch (dallasState) {
     case 0:  {
       dallas.requestTemperatures(); // Send the command to get temperature conversions
       dallasReadTime = currentMillis; // keep track of time of start
       dallasState = 1; // change to waiting state
       break;
     }
     case 1: {// bypass until temp conversion is ready
     if (currentMillis - dallasReadTime >= dallasPeriod) { // done waiting if true
         i++;
         if(i >= dallasBufferSize) {  //restart buffer if it's full
           i=0;
         }
         tempArray[i] = dallas.getTempC(dallasAddress); //store new temp value (*note, a float is recast to double)
         tempAverage = 0;  //calc new average temp...
         for(int k = 0; k < dallasBufferSize; k++) {
           tempAverage += tempArray[k];
         }
         tempAverage = tempAverage / dallasBufferSize;
         dallasState = 0; // start another conversion next time around
       }
       break;
     }
  }
}
#endif

// Update DHT humidity average (non-blocking) //////////////////////////////////
void readDHT(void)  {
  if(currentMillis - dhtReadTime < dhtPeriod) return; //return early if it's not yet time
  double humidA = dht.readHumidity();        // RH %
  #ifndef dallasSensorEnabled
    double tempA =  dht.readTemperature();
  #endif
  dhtReadTime = currentMillis;
  bool haveNAN = 0;
  #ifdef dallasSensorEnabled
    if(isnan(humidA)) haveNAN = 1;// Drop the reading if we get any NAN's  //testing a trick here
  #endif
  #ifndef dallasSensorEnabled
    if(isnan(humidA) || isnan(tempA)) haveNAN = 1;// Drop the reading(s) if we get any NAN's  //testing a trick here
  #endif
  if(haveNAN) {
      dhtRetries ++;
      if(dhtRetries > dhtRetriesMax)  { // Too many NAN's in a row, reboot... (hopefully rare w/ enough sample time)
        resetFunc();
        return;
    }
  }
  else {                              // Cool, no NAN's, let's count it...
    dhtRetries = 0;
    dhtArray[j] = humidA;
    humidAverage = 0;
    #ifndef dallasSensorEnabled
      tempArray[j] = tempA;
      tempAverage = 0;
    #endif
    for(int l = 0; l < dhtBufferSize; l++) {
      humidAverage += dhtArray[l];
      #ifndef dallasSensorEnabled
        tempAverage += tempArray[l];
      #endif
    }
    #ifndef dallasSensorEnabled
      tempAverage  = tempAverage / dhtBufferSize;
    #endif
    humidAverage = humidAverage / dhtBufferSize;
    j++;
    if(j >= dhtBufferSize) j=0;  //reset index if needed
    return;
  }
}
