#include <SD.h>

#define SD_CSpin 10 // Chip-select pin for SD

//logDataType vd_V, vd_I, vd_VPV, vd_PPV;
//UINT16 nSamples;
//const UINT16 samplesToAverage = 60;   // TODO: make this an EEPROM parameter
//UINT32 present; // current UNIX time
//

void loggerInit()
{
//  vd_V = vd_I = vd_VPV = vd_PPV = 0;
//  nSamples = 0;
//  rtcInit();
  SD.begin(SD_CSpin);
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
  while(int n = f.available()) {
    while(EOF != (c = f.read()))
      p.write(c);  // TODO: pacing may be required!
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
