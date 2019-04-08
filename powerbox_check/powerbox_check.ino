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
 * -> talks to both Serial (the USB serial) and Serial1 (the stereo jack)
 * -> prints "connected." to both com. ports at the beginning so you know it's alive
 * -> victron communication is through Serial2 at 19200 baud
 * -> this program will work through charaters in a line, ie "rg" will toggle the
 *    red then the green led
 * -> If you are using the stereo jack serial, it is suggested to plug that in
 *    before powering up the Arduino, so you don't miss anything
 */

#include "RTClib.h"
#include <SD.h>

// hardware settings

int pinRed = 7; // red led pin
int pinGrn = 6; // green led pin
int pinSSR = 5; // solid state relay (load toggle) pin 

RTC_PCF8523 rtc;
DateTime present;

#define pinSD 10 // I don't really know; makes the SD work

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

void talk(char* message) { // helps me talk to two serials at once
  Serial.print(message);
  Serial1.print(message);
}

void talkln(char* message) { // sometimes I like a \n too
  Serial.println(message);
  Serial1.println(message);
}

int togglePin(int counter, int pin) { // toggle the state of a digital pin
  counter++;
  if (counter%2) {
    digitalWrite(pin, HIGH);
  } else {
    digitalWrite(pin, LOW);
  }
  return counter;
}

void sayState(int counter) { // display state of a digital pin (or really whether its counter is even)
  if (counter%2) {
    talkln("on");
  } else {
    talkln("off");
  }
}

void execute(char command) { // the meat of the program, take a char and do corresponding thing, if any
  switch(command) {
    case 'l':
      togglesSSR = togglePin(togglesSSR, pinSSR);
      talk("solid state relay toggled ");
      sayState(togglesSSR);
      break;
    case 'g':
      togglesGrn = togglePin(togglesGrn, pinGrn);
      talk("green LED toggled ");
      sayState(togglesGrn);  
      break;    
    case 'r':
      togglesRed = togglePin(togglesRed, pinRed);
      talk("red LED toggled ");
      sayState(togglesRed);
      break;
    case 'v':
      victrON = !victrON;
      talk("display victron data stream: ");
      if (victrON) {
        talkln("true");
      } else {
        talkln("false");
      }
      break;
    case 't':
      present = rtc.now();
      talk("UNIX time: ");
      Serial.println(present.unixtime()); 
      Serial1.println(present.unixtime()); // talk() only likes char*, not long 
      break;   
    case 's':
      datafile = SD.open(filename, FILE_WRITE);
      if (datafile) {
        datafile.println(SDstring);
        datafile.close();
        talkln("successfully written. go check!");
      } else {
        talk("ERR: couldn't open file on SD");
      }
      break;
    case '\n': // so it doesn't complain when you hit enter
      break; 
    default:
      talkln("command not recognised.");
  }
}

void setup() {
  // init serial
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(19200);
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
    while (Serial2.available()) { 
      serialbuf = Serial2.read();
      Serial.print(serialbuf);
      Serial1.print(serialbuf);
    }
  }

  // await further commands
  while (Serial.available()) {
    command = Serial.read();
    execute(command);
  }

  while (Serial1.available()) {
    command = Serial1.read();
    execute(command);
  }
}
