#include "mytypes.h"
#define loadPin 5 // On/off control for load switch

long time_of_day;
long loadctl_cooldown = 500; // miliseconds between full runs
long loadctl_last_run = 0; // last time this loop ran

void loadctlInit()
{
  pinMode(loadPin, OUTPUT);
}

bool lowVoltageCutoff = false;
bool dataAgeCutoff = false;
bool dayTime = false;
bool loadOn = false;

void loadctlLoop()
{
  // TODO: be sure this works when millis() wraps, I think it's probably OK (EE)
  if (millis() - loadctl_last_run >= loadctl_cooldown) {
    loadctl_last_run = millis();

//    monitorPort.println("Running loadctl loop");
    
    INT32 dayStart = cfg_fieldValue(ndx_dayStart);
    INT32 dayEnd = cfg_fieldValue(ndx_dayEnd);
    INT32 minrunvolts_mV = cfg_fieldValue(ndx_vbatCutoffMv);
    long maxBatVoltageAge = 1000; // arbitrary value for now
    
    time_of_day = rtcGetTime() % 86400; // 86400 seconds in a day

    // TODO: is there anything to do, if this is not the case?
    ASSERT(dayStart < dayEnd); // expressed in seconds since midnight, this should always be true

    // See if the voltage is too low.
    // If we haven't seen a sample yet, don't make any decision about voltage cutoffs.
    // (The data age cutoff will kick in if we never get a sample.) 
    if (victronSampleSeen()) {
      bool newLVCutoff = victronGetFieldValue(FI_V) < cfg_fieldValue(ndx_vbatCutoffMv);
      statuslogCheckChange("LV cutoff", newLVCutoff, lowVoltageCutoff);
    }

    // See if it's been too long since we got a data packet from the Victron
    bool newDataAgeCutoff = victronGetDataAge() > cfg_fieldValue(ndx_maxVDataAge);
    statuslogCheckChange("data age cutoff", newDataAgeCutoff, dataAgeCutoff);

    // See if it's daytime
    bool newDayTime = time_of_day < dayStart || time_of_day >= dayEnd;
    statuslogCheckChange("daytime", newDayTime, dayTime);

    // See if we're turning 
    bool newLoadOn = !lowVoltageCutoff & !dataAgeCutoff & !dayTime;
    statuslogCheckChange("load on/off", newLoadOn, loadOn);

    digitalWrite(loadPin, loadOn); // on during night
  }
}

void statuslogCheckChange(char *string, bool newValue, bool &currentValue)
{
  // TODO: actually print to log file
  if (newValue != currentValue) {
    monitorPort.print("Change in ");
    monitorPort.print(string);
    monitorPort.print(", new value is ");
    monitorPort.println(newValue);
  }
  currentValue = newValue;    
}
