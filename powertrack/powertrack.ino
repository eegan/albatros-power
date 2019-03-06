#include "mytypes.h"

const unsigned long loopInterval = 200;           // main loop pace (interval between runs)
const unsigned long mainLoopSleepTimeMs = 100;    // time to sleep
unsigned long lastLoopBegin;

void setup()
{
  lastLoopBegin = millis();
  victron_serviceDatastreamInit();
  debugInit();
  logger_init();
}

void loop()
{
  // Pace the main loop
  // TODO: implement sleep
  #if 0
  long sinceLastLoopBegin;
  
  unsigned long now;
  do {
    delay(mainLoopSleepTimeMs); // TODO: replace with power-down sleep, but maybe not - maybe just during non-reception of serial chars
    now = millis();
    sinceLastLoopBegin = now - lastLoopBegin;
  }
  while (sinceLastLoopBegin < loopInterval);
  
  lastLoopBegin = now;
  #endif
  victron_serviceDatastream();
  
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
HardwareSerial &debug = Serial;
void debugInit()
{
  debug.begin(9600);
}
