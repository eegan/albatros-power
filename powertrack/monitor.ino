#include <EEPROM.h>

#define MAX_BUF_LEN 30 // max number of bytes in a command
#define MAX_ARG_LEN 12 // max characters per arg
#define MAX_ARG_NUM 3  // max number of args per line

char mon_buf[MAX_BUF_LEN];
bool error_flag = false;

/*
 * COMMANDS:
 * -> 'dmp' dump contents of EEPROM
 * -> 'mod field val' modify (in memory) the value of given field (originally from EEPROM)
 * -> 'com' commit local changes to the EEPROM
 * -> 'rtc (unixtime)' no arg: display current time. w/ arg: set current time to that UNIX timestamp.
 */

void monitorInit() {
  monitorPort.begin(9600);
  monitorPort.setTimeout(5000); // 5 seconds
  monitorPort.println("ALBATROS power system serial monitor v 1.0");
}

void monitor_handle() {
  char *command, *arg1, *arg2;
  
  mon_buf[monitorPort.readBytesUntil('\r', mon_buf, MAX_BUF_LEN)] = 0;  // read and null-terminate

  //monitorPort.print(mon_buf); // when using the stereo jack you don't see what you typed. Here it is.

  // parse the line into ' ' separated arguments
  command = strtok(mon_buf, " ");
  arg1 = strtok(NULL, " ");
  arg2 = strtok(NULL, " ");
  
  // TODO: add help if nothing is recognized
  if (0 == strcmp(command, "dmp")) {
    cfgman_dumpParameters();
  }
  else if (0 == strcmp(command, "mod")) {
    // now looking for valid fields
//    if (strcmp(args[1], "test") == 0 && strcmp(args[2], blank) != 0) {
//      cfg.test = atol(args[2]);
//    }
  }
  else if (0 == strcmp(command, "com")) {
    cfgman_saveConfig();
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
}
