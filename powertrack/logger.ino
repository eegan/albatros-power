/////////////////////////////////////////////////////////////////////////////////////
// Logging functions
// Regular (time based) logging, and event logging
/////////////////////////////////////////////////////////////////////////////////////

// TODO: split up log file (averaged etc) stuff from event logging?

// Tested against SD Card library (built-in) version 1.2.2
#include "powertrack.h"
#include <SD.h>

const uint16_t MaxLogVars = 20;
uint16_t nLogVars = 0;

// TODO: could/should we use runtimer?
long timeSinceLastLogEntry;

/////////////////////////////////////////////////////////////////////////////////////
// loggerInit
// Init (called from main)
/////////////////////////////////////////////////////////////////////////////////////
void loggerInit()
{
  SD.begin(SD_CSpin);
  loggerZeroVariables();
  timeSinceLastLogEntry = millis();
  nLogVars = 0;
}

/////////////////////////////////////////////////////////////////////////////////////
// loggerLoopHandler
// Loop handler (called from main)
/////////////////////////////////////////////////////////////////////////////////////
void loggerLoopHandler()
{
  long now = millis();
  if (now - timeSinceLastLogEntry > cfg_fieldValue(ndx_secsPerLog)*1000) {
    loggerMakeLogEntry();
    timeSinceLastLogEntry = now;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// SD-card utility functions
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
// loggerRootDir
// list directory of SD root, to port p
/////////////////////////////////////////////////////////////////////////////////////////////////
void loggerRootDir (Stream &p)
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
void loggerDumpFile(Stream &p, const char *filename)
{
  char c;
  File f = SD.open(filename);
  if (!f) {
    p.println("File not found");
    return;
  }

  while( EOF != (c = f.read()) ) {    // until EOF
    while (0 == p.write(c));          // keep trying until the character is written (pacing)
    if (0x03 == p.read()) {
      p.write(CRLF);
      goto exit;  // check for CTRL-C
    }
  }
  exit:
  f.close();
}
/////////////////////////////////////////////////////////////////////////////////////////////////
// loggerEraseFile
// erase specified file
/////////////////////////////////////////////////////////////////////////////////////////////////
void loggerEraseFile (Stream &p, const char *filename)
{
  SD.remove(filename);
}

enum logVarType {
  lvtNumeric      // numeric value, will accumulate averaged/min/max
 ,lvtEnumSample   // enum value, will sample current value
 ,lvtEnumAccum    // enum value, will accumulate bitmap of values seen in log entry interval
// ,lvtLoad         // special case: read the load state
};


// Array of structs detailing variables being logged
struct {
  char const *name;       // name for CSV header
  logVarType type;  // type
  long sum;         // accumulated sum or bitmap
  long min;         // accumulated min
  long max;         // accumulated max
  int sampleCount;  // number of samples
} logVariables[MaxLogVars];

/////////////////////////////////////////////////////////////////////////////////////////////////
// loggerLogSample
// Called from client module init routine.
// Log sample using index returned when registering the variable
/////////////////////////////////////////////////////////////////////////////////////////////////
void loggerLogSample(uint16_t index, long value)
{
    BC(logVariables, index);
    if (index < nLogVars) {
      switch(logVariables[index].type) {
        case lvtNumeric:
          logVariables[index].sum += value;
          logVariables[index].min = min(logVariables[index].min, value);
          logVariables[index].max = max(logVariables[index].max, value);
          break;
        case lvtEnumSample:
          logVariables[index].sum = value;
          break;
        case lvtEnumAccum:
          logVariables[index].sum |= 1 << value;
          break;
      }
      logVariables[index].sampleCount++;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// loggerRegisterLogVariable
// Called from client module init routine.
// Register variable name and type; get index to use when logging
/////////////////////////////////////////////////////////////////////////////////////////////////
//uint16_t loggerRegisterLogVariable(char *name, enum logVarType type)
uint16_t loggerRegisterLogVariable(const char *name, uint16_t type)
{
  BC(logVariables, nLogVars);
  if (nLogVars < MaxLogVars) {
    logVariables[nLogVars].name = name;
    logVariables[nLogVars].type = (logVarType)type;
    return nLogVars++;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// loggerMakeLogEntry
// Check if the day has changed and if so, format a new filename
// Open the file and write a line from the log
// Reset the log variables
/////////////////////////////////////////////////////////////////////////////////////////////////
char logFileName[13]; // 8.3 + null
void loggerMakeLogEntry() 
{
  rtcReadTime();
  // Format filename based on day
  snprintf(logFileName, sizeof logFileName, "%04d%02d%02d.csv", rtcYear(), rtcMonth(), rtcDay());
  bool newDay = 0 == SD.exists(logFileName);
    
  // open the file
  // TODO: this could be simpler and just report the condition, and let latching take care of it
  File f = SD.open(logFileName, FILE_WRITE);
  if (!f) {
    statusReportStatus(statusSDAccessError, true);
    return;
  }
  
  if (newDay) {
    // Print the header:
    // time, number of samples, data field names
    
    f.print("Time, smpCount");

    for (uint16_t i=0; i < nLogVars; i++) {
      f.print(", ");
      switch(logVariables[i].type) {
        case lvtNumeric:
          f.print(logVariables[i].name);
          f.print("_avg, ");
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

  // NOTE: this is using entry zero as representative.
  // This will be part of the Victron data, if Victron init is called before anything 
  // else that registers a log variable.
  f.print(logVariables[0].sampleCount);

  // Write out the data line
  for (uint16_t i=0; i < nLogVars; i++) {

    // Print the variables
    f.print(", ");
    switch(logVariables[i].type) {
      case lvtNumeric:
        f.print((float)logVariables[i].sum / logVariables[i].sampleCount, 2); // 2 decimals, should be enough precision
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
  for (uint16_t i=0; i<nLogVars; i++) {
    BC(logVariables, i);
    logVariables[i].sum = 0;
    logVariables[i].min = 0x7FFFFFFF;
    logVariables[i].max = 0x80000000;
    logVariables[i].sampleCount = 0;
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Status log functions
//
// A line is written to the status log for various events including state transitions
/////////////////////////////////////////////////////////////////////////////////////////////////

#define statusLogFilename "status.log"

/////////////////////////////////////////////////////////////////////////////////////////////////
// statuslogCheckChange
// Assign new value to old value, printing a log message (and returning true) if it is a change
/////////////////////////////////////////////////////////////////////////////////////////////////
bool statuslogCheckChange(char const *string, bool newValue, bool &currentValue)
{
  bool changed = false;
  if (newValue != currentValue) {
    currentValue = newValue;
    changed = true;
    statusLogPrint(string, newValue);
  }
  return changed;
}

/* bool statuslogCheckChange(char const *string, bool newValue, bool &currentValue, uint32_t a, uint32_t b)
{
  bool changed = false;
  if (newValue != currentValue) {
    currentValue = newValue;
    changed = true;
    statusLogPrint(string, newValue, a, b);
  }
  return changed;
} */

/////////////////////////////////////////////////////////////////////////////////////////////////
// Status log print functions for strings, longs and floats
/////////////////////////////////////////////////////////////////////////////////////////////////
// for flags
void statusLogPrint(char const *string, bool flag)
{
  const int buflen = 100;
  char buf[buflen];

  snprintf(buf, sizeof buf, "%19s %30s = %5s", rtcPresentTime(), string, flag? "TRUE" : "FALSE");

  statuslogWriteLine(buf);
}

// for flags with additional parameters
void statusLogPrint(char const *string, bool flag, uint32_t a, uint32_t b)
{
  const int buflen = 100;
  char buf[buflen];

  snprintf(buf, sizeof buf, "%19s %30s = %5s %ld %ld", rtcPresentTime(), string, flag? "TRUE" : "FALSE", a, b);

  statuslogWriteLine(buf);
}


// This one is for logging commands
// TODO: This really should explicitly say echo or not...
void statusLogPrint(char const *string)
{
  const int buflen = 100;
  char buf[buflen];

  snprintf(buf, sizeof buf, "%19s %30s", rtcPresentTime(), string);

  // don't echo since this is a command
  statuslogWriteLine(buf, false);
}

// for long
void statusLogPrint(char const *string, long l)
{
  const int buflen = 100;
  char buf[buflen];

  snprintf(buf, sizeof buf, "%19s %30s = %ld", rtcPresentTime(), string, l);

  statuslogWriteLine(buf);
}

// for double value
void statusLogPrint(char const *string, double d)
{
  const int buflen = 100;
  char buf[buflen];
  char bufd[30];  // buffer for number

  if (abs(d) > 1e15) // lots of margin I hope
    strcpy(bufd, "*****");
  else
    dtostrf(d, 7, 5, bufd);

  snprintf(buf, sizeof buf, "%19s %30s = %s", rtcPresentTime(), string, bufd);

  statuslogWriteLine(buf);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// statusLogWriteLine
// write a line in the status log as well as to the monitor if called for
/////////////////////////////////////////////////////////////////////////////////////////////////
void statuslogWriteLine(char const *string, bool echo)
{
  char logFileName[13]; // 8.3 + null  
  
  if (echo) monitorPort.println(string);

  // Format filename based on month
  snprintf(logFileName, sizeof logFileName, "%04d%02d.log", rtcYear(), rtcMonth());
    
  // open the file
  // TODO: could rely on latching for condition code
  File f = SD.open(logFileName, FILE_WRITE);
  if (!f) {
    statusReportStatus(statusSDAccessError, true);
    return;    
  }

  if (f) {
    f.println(string);
    f.close();
  }  
}

// default echo to true for old callers
void statuslogWriteLine(char const *string)
{
  statuslogWriteLine(string, true);
}
