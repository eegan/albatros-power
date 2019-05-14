/////////////////////////////////////////////////////////////////////////////////////
// Config and debug monitor, accessible over serial port
/////////////////////////////////////////////////////////////////////////////////////

#include "powertrack.h"
#include "githash.h"
#include <EEPROM.h>
#include <avr/pgmspace.h>

#define LINE_BUF_LEN 30 // max number of bytes in a command
#define MAX_ARG_LEN 12 // max characters per arg
#define MAX_ARG_NUM 3  // max number of args per line

int inbufpos;

const char helpstring[] PROGMEM = 
      "def                  - dump EEPROM fields" CRLF
      "inv                  - invalidate EEPROM (use to reinitialize or to test init code)" CRLF
      "com                  - commit: save in-memory config fields to EEPROM" CRLF
      "set index value      - set config field[index] to value (in memory)" CRLF
      "rtc                  - read current RTC setting" CRLF
      "rtc time hh:mm:ss    - set RTC time (not into hardware)" CRLF
      "rtc date yyyy/mm/dd  - set RTC date (not into hardware)" CRLF
      "rtc adj              - actually adjust RTC" CRLF
      "vs                   - Victron status, print out current Victron parameters" CRLF
      "ls                   - list SD card root files" CRLF
      "cat filename         - dump contents of specified file" CRLF
      "rm filename          - remove specified file" CRLF
      "reset                - jump to address zero" CRLF
      "reinit               - reread variables from EEPROM" CRLF
      "status               - dump system status (including codes)" CRLF
      "clear code           - clear specified status code" CRLF
      "clear *              - clear all status codes" CRLF
      "latch code           - set code as latching" CRLF
      "unlatch code         - set code as non-latching" CRLF
      "block code           - set code as blocked" CRLF
      "unblock code         - set code as unblocked" CRLF
      "flags                - list control flags" CRLF
      "fset index value     - set flag variables" CRLF
      "safe                 - put system in safe mode for power down (no SD access)" CRLF
      // "test                 - whatever we decide" CRLF
      ;
      
/////////////////////////////////////////////////////////////////////////////////////
// monitorInit
// Init (early, called from main)
/////////////////////////////////////////////////////////////////////////////////////
void monitorInit() {
  monitorPort.begin(9600);
  monitorPort.setTimeout(10000); // 5 seconds
}

/////////////////////////////////////////////////////////////////////////////////////
// monitorInit
// Init (late, called from main)
/////////////////////////////////////////////////////////////////////////////////////
void monitorInit2() {
  statuslogWriteLine("ALBATROS power system v 1.0");
  statuslogWriteLine(rtcPresentTime());
  statuslogWriteLine("Compile date: "  __DATE__ " time: " __TIME__);
  statuslogWriteLine("Git hash: " GITHASH);

  monitorPort.println("Serial monitor - commands:");
  monitorPrintHelp();
  inbufpos = 0;
}

void monitorPrintHelp() {
  char localHelp[sizeof helpstring];
  strncpy_P(localHelp, helpstring, sizeof localHelp);
  monitorPort.print(localHelp);
}

/////////////////////////////////////////////////////////////////////////////////////
// monitorLoopHandler
// Loop handler (called from main)
/////////////////////////////////////////////////////////////////////////////////////
void monitorLoopHandler() {
  char mon_buf[LINE_BUF_LEN];
  char mon_buf_copy[LINE_BUF_LEN]; // for logging
  char *command, *arg1, *arg2;
  char c, lastValidc = 0;
  
  while (EOF != (c = monitorPort.read())) {
    lastValidc = c;

    if ('\b' == c || 0x7f == c) { // backspace or rubout
      if (0 != inbufpos) {
        inbufpos--;
        monitorPort.print("\b \b");
      }
    }
    else {
      if (inbufpos < LINE_BUF_LEN-1) {
        BC(mon_buf, inbufpos);
        mon_buf[inbufpos++] = c;
        monitorPort.print(c);
      }
    }
  }

  // no fixing up needed (and nothing more to do) if there's nothing in the buffer
  if (0 == inbufpos)
    return;

  // If the last character was CR, we're done; replace it with null
  // Else quit and come back later

  // This hack lets you enter a line that is filled with gibberish so you don't think the monitor is dead after this is done
//  if ('\r' == mon_buf[inbufpos-1])
  if ('\r' == lastValidc) {
    BC(mon_buf, inbufpos);
    mon_buf[inbufpos-1] = 0;
  }
  else
    return;

  // show what was received as a command line
  monitorPort.print("<");
  monitorPort.print(mon_buf);
  monitorPort.println(">");

  // This is so that I have a copy that strtok hasn't ruined (for logging certain commands which should be logged)
  strcpy(mon_buf_copy, mon_buf);
  
  // parse the line into ' ' separated arguments
  command = strtok(mon_buf, " ");
  arg1 = strtok(NULL, " ");
  arg2 = strtok(NULL, " ");
  
  inbufpos = 0;
  
  if (0 == strcmp(command, "def")) {
    cfgDumpFieldValues(monitorPort);
  }
  else if (0 == strcmp(command, "inv")) {
    statusLogPrint(mon_buf_copy);
    cfg_invalidateEE();
  }  
  else if (0 == strcmp(command, "com")) {
    statusLogPrint(mon_buf_copy);    
    cfg_saveConfig();
  }
  else if (0 == strcmp(command, "set")) {
    statusLogPrint(mon_buf_copy);
    cfg_set(arg1, arg2);
  }
  else if (0 == strcmp(command, "rtc")) {
    if ('\0' == *arg1) {  // no arguments - display time
      //present = rtc.now().unixtime();   // I really think we will want YYYY/MM/DD HH:MM:SS
      monitorPort.print("current time: ");
      monitorPort.println(rtcPresentTime());
    }
    else if (0 == strcmp(arg1, "date")) {  
      monitorPort.print("setting date to: <");
      monitorPort.print(arg2);
      monitorPort.println(">");
      statusLogPrint(mon_buf_copy);
      rtcSetDate(arg2);
    }
    else if (0 == strcmp(arg1, "time")) {  
      monitorPort.print("setting time to: <");
      monitorPort.print(arg2);
      monitorPort.println(">");
      statusLogPrint(mon_buf_copy);
      rtcSetTime(arg2);
    }    
    else if (0 == strcmp(arg1, "adj")) {  
      statusLogPrint(mon_buf_copy);
      monitorPort.println("adjusting rtc");
      rtcAdjust();
    }
  }
  else if (0 == strcmp(command, "vs")) {  
    monitorPort.println("Victron status:");
    victronDumpStatus(monitorPort);
  }    
  else if (0 == strcmp(command, "ls")) {  
    monitorPort.println("SD card root files:");
    loggerRootDir(monitorPort);
  }    
  else if (0 == strcmp(command, "cat")) {  
    monitorPort.println(arg1);
    loggerDumpFile(monitorPort, arg1);
  }  
  else if (0 == strcmp(command, "rm")) {  
    monitorPort.println(arg1);
    statusLogPrint(mon_buf_copy);
    loggerEraseFile(monitorPort, arg1);
  }
  else if (0 == strcmp(command, "reset")) {
    statusLogPrint(mon_buf_copy);
    (*(void (*)())(0))();
  }
  else if (0 == strcmp(command, "reinit")) {
    statusLogPrint(mon_buf_copy);
    cfgInit();
  }
  else if (0 == strcmp(command, "status")) {
    statusDumpStatus(monitorPort);
  }
  else if (0 == strcmp(command, "clear")) {
    if ('*' == *arg1)
      statusClearAll();
    else
      statusClear(atoi(arg1));
  }
  
  else if (0 == strcmp(command, "latch")) {
    statusSetLatching(atoi(arg1), true);
  }
  else if (0 == strcmp(command, "unlatch")) {
    statusSetLatching(atoi(arg1), false);
  }
  else if (0 == strcmp(command, "block")) {
    statusSetBlocked(atoi(arg1), true);
  }
  else if (0 == strcmp(command, "unblock")) {
    statusSetBlocked(atoi(arg1), false);
  }
  else if (0 == strcmp(command, "flags")) {
    loadListFlags(monitorPort);
  }
  else if (0 == strcmp(command, "fset")) {
    loadSetFlag(atoi(arg1), atoi(arg2));
  }
  else if (0 == strcmp(command, "safe")) {
    char c = 0;
    do {
      monitorPort.println("Safe for power down, all operations suspended, hit X to cancel.");
      delay(1000);
      while (EOF != (c = monitorPort.read()))
        if ('X' == c) break;
    } while ('X' != c);
    monitorPort.println("Safe mode cancelled.");
  }  
  // else if (0 == strcmp(command, "test")) {
  // }
  
  else {
        monitorPrintHelp();;
  } 
}
