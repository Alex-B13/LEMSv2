// Pin Defines ------------------------------------------------------------------------------------

#define GPS_POWER1 4        // GPS power pin, could also use D4

#define GPS_RX A2           // GPS recieve data
#define GPS_TX A1           // Do not plug in TX, it will make the SD card unable to initialize for an unknown reason

#define CARDSELECT 5        // SD  card chip select pin
#define RTC_ALARM_PIN 9     // DS3231 Alarm pin


#define TIMEOUT 5000

const uint8_t deltaT = 10;



// Libraries --------------------------------------------------------------------------------------
#include <SD.h>                  // SD Card Library
#include <TinyGPS++.h>          // GPS library
#include <SPI.h>                // SPI Library
//#include "DS2.h"                // DS2 Library
//#include "d5TM.h"               // 5TM Library
#include <math.h>               // Math Library - https://www.arduino.cc/en/Math/H
#include <Wire.h>               // I2C Library
#include <RTClibExtended.h>             // RTC Library - https://github.com/adafruit/RTClib
// added '#define _BV(bit) (1 << (bit))' to RTClibExtended.h

//#include <RTClib.h>             // RTC Library - https://github.com/adafruit/RTClib
//#include "DS3231_Alarm1.h"      // RTC Alarm Library Files
//#include <Adafruit_SHT31.h>     // SHT31 - https://github.com/adafruit/Adafruit_SHT31
//#include <Adafruit_Sensor.h>    // Necessary for BMP280 Code - https://github.com/adafruit/Adafruit_Sensor
//#include <Adafruit_BMP280.h>    // BMP280 - https://github.com/adafruit/Adafruit_BMP280_Library
//#include <Adafruit_ADS1015.h>   // ADS1115 - https://github.com/soligen2010/Adafruit_ADS1X15
//#include <Adafruit_MLX90614.h>  // MLX90614 Library - https://github.com/adafruit/Adafruit-MLX90614-Library
#include "wiring_private.h"     // ESP8266 Library
//#include "SERCOM.h"             // ArduinoCore-samd
//#include "variant.h"            // Multiplexing library for SAMD boards

// Initialize Variables----------------------------------------------------------------------------

// Real Time Clock
RTC_DS3231 rtc;                 // Real Time Clock Class
//DS3231_Alarm1 rtcAlarm1;        // RTC Alarm
volatile bool rtcFlag = false;  // Used for ISR to know when to take measurements

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// GPS TinyGPS++.h
TinyGPSPlus gps;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
char* position_filename = "GPSfile.csv";
File myfile;

// Create the new UART instance assigning it to pin 15,16
Uart gpsSerial(&sercom4, A2, A1, SERCOM_RX_PAD_1, UART_TX_PAD_0); //Pin A1 is I/O, Pin A2 is I/O

// Attach the interrupt handler to the SERCOM
void SERCOM4_Handler()
{
  gpsSerial.IrqHandler();
}

#define Serial SerialUSB //SAMD21 dev breakout only accepts SerialUSB for serial monitor

bool encode_GPS_time(int);
bool encode_GPS_position(int);

void wake() {}
bool get_position_points();
void standbySleep(void);

// Data array size for lat and long
double locations[10][2]; // (lat, lng)

void setup() {

  // Serial begins
  Serial.begin(9600);
  gpsSerial.begin(9600);
//  SD.begin();            // if the SD card is initialized, then data cannot be recieved on A2 pin
  while (!Serial);

  pinPeripheral(A1, PIO_SERCOM_ALT);  // TX
  pinPeripheral(A2, PIO_SERCOM_ALT);  // RX

  Serial.println("A"); //for debugging

  // Set pin modes
  pinMode(GPS_POWER1, OUTPUT);


  // Setup RTC
  if (!rtc.begin()) {
    Serial.println("RTC failed to start");
    while (1);
  }

  if (rtc.lostPower()) {
    //Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    //    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0)); // it has been observed that the time must be set to ANY TIME before time can be set to actual time
  }



  Serial.println("B"); // for debugging


  // Turning on GPS to get time
  digitalWrite(GPS_POWER1, HIGH);
  delay(250);


  if (encode_GPS_time(240000)) { // 4 min max wait to get GPS time, extremely unlikely that time data will not be received
    // print time
    DateTime now = rtc.now();

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
  } else {
    Serial.println("Could not get initial GPS data");
  }



  Serial.println("C"); // for debugging


  // Get 10 Position points

  get_position_points();
    // Turn off GPS
  digitalWrite(GPS_POWER1, LOW); //transistor or controllable voltage reg is attached to 3.3V pin and D4, the A2 pin is recieveing data from GPS module while SD card is attempting trying to communicate on this channel
//  
  //pinMode(A2, OUTPUT); 
  //digitalWrite(A2, HIGH);
//  SD.begin(); // tried here didnt work
  delay(100);
   
  if (!SD.begin(5)) {
    Serial.println("initialization failed!");
  } else {
    Serial.println("SD init worked");
  }
  
// Currently unknown why SD.begin has to be called twice. A few forums online have noted that in some applications, the default SD library can glitchy

  if (!SD.begin(5)) {

    Serial.println("initialization failed!");
  } else {
    Serial.println("SD init worked");
  }

  // Open SD file
  
  myfile = SD.open(position_filename, FILE_WRITE);

  if (!myfile) {
    Serial.print("Error opening: ");
    Serial.println(position_filename);
  } else {
    // Save Data to SD
    for (int i = 0; i < 10; i++) {
      Serial.print(locations[i][0], 6);
      Serial.print(",");
      Serial.println(locations[i][1], 6);
      myfile.print(locations[i][0], 6);
      myfile.print(",");
      myfile.println(locations[i][1], 6);
    }

    Serial.println("Done writing to SD");
  }
  myfile.close();



  Serial.println("D");



  Serial.println("E");


}


void loop() {

  // If incorrect time
  // Get time from GPS, set RTC
  if (rtc.lostPower() ) {
    Serial.println("F");

    // Turning on GPS to get time
    digitalWrite(GPS_POWER1, HIGH);

    encode_GPS_time(60000);
    digitalWrite(GPS_POWER1, LOW);

    Serial.println("G");

  }


  // Get data and RTC time
  DateTime time = rtc.now();
  DateTime wake_time  = time + TimeSpan(deltaT);

  // Read sensor data


  // Log sensor data with timestamp




  // Set alarm
  rtc.setAlarm(ALM1_MATCH_HOURS, wake_time.second(), wake_time.minute(), wake_time.hour(), 0);
  rtc.alarmInterrupt(1, true);

  // Sleep 10 seconds
  standbySleep();

  // Sleeping ...

  // Wake and continue
}



bool encode_GPS_time(int timeout) {
  Serial.println("encode_GPS_time");
  unsigned long startTime = millis();

  // Repeat
  while (1) {

    // If data available
    if (gpsSerial.available()) {

      Serial.println("gpsSerial.available");

      // While data available
      while (gpsSerial.available() > 0) {
        // GPS Encode(Read in data)
        gps.encode(gpsSerial.read());
      }

      Serial.println("Any data available");

      // Get time from GPS data when the time date and time has been updated
      if (gps.date.isUpdated() && gps.time.isUpdated()) {

        rtc.adjust(DateTime(
                     gps.date.year(),
                     gps.date.month(),
                     gps.date.day(),
                     gps.time.hour(),
                     gps.time.minute(),
                     gps.time.second()
                   ));

        Serial.println("Time and date Set");
        return true;
      } else {
        Serial.println("Not time data");
       
      }

    } // of if (.availalble)



    // If timeout duration reached
    if ((millis() - startTime) > timeout)
    {
      Serial.println("Timeout reached, exiting");
      return false;
    }
  }
}




bool encode_GPS_position(int timeout) {
  Serial.println("encode_GPS_position");
  unsigned long startTime = millis();

  // Repeat
  while (1) {

    // If data available
    if (gpsSerial.available()) {

      Serial.println("gpsSerial.available");

      // While data available
      while (gpsSerial.available() > 0) {
        // GPS Encode(Read in data)
        gps.encode(gpsSerial.read());
      }


      Serial.println("Any data available");

      // Get time from GPS data
      if (gps.location.isUpdated() ) {


        Serial.println("Location available and updated");
        return true;
      } else {
        Serial.println("Not position data");
      }

    } // of if (.availalble)



    // If timeout duration reached
    if ((millis() - startTime) > timeout)
    {
      Serial.println("Timeout reached, exiting");
      return false;
    }
  }
}


bool get_position_points()
{

  // Turn on GPS
  digitalWrite(GPS_POWER1, HIGH);


  // For 100 position points
  for (int i = 0; i < 10; i++) {

    // Try to encode GPS
    //    if ( encode_GPS(5000) && gps.location.isUpdated() ) {
    if ( encode_GPS_position(1000) ) {

      Serial.println(gps.location.lat());
      Serial.println(gps.location.lng());

      locations[i][0] = gps.location.lat();
      locations[i][1] = gps.location.lng();
      delay(1000);
    }
  }
  for (int i = 0; i < 10; i++) {         //print out the lists to see what the data looks like.
    Serial.print(locations[i][0], 6);
    Serial.print(",");
    Serial.println(locations[i][1], 6);
  }


  // Turn off GPS
  digitalWrite(GPS_POWER1, LOW);

  delay(50);
}



// Function to put SAMD21 into standby sleep
void standbySleep(void) {
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
  __WFI();
}
