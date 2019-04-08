#define loadPin 5 // 80% sure this is the right pin

long dayStart;
long dayEnd;
long time_of_day;
long loadctl_cooldown = 60000; // miliseconds between calls
long loadctl_last_run = 0; // last time this loop ran

void loadctl_init()
{
  // Be sure to update these variables somehow when EEPROM written to!
  dayStart = cfg_fieldValue(ndx_dayStart);
  dayEnd = cfg_fieldValue(ndx_dayEnd);
}

void loadctl_loop()
{
  if (millis() - loadctl_last_run < loadctl_cooldown) 
    return;
  else
  {
    time_of_day = rtcGetUnix() % 86400; // 86400 seconds in a day

    ASSERT(dayStart < dayEnd); // expressed in seconds since midnight, this should always be true
    if (time_of_day < dayStart || time_of_day >= dayEnd)
      digitalWrite(loadPin, HIGH); // on during night
    else if (time_of_day >= dayStart && time_of_day < dayEnd)
      digitalWrite(loadPin, LOW); // off during day
    loadctl_last_run = millis();
  }
}
