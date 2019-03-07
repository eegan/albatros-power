#include <SD.h>
#include "RTClib.h"
#include "mytypes.h"

extern logDataType parsedData[];
const UINT16 NDATA = COUNT_OF(fieldDescriptors);

logDataType avgdata[NDATA], mindata[NDATA], maxdata[NDATA];
// Questions
// where do we decide if we are logging a given field?

// bitmap for which fields to log (20 fields)
const UINT32 LOG_FIELD_BITMAP = 0xfffff;

// for the data logging shield, we use digital pin 10 for the SD cs line
// TODO: put into hardware definition includefile
const int SD_CS_pin = 10;

// define the RTC object
RTC_PCF8523 rtc;
UINT32 present;

UINT32 secondsWhenNewFile = 0;
const UINT32 secondsToNewFile = 86400; // NB also rows per file (ideally) (not including header)

UINT16 fileNumber = 0; // currently writing to which file
File logfile;

char filename[13];
char formatbuf[20]; // for converting logDataTypes to chars to save

extern logDataType parsed[];

void logger_init()
{
  // SD
  debug.print("initializing SD card...");
  pinMode(SD_CS_pin, OUTPUT);
  if(!SD.begin(SD_CS_pin)) {
    debug.println("failed.");
  }
  while(!SD.begin(SD_pin)){}
  debug.println("success.");

  sprintf(filename, "log%05i.csv", fileNumber); // ensures no overwiting takes place
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

<<<<<<< HEAD
void logger_save() // ALWAYS operates on the same array of logDataTypes
{  
=======
void logger_reset()
{
  vd_V = vd_I = vd_VPV = vd_PPV = 0;
  nSamples = 0;
}

char filename[20];  // should only be 13
char formatbuffer[20]; // for whenever we need to format into somewhere

#define bitmask(x) (1 << x)

// TODO
// Fix this to only log what we want - get rid of strings, or figure out how to handle them
const UINT32 LOG_FIELD_BITMAP = 0xffffffff;

/*
= bitmask (FI_V)
| bitmask (FI_whatever)
| etc ...
*/

// field is treated as quantity for avg/min/max
bool isQuantity(int index) {
  return fieldDescriptors[index].type == FT_int;
}

void logger_save()
{
  // making the data into a char array to save
  // (Eamon, feel free to make this part much better)

  // check if file name needs to change
  UINT32 present;
>>>>>>> 3e66e2cb8e7bd706a434cf0df8c5144d58725b71
  present = rtc.now().unixtime();
  
  // create new file if needed
  if (present - secondsWhenNewFile >= secondsToNewFile) {
    secondsWhenNewFile = present;
<<<<<<< HEAD
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
      ltoa(parsed[i], formatbuf, 10);
      logfile.print(",");
      logfile.print(formatbuf);
    }
  }
  logfile.println();
  logfile.close();
  debug.println("done.");
}
=======
    sprintf(filename, "log%05ld.csv", fileNumber);

    // create header
    logfile = SD.open(filename, FILE_WRITE); //creates on not found, appends
    logfile.print("timestamp");
    for (uint8_t i = 0; i <= COUNT_OF(fieldDescriptors); i++) {
      if ((1 << i) & LOG_FIELD_BITMAP) {
        if (isQuantity(i)) {
          logfile.print(", "); 
          logfile.print(fieldDescriptors[i].tag);
          logfile.print("_avg");

          logfile.print(", "); 
          logfile.print(fieldDescriptors[i].tag);
          logfile.print("_min");
                    
          logfile.print(", "); 
          logfile.print(fieldDescriptors[i].tag);                            
          logfile.print("_max");
        }
        else {
          logfile.print(", "); 
          logfile.print(fieldDescriptors[i].tag);
        }
      }
      logfile.println();
    }
    logfile.close();    
  }
  
  debug.print("saving to ");
  debug.print(filename);
  debug.print("...");
  
  logfile = SD.open(filename, FILE_WRITE); //creates on not found, appends

  // TODO: print timestamp in record
  
  for (uint8_t i = 0; i <= COUNT_OF(fieldDescriptors); i++) {

    if ((1 << i) & LOG_FIELD_BITMAP) {
        if (isQuantity(i)) {
          logfile.print(", "); 
          ltoa(avgdata[i], formatbuffer, 10);
          logfile.print(formatbuffer);      

          logfile.print(", "); 
          ltoa(mindata[i], formatbuffer, 10);
          logfile.print(formatbuffer);      
                    
          logfile.print(", "); 
          ltoa(maxdata[i], formatbuffer, 10);
          logfile.print(formatbuffer);      
        }
        else {
          logfile.print(", "); 
          logfile.print(fieldDescriptors[i].tag);
        }
    }

  }

  logfile.close();  
  debug.println("done.");
}

// TODO
// Get working simply setting avg, min and max to the data, with 1 sample accumulated
// Then put in code to do avg, min, max, as well as bitmask creation for other things

void logger_accumulateSample(logDataType* data) 
{
  nSamples++;
  logDataType value;
  for (int i = 0; i < COUNT_OF(fieldDescriptors); i++) {
    value = data[i];
    //...
  }
  if (nSamples >= samplesToAverage) {
    logger_save(); //NB only "data" since I never actually average anything
  }
}
>>>>>>> 3e66e2cb8e7bd706a434cf0df8c5144d58725b71
