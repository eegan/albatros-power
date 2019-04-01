#include <RTClib.h>

RTC_PCF8523 rtc;

logDataType vd_V, vd_I, vd_VPV, vd_PPV;
UINT16 nSamples;
const UINT16 samplesToAverage = 60;   // TODO: make this an EEPROM parameter

UINT32 present; // current UNIX time

void logger_init()
{
  vd_V = vd_I = vd_VPV = vd_PPV = 0;
  nSamples = 0;

  if (!rtc.begin()) {
    // event handler complain
    // enact fallback plan
  }
  if (!rtc.initialized()) {
    // event handler complain
    // autoset by computer at compiletime 
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

//void accumulateSample(UINT16 index, UINT32 value) 
void logger_accumulateSample(unsigned int index, long value) 
{
  present = rtc.now().unixtime();
  // case on index (see values below)
  // accumulate into one of teh vd_X variables

  // NOTE: index value right now JUST HAPPENS to index into the array
  //debug.print(fieldDescriptors[index].index); debug.print(" = "); 
  debug.println(value);
  
//   {"V",      FI_V,     FT_int}   // main battery voltage (mV)
//  ,{"VPV",    FI_VPV,   FT_int}   // panel voltage (mV)
//  ,{"PPV",    FI_PPV,   FT_int}   // panel power (W)
//  ,{"I",      FI_I,     FT_int}   // battery current (mA, signed)  
}

void logger_finalizeSample()
{
  // increment nSamples, if >= s2avg, /, call log output function, and reset
  debug.println("Finalizing");
  logger_init();
}
