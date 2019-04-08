#include <EEPROM.h>

/////////////////////////////////////////////////////////////////////////////////////////////////
// EEPROM configuration parameters
/////////////////////////////////////////////////////////////////////////////////////////////////

// Tell it where to store your config data in EEPROM
#define CONFIG_START 0
#define FIELDS_START 32

// ID of the header block
const UINT16 CONFIG_LEVEL = 1;      // increment for each non-backwards compatible configuration structure upgrade
const UINT32 FW_VERSION = 0x10000;  // the current firmware version

/////////////////////////////////////////////////////////////////////////////////////////////////
// EEPROM header data structure
/////////////////////////////////////////////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////////////////////////////////////////////
// EEPROM field data structures
// 4 entries must be maintained for each field
/////////////////////////////////////////////////////////////////////////////////////////////////

// Field types
enum e_ft {
  FT_UINT32, FT_INT32, FT_TIME
} ;

// Index values - to be used by set/get accessors
enum e_fndx {
  ndx_vbatCutoffMv, 
  ndx_measureStart, 
  ndx_measureEnd,
  ndx_dayStart,
  ndx_dayEnd,
  ndx_hoursReserve,
  ndx_slope 
};

// The in-memory copy of the fields, with type tags
struct {
  e_ft type;
  long value;
} eeprom_fields[] = {
   {FT_UINT32, 25000L}  // 25 volts
  ,{FT_TIME, 23*3600L}  // 2300h
  ,{FT_TIME, 03*3600L}  // 0300h
  ,{FT_TIME, 06*3600L}  // start of daytime mode
  ,{FT_TIME, 18*3600L}  // end of daytime mode
  ,{FT_UINT32, 48L}     // minimum calculated hours of reserve to run during the day
  ,{FT_UINT32, 36360L}  // uV / hour nominal discharge rate (220Ah, 50% discharge, 2V span, 2A load)
  
};

// The names of the fields
char *fieldNames[] = 
{
  "VBAT_CUTOFF_MV", "MEASURE_START",  "MEASURE_END", "DAY_START", "DAY_END", "HOURS_RESERVE", "SLOPE"       
};


/////////////////////////////////////////////////////////////////////////////////////////////////
// External functions
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
// cfgInit
/////////////////////////////////////////////////////////////////////////////////////////////////
// Called from main init function
void cfgInit() {
  checkConfig();
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// cfgFieldValue
/////////////////////////////////////////////////////////////////////////////////////////////////
// Return field value given an index
long cfgFieldValue(int ndx)
{
  return eeprom_fields[ndx].value;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// cfgSet (string, string)
// set in-memory value given string versions of index and value
/////////////////////////////////////////////////////////////////////////////////////////////////
// Set in-memory configuration
void cfgSet(char *indexString, char *valueString) {
  int index = atoi(indexString);
  long value = parseValue(index, valueString);
  cfgSet(index, value);
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// cfgSet (int, long)
// set in-memory value given index and string
/////////////////////////////////////////////////////////////////////////////////////////////////
// Set in-memory configuration
void cfgSet(int index, long value) {
  eeprom_fields[index].value = value;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// cfgInvalidateEE
// Invalidate EEPROM header, so it loads defaults on the next restart
// Used for testing, or for reinitializing in actual operation
/////////////////////////////////////////////////////////////////////////////////////////////////
void cfgInvalidateEE() {
  EEPROM.write(CONFIG_START+0, 0); // overwrite first MAGIC byte to invalidate EEPROM contents
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// cfgSaveConfig
// save configuration from memory to EEPROM
/////////////////////////////////////////////////////////////////////////////////////////////////
void cfgSaveConfig() {
    UINT16 i;
    for (i=0; i<sizeof cfg; i++)
      EEPROM.write(CONFIG_START + i, *((char*)&cfg + i));
    for (i=0; i<sizeof eeprom_fields; i++)
      EEPROM.write(FIELDS_START + i, *((char*)eeprom_fields + i));
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// cfgDumpFieldValues
// print out all field values with indexes
/////////////////////////////////////////////////////////////////////////////////////////////////
void cfgDumpFieldValues() {
  monitorPort.write("magic:    "); monitorPort.println(cfg.validation.magic, 16);
  monitorPort.write("cfglev:   "); monitorPort.println(cfg.validation.cfg_level, 10);
  monitorPort.write("new:      "); monitorPort.println(configNew, 10);

  for (int i=0; i<COUNT_OF(eeprom_fields); i++)
  {
    char buf[30];
    sprintf(buf, "%d - %s  =  %s", i, fieldNames[i], fieldValueString(i));
    monitorPort.println(buf);
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Internal functions - do not call from outside
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
// checkConfig
/////////////////////////////////////////////////////////////////////////////////////////////////
// Load header block from EEPROM and check if configuration is valid
// If valid, read the rest of the EEPROM
// If invalid, write hard-coded defaults to the EEPROM
void checkConfig() {
  // read out the validation sub-struct
  for (UINT16 i=0; i<sizeof cfg.validation; i++)
    ((byte*)&cfg)[i] = EEPROM.read(CONFIG_START+i);

  monitorPort.print("Magic number: "); monitorPort.println(cfg.validation.magic);
  
  // check the magic number and the configuration level
  if (  MAGIC  !=  cfg.validation.magic
      || CONFIG_LEVEL <  cfg.validation.cfg_level
  ) {
    // invalid or obsolete configuration - set new flag and fix up validation sub-struct
    monitorPort.println("EEPROM invalid - reinitializing");
    
    configNew = true;
    cfg.validation.magic = MAGIC;
    cfg.validation.cfg_level = CONFIG_LEVEL;

    // save everything (using default values from initializers)
    // TODO: this is lazy compared with just writing what we need
    cfgSaveConfig();
  }
  else {
    // valid configuration - load it
    monitorPort.println("EEPROM valid - loading");
    loadConfig();
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// loadConfig
/////////////////////////////////////////////////////////////////////////////////////////////////
// Load header block from EEPROM and check if configuration is valid
void loadConfig() {
    // load the header block
    for (UINT16 i=sizeof cfg.validation; i<sizeof cfg; i++)
      ((byte*)&cfg)[i] = EEPROM.read(CONFIG_START+i);
     // load the fields array 
    for (UINT16 i=0; i<sizeof eeprom_fields; i++)
      ((byte*) eeprom_fields)[i] = EEPROM.read(FIELDS_START+i);
}

//char *fieldName(int ndx)
//{
//  return fieldNames[ndx];
//}

/////////////////////////////////////////////////////////////////////////////////////////////////
// fieldValueString - format the field value as a string
/////////////////////////////////////////////////////////////////////////////////////////////////
char *fieldValueString(int ndx)
{
  static char buf[20];
  long v = eeprom_fields[ndx].value;
  switch(eeprom_fields[ndx].type) {
    case FT_UINT32:
      ltoa(v, buf, 10);
      break;
    case FT_TIME:
      sprintf(buf, "%02ld:%02ld:%02ld", v/3600L, (v%3600L)/60L, v%60L);
      break;
  }
  return buf;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// parseValue - parse the string value according to the type
/////////////////////////////////////////////////////////////////////////////////////////////////
long parseValue(int ndx, char *str)
{
  long v;
  int h=0, m=0, s=0;
  switch(eeprom_fields[ndx].type) {
    case FT_UINT32:
      v = atol(str);
      break;
    case FT_TIME:
      sscanf(str, "%02d:%02d:%02d", &h, &m, &s);
      v = h*3600L+m*60L+s;
      break;
  }
  return v;
}