   // ------------------------------------------------------------------------------------------------
// Filename:
//		LEMSv2.ino
//
// Author:
//		Nipun Gunawardena
//
// Description:
//		LEMSv2 Arduino Code
//
// Credits:
//		Thanks to all the 3rd party libraries, especially to Adafruit for the excellent
//		libraries and hardware. More specifics to be added as needed
//    - Sleep code taken from https://github.com/rocketscream/Low-Power/blob/master/LowPower.cpp
//      Thanks to @sandeepmistry at https://github.com/arduino/ArduinoCore-samd/issues/142 for further help
//
// Requirements:
//		- LEMS Hardware
//		- Libraries in libraries list
//
// Notes:
//		- As far as I know, extra files have to be in the same folder as the sketch. Therefore,
//      the DS3231 alarm files have to be copied from their test folder. This means there are
//      possibilities for code divergence. Be aware.
//
// Todo:
//    - Fix battery and Licor Resistors
//
// ------------------------------------------------------------------------------------------------



// Sensor Defines: Plugged in sensors should be defined as 1 --------------------------------------
// TODO: Depreciate Wind
#define TEMPRH 1
#define IR 1
#define UPPERSOIL 1
#define LOWERSOIL 1
#define SUNLIGHT 0
#define PRESSURE 1
#define SONIC 1
#define WIND 0
#define PYRGEOMETER 0
#define GPS 1
#define SNOW 1


// Other Defines ----------------------------------------------------------------------------------
// !! ALERT !! When DEBUG is defined as 0, the LEMSv2 will only work properly under battery or
// external power, NOT under USB power. If you would like to power the LEMS via USB but have DEBUG
// set to 0, uncomment the three lines centered around the line that has USBDevice.detach(); This
// should be near line 207 in the code.
#define DEBUG 0
//#define TIMEOUT 5000


// Pin Defines ------------------------------------------------------------------------------------
#define GREEN_LED_PIN 6     // Green LED pin
#define RED_LED_PIN 7       // Red LED pin
#define BLUE_LED_PIN 13     // Built-in Blue LED - Sparkfun SAMD21 Dev Board

#define CARDSELECT 5        // SD  card chip select pin
#define RTC_ALARM_PIN 9     // DS3231 Alarm pin
#define BAT_PIN A5          // Battery resistor div. pin
#define FAN_PIN 4          // Pin for cooling fan

#define WDIR_PIN A4         // Davis wind direction pin
#define WSPD_PIN 8          // Davis wind speed pin

#define USOIL_POW_PIN 2     // 5TM Power Pin, Upper - White 5TM Wire
#define LSOIL_POW_PIN 38    // 5TM Power Pin, Lower - White 5TM Wire
#define USOIL_SER Serial1   // 5TM Serial Port, Upper RX = D0, TX = D1; Red 5TM Wire
#define LSOIL_SER Serial    // 5TM Serial Port, Lower RX = D31, TX = D30; Red 5TM Wire

#define SONIC_PIN 10        // DS2 Data Pin, Red DS2 Wire

#define ADC_RES 12          // Number of bits Arduino ADC has
#define ADS_LI200_PIN 0     // LiCor Li200 pin on ADS1115
#define ADS_VCC_PIN 1       // VCC pin on ADS1115

#define ADS_SNOW_PIN 2      //MB 7334-100 (snow depth) pin on ADS1115

#define THERM_PIN A3        // SL-610 Thermistor output
#define THERM_POW 3         // Thermistor power pin
// Positive thermopile pin is in AIN2, negative in AIN3.
// Serial Number: SL-610-SS_1034 !! USES NEW WIRING COLORS !!
// OR
// Serial Number: SL-510-SS_1076 !! USES NEW WIRING COLORS !!
#define GPS_POWER1 11        // GPS power pin, could also use D12

#define GPS_RX A2           // GPS recieve data
#define GPS_TX A1           // Do not plug in TX, it will make the SD card unable to initialize for an unknown reason






// Libraries --------------------------------------------------------------------------------------
#include <SD.h>			            // SD Card Library
#include <SPI.h>		            // SPI Library
#include "DS2.h"                // DS2 Library
#include "d5TM.h"               // 5TM Library
#include <math.h>               // Math Library - https://www.arduino.cc/en/Math/H
#include <Wire.h>		            // I2C Library
//#include <RTClib.h>		          // RTC Library - https://github.com/adafruit/RTClib
#include "DS3231_Alarm1.h"      // RTC Alarm Library Files
#include <Adafruit_SHT31.h>     // SHT31 - https://github.com/adafruit/Adafruit_SHT31
#include <Adafruit_Sensor.h>    // Necessary for BMP280 Code - https://github.com/adafruit/Adafruit_Sensor
#include <Adafruit_BMP280.h>    // BMP280 - https://github.com/adafruit/Adafruit_BMP280_Library
#include <Adafruit_ADS1015.h>   // ADS1115 - https://github.com/soligen2010/Adafruit_ADS1X15
#include <Adafruit_MLX90614.h>  // MLX90614 Library - https://github.com/adafruit/Adafruit-MLX90614-Library
#include <RTClibExtended.h>
#include <TinyGPS++.h>          // GPS 
#include "wiring_private.h"     // ESP8266 Library
// Initialize Variables----------------------------------------------------------------------------

// Real Time Clock
RTC_DS3231 rtc;                 // Real Time Clock Class
DS3231_Alarm1 rtcAlarm1;        // RTC Alarm
volatile bool rtcFlag = false;  // Used for ISR to know when to take measurements
const uint8_t deltaT = 10;      // Sampling time - Seconds

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};





// SD Card
File logfile;                       // File object
char filename[] = "LEMSUA00.CSV";   // Initial filename - Whatever this is named, the filename should be 8 characters long and the last two should be digits!



// GPS
#if GPS
// GPS TinyGPS++.h
TinyGPSPlus gps;
char* position_filename = "GPSfile0.csv";  //Check this if having problems
File myfile;



// Create the new UART instance assigning it to pin 15,16
Uart gpsSerial(&sercom4, A2, A1, SERCOM_RX_PAD_1, UART_TX_PAD_0); //Pin A1 is I/O, Pin A2 is I/O

// Attach the interrupt handler to the SERCOM
void SERCOM4_Handler()
{
  gpsSerial.IrqHandler();
}


bool encode_GPS_time(int);
bool encode_GPS_position(int);

void wake() {}
bool get_position_points();
void standbySleep(void);

// Data array size for lat and long
double locations[10][2]; // (lat, lng)

const unsigned int trigger_count = 8640/6; //number of cycles before the GPS takes a reading (# of 10-second cycles in a day divided by number of times to take a reading per day)
unsigned int counter = trigger_count; // variable for counting cycles since last measure, initially set so the first reading will be taken on first cycle
#endif


// ADS1115
Adafruit_ADS1115 ads;
double vcc;                         // Actual value of 3.3V supply voltage

// Battery Level - Max output of 14.6V outputs 3.2444V on A5
double vBat;
const unsigned long R1 = 56000;         // Battery Side Resistor - See https://en.wikipedia.org/wiki/Voltage_divider
const unsigned long R2 = 16000;         // Ground Side Resistor

// MLX90614
#if IR
double mlxIR;                                   // IR values from MLX90614
double mlxAmb;                                  // Ambient temp values from MLX90614
Adafruit_MLX90614 mlx = Adafruit_MLX90614();    // MLX90614 class
#endif

// Upper soil
#if UPPERSOIL
d5TM upperSoil(USOIL_SER, 1200, USOIL_POW_PIN); // Initialize 5TM class
#endif

// Lower soil
#if LOWERSOIL
d5TM lowerSoil(LSOIL_SER, 1200, LSOIL_POW_PIN); // Initialize 5TM class
#endif

// BMP280
#if PRESSURE
Adafruit_BMP280 bmp;         // Initialize BMP280 class
double pressure;             // Barometric pressure
double bmpAmb;               // Temperature from BMP280
double approxAlt;            // Approximate altitude from BMP280
#endif

// Wind
#if WIND
unsigned long startTime;            // Used to measure averaging period for wind speed
volatile unsigned long windCount;   // Counts to convert to speed
double wDir;                         // Wind direction
double wSpd;                         // Wind speed
#endif

#if SUNLIGHT
double rawSun = 0;                       // Float to hold voltage read from ADS1115
const double liConst = 6.5600E-5 / 1000;  // Licor Calibration Constant. Units of (Amps/(W/m^2))
const double ampResistor = 52300;        // Exact Resistor Value used by Op-Amp in Ohms
double sunlight = 0.0;                   // Converted Value
#endif

// SHT21
#if TEMPRH
Adafruit_SHT31 sht = Adafruit_SHT31();
double shtAmb;               // Ambient temp values from SHT21
double shtHum;               // Relative humidity values from SHT21
#endif

// DS2
#if SONIC
DS2 sonic(SONIC_PIN, '0');   // Initialize DS2 class
double sonicDir;             // Wind direction from sonic
double sonicSpd;             // Wind speed from sonic
double sonicGst;             // Wind gust from sonic
double sonicTmp;             // Wind temperature from sonic
#endif

// Pyrgeometer
#if PYRGEOMETER

// Thermistor variables
double thermV;            // Voltage reading from arduino ADC for thermistor (volts)
double Rt;                // Thermistor resistance (ohm)
double thermTemp;         // Temperature as measured by thermistor (Kelvin)
double Agz = 9.32793534266128e-4;  // Greater than 0 C thermistor constants
double Bgz = 2.21450736014070e-4;  // "
double Cgz = 1.26232582309837e-7;  // "
double Alz = 9.32959957496852e-4;  // Less than 0 C thermistor constants
double Blz = 2.21423593265217e-4;  // "
double Clz = 1.26328669787011e-7;  // "
unsigned int rawTemp = 0;          // Analog read from thermistor

// Thermopile Constants & variables
double k1 = 9.712;     // Wm^-2 per mV - Serial number 1034
double k2 = 1.028;     // Unitless - Serial number 1034
//double k1 = 9.786;      // Serial number 1076
//double k2 = 1.031;       // "
double pilemV;         // Voltage from thermopile
double LWi;            // Incoming longwave radiation
#endif

// MB 7334-100
#if SNOW
double depthVolt = 0;       //Volatge coming out of the sensor
double snowDepth = 0;       //depth of snow
#endif




// Setup ------------------------------------------------------------------------------------------
// ************************************************************************************************
void setup() {

#if DEBUG
  while (!SerialUSB);
  SerialUSB.begin(9600);
  SerialUSB.println("Starting Sketch");
#endif
  delay(3000);                                  // Allows to unplug before main code starts

  // Set pins
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(CARDSELECT, OUTPUT);
  analogReadResolution(ADC_RES);                // Set arduino resolution to 12 bit - only works on ARM arduino boards

  // Set fan low in case it's plugged in but not wanting to be used
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);

  // Turn on LED during setup process
  digitalWrite(GREEN_LED_PIN, HIGH);

  // ADS1115 begin
  ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 0.000125V
  ads.begin();

  // Setup registers so interrupts wake board from sleep
  SYSCTRL->VREG.bit.RUNSTDBY = 1;              // Configure regulator to run normally in standby mode, so not limited to 50uA
  SYSCTRL->DFLLCTRL.bit.RUNSTDBY = 1;          // Enable DFLL48MHz clock in standby mode

  // Disable USB device to avoid USB interrupts. Necessitates double press of reset button to upload though
  // This is only needed when DEBUG is 0, AND you want to power the LEMSv2 from USB power for some reason
  //  #if !DEBUG
  //    USBDevice.detach();
  //  #endif

  // RTC Setup
  if (!rtc.begin()) {
    error("Couldn't Find RTC!");
  }

  // Attach Interrupt - Interrupts from RTC used instead of delay()
  attachInterrupt(RTC_ALARM_PIN, rtcISR, FALLING);
  

  #if GPS
  
    gpsSerial.begin(9600);

  pinPeripheral(A1, PIO_SERCOM_ALT);  // TX
  pinPeripheral(A2, PIO_SERCOM_ALT);  // RX
  
  
  // Set pin modes
  pinMode(GPS_POWER1, OUTPUT);
    
//  if (rtc.lostPower()) {
//    //Serial.println("RTC lost power, lets set the time!");
//    // following line sets the RTC to the date & time this sketch was compiled
//    //    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//    // This line sets the RTC with an explicit date & time, for example to set
//    // January 21, 2014 at 3am you would call:
//    rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0)); // it has been observed that the time must be set to ANY TIME before time can be set to actual time
//  }

   // Turning on GPS to get time
  digitalWrite(GPS_POWER1, HIGH);
  delay(250);


  if (encode_GPS_time(240000)) { // 4 min max wait to get GPS time, extremely unlikely that time data will not be received
    // print time
    DateTime now = rtc.now();
  }
  digitalWrite(GPS_POWER1, LOW);

   get_position_points();
  
#endif
  
  // Check for Card Present
    // Check for Card Present
  if (!SD.begin(CARDSELECT)) {
  }
  if (!SD.begin(CARDSELECT)) {
    error("Card Init. Failed!");
  }
  
#if GPS
 // Open SD file
  
  myfile = SD.open(position_filename, FILE_WRITE);
 if (!myfile) {
   // Serial.print("Error opening: ");
   // Serial.println(position_filename);
  } else {
    // Save Data to SD
    for (int i = 0; i < 10; i++) {
     // Serial.print(locations[i][0], 6);
     // Serial.print(",");
     // Serial.println(locations[i][1], 6);
      myfile.print(locations[i][0], 6);
      myfile.print(",");
      myfile.println(locations[i][1], 6);
    }
    myfile.flush();

   // Serial.println("Done writing to SD");
  }
  myfile.close();



#endif


  // Check for new filename
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i / 10;
    filename[7] = '0' + i % 10;
    if (! SD.exists(filename)) {
      break;
    }
  }

  // Open file
  logfile = SD.open(filename, FILE_WRITE);
  if (!logfile) {
    error("Couldn't Create File!");
  }
#if DEBUG
  SerialUSB.print("Writing to ");
  SerialUSB.println(filename);
#endif

  // Write Header - Debug header at end of setup()
  // Sensors also initiated here
  logfile.print("Year,Month,Date,Hour,Minute,Second,Bat_Lvl");
#if IR
  mlx.begin();
  logfile.print(",MLX_IR_C,MLX_Amb_C");
#endif
#if UPPERSOIL
  upperSoil.begin();
  logfile.print(",Upper_Soil_Temp,Upper_Soil_Mois");
#endif
#if LOWERSOIL
  lowerSoil.begin();
  logfile.print(",Lower_Soil_Temp,Lower_Soil_Mois");
#endif
#if PRESSURE
  bmp.begin();
  logfile.print(",Pressure,BMP_Amb");
#endif
#if WIND
  logfile.print(",Wind_Dir,Wind_Spd");
  pinMode(WSPD_PIN, INPUT_PULLUP);
#endif
#if SONIC
  logfile.print(",Sonic_Dir,Sonic_Spd,Sonic_Gst,Sonic_Tmp");
  sonic.begin();
#endif
#if SUNLIGHT
  logfile.print(",Sunlight");
#endif
#if PYRGEOMETER
  pinMode(THERM_POW, OUTPUT);
  digitalWrite(THERM_POW, LOW);
  logfile.print(",Longwave,Thermistor_Tmp,Pile_mV,thermV");
#endif
#if TEMPRH
  sht.begin(0x44);    // Set to 0x45 for alt. i2c address
  logfile.print(",SHT_Amb_C,SHT_Hum_Pct");
#endif
#if SNOW
  logfile.print(",Snow_Depth");
#endif
  logfile.println();

  // Delay and indicate start
  delay(3500);
  digitalWrite(GREEN_LED_PIN, LOW);

  // Wait for next time with evenly divisible deltaT, then set alarm
  // If alarm set time is <1 second away, wait for next even value
  DateTime now = rtc.now();
  uint8_t setSec;                           // Seconds value to set alarm to
  uint8_t rem = now.second() % deltaT;      // Seconds since last set point
  rem = deltaT - rem;                       // Calculate seconds until next set point
  setSec = ((rem == 1) ? 1 + deltaT : rem); // Set to next *convenient* set point
  rtcAlarm1.alarmSecondsSet(now, setSec);
  rtcAlarm1.alarmOn();
  rtcFlag = false;
#if DEBUG
  SerialUSB.print("Setting RTC alarm to ");
  SerialUSB.print(deltaT);
  SerialUSB.println(" second evenly divisible start point");
  SerialUSB.print("Current Seconds: ");
  SerialUSB.print(now.second());
  SerialUSB.print(" || Setting alarm to ");
  SerialUSB.print(setSec);
  SerialUSB.println(" seconds from now. Entering Loop...");
  SerialUSB.println();
#endif

} // End setup()






// Loop -------------------------------------------------------------------------------------------
// ************************************************************************************************
void loop() {
  // Wait for interrupt

  digitalWrite(GREEN_LED_PIN, rtcFlag);    // Turn LED off
#if DEBUG
  while (!rtcFlag);                        // Use while loop to avoid sleep states or debugging
#else
  standbySleep();                          // Use standbySleep() to reduce power consumption
#endif

#if GPS
  if (rtc.lostPower() || counter == trigger_count ) {

    // Turning on GPS to get time
    digitalWrite(GPS_POWER1, HIGH);
    delay(250);

    
   if (encode_GPS_time(60000);
   //adjust time here?//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        rtc.adjust(DateTime(
                     gps.date.year(),
                     gps.date.month(),
                     gps.date.day(),
                     gps.time.hour(),
                     gps.time.minute(),
                     gps.time.second()
                   ));
   
    digitalWrite(GPS_POWER1, LOW);

  counter = 0;
  
    logfile.close;
    myfile = SD.open(position_filename, FILE_WRITE);
 if (!myfile) {
   // Serial.print("Error opening: ");
   // Serial.println(position_filename);
  } else {
    // Save Data to SD
    for (int i = 0; i < 10; i++) {
     // Serial.print(locations[i][0], 6);
     // Serial.print(",");
     // Serial.println(locations[i][1], 6);
      myfile.print(locations[i][0], 6);
      myfile.print(",");
      myfile.println(locations[i][1], 6);
    }
    myfile.flush();

    logfile = SD.open(filename, FILE_WRITE);

   // Serial.println("Done writing to SD");
  }
  myfile.close();
  
  } else {
    counter++;
  }

#endif

  // Gather Measurements
  DateTime now = rtc.now();
  vcc = ads.readADC_SingleEnded_V(ADS_VCC_PIN);       // VCC connected to AIN1
  vBat = double(analogRead(BAT_PIN)) * (vcc / pow(2, ADC_RES)) * double(R1 + R2) / double(R2);
#if IR
  mlxIR = mlx.readObjectTempC();
  mlxAmb = mlx.readAmbientTempC();
#endif
#if UPPERSOIL
  upperSoil.getMeasurements();
#endif
#if LOWERSOIL
  lowerSoil.getMeasurements();
#endif
#if PRESSURE
  bmpAmb = bmp.readTemperature();
  pressure = bmp.readPressure();
#endif
#if SUNLIGHT
  rawSun = ads.readADC_SingleEnded_V(ADS_LI200_PIN);                     // LiCor Amplifier connected to AIN0
  sunlight = rawSun * (1.0 / ampResistor) * (1.0 / liConst); // Convert to W/m^2
#endif
#if TEMPRH
  shtAmb = sht.readTemperature();
  shtHum = sht.readHumidity();
#endif
#if SONIC
  sonic.getMeasurements();
#endif
#if PYRGEOMETER
  ads.setGain(GAIN_SIXTEEN);

  // Read thermistor temperature
  digitalWrite(THERM_POW, HIGH);
  rawTemp = analogRead(THERM_PIN);
  digitalWrite(THERM_POW, LOW);
  thermV = double(rawTemp) / (pow(2, ADC_RES) - 1) * vcc;
  Rt = 24900.0 * thermV / (vcc - thermV);
  if (Rt >= 94980) { // If greater than this value, means temp is less than zero
    thermTemp = 1.0 / (Alz + Blz * log(Rt) + Clz * pow(log(Rt), 3));
  } else {
    thermTemp = 1.0 / (Agz + Bgz * log(Rt) + Cgz * pow(log(Rt), 3));
  }

  // Read thermopile
  pilemV = ads.readADC_Differential_2_3_V() * 1000.0; // Positive thermopile pin is in AIN2, negative in AIN3.
  LWi = k1 * pilemV + k2 * 5.6704e-8 * pow(thermTemp, 4);

  // Convert to Celsius
  thermTemp = thermTemp - 273.15;

  ads.setGain(GAIN_ONE);
#endif
#if WIND
  wDir = (double)analogRead(WDIR_PIN) * 360.0 / pow(2, ADC_RES);  // Map from analog count to 0-360, with 360=North
  startTime = millis();
  attachInterrupt(WSPD_PIN, windCounter, RISING);                 // Attach interrupt for wind speed pin
  while (millis() - startTime < 4500);                            // Measure number of counts in 4.5 sec
  detachInterrupt(WSPD_PIN);
  wSpd = windCount * (2.25 / 4.5);                                // mph = (counts)*(2.25)/(sample period in seconds). If period is 2.25 seconds, mph = counts
  wSpd = 0.447 * wSpd;                                            // Convert to m/s
#endif
#if SNOW
  depthVolt = ads.readADC_SingleEnded_V(ADS_SNOW_PIN);            //voltage from the MB 7334-100
  snowDepth = depthVolt*5120.0/vcc;                               //depth [mm] = (voltage out of sensor)*5120/(voltage into sensor)
#endif


  // Log Data
  digitalWrite(GREEN_LED_PIN, rtcFlag);    // Turn LED on
  logfile.print(now.year());
  logfile.print(",");
  logfile.print(now.month());
  logfile.print(",");
  logfile.print(now.day());
  logfile.print(",");
  logfile.print(now.hour());
  logfile.print(",");
  logfile.print(now.minute());
  logfile.print(",");
  logfile.print(now.second());
  logfile.print(",");
  logfile.print(vBat);
#if IR
  logfile.print(",");
  logfile.print(mlxIR);
  logfile.print(",");
  logfile.print(mlxAmb);
#endif
#if UPPERSOIL
  logfile.print(",");
  logfile.print(upperSoil.temperature);
  logfile.print(",");
  logfile.print(upperSoil.moisture);
#endif
#if LOWERSOIL
  logfile.print(",");
  logfile.print(lowerSoil.temperature);
  logfile.print(",");
  logfile.print(lowerSoil.moisture);
#endif
#if PRESSURE
  logfile.print(",");
  logfile.print(pressure);
  logfile.print(",");
  logfile.print(bmpAmb);
#endif
#if WIND
  logfile.print(",");
  logfile.print(wDir);
  logfile.print(",");
  logfile.print(wSpd);
#endif
#if SONIC
  logfile.print(",");
  logfile.print(sonic.wDir);
  logfile.print(",");
  logfile.print(sonic.wSpd);
  logfile.print(",");
  logfile.print(sonic.wGst);
  logfile.print(",");
  logfile.print(sonic.wTmp);
#endif
#if SUNLIGHT
  logfile.print(",");
  logfile.print(sunlight);
#endif
#if PYRGEOMETER
  logfile.print(",");
  logfile.print(LWi);
  logfile.print(",");
  logfile.print(thermTemp);
  logfile.print(",");
  logfile.print(pilemV,6);
  logfile.print(",");
  logfile.print(thermV,6);
#endif
#if TEMPRH
  logfile.print(",");
  logfile.print(shtAmb);
  logfile.print(",");
  logfile.print(shtHum);
#endif
#if SNOW
  logfile.print(",");
  logfile.print(snowDepth);
#endif
  logfile.println();
  logfile.flush();


  // Debug Info
#if DEBUG
  SerialUSB.print(now.year(), DEC);
  SerialUSB.print(", ");
  SerialUSB.print(now.month(), DEC);
  SerialUSB.print(", ");
  SerialUSB.print(now.day(), DEC);
  SerialUSB.print(", ");
  SerialUSB.print(now.hour(), DEC);
  SerialUSB.print(", ");
  SerialUSB.print(now.minute(), DEC);
  SerialUSB.print(", ");
  SerialUSB.print(now.second(), DEC);
  SerialUSB.print(", ");
  SerialUSB.print(vBat);
#if IR
  SerialUSB.print(", ");
  SerialUSB.print(mlxIR);
  SerialUSB.print(", ");
  SerialUSB.print(mlxAmb);
#endif
#if UPPERSOIL
  SerialUSB.print(", ");
  SerialUSB.print(upperSoil.temperature);
  SerialUSB.print(", ");
  SerialUSB.print(upperSoil.moisture);
#endif
#if LOWERSOIL
  SerialUSB.print(", ");
  SerialUSB.print(lowerSoil.temperature);
  SerialUSB.print(", ");
  SerialUSB.print(lowerSoil.moisture);
#endif
#if PRESSURE
  SerialUSB.print(", ");
  SerialUSB.print(pressure);
  SerialUSB.print(", ");
  SerialUSB.print(bmpAmb);
#endif
#if WIND
  SerialUSB.print(", ");
  SerialUSB.print(wDir);
  SerialUSB.print(", ");
  SerialUSB.print(wSpd);
#endif
#if SONIC
  SerialUSB.print(", ");
  SerialUSB.print(sonic.wDir);
  SerialUSB.print(", ");
  SerialUSB.print(sonic.wSpd);
  SerialUSB.print(", ");
  SerialUSB.print(sonic.wGst);
  SerialUSB.print(", ");
  SerialUSB.print(sonic.wTmp);
#endif
#if SUNLIGHT
  SerialUSB.print(", ");
  SerialUSB.print(sunlight);
#endif
#if PYRGEOMETER
  SerialUSB.print(", ");
  SerialUSB.print(LWi);
  SerialUSB.print(", ");
  SerialUSB.print(thermTemp);
#endif
#if TEMPRH
  SerialUSB.print(", ");
  SerialUSB.print(shtAmb);
  SerialUSB.print(", ");
  SerialUSB.print(shtHum);
#endif
  SerialUSB.println();
#endif

  // Delay just a little bit for longer blink
  delay(50);

  // Reset any variables and turn alarm on for next measurement
#if WIND
  windCount = 0;
#endif
  rtcAlarm1.alarmSecondsSet(now, deltaT);
  rtcFlag = false;


} // End loop()






// Functions --------------------------------------------------------------------------------------
// ************************************************************************************************

// Turn red LED on and loop. Used in case of errors
void error(String errorMsg) {
#if DEBUG
  SerialUSB.println(errorMsg);
#endif
  digitalWrite(GREEN_LED_PIN, LOW);   // Ensure green LED is off
  while (true) {
    digitalWrite(RED_LED_PIN, HIGH);
    delay(300);
    digitalWrite(RED_LED_PIN, LOW);
    delay(3000);
  }
}


// ISR for RTC alarm. Used to break out of while loop used instead of delay
void rtcISR(void) {
  rtcFlag = true;
}


// ISR for Davis Anemometer. Increments wind counter with every revolution
#if WIND
void windCounter() {
  windCount++;
}
#endif


// Function to put SAMD21 into standby sleep
void standbySleep(void) {
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
  __WFI();
}


bool encode_GPS_time(int timeout) {
  //Serial.println("encode_GPS_time");
  unsigned long startTime = millis();
      
  // Repeat
  while (1) {
  
    // If data available
    if (gpsSerial.available()) {

      //Serial.println("gpsSerial.available");

      // While data available
      while (gpsSerial.available() > 0) {
        // GPS Encode(Read in data)
        gps.encode(gpsSerial.read());
      }

      //Serial.println("Any data available");

      // Get time from GPS data when the time date and time has been updated
      if (gps.date.isUpdated() && gps.time.isUpdated()) {
        if (gps.date.isValid() && gps.time.isValid()){

//        rtc.adjust(DateTime(
//                     gps.date.year(),
//                     gps.date.month(),
//                     gps.date.day(),
//                     gps.time.hour(),
//                     gps.time.minute(),
//                     gps.time.second()
//                   ));

        //Serial.println("Time and date Set");
        return true;
        }
      } else {

        
        //Serial.println("Not time data");
      }

    } // of if (.availalble)



    // If timeout duration reached
    if ((millis() - startTime) > timeout)
    {
      //Serial.println("Timeout reached, exiting");
      return false;
    }
  } 
}


bool encode_GPS_position(int timeout) {
  //Serial.println("encode_GPS_position");
  unsigned long startTime = millis();

  // Repeat
  while (1) {

    // If data available
    if (gpsSerial.available()) {

      //Serial.println("gpsSerial.available");

      // While data available
      while (gpsSerial.available() > 0) {
        // GPS Encode(Read in data)
        gps.encode(gpsSerial.read());
      }


      //Serial.println("Any data available");

      // Get time from GPS data
      if (gps.location.isUpdated() && gps.location.isValid() ) {


        //Serial.println("Location available and updated");
        return true;
      } else {
        //Serial.println("Not position data");
      }

    } // of if (.availalble)



    // If timeout duration reached
    if ((millis() - startTime) > timeout)
    {
      //Serial.println("Timeout reached, exiting");
      return false;
    }
  }
}


bool get_position_points()
{

  // Turn on GPS
  digitalWrite(GPS_POWER1, HIGH);
  delay(250);

  // For numToGet many position points
  for (int i = 0; i < numToGet; i++) {

    // Try to encode GPS
    //    if ( encode_GPS(5000) && gps.location.isUpdated() ) {
    if ( encode_GPS_position(1000)) {

      //Serial.println(gps.location.lat());
      //Serial.println(gps.location.lng());

      locations[i][0] = gps.location.lat();
      locations[i][1] = gps.location.lng();
      delay(1000);
    } else{ i = i- 1;}
    
  }
//  for (int i = 0; i < 10; i++) {         //print out the lists to see what the data looks like.
//    //Serial.print(locations[i][0], 6);
//    //Serial.print(",");
//    //Serial.println(locations[i][1], 6);
//  }


  // Turn off GPS
  digitalWrite(GPS_POWER1, LOW);

  delay(50);
}
