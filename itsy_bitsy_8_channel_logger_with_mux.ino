//<debuuging defines>
#define _DEBUG_ // for serial debugging
//#undef    _DEBUG_
//</debuging defines>
#define _TIMESTAMP_PER_MINUTE_
#undef _TIMESTAMP_PER_MINUTE_
#define _TIMESTAMP_PER_POWERUP_
#undef _TIMESTAMP_PER_POWERUP_

//<pin defines>

#define HEAT_1 5          // digital input
#define HEAT_2 6          // digital input
#define HEAT_3 7          // digital input
#define HEAT_4 8          // digital input
#define DEBUG_HEARTBEAT 9 // tracking millis() accuracy

//</pin defines>

#define B1OXYGEN A0
#define B2OXYGEN A1
#define B3OXYGEN A2
#define B4OXYGEN A3
#define TPOS A4
#define REF_5V_2 A5
#define REF_5V_3 A6
#define REF_5V_4 A7

#define VG_TWO  2560
#define VG_HALF 5000

#include <Wire.h>
#include <stdio.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"

RTC_PCF8523 rtc;
//char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

unsigned long startmillis = 0;
unsigned long currentmillis = 0;

int adcaverage(int channel, int scale);

//  used to timestamp output to file every minute
#ifdef _TIMESTAMP_PER_MINUTE_
unsigned char loopcount = 0;
#endif

//  cleared first trip through the loop
//  used to time stamp startup.
unsigned char powerup = 1;

/*
long mymap(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min + 1) / (in_max - in_min + 1) + out_min;
}
*/
void setup()
{
  // put your setup code here, to run once:
  //
  // first set adc reference to external.
  analogReference(INTERNAL); // 2.56 Volt bandgap reference used
  // disable input buffers on ADC pins,
  // per datasheet page 43
  DIDR0 |= _BV(ADC0D) | _BV(ADC1D) | _BV(ADC2D) | _BV(ADC3D) |
           _BV(ADC4D) | _BV(ADC5D) | _BV(ADC6D) | _BV(ADC7D);

  // pins unused
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);

  // pins in use>
  pinMode(DEBUG_HEARTBEAT, OUTPUT);
  pinMode(HEAT_1, INPUT);
  pinMode(HEAT_2, INPUT);
  pinMode(HEAT_3, INPUT);
  pinMode(HEAT_4, INPUT);

  // real time clock check
  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");

    while (1) // no clock, die here.
    ;
  }

//delay(20); // a short delay to let things stabilize
#ifdef _DEBUG_

  Serial.begin(57600);
  Serial.print("Initializing SD card...");
#endif
  // see if the card is present and can be initialized:
  if (!SD.begin(12))
  {
#ifdef _DEBUG_
    Serial.println("Card failed, or not present");
// don't do anything more:
#endif

     while (1)
    ; // hang till power down and card inserted
  }
#ifdef _DEBUG_
  Serial.println("card initialized.");
#endif
  String StartString = "";
  StartString += "Startup";

  startmillis = millis();
}

void loop()
{
  // put your main code here, to run repeatedly:

  int ref_5v_2, tpos, b1oxygen, b2oxygen, b3oxygen, b4oxygen, temp;

  String dataString = "";
  currentmillis = millis();
#ifdef _TIMESTAMP_PER_MINUTE_

  DateTime now = rtc.now();
  dataString += now.unixtime();
  dataString += ",";

#endif
#ifdef _TIMESTAMP_PER_POWERUP_
  powerup = 0;
  DateTime now = rtc.now();
  dataString += now.unixtime();
  dataString += ",";

#endif

  dataString += String(millis() - startmillis);
  dataString += String(",");

  // <get us some heater info>
  dataString += String(digitalRead(HEAT_1));
  dataString += String(",");
  dataString += String(digitalRead(HEAT_2));
  dataString += String(",");
  dataString += String(digitalRead(HEAT_3));
  dataString += String(",");
  dataString += String(digitalRead(HEAT_4));
  dataString += String(",");

  // </get us some heater info>

  //<get us some o2 info>
  // get 4 samples and then average them
  dataString += String(adcaverage(B1OXYGEN, VG_TWO));
  dataString += String(",");
  //</get us some o2 info>

  //<get us some o2 info>
  // get 4 samples and then average them
  dataString += String(adcaverage(B2OXYGEN, VG_TWO));
  dataString += String(",");
  //</get us some o2 info>

  //<get us some o2 info>
  // get 4 samples and then average them
  dataString += String(adcaverage(B3OXYGEN, VG_TWO));
  dataString += String(",");
  //</get us some o2 info>

  //<get us some o2 info (gain of 2(4))>
  // get 4 samples and then average them
  dataString += String(adcaverage(B4OXYGEN, VG_TWO));
  dataString += String(",");
  //</get us some o2 info (gain of 2(4))>

  // <get us some throttle info>
  // get 4 samples and then average them
  dataString += String(adcaverage(TPOS, VG_HALF));
  dataString += String(",");
  // </get us some throttle info>

  // <get us some REF_5V_2 channel>
  // get 4 samples and then average them
  dataString += String(adcaverage(REF_5V_2, VG_HALF));
  dataString += String(",");
  // </get us some unused channel>

  // <get us some REF_5V_3 channel>
  // get 4 samples and then average them
  dataString += String(adcaverage(REF_5V_3, VG_HALF));
  dataString += String(",");
  // </get us some unused channel)>

  // <get us some REF_5V_4 channel>
  // get 4 samples and then average them
  dataString += String(adcaverage(REF_5V_4, VG_HALF)); // last channel no comma appended
  // </get us some unused channel>

  // <SD card setup>

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.csv", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile)
  {
    dataFile.println(dataString);
    dataFile.close();
#ifdef _DEBUG_
    // print to the serial port too:
    Serial.println(dataString);
#endif
  }
  // if the file isn't open, pop up an error:
  else
  {
#ifdef _DEBUG_
    Serial.println("error opening datalog.csv");
#endif
  }

  while (millis() - currentmillis < 100)
    ; // do every 100 millis aka 10 sample / sec.
#ifdef _TIMESTAMP_PER_MINUTE_
  if (loopcount <= 600)
  {
    loopcount++;
  }
  else
  {
    loopcount = 0; //reset each minute
                   // see beginning of loop for the usage of this}
  }
#endif
  digitalWrite(DEBUG_HEARTBEAT, !digitalRead(DEBUG_HEARTBEAT));
}

int adcaverage(int channel, int scale)
{
  int temp = 0;
  temp += analogRead(channel);
  temp += analogRead(channel);
  temp += analogRead(channel);
  temp += analogRead(channel);
  temp /= 4;
  return map(temp, 0, 1023, 0, scale);
}