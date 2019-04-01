#include <EEPROM.h>
// Tell it where to store your config data in EEPROM
#define CONFIG_START 32

// ID of the settings block
const UINT16 CONFIG_LEVEL = 1;      // increment for each non-backwards compatible configuration structure upgrade
const UINT32 FW_VERSION = 0x10000;  // the current firmware version

bool configNew = false;
const UINT32 MAGIC = 0xABCD1234;

struct valstructtag {
  // validate settings
  struct {
    UINT32 magic;
    UINT16 cfg_level;
  } validation;

  // parameters go here
  UINT32 fw_rev;
  UINT32 test;
  
} cfg = {
  {0, 0} // will be overwritten anyway
  // defaults go here
  , FW_VERSION
  , 0xDEADBEEF 
};

void cfgman_loadConfig() {
  // read out the validation sub-struct
  for (UINT16 i=0; i<sizeof cfg.validation; i++)
    ((byte*)&cfg)[i] = EEPROM.read(CONFIG_START+i);

//  monitorPort.write("In loadConfig\r\n");
//  monitorPort.write("magic:    "); monitorPort.println(cfg.validation.magic, 16);
//  monitorPort.write("cfglev:   "); monitorPort.println(cfg.validation.cfg_level, 10);

  // check the magic number and the configuration level
  if (  MAGIC  !=  cfg.validation.magic
      || CONFIG_LEVEL <  cfg.validation.cfg_level
  ) {
    // invalid or obsolete configuration - set new flag and initialize validation sub-struct
    configNew = true;
    cfg.validation.magic = MAGIC;
    cfg.validation.cfg_level = CONFIG_LEVEL;
  }
  else
    for (UINT16 i=sizeof cfg.validation; i<sizeof cfg; i++)
      ((byte*)&cfg)[i] = EEPROM.read(CONFIG_START+i);
}

void cfgman_saveConfig() {
    for (UINT16 i=0; i<sizeof cfg; i++)
      EEPROM.write(CONFIG_START + i, *((char*)&cfg + i));
}



void cfgman_dumpParameters() {
  monitorPort.write("magic:    "); monitorPort.println(cfg.validation.magic, 16);
  monitorPort.write("cfglev:   "); monitorPort.println(cfg.validation.cfg_level, 10);
  monitorPort.write("new:      "); monitorPort.println(configNew, 10);
}


/*
void handleMonitor()
{
  const UINT16 BUFLEN = 100;
  char buf[BUFLEN];
  if (UINT16 n = monitorPort.readBytesUntil('\r', buf, BUFLEN)) {
    switch(buf[0]) {
      case 'd':
        dumpParameters();
        break;
      case 'w':
        saveConfig(); 
        break;     
    }
  }  
}
*/

#if 0
void setup() {
  monitorPort.begin(9600);
  loadConfig();
}

void loop() {
  handleMonitor();
}
#endif
