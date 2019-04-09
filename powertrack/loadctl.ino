#include "mytypes.h"
#define loadPin 5 // On/off control for load switch
//#define loadPin 6 // pretend an LED is the load switch

#define DAY_SECONDS 86400

long time_of_day;
long loadctl_cooldown = 500; // miliseconds between full runs
long loadctl_last_run = 0; // last time this loop ran

void loadctlInit()
{
  pinMode(loadPin, OUTPUT);
}

// Conditions that go into determining load status (plus load status)
bool lowVoltageCutoff = false;  // True if cut off because of low voltage. Hysteresis implemented.
bool dataAgeCutoff = false;     // True if cut off because of Victron data being too old
bool dayTime = false;           // True if we are in daytime
bool loadOn = false;

void loadctlLoopHandler()
{
  // TODO: be sure this works when millis() wraps, I think it's probably OK (EE)
  if (millis() - loadctl_last_run >= loadctl_cooldown) {
    loadctl_last_run = millis();
    
    INT32 dayStart = cfg_fieldValue(ndx_dayStart);
    INT32 dayEnd = cfg_fieldValue(ndx_dayEnd);
    long maxBatVoltageAge = 1000; // arbitrary value for now
    
    time_of_day = rtcGetTime() % DAY_SECONDS;

    // TODO: is there anything to do, if this is not the case?
    ASSERT(dayStart < dayEnd); // expressed in seconds since midnight, this should always be true

    // See if the voltage is too low.
    // If we haven't seen a sample yet, don't make any decision about voltage cutoffs.
    // (The data age cutoff will kick in if we never get a sample.) 
    if (victronSampleSeen()) {
      long bat = victronGetFieldValue(FI_V);
      bool aboveOffThreshold = bat > cfg_fieldValue(ndx_vbatOffThresholdMv);
      bool aboveOnThreshold  = bat > cfg_fieldValue(ndx_vbatOnThresholdMv);

      // hysteresis logic
      bool newLVCutoff = !aboveOffThreshold | !aboveOnThreshold & lowVoltageCutoff;
      char buf[25];
      strcpy(buf, "LV cutoff ");
      ltoa(bat, buf+strlen(buf),  10);
      statuslogCheckChange(buf, newLVCutoff, lowVoltageCutoff);
    }

    // TODO: test data age by setting it smallish and unplugging the Victron data
    // TODO: test the daytime code

    // See if it's been too long since we got a data packet from the Victron
    bool newDataAgeCutoff = victronGetDataAge() > cfg_fieldValue(ndx_maxVDataAge);
    statuslogCheckChange("data age cutoff", newDataAgeCutoff, dataAgeCutoff);

    // See if it's daytime
	long dayLength = dayEnd - dayStart; 
	if (dayLength < 0) dayLength += DAY_SECONDS;
    
	long timeAfterDayStart = time_of_day - dayStart; 
	if (timeAfterDayStart < 0) timeAfterDayStart += DAY_SECONDS;
	
//    bool newDayTime = time_of_day > dayStart && time_of_day < dayEnd;
    bool newDayTime = timeAfterDayStart < dayLength;
	
    statuslogCheckChange("daytime", newDayTime, dayTime);

    // See if we're turning on (or off)
    // On unless it's daytime, too old data, or too low battery voltage
    bool newLoadOn = !lowVoltageCutoff & !dataAgeCutoff & !dayTime;
    statuslogCheckChange("load ON", newLoadOn, loadOn);
    
    digitalWrite(loadPin, loadOn);
  }
}
