#include <RTClib.h>

enum {PRE_INIT, NOT_PRESENT, UNINITIALIZED, INITIALIZED} e_rtcState;

RTC_PCF8523 rtc;

void rtcInit()
{
  if (!rtc.begin()) {
    e_rtcState = NOT_PRESENT;
    return;
  }
  
  e_rtcState = rtc.initialized() ? INITIALIZED : UNINITIALIZED;
      
}

char *rtcPresentTime()
{
  static char buf[20] = (__TIME__);
  // TODO: format 
  DateTime now = rtc.now();
  sprintf(buf,"%d/%d/%d %d:%d:%d", 
    now.year(), now.month(), now.day(),
    now.hour(), now.minute(), now.second() );
//  itoa(secs, buf, 10);
  return buf;
}

// temporary storage while setting RTC
DateTime dt;
TimeSpan tm;

//HardwareSerial &mp = monitorPort;

void rtcSetTime(char *s)
{
  int hour,min,sec;
  sscanf(s, "%d:%d:%d", &hour, &min, &sec );
//  mp.println(hour);
//  mp.println(min);
//  mp.println(sec);
  tm = TimeSpan(0,hour,min,sec);
}

void rtcSetDate (char *s) 
{
  int year, month, day;
  sscanf(s, "%d/%d/%d", &year, &month, &day );
//  mp.println(year);
//  mp.println(month);
//  mp.println(day);
  dt = DateTime(year, month, day, 0, 0, 0);
}

void rtcAdjust()
{
  DateTime t = dt+tm;
  rtc.adjust(dt+tm);
}
