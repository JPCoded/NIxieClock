/* Arduino-free nixie clock, using ESP323, RTC and Shift Registers
 *
 * For now clock has 1 button to get updated time from NTP
 * A DS2321 real-time clock is also included in this implementation, allowing for the clock to keep more accurate time and retain it.
 *
 * The clock can also output data via serial
 *
 * Nixie10 headers and original code outtline by Chris Green - 1/11/2016
 * Modified heavily by John Pomeroy
 *
 */

#include <nixie10.h>  // nixie driver library
#include <Wire.h>     // I2C library
#include "RTClib.h"   // adafruit library to drive DS1307 RTC connected via I2C
#include <time.h>     // Time library to get struct for time
#include <ArduinoJson.h> // Json header
#include "LittleFS.h" // LittleFS header

// WiFi headers
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Register Pins
#define LATCHPIN 33 // latch pin or RCK
#define CLOCKPIN 26 // clock pin or SCK
#define DATAPIN 35 // data line or SER

// Button Pins
// #define HOURBUTTONPIN 5
// #define MINUTEBUTTONPIN 6
#define WIFIBUTTON 32

#define LEDPIN 13
#define WIFILED 14
#define NTPLED 12

// RTC pins
// SQW PIN 4
// SCL PIN 22
// SDA PIN 21

// I like this amount of delay for setting my clock, but change it as you like
#define CLOCKSETDELAY 400
// setting a tube to a value higher than 9 turns it off
#define OFF 11
struct tm timeinfo;
// NPT server info
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -21600;
const int daylightOffset_sec = 3600;

int hour_tens, hour_ones, min_tens, min_ones, hours, minutes;  // some global variables to hold current time

nixie10 outReg;  // tube-register object
RTC_DS3231 rtc;  // real-time-clock object

// Replace with your network credentials
// const char* ssid = "";
// const char* password = "";

const char* jsonFilePath = "/config.json";
const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(1) + 60;  // Adjust the capacity based on your JSON
DynamicJsonDocument jsonDoc(capacity);

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


void readJsonFile(const char* path, DynamicJsonDocument& jsonDoc) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  DeserializationError error = deserializeJson(jsonDoc, file);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    file.close();
    return;
  }

  file.close();
}

void printLocalTime() {
  
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ";
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

  Serial.println("Time variables:");
  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  Serial.println(timeHour);
  char timeWeekDay[10];
  strftime(timeWeekDay, 10, "%A", &timeinfo);
  Serial.println(timeWeekDay);
  Serial.println();
}

void setup() {
  
  pinMode(LEDPIN, OUTPUT);
  outReg.initialize_16reg(LATCHPIN, CLOCKPIN, DATAPIN);  // initialize the output register array

  Serial.begin(115200);
  Serial.println("Setup Started");

  Serial.println("Begin LittleFS");

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }
  Serial.println("Read Json File");
  readJsonFile(jsonFilePath, jsonDoc);

  const char* ssidJ = jsonDoc["ssid"];
  const char* passwordJ = jsonDoc["password"];
  Serial.print("JSON SSID: ");
  Serial.print(ssidJ);
  Serial.println("");
  Serial.print("JSON Password: ");
  Serial.print(passwordJ);
  println("");
  // Connecting to WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, passwordJ);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  // Initialize a NTPClient to get time
  //  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  //  timeClient.setTimeOffset(-21600);

  // Setting up RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }

  // set clock if RTC is not set
  if (rtc.lostPower()) {
    Serial.println("Power Loss or First Bootup; Setting a default time");
    //  following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}

void loop() {

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(WIFILED, HIGH);
  } else {
    digitalWrite(WIFILED, LOW);
  }


  char Hours[3];
  char Minutes[3];
  DateTime now = rtc.now();                // take a "snapshot" of the time
  hours = (now.hour() ? now.hour() : 12);  // account for the fact that in 24hr time, this is zero for 12AM
  minutes = now.minute();

  //  while(!timeClient.update()) {
  //  timeClient.forceUpdate();
  // }


  //Comment out after buttons installed
  digitalWrite(NTPLED, HIGH);
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  strftime(Hours, 3, "%H", &timeinfo);
  strftime(Minutes, 3, "%I", &timeinfo);

  // Convert minutes and hours we got from NTP to int
  minutes = atoi(Minutes);
  hours = atoi(Hours);
  rtc.adjust(DateTime(now.year(), now.month(), now.day(), hours, minutes, now.second()));


  /* uncomment after buttons installed
  if (digitalRead(WIFIBUTTON))
  {
    digitalWrite(NTPLED, high);
    if (!getLocalTime(&timeinfo))
    {
      Serial.println("Failed to obtain time");
      return;
    }

    strftime(Hours, 3, "%H", &timeinfo);
    strftime(Minutes, 3, "%I", &timeinfo)
    
    // Convert minutes and hours we got from NTP to int
    minutes = atoi(Minutes);
    hours = atoi(Hours);
    rtc.adjust(DateTime(now.year(), now.month(), now.day(), hours, minutes, now.second()));
  }
  else
  {
     DateTime now = rtc.now();                // take a "snapshot" of the time
     hours = (now.hour() ? now.hour() : 12);  // account for the fact that in 24hr time, this is zero for 12AM
     minutes = now.minute();
     digitalWrite(NTPLED, low);
  }
*/
  int time = millis();
  if (!(time % 1000)) {
    digitalWrite(LEDPIN, HIGH);
    delay(200);
    digitalWrite(LEDPIN, LOW);
  }

  // Convert minutes and hours we got from NTP to int
  
 // minutes = atoi(Minutes);
  Serial.println(minutes);
  //hours = atoi(Hours);
 // Serial.println(hours);
  // break up current time into individual digits and convert to 12hr format
  min_ones = minutes % 10;
  min_tens = (minutes - min_ones) / 10;
  hour_ones = (hours > 12) ? (hours - 12) % 10 : hours % 10;                            // convert to 12hr format
  hour_tens = (hours > 12) ? (hours - hour_ones - 12) / 10 : (hours - hour_ones) / 10;  // convert to 12hr format
  /*
  Serial.print("Current Time: ");
  Serial.print(hour_tens);
  Serial.print(hour_ones);
  Serial.print(":");
  Serial.print(min_tens);
  Serial.print(min_ones);
  Serial.println("");
  */
  outReg.set_16reg((hour_tens ? hour_tens : OFF), hour_ones, min_tens, min_ones);       // push out the current time to the register array, turning off 10s hour tube if zero.
  delay(1000);
}
