// Tested against SD Card library (built-in) version 1.2.2
#include "powertrack.h"
#include <SD.h>

#define SD_CSpin 10 // Chip-select pin for SD

int sampleCount = 0;
long timeSinceLastLogEntry;

/////////////////////////////////////////////////////////////////////////////////////////////////
// main routine-called functions
/////////////////////////////////////////////////////////////////////////////////////////////////

void loggerInit()
{
  SD.begin(SD_CSpin);
  loggerZeroVariables();
  timeSinceLastLogEntry = rtcGetTime();
}

void loggerLoopHandler()
{
  long now = rtcGetTime();
  if (now - timeSinceLastLogEntry > cfg_fieldValue(ndx_secsPerLog)) {
    loggerLogEntry();
    timeSinceLastLogEntry = now;
    monitorPort.println("Logging");
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// SD-card utility functions
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
// loggerRootDir
// list directory of SD root, to port p
/////////////////////////////////////////////////////////////////////////////////////////////////
void loggerRootDir (HardwareSerial &p)
{
  File dir = SD.open("/");
  while (true)
  {
    File entry = dir.openNextFile();
    if (! entry)
    {
      p.println("** Done **");
      break;
    }
    p.print(entry.name());
    if (entry.isDirectory())
    {
      p.println("/ (DIR!)");
    }
    else
    {
      p.print("\t\t");
      p.println(entry.size(), DEC);
    }
    entry.close();
  }
  dir.close();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// loggerDumpFile
// dump contents of specified file, to port p
/////////////////////////////////////////////////////////////////////////////////////////////////
void loggerDumpFile(HardwareSerial &p, char *filename)
{
  char buf[20];
  char c;
  File f = SD.open(filename);
  if (!f) {
    p.println("File not found");
    return;
  }

  // TODO: probably don't need the first while
  while(int n = f.available()) {
    while( EOF != (c = f.read()) )  // until EOF
      while (0 == p.write(c));      // keep trying until the character is written (pacing)
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////
// loggerEraseFile
// erase specified file
/////////////////////////////////////////////////////////////////////////////////////////////////
void loggerEraseFile (HardwareSerial &p, char *filename)
{
  SD.remove(filename);
}

enum logVarType {
  lvtNumeric      // numeric value, will accumulate averaged/min/max
 ,lvtEnumSample   // enum value, will sample current value
 ,lvtEnumAccum    // enum value, will accumulate bitmap of values seen in log entry interval
};

struct {
  char *name;       // name for CSV header
  int sourceIndex;  // index in Victron data array
  logVarType type;  // type
  long sum;         // accumulated sum or bitmap
  long min;         // accumulated min
  long max;         // accumulated max
} logVariables[] = 

{
    {"BatVolt", FI_V,   lvtNumeric}  
  ,{"PVVolt",   FI_VPV, lvtNumeric}  
  ,{"PVPwr",    FI_PPV, lvtNumeric}
  ,{"BatCur",   FI_I,   lvtNumeric}
  ,{"Error",    FI_ERR, lvtEnumSample}
  ,{"State",    FI_CS,  lvtEnumAccum}
  ,{"MPPT",     FI_CS,  lvtEnumAccum}
    
};


// receive notification of a new Victron sample
void loggerNotifyVictronSample()
{
  for (int i=0; i< COUNT_OF(logVariables); i++) {
    long value = victronGetFieldValue(logVariables[i].sourceIndex);    
    switch(logVariables[i].type) {
      case lvtNumeric:
        logVariables[i].sum += value;
        logVariables[i].min = min(logVariables[i].min, value);
        logVariables[i].max = max(logVariables[i].max, value);
        break;
      case lvtEnumSample:
        logVariables[i].sum = value;
        break;
      case lvtEnumAccum:
        logVariables[i].sum |= 1 << value;
        break;
    }
  }
  sampleCount++;
  
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// loggerLogEntry
// Check if the day has changed and if so, format a new filename
// Open the file and write a line from the log
// Reset the log variables
/////////////////////////////////////////////////////////////////////////////////////////////////
char logFileName[13]; // 8.3 + null
void loggerLogEntry() 
{
  rtcReadTime();
  // check if the day has changed and if so, format new filename
  long day = rtcGetTime() / DAY_SECONDS;
  
//  monitorPort.println(day);
//  monitorPort.println(logFilenameDay);
//  monitorPort.println(newDay);

  // Format filename based on day
  sprintf(logFileName, "%04d%02d%02d.log", rtcYear(), rtcMonth(), rtcDay());
  bool newDay = 0 == SD.exists(logFileName);
    
  // open the file
  File f = SD.open(logFileName, FILE_WRITE);
  if (NULL == f) {
    reportStatus(statusSDError);
    return;    
  }
  
  if (newDay) {
    // Print the header:
    // time, number of samples, data field names
    
    f.print("Time, smpCount");

    for (int i=0; i< COUNT_OF(logVariables); i++) {
      f.print(", ");
      switch(logVariables[i].type) {
        case lvtNumeric:
          f.print(logVariables[i].name);
          f.print("_sum, ");
          f.print(logVariables[i].name);
          f.print("_min, ");
          f.print(logVariables[i].name);
          f.print("_max");
          break;
        case lvtEnumSample:
        case lvtEnumAccum:
          f.print(logVariables[i].name);
          break;
      }
    }
    f.println();
  }


  // Write out the timestamp and the sample count
  f.print(rtcGetTime());
  f.print(", ");
  f.print(sampleCount);

  // Write out the data line
  for (int i=0; i< COUNT_OF(logVariables); i++) {

    // Print the variables
    f.print(", ");
    switch(logVariables[i].type) {
      case lvtNumeric:
        f.print((float)logVariables[i].sum / sampleCount, 3);
        f.print(", ");
        f.print(logVariables[i].min);
        f.print(", ");
        f.print(logVariables[i].max);
        break;
      case lvtEnumSample:
      case lvtEnumAccum:
        f.print(logVariables[i].sum);
        break;
    }
  }

  f.println();
  f.close();

  // reset logger variables
  loggerZeroVariables();
}

void loggerZeroVariables()
{
  for (int i=0; i<COUNT_OF(logVariables); i++) {
    logVariables[i].sum = 0;
    logVariables[i].min = 0x7FFFFFFF;
    logVariables[i].max = 0x80000000;
  }
  sampleCount = 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Status log functions
//
// A line is written to the status log for various events including state transitions
/////////////////////////////////////////////////////////////////////////////////////////////////

#define statusLogFilename "status.log"

/////////////////////////////////////////////////////////////////////////////////////////////////
// statuslogCheckChange
// Assign new value to old value, printing a log message if it is a change
/////////////////////////////////////////////////////////////////////////////////////////////////
void statuslogCheckChange(char *string, bool newValue, bool &currentValue)
{
  char buf[50];  // should be enough?
  // TODO: actually print to log file
  if (newValue != currentValue) {
    currentValue = newValue;    
      
    strcpy(buf, rtcPresentTime());
    strcat(buf, " ");
    strcat(buf, string);
    strcat(buf, " = ");
    strcat(buf, currentValue? "TRUE" : "FALSE");

    statuslogWriteLine(buf);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// statusLogWriteLine
// write a line in the status log
/////////////////////////////////////////////////////////////////////////////////////////////////
void statuslogWriteLine(char *string)
{
  monitorPort.println(string);

  File f = SD.open(statusLogFilename, FILE_WRITE);
  if (f) {
    f.println(string);
    f.close();
  }  
}
