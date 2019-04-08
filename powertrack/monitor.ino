#include <EEPROM.h>

#define MAX_BUF_LEN 30 // max number of bytes in a command
#define MAX_ARG_LEN 12 // max characters per arg
#define MAX_ARG_NUM 3  // max number of args per line

bool error_flag = false;

void monitorInit() {
  monitorPort.begin(9600);
  monitorPort.setTimeout(5000); // 5 seconds
  monitorPort.println("ALBATROS power system serial monitor v 1.0");
}

void monitor_handle() {
  char mon_buf[MAX_BUF_LEN];
  char *command, *arg1, *arg2;
  
  mon_buf[monitorPort.readBytesUntil('\r', mon_buf, MAX_BUF_LEN)] = 0;  // read and null-terminate

  //monitorPort.print(mon_buf); // when using the stereo jack you don't see what you typed. Here it is.

  // parse the line into ' ' separated arguments
  command = strtok(mon_buf, " ");
  arg1 = strtok(NULL, " ");
  arg2 = strtok(NULL, " ");
  
  if (0 == strcmp(command, "dmp")) {
    cfgDumpFieldValues();
  }
  else if (0 == strcmp(command, "inv")) {
    cfg_invalidateEE();
  }  
  else if (0 == strcmp(command, "com")) {
    cfg_saveConfig();
  }
  else if (0 == strcmp(command, "set")) {
    cfg_set(arg1, arg2);
  }
  else if (0 == strcmp(command, "rtc")) {
    if (NULL == *arg1) {  // no arguments - display time
    
      //present = rtc.now().unixtime();   // I really think we will want YYYY/MM/DD HH:MM:SS
      monitorPort.print("current time: ");
      monitorPort.println(rtcPresentTime());
    }
    else if (0 == strcmp(arg1, "date")) {  
      monitorPort.print("setting date to: <");
      monitorPort.print(arg2);
      monitorPort.println(">");
      rtcSetDate(arg2);
    }
    else if (0 == strcmp(arg1, "time")) {  
      monitorPort.print("setting time to: <");
      monitorPort.print(arg2);
      monitorPort.println(">");
      rtcSetTime(arg2);
    }    
    else if (0 == strcmp(arg1, "adj")) {  
      monitorPort.println("adjusting rtc");
      rtcAdjust();
    }   
  }
  else {
      monitorPort.print(
      "dmp - dump all EEPROM fields\n"
      "set index value     - set EEPROM field[index] to value\n"
      "rtc                 - read current RTC setting\n"
      "rtc time hh:mm:ss   - set RTC time (not into hardware)\n"
      "rtc date yyyy/mm/dd - set RTC date (not into hardware)\n"
      "rtc adj             - actually adjust RTC\n"
      "\n"
      "\n"
    );
  } 
}
