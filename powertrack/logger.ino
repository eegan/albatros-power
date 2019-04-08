// Tested against SD Card library (built-in) version 1.2.2
#include <SD.h>

#define SD_CSpin 10 // Chip-select pin for SD

//logDataType vd_V, vd_I, vd_VPV, vd_PPV;
//UINT16 nSamples;
//const UINT16 samplesToAverage = 60;   // TODO: make this an EEPROM parameter
//UINT32 present; // current UNIX time
//

void loggerInit()
{
  SD.begin(SD_CSpin);
}

void loggerLoopHandler()
{
  
}

//
//
//  //   {"V",      FI_V,     FT_int}   // main battery voltage (mV)
//  //  ,{"VPV",    FI_VPV,   FT_int}   // panel voltage (mV)
//  //  ,{"PPV",    FI_PPV,   FT_int}   // panel power (W)
//  //  ,{"I",      FI_I,     FT_int}   // battery current (mA, signed)
//
//
//void logger_finalizeSample()
//{
//  // increment nSamples, if >= s2avg, /, call log output function, and reset
////  debug.println("Finalizing");
//  loggerInit();
//}

// receive notification of a new Victron sample
void loggerNotifyVictronSample()
{
  // go read it
}


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

void loggerEraseFile (HardwareSerial &p, char *filename)
{
  SD.remove(filename);
}


static UINT32 currentFilenameDay = 0;
char logFileName[13]; // 8.3 + null
void loggerLogEntry() 
{

  // Eamon was here
//  if (rtc.
  
  File f = SD.open(logFileName);
}

#define statusLogFilename "status.log"

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

void statuslogWriteLine(char *string)
{
  monitorPort.println(string);

  File f = SD.open(statusLogFilename, FILE_WRITE);
  if (f) {
    f.println(string);
    f.close();
  }  
}
