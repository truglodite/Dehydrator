////////////////////////////////////////////////////////////////
// configuration.h
// by: Truglodite
// updated: 5-25-2019
//
// User configuration for Dehydrator.ino.
////////////////////////////////////////////////////////////////

// Pins ////////////////////////////////////////////////////////
#define dataPin      A4   //LCD i2c data
#define clockPin     A5   //LCD i2c clock
#define dallasPin    2    //dallas data
#define dhtPin       10   //DHT data
#define fanPin       3    //fan relay
#define heaterPin    9    //heater relay
#define selectPin    4    //N.O. button to ground
#define upPin        5    //"  "
#define downPin      6    //"  "
#define ledPin       7    //LED heater indicator

// Preferences ///////////////////////////////////////////////
#define dallasSensorEnabled//comment out to use DHTXX for temperature instead
#define debounce     350  //button debounce/repeat delay (millis)
#define splashTime   2000 //millis to show splash screen (give enough time to release bootup buttons)
#define displayPeriod 200  //millis to show display (antiflicker... <debounce!)

#define defaultTemp  45   //default desired Celsius setting (45)
#define defaultTempHolding 35
#define defaultHumid 10   //bootup humidity setpoint in percent (other values available via filament table)
#define defaultDryTime 8  //default drying time in hours
#define maxTemp      80   //max desired temperature allowed (80, note DHT11 rated 50C!!!)
#define minTemp      20   //min desired temperature allowed
#define tempIncrement 1   //amount to increase/decrease temp for each button push/repeat
#define tempHysteresisDefault 0.4
#define tempHysteresisIncrement 0.1
#define tempHysteresisMax 5.0
#define tempHysteresisMin 0.1
#define humidIncrement 1
#define humidMax     80
#define humidMin     0
#define humidHysteresis 1 //% hysteresis for holding mode
#define dhtBufferSize 2   //
#define dhtType      DHT11 // DHT22, DHT11, etc...
#define dhtPeriod    2000 //millis between dht readings
#define dhtRetriesMax 5   //# of consecutive DHT NAN's before soft reset

#define dallasResolution 12 //dallas bit resolution (4,8, or 12)
#define dallasBufferSize 3 //number of temperature readings to store for averaging (1 = disable averaging, more uses memory)
#define dallasPeriod 800  //millis required before a requested temp is used (>760 for 12bit)

#define heaterMinSwitchTime 5000 // the heater will not be on or off for shorter than this many millis
#define heaterFanDelay 15000 //millis to leave the fan on after heater turns off in holding mode
#define heaterDelay 500  //millis pause button readings after turning on/off heater. This may prevent glitches from transient spikes.

// Filament Tables ////////////////////////////////////////////////
// Copied from PrintDry.com
//  **Add to filamentCount if you add new stuff.
//  **New stuff needs both data and a name.
//
//data format: {drying celsius,drying hours,holding celsius,holding humidity %},
#define filamentCount 10  //# of filaments in the tables below
double filamentData[filamentCount][4]= {
  {45,4,35,10},  //0- PLA
  {60,2,45,10},  //1- ABS
  {65,2,50,10},  //2- PETG
  {70,12,50,10}, //3- NYLON
  {65,3,50,10},  //4- Dessicant
  {45,4,35,10},  //5- PVA
  {50,3,40,10},  //6- TPU/TPE
  {60,4,45,10},  //7- ASA
  {55,6,45,10},  //8- PP
  {80,3,45,10}   //9- SUPER HOT  ;) LOL!
};

//name format:  "exactly 16 characters long",
String filamentName[filamentCount]={
//"1234567890123456",
  "      PLA       ",//0
  "      ABS       ",//1
  "      PETG      ",//2
  "     NYLON      ",//3
  "   Dessicant    ",//4
  "      PVA       ",//5
  "    TPU/TPE     ",//6
  "      ASA       ",//7
  "       PP       ",//8
  "   SUPER HOT!   " //9
//"1234567890123456"
};
