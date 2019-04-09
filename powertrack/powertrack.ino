// Library dependencies:
// SD library (built-in): tested against v. 1.2.2
// other built-in libraries (not mentioned)
// RTCLib by Adafruit: tested against v. 1.2.0

#include "mytypes.h"

const long loopInterval = 200;           // main loop pace (interval between runs)
const long mainLoopSleepTimeMs = 100;    // time to sleep
long lastLoopBegin;

HardwareSerial &monitorPort = Serial;

void setup()
{
  monitorInit();  // do this first so we can print status messages  
  rtcInit();      // do this next so we know what time it is before we depend on this
  monitorInit2(); // after RTC init
  cfgInit();
  lastLoopBegin = millis();
  victronInit();
  // commented out since it re-inits the serial port also used for monitor; TODO implement a flag here
  //debugInit();
  loggerInit();
  loadctlInit();
}

void loop()
{
  paceLoop();
  
  victronLoopHandler();
  monitorLoopHandler();
  loadctlLoopHandler();
  loggerLoopHandler();
}

void paceLoop()
{
  // Pace the main loop
  // TODO: implement sleep
  #if 0
  long sinceLastLoopBegin;
  
  long now;
  do {
    delay(mainLoopSleepTimeMs); // TODO: replace with power-down sleep, but maybe not - maybe just during non-reception of serial chars
    now = millis();
    sinceLastLoopBegin = now - lastLoopBegin;
  }
  while (sinceLastLoopBegin < loopInterval);
  
  lastLoopBegin = now;
  #endif
}

////////////////////////////////////////////////////////////////////////////////////
// Assert support
////////////////////////////////////////////////////////////////////////////////////

#define DEBUG 1

#if DEBUG
#define ASSERT(x) {if (0==(x)) assertfail(__FILE__, __LINE__);}
#else
#define ASSERT(x)
#endif

void assertfail(char *file, long line)
{
  // TODO: something
  // like print a message on the debug port
}
////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////
// Debug support
////////////////////////////////////////////////////////////////////////////////////
// Set to Serial to see it on the Arduino serial monitor, set it to Serial3 to probably not see it at allgg

HardwareSerial &debug = Serial;
void debugInit()
{
  debug.begin(9600);
}
