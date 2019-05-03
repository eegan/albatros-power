/////////////////////////////////////////////////////////////////////////////////////
// Load switch control
// Turns load switch on and off from voltage, time of day etc.
/////////////////////////////////////////////////////////////////////////////////////

#include "powertrack.h"

long timeOfDay;
runTimer loadRunTimer(1* 1000L);  // run interval

/////////////////////////////////////////////////////////////////////////////////////
// loadctlInit
// Init (called from main)
/////////////////////////////////////////////////////////////////////////////////////
void loadctlInit()
{
  // Set load control pin to output state
  // Hi = on, Lo = off
  pinMode(loadPin, OUTPUT);
}

// Conditions that go into determining load status (plus load status)
// TODO: prefix these with bool

bool lowVoltageCutoff = false;  // True if cut off because of low voltage. Hysteresis implemented.
bool dataAgeCutoff = false;     // True if cut off because of Victron data being too old
bool daytime = false;           // True if we are in daytime
bool measureTime = false;       // True if we are within the interval in which we characterize the battery voltage
bool loadOn = false;
bool runDuringDay = false;      // True if we are scheduled to run during the (following) day

long batVoltN = 0;
double batTimeSum = 0;
double batVoltSum = 0;
double batTimeSqSum = 0;
double batTVSum = 0;

long measureStartTime;

/////////////////////////////////////////////////////////////////////////////////////
// loadctlLoopHandler
// Loop handler (called from main)
/////////////////////////////////////////////////////////////////////////////////////
void loadctlLoopHandler()
{
  if (loadRunTimer.runNow()) {        
    timeOfDay = rtcGetTime() % DAY_SECONDS;
    rtcReadTime();
    
    // See if the voltage is too low.
    // If we haven't seen a sample yet, don't make any decision about voltage cutoffs.
    // (The data age cutoff will kick in if we never get a sample.) 
    if (victronSampleSeen()) {
      long bat = victronGetFieldValue(FI_V);
      bool aboveOffThreshold = bat > cfg_fieldValue(ndx_vbatOffThresholdMv);
      bool aboveOnThreshold  = bat > cfg_fieldValue(ndx_vbatOnThresholdMv);

      // hysteresis logic
      bool newLVCutoff = !aboveOffThreshold || ( !aboveOnThreshold && lowVoltageCutoff );
      char buf[25];
      snprintf(buf, sizeof buf, "LV cutoff %d", bat);
      statuslogCheckChange(buf, newLVCutoff, lowVoltageCutoff);
      statusReportStatus(statusBatteryLowError, lowVoltageCutoff);
    }

    // TODO: test data age by setting it smallish and unplugging the Victron data
    // TODO: test the daytime code (mostly done)

    // See if it's been too long since we got a data packet from the Victron
    bool newDataAgeCutoff = victronGetDataAge() > cfg_fieldValue(ndx_maxVDataAge) * 1000;
    statuslogCheckChange("data age cutoff", newDataAgeCutoff, dataAgeCutoff);
    statusReportStatus(statusVictronTimeoutError, dataAgeCutoff);

    // See if the voltage measurement flag changes
    bool newMeasureTime = betweenTimes(timeOfDay, cfg_fieldValue(ndx_measureStart), cfg_fieldValue(ndx_measureEnd));  
    if (statuslogCheckChange("battery measurement time", newMeasureTime, measureTime)) {
      // handle change
      if (measureTime) {
        // It just started. Initialize it.
        batVoltN = 0;
        batTimeSum = 0;
        batVoltSum = 0;
        batTimeSqSum = 0;
        batTVSum = 0;
        measureStartTime = rtcTime();
      }
      else {
        // It just ended. Analyze it and make conclusions.
        double computedSlope = ( batTVSum - batTimeSum * batVoltSum / batVoltN) / (batTimeSqSum - batTimeSum*batTimeSum / batVoltN );
        double intercept = (batVoltSum - computedSlope * batTimeSum) / batVoltN;

        monitorPort.print("Computed slope: "); monitorPort.println(computedSlope);
        monitorPort.print("Intercept: "); monitorPort.println(intercept);
        statusLogPrint("Computed slope", computedSlope);
        statusLogPrint("Intercept", intercept);
        
        // it might be different if we smoothe with the EEPROM value
        double slope = computedSlope;  // for now, no smoothing
          
        if (cfg_fieldValue(ndx_hoursReserve) < 0) {
          statusLogPrint("Run during day disabled, reserve hours param < 0");
        }
        else  {
          double proj = intercept + slope * cfg_fieldValue(ndx_hoursReserve) * 3600L;
          runDuringDay = proj > cfg_fieldValue(ndx_vbatOffThresholdMv);
          statusLogPrint("Projected value", proj);
          statusLogPrint("Run during day", runDuringDay);
        }
      }
    }
    
    // See if it's daytime
    bool newDaytime = betweenTimes(timeOfDay, cfg_fieldValue(ndx_dayStart), cfg_fieldValue(ndx_dayEnd));
    statuslogCheckChange("daytime", newDaytime, daytime);

    
    // See if we're turning on (or off)
    // On unless it's daytime, too old data, or too low battery voltage
    bool newLoadOn = !lowVoltageCutoff && !dataAgeCutoff && (!daytime || runDuringDay);
    statuslogCheckChange("load ON", newLoadOn, loadOn);
    
    digitalWrite(loadPin, loadOn);
    statusNotifyLoad(loadOn);    

  }
}


void  loadctlNotifyVictronSample()
{
  // handle measurement, if active
  if (measureTime) {
    // accumulate statistics
    double batVolt = victronGetFieldValue(FI_V);
    double batTime = rtcTime() - measureStartTime;
    
    batVoltN++;
    batTimeSum += batTime;
    batVoltSum += batVolt;
    batTimeSqSum += batTime*batTime;
    batTVSum += batTime * batVolt;

//    monitorPort.print("measure voltage - ");
//    monitorPort.print(batVoltN); monitorPort.print(" ");     
//    monitorPort.print(batVolt); monitorPort.print(" "); 
//    monitorPort.print(batTime); monitorPort.print(" "); 
//    monitorPort.println();
            
    }
}

bool betweenTimes(long timeOfDay, long startTime, long endTime)
{
  long interval = endTime - startTime; 
  if (interval < 0) interval += DAY_SECONDS;

  long timeAfterStart = timeOfDay - startTime; 
  if (timeAfterStart < 0) timeAfterStart += DAY_SECONDS;
  
  return timeAfterStart < interval;  
}

bool loadctlGetLoadState()
{
  return loadOn;
}


void loadListFlags(Stream &p)
{
  uint16_t n = 0;
  listFlag(p, "Low voltage cutoff", lowVoltageCutoff, n++);
  listFlag(p, "Data age cutoff   ", dataAgeCutoff, n++);
  listFlag(p, "daytime           ", daytime, n++);
  listFlag(p, "measure time      ", measureTime, n++);
  listFlag(p, "load on           ", loadOn, n++);
  listFlag(p, "run during day    ", runDuringDay, n++);
}

void listFlag(Stream &p, const char *name, bool value, uint16_t index)
{
  p.print(index); p.print(" ");
  p.print(name); p.print(": ");
  p.println(value);
}

void loadSetFlag(uint16_t index, uint16_t value)
{
  char message[40];
  if (index > 5) return;
  snprintf(message, sizeof message, "Overriding flag %d, new value is %d", index, value);
  statusLogPrint(message);
  
  switch(index) {
    case 0: lowVoltageCutoff = value; break;
    case 1: dataAgeCutoff = value; break;
    case 2: daytime = value; break;
    case 3: measureTime = value; break;
    case 4: loadOn = value; break;
    case 5: runDuringDay = value; break;
  }
}