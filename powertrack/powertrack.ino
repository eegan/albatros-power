/////////////////////////////////////////////////////////////////////////////////////
// Main ALBATROS power system .ino file
/////////////////////////////////////////////////////////////////////////////////////

// Library dependencies:
// SD library (built-in): tested against v. 1.2.2
// other built-in libraries (not mentioned)
// RTCLib by Adafruit: tested against v. 1.2.0

#include "powertrack.h"

// This is a measure to save power. It is not necessarily fully implemented, 
// and not debugged, so #ifdefed out for now
#define PACE_SLEEP_MAIN_LOOP 0

#if PACE_SLEEP_MAIN_LOOP
const long loopInterval = 200;           // main loop pace (interval between runs)
const long mainLoopSleepTimeMs = 100;    // time to sleep
long lastLoopBegin;
#endif

HardwareSerial &monitorPort = Serial1;

void setup()
{
  debugInit();
  monitorInit();  // do this first so we can print status messages  
  rtcInit();      // do this next so we know what time it is before we depend on this
  cfgInit();

  #if PACE_SLEEP_MAIN_LOOP
  lastLoopBegin = millis();
  #endif
  
  victronInit();
  // commented out since it re-inits the serial port also used for monitor; TODO implement a flag here
  loggerInit();
  monitorInit2(); // after RTC and logger init
  loadctlInit();
  statusInit();
}

void loop()
{
  #if PACE_SLEEP_MAIN_LOOP
  paceLoop();
  #endif
  
  victronLoopHandler();
  monitorLoopHandler();
  loadctlLoopHandler();
  loggerLoopHandler();
  statusLoopHandler();
}

#if PACE_SLEEP_MAIN_LOOP
void paceLoop()
{
  // Pace the main loop
  // TODO: implement sleep

  long sinceLastLoopBegin;
  
  long now;
  do {
    delay(mainLoopSleepTimeMs); // TODO: replace with power-down sleep, but maybe not - maybe just during non-reception of serial chars
    now = millis();
    sinceLastLoopBegin = now - lastLoopBegin;
  }
  while (sinceLastLoopBegin < loopInterval);
  
  lastLoopBegin = now;
}
#endif

////////////////////////////////////////////////////////////////////////////////////
// Assert support
// NOT REALLY USED AT THIS POINT
////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
void assertfail(char const *file, long line)
{
  char message[70];
  snprintf(message, sizeof message, "Assert fail line %d", line);
  statusLogPrint(message);
  statusLogPrint(file);
  monitorPort.println("Assert fail!");
}
#endif


////////////////////////////////////////////////////////////////////////////////////
// Debug support
// NOT REALLY USED AT THIS POINT
////////////////////////////////////////////////////////////////////////////////////

// Set to Serial to see it on the Arduino serial monitor, set it to Serial3 to probably not see it at all
HardwareSerial &debug = Serial;

void debugInit()
{
  // Does not need to be called, if it uses the monitor port that's already initialized
  debug.begin(9600);
}
