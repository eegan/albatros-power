/*
 * Necessary libraries: (from Arduino IDE library manager)
 * -> RTClib by Adafruit
 *  
 * Commands:
 * -> l | toggle load switch
 * -> g | toggle green led
 * -> r | toggle red led
 * -> v | toggle victron data stream
 * -> t | display current time (according to the RTC)
 * -> s | append the line "this,is,data" to REALDATA.CSV on the SD card
 * 
 * Notes:
 * -> talks to both Serial (the USB serial) and Serial2 (the stereo jack)
 * -> prints "connected." to both com. ports at the beginning so you know it's alive
 * -> victron communication is through Serial1 at 19200 baud
 * -> this program will work through charaters in a line, ie "gg" will toggle the
 *    green led twice
 */

#include <SoftwareSerial.h>
#include "RTClib.h"
#include <SD.h>

// hardware settings

// these are ints so I can pass them to togglePin()
int pinRed = 7;
int pinGrn = 6;
int pinSSR = 5;

RTC_PCF8523 rtc;
DateTime present;

// this doesn't need to be passed anywhere
#define pinSD 10

//program-specific variables
char command;
bool victrON = false; // is the victron on?
char serialbuf;
char* SDstring = "this,is,data";
char* filename = "REALDATA.CSV";
File datafile;
int togglesRed = 0;
int togglesGrn = 0;
int togglesSSR = 0;

void talk(char* message) {
  Serial.print(message);
  Serial2.print(message);
}

void talkln(char* message) {
  Serial.println(message);
  Serial2.println(message);
}

int togglePin(int counter, int pin) {
  counter++;
  if (counter%2) {
    digitalWrite(pin, LOW);
  } else {
    digitalWrite(pin, HIGH);
  }
  return counter;
}

void execute(char command) {
  switch(command) {
    case 'l':
      togglesSSR = togglePin(togglesSSR, pinSSR);
      talk("solid state relay toggled ");
      talk(togglesSSR);
      talkln(" times.");
    case 'g':
      togglesGrn = togglePin(togglesGrn, pinGrn);
      talk("green LED toggled ");
      talk(togglesGrn);
      talkln(" times.");      
    case 'r':
      togglesRed = togglePin(togglesRed, pinRed);
      talk("red LED toggled ");
      talk(togglesRed);
      talkln(" times.");
    case 'v':
      victrON = !victrON;
      talk("display victron data stream: ");
      talkln(victrON);
    case 't':
      present = rtc.now();
      talk("UNIX time: ");
      talkln(present.unixtime());     
    case 's':
      datafile = SD.open(filename, FILE_WRITE);
      if (datafile) {
        datafile.println(SDstring);
        datafile.close();
        talkln("successfully written. go check!");
      } else {
        talk("error opening file on SD");
      }
    default:
      talkln("command not recognised.");
  }
}

void setup() {
  // init serial
  Serial.begin(9600);
  Serial2.begin(9600);
  Serial1.begin(19200);
  talkln("connected.");

  // init digital pins
  pinMode(pinRed, OUTPUT);
  pinMode(pinGrn, OUTPUT);
  pinMode(pinSSR, OUTPUT);

  // init RTC
  talkln("initializing RTC...");
  if (!rtc.begin()) {
    talkln("ERR: couldn't find RTC!");
  }
  if (!rtc.initialized()) {
    talkln("ERR: RTC is NOT running!");
    // set time from computer when it uploads
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // to set to explicit date and time, for example
    // January 21, 2014 at 3:00:00am, you would call
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // init SD
  talkln("initializing SD...");
  if (!SD.begin(pinSD)) {
    talkln("ERR: failed, or no card.");
  }
  
  talkln("initialization complete.");
}

void loop() {

  // if we want to, and it exists, regurgitate victron data
  if (victrON) {
    while (Serial1.available()) { 
      serialbuf = Serial1.read();
      talk(serialbuf);
    }
  }

  while (Serial.available()) {
    command = Serial.read();
    execute(command);
  }

  while (Serial2.available()) {
    command = Serial2.read();
    execute(command);
  }
}
