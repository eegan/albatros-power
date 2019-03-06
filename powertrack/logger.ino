#include <SD.h>
#include "RTClib.h"

// for the data logging shield, we use digital pin 10 for the SD cs line
const int SD_pin = 10;

// define the RTC object
RTC_PCF8523 rtc;

logDataType vd_V, vd_I, vd_VPV, vd_PPV;
UINT16 nSamples;
UINT32 secondsWhenNewFile = 0;

const UINT16 samplesToAverage = 1;  
const UINT32 secondsToNewFile = 86400; 

UINT16 fileNumber = 0; // currently writing to which file
File logfile;

void logger_init()
{
  // SD
  debug.print("initializing SD card...");
  pinMode(SD_pin, OUTPUT);
  if(!SD.begin(SD_pin)) {
    debug.println("failed.");
  }
  debug.println("success.");

  // RTC
  if (! rtc.begin()) {
    debug.println("Couldn't find RTC!");
  }
  if (!rtc.initialized()) {
    debug.println("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  
  }
}

void logger_reset()
{
  vd_V = vd_I = vd_VPV = vd_PPV = 0;
  nSamples = 0;
}

void logger_save(logDataType* data)
{
  // making the data into a char array to save
  // (Eamon, feel free to make this part much better)
  char data_as_chars[21][11]; // 20 fields, 1 extra justin case; max value 4294967295 (10 places) and then a null at the end
  char save_string[500] = ""; // too big? (approx 25 chars per field)
  
  for (uint8_t i = 0; i <= COUNT_OF(fieldDescriptors); i++) {
     itoa(data[i], data_as_chars[i], 10); // convert to char array element-wise
  }
  debug.println(data_as_chars[1]);
  for (int i = 0; i <= 20; i++) { //HARDCODED field numbers. COUNT_OF(fieldDescriptors) failed here for some reason...
      sprintf(save_string, "%s,%s", save_string, data_as_chars[i]); //concatenate with commas in between
  }
  
  // the actual saving...
  UINT32 present;
  present = rtc.now().unixtime();
  if (present - secondsWhenNewFile >= secondsToNewFile) {
    fileNumber += 1;
    secondsWhenNewFile = present;
  }
  char filename[] = "log00000.csv";
  for (uint8_t i = 0; i <= 5; i++) {
    if (i == 0) {
      filename[8-i] = fileNumber/100000;
    } else {       
      filename[8-i] = fileNumber%(100000/(i*10));
    }
  }

  debug.print("saving to ");
  debug.print(filename);
  debug.print("...");
  
  logfile = SD.open(filename, FILE_WRITE); //creates on not found, appends
  logfile.println(save_string);
  logfile.flush();
  logfile.close();
  
  debug.println("done.");
}

void logger_accumulateSample(logDataType* data) 
{
  nSamples += 1;
  logDataType value;
  for (int i = 0; i < COUNT_OF(fieldDescriptors); i++) {
    value = data[i];
    //...
  }
  if (nSamples >= samplesToAverage) {
    logger_save(data); //NB only "data" since I never actually average anything
  }
}
