#include "mytypes.h"

const unsigned long loopInterval = 200;           // main loop pace (interval between runs)
const unsigned long mainLoopSleepTimeMs = 100;    // time to sleep
unsigned long lastLoopBegin;

typedef unsigned long UINT32;
typedef unsigned short UINT16;

HardwareSerial &monitorPort = Serial;

void setup()
{
  cfgman_loadConfig();
  lastLoopBegin = millis();
  serviceVictronDatastreamInit();
  debugInit();
  logger_init();
  monitor_init();
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
  serviceDatastream();
  if (monitorPort.available()) {
    monitor_handle();
  }
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
