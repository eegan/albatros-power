#include <SD.h>
#include "RTClib.h"

// bitmap for which fields to log (20 fields)
const UINT32 LOG_FIELD_BITMAP = 0xfffff;

// for the data logging shield, we use digital pin 10 for the SD cs line
const int SD_pin = 10;

// define the RTC object
RTC_PCF8523 rtc;
UINT32 present;

UINT32 secondsWhenNewFile = 0;
const UINT32 secondsToNewFile = 86400; // NB also rows per file (ideally) (not including header)

UINT16 fileNumber = 0; // currently writing to which file
File logfile;

char filename[20];
char formatbuf[20]; // for converting logDataTypes to chars to save

extern logDataType parsed_data[];

void logger_init()
{
  // SD
  debug.print("initializing SD card...");
  pinMode(SD_pin, OUTPUT);
  if(!SD.begin(SD_pin)) {
    debug.println("failed.");
  }
  while(!SD.begin(SD_pin)){}
  debug.println("success.");

  sprintf(filename, "log%05i.csv", fileNumber); // ensures no confusion takes place if we ever reinit
                                                 // at least until we move on from this primitive naming system
  // RTC
  if (!rtc.begin()) {
    debug.println("Couldn't find RTC!");
  }
  if (!rtc.initialized()) {
    debug.println("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  
  }
}

void logger_save() // ALWAYS operates on the same array of logDataTypes
{  
  present = rtc.now().unixtime();
  
  // create new file if needed
  if (present - secondsWhenNewFile >= secondsToNewFile) {
    secondsWhenNewFile = present;
    sprintf(filename, "log%05i.csv", fileNumber);
    //create a header
    logfile = SD.open(filename, FILE_WRITE); // creates on not found, appends
    logfile.print("UNIX time");
    for (uint8_t i = 0; i <= COUNT_OF(fieldDescriptors); i++) {
      if ((1 << i) & LOG_FIELD_BITMAP) {
        logfile.print(",");
        logfile.print(fieldDescriptors[i].tag);
      }
    }
    logfile.println();
    fileNumber += 1;
  } else {
    logfile = SD.open(filename, FILE_WRITE); // creates on not found, appends
  }
  debug.print("saving to "); debug.print(filename); debug.print("...");
  
  // save the data as char arrays
  ltoa(present, formatbuf, 10);
  logfile.print(formatbuf);
  for (uint8_t i = 0; i <= COUNT_OF(fieldDescriptors); i++) {
    if ((1 << i) & LOG_FIELD_BITMAP) {
      ltoa(parsed_data[i], formatbuf, 10);
      logfile.print(",");
      logfile.print(formatbuf);
    }
  }
  logfile.println();
  logfile.close();
  debug.println("done.");
}
