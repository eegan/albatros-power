#include <EEPROM.h>

#define MAX_BUF_LEN 30 // max number of bytes in a command
#define MAX_ARG_LEN 12 // max characters per arg
#define MAX_ARG_NUM 3  // max number of args per line

char mon_buf[MAX_BUF_LEN];
bool error_flag = false;

char *argument;
char blank[MAX_BUF_LEN] = "           "; //HARDCODED "MAX_ARG_LEN"-many spaces
char args[MAX_ARG_NUM][MAX_ARG_LEN]; // memory is not an issue, is it?
UINT16 argnum = 0;
UINT16 func; // function to be used
UINT16 field; // field to modify
UINT32 value; // value to change field to

DateTime set;

/*
 * COMMANDS:
 * -> 'dmp' dump contents of EEPROM
 * -> 'mod field val' modify (in memory) the value of given field (originally from EEPROM)
 * -> 'com' commit local changes to the EEPROM
 * -> 'rtc (unixtime)' no arg: display current time. w/ arg: set current time to that UNIX timestamp.
 */

void monitor_init() {
  monitorPort.begin(9600);
  monitorPort.setTimeout(5000); // 5 seconds
}

void monitor_handle() {
  // get a line
  monitorPort.readBytesUntil('\r', mon_buf, MAX_BUF_LEN); // even if this halts all other operation, it won't for more than 5s
  monitorPort.print(mon_buf); // when using the stereo jack you don't see what you typed. Here it is.

  // parse the line into ' ' separated arguments
  argument = strtok(mon_buf, ' ');
  for (UINT16 i = 0; i < COUNT_OF(args); i++) {
    strcpy(args[i], "           ");
  }
  argnum = 0;
  while (argument != NULL && argnum < MAX_ARG_NUM && sizeof argument <= MAX_ARG_LEN)  {
    strcpy(args[argnum], argument);
    argnum++;
  }

  // do thing. 
  // nested ifs seem both most explicit and easy
  // TODO: add helpful messages if it doesn't get expected args
  if (strcmp(args[0], "dmp") == 0) {
    cfgman_dumpParameters();
  }
  else if (strcmp(args[0], "mod") == 0) {
    // now looking for valid fields
    if (strcmp(args[1], "test") == 0 && strcmp(args[2], blank) != 0) {
      cfg.test = atol(args[2]);
    }
  }
  else if (strcmp(args[0], "com") == 0) {
    cfgman_saveConfig();
  }
  else if (strcmp(args[0], "rtc") == 0) {
    if (strcmp(args[1], blank) == 0) {
      present = rtc.now().unixtime();
      monitorPort.print("current time: ");
      monitorPort.println(present);
    }
    else if (strcmp(args[2], blank) != 0) {
      monitorPort.print("setting time to :");
      monitorPort.print(args[2]);
      set = DateTime(args[2]); // TODO: does this work?
      rtc.adjust(set);
    }
  }
  
}
